#include <stdint.h>

#pragma pack(push, 1) // Prevent padding between fields

typedef struct {
    char     riff[4];         // "RIFF"
    uint32_t chunk_size;      
    char     wave[4];         // "WAVE"

    // fmt subchunk
    char     fmt[4];          // "fmt "
    uint32_t subchunk1_size;  // Size of the fmt chunk (16 for PCM)
    uint16_t audio_format;    // PCM = 1
    uint16_t num_channels;    // Mono = 1, Stereo = 2
    uint32_t sample_rate;     // 44100, 48000, etc.
    uint32_t byte_rate;       // sample_rate * num_channels * bits_per_sample / 8
    uint16_t block_align;     // num_channels * bits_per_sample / 8
    uint16_t bits_per_sample; // 8, 16, etc.

    // data subchunk
    char     data[4];         // "data"
    uint32_t subchunk2_size;  // num_samples * num_channels * bits_per_sample / 8
} WAVHeader;

#pragma pack(pop)