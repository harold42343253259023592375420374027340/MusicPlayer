#include <stdio.h>
#include "wav.h"
#include "esp_vfs_fat.h"

uint8_t play_wav_file(FIL* music_ptr, WAVHeader wav_header) {
    uint8_t audio_buffer[512];
    unsigned int bytes_read;
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
    gpio_set_direction(AUX,GPIO_MODE_OUTPUT);
    
    while (bytes_to_read > 0) {
        uint32_t bytes_read_this_time = (bytes_to_read > sizeof(audio_buffer)) ? sizeof(audio_buffer) : bytes_to_read;
        
        f_read(music_ptr, audio_buffer, bytes_read_this_time, &bytes_read);
        
        for (uint32_t i = 0; i < bytes_read_this_time; i++) {
            uint8_t audio_sample = audio_buffer[i];
            gpio_set_level(AUX,audio_sample * VOLUME);
            
            vTaskDelay(pdMS_TO_TICKS(1000 / audio_sample_rate));
        }

        bytes_to_read -= bytes_read_this_time;
    }
    return 0;
}