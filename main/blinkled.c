#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "esp_check.h"

#include "esp_vfs_fat.h"

#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"

#include "driver/i2c_master.h"

#include "sdkconfig.h"

#include "sdmmc_cmd.h"
#include "wav.h"

#define MAX_FILES 2048
#define MAX_SONGS MAX_FILES


#define PIN_NUM_CS   GPIO_NUM_10
#define PIN_NUM_MOSI GPIO_NUM_11
#define PIN_NUM_CLK  GPIO_NUM_12
#define PIN_NUM_MISO GPIO_NUM_13
#define UPBUTTON GPIO_NUM_43
#define DOWNBUTTON GPIO_NUM_45
#define PLAYBUTTON GPIO_NUM_38
#define AUX GPIO_NUM_39
#define VOLUME GPIO_NUM_21
#define SCL GPIO_NUM_17
#define SDA GPIO_NUM_18

#define MCP4725_ADDR 0x60
#define MCP4725_GENERAL_CALL_ADDR   0x00
#define MCP4725_GENERAL_CALL_RESET  0x06
#define MCP4725_GENERAL_CALL_WAKE   0x09


static int volume = 50;
static const char* TAG = "Music Player";
static int songLocation = 0;
static uint8_t isPlaying = 0;
static FILINFO music_file_info;
static FF_DIR music_dir;
static FIL* music_ptr;
static WAVHeader wav_header;
static unsigned int bytes_read;
static uint8_t audio_buffer[512];



uint8_t play_music(void) {
    f_read(music_ptr, &wav_header.riff, sizeof(char[4]), &bytes_read); // check if RIFF
    if (strcmp(wav_header.riff,"RIFF") != 0) {
        ESP_LOGE(TAG,"Incorrect file format");
        return 1;
    }
    f_read(music_ptr, &wav_header.chunk_size,sizeof(uint32_t),&bytes_read);
    if (bytes_read != 4) {
        ESP_LOGE(TAG, "File is FUBAR");
        return 1;
    }
    f_read(music_ptr, &wav_header.wave, sizeof(char[4]), &bytes_read);
    if (strcmp(wav_header.wave,"WAVE") != 0) {
        ESP_LOGE(TAG,"Incorrect file format");
        return 1;
    }
    f_read(music_ptr, &wav_header.fmt, sizeof(char[4]), &bytes_read);
    if (strcmp(wav_header.wave,"fmt ") != 0) {
        ESP_LOGE(TAG,"File is FUBAR");
        return 1;
    }
    f_read(music_ptr, &wav_header.subchunk1_size, sizeof(uint32_t), &bytes_read);
    if (bytes_read != 4) {
        ESP_LOGE(TAG,"File is FUBAR");
        return 1;
    }   
    f_read(music_ptr, &wav_header.audio_format, sizeof(uint16_t), &bytes_read);
    if (bytes_read != 2) {
        ESP_LOGE(TAG,"File is FUBAR");
        return 1;
    }   
    f_read(music_ptr, &wav_header.num_channels, sizeof(uint16_t), &bytes_read);
    if (wav_header.num_channels != 1) {
        ESP_LOGE(TAG, "Max channels supported is 1, this file has %i", wav_header.num_channels);
        return 1;
    }
    f_read(music_ptr, &wav_header.sample_rate, sizeof(uint32_t), &bytes_read);
    if (bytes_read != 4) {
        ESP_LOGE(TAG,"File is FUBAR");
        return 1; 
    }
    f_read(music_ptr, &wav_header.byte_rate, sizeof(uint32_t), &bytes_read);
    if (bytes_read != 4) {
        ESP_LOGE(TAG,"File is FUBAR");
        return 1; 
    }
    f_read(music_ptr, &wav_header.block_align, sizeof(uint16_t), &bytes_read);
    if (bytes_read != 2) {
        ESP_LOGE(TAG,"File is FUBAR");
        return 1; 
    }
    f_read(music_ptr, &wav_header.bits_per_sample, sizeof(uint32_t), &bytes_read);
    if (bytes_read != 4) {
        ESP_LOGE(TAG,"File is FUBAR");
        return 1; 
    }   
    f_read(music_ptr, &wav_header.data, sizeof(char[4]), &bytes_read);
    if (strcmp(wav_header.data, "data") != 0) {
        ESP_LOGE(TAG,"File is FUBAR");
        return 1;
    }
    f_read(music_ptr, &wav_header.subchunk2_size, sizeof(uint32_t), &bytes_read);
    if (bytes_read != 4) {
        ESP_LOGE(TAG,"File is FUBAR");
        return 1;        
    }


    uint32_t audio_sample_rate = wav_header.sample_rate;
    uint32_t audio_data_size = wav_header.subchunk2_size;
    uint32_t total_samples = audio_data_size / (wav_header.bits_per_sample / 8);
    uint32_t bytes_to_read = audio_data_size;


    while (bytes_to_read > 0) {
        uint32_t bytes_read_this_time = (bytes_to_read > sizeof(audio_buffer)) ? sizeof(audio_buffer) : bytes_to_read;
        
        f_read(music_ptr, audio_buffer, bytes_read_this_time, &bytes_read);
        
        for (uint32_t i = 0; i < bytes_read_this_time; i++) {

            vTaskDelay(pdMS_TO_TICKS(1000 / audio_sample_rate));
        }
        bytes_to_read -= bytes_read_this_time;
    }
    return 0;
}


