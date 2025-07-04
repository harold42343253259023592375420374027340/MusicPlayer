#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_vfs_fat.h"
#include "wav.h"  // Include wav.h, which fully defines WAVHeader

#define PIN_NUM_CS   GPIO_NUM_10
#define PIN_NUM_MOSI GPIO_NUM_11
#define PIN_NUM_CLK  GPIO_NUM_12
#define PIN_NUM_MISO GPIO_NUM_13
#define UPBUTTON GPIO_NUM_43
#define DOWNBUTTON GPIO_NUM_45
#define PLAYBUTTON GPIO_NUM_38
#define AUX GPIO_NUM_47
#define VOLUME GPIO_NUM_21
#define MAX_FILES 2048
#define MAX_SONGS MAX_FILES

static int volume = 50;
static const char* TAG = "Music Player";
static int songLocation = 0;
static uint8_t isPlaying = 0;
static FILINFO music_file_info;
static FF_DIR music_dir;
static FIL* music_ptr;
static WAVHeader wav_header; 
static unsigned int bytes_read;

#endif