void up_button(void) {
    f_close(music_ptr);
    
    if (songLocation > 0) {
        songLocation--;
    }


    for (int i = 0; i <= songLocation; i++) {
        if (f_readdir(&music_dir, &music_file_info) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read directory");
            return;
        }
    }

    ESP_LOGI(TAG, "Current Selection: %s", music_file_info.fname);
    f_open(music_ptr, music_file_info.fname, FA_READ); 
}

void down_button(void) {
    f_close(music_ptr);

    songLocation++; 

    for (int i = 0; i <= songLocation; i++) {
        if (f_readdir(&music_dir, &music_file_info) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read directory");
            return;
        }
    }

    ESP_LOGI(TAG, "Current Selection: %s", music_file_info.fname);
    f_open(music_ptr, music_file_info.fname, FA_READ);  
}

void play_button(void) {
    isPlaying = !isPlaying;

    f_readdir(&music_dir,&music_file_info);

    ESP_LOGE(TAG, "Failed to read 'music' directory");

    ESP_LOGI(TAG,"SONG NAME: %s", music_file_info.fname);

    if (isPlaying == 1) {
        f_open(music_ptr,"/music/",'r');
        if (play_music() != 0) {
            ESP_LOGE(TAG, "music failed to play");
        }
    }
    return;
}

void read_and_set_volume() {
    volume = gpio_get_level(VOLUME);
    if (volume < 0) {
        volume = 0;
    } 
    if (volume > 100) {
        volume = 100;
    }
    return;
}

void up_button_isr_handler(void* arg) {
    up_button();
}

void down_button_isr_handler(void* arg) {
    down_button();
}

void play_button_isr_handler(void* arg) {
    play_button();
}



void app_main(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST; 

    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = MAX_FILES,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    ret = esp_vfs_fat_sdspi_mount("/music", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount filesystem (%s)", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "SD card mounted, allocation_unit_size: %d", mount_config.allocation_unit_size);

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .pin_bit_mask = (1ULL << UPBUTTON) | (1ULL << DOWNBUTTON) | (1ULL << PLAYBUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };

    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);

    gpio_isr_handler_add(UPBUTTON, up_button_isr_handler, NULL);
    gpio_isr_handler_add(DOWNBUTTON, down_button_isr_handler, NULL);
    gpio_isr_handler_add(PLAYBUTTON, play_button_isr_handler, NULL);
    gpio_set_direction(VOLUME, GPIO_MODE_INPUT);
    f_opendir(&music_dir,"/music");

    while (1) {
        read_and_set_volume();
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

}
