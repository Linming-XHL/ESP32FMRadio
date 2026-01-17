#include "wav_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

bool wav_open(const char *filename, wav_file_t *wav) {
    assert(filename && wav);
    
    // Clear the WAV structure
    memset(wav, 0, sizeof(wav_file_t));
    
    // Open the WAV file
    wav->file = fopen(filename, "rb");
    if (!wav->file) {
        return false;
    }
    
    // Read RIFF header
    if (fread(&wav->riff, sizeof(wav_riff_header_t), 1, wav->file) != 1) {
        fclose(wav->file);
        return false;
    }
    
    // Check RIFF signature
    if (memcmp(wav->riff.chunk_id, "RIFF", 4) != 0 || 
        memcmp(wav->riff.format, "WAVE", 4) != 0) {
        fclose(wav->file);
        return false;
    }
    
    // Read format header
    if (fread(&wav->fmt, sizeof(wav_format_header_t), 1, wav->file) != 1) {
        fclose(wav->file);
        return false;
    }
    
    // Check format signature
    if (memcmp(wav->fmt.subchunk1_id, "fmt ", 4) != 0) {
        fclose(wav->file);
        return false;
    }
    
    // Check if it's PCM format
    if (wav->fmt.audio_format != 1) {
        fclose(wav->file);
        return false;
    }
    
    // Check sample format (8 or 16 bits)
    if (wav->fmt.bits_per_sample != 8 && wav->fmt.bits_per_sample != 16) {
        fclose(wav->file);
        return false;
    }
    
    // Find data chunk
    wav_data_header_t data_header;
    bool found_data = false;
    
    while (!found_data) {
        // Read subchunk header
        if (fread(&data_header, sizeof(data_header.subchunk2_id) + sizeof(data_header.subchunk2_size), 1, wav->file) != 1) {
            fclose(wav->file);
            return false;
        }
        
        // Check if it's data chunk
        if (memcmp(data_header.subchunk2_id, "data", 4) == 0) {
            found_data = true;
            wav->data = data_header;
            break;
        } else {
            // Skip other chunks
            uint32_t chunk_size = data_header.subchunk2_size;
            if (fseek(wav->file, chunk_size, SEEK_CUR) != 0) {
                fclose(wav->file);
                return false;
            }
        }
    }
    
    if (!found_data) {
        fclose(wav->file);
        return false;
    }
    
    // Record data offset
    wav->data_offset = ftell(wav->file);
    wav->data_pos = 0;
    wav->is_open = true;
    
    return true;
}

bool wav_read_sample(wav_file_t *wav, int16_t *sample) {
    assert(wav && sample);
    
    if (!wav->is_open || !wav->file) {
        return false;
    }
    
    // If we've reached the end of data, reset to beginning
    if (wav->data_pos >= wav->data.subchunk2_size) {
        wav_reset(wav);
    }
    
    uint8_t byte_sample;
    int16_t int_sample;
    
    if (wav->fmt.bits_per_sample == 8) {
        // 8-bit sample (unsigned)
        if (fread(&byte_sample, sizeof(uint8_t), 1, wav->file) != 1) {
            return false;
        }
        // Convert to signed 16-bit
        *sample = (int16_t)((byte_sample - 128) << 8);
        wav->data_pos += sizeof(uint8_t);
    } else if (wav->fmt.bits_per_sample == 16) {
        // 16-bit sample (signed, little-endian)
        if (fread(&int_sample, sizeof(int16_t), 1, wav->file) != 1) {
            return false;
        }
        *sample = int_sample;
        wav->data_pos += sizeof(int16_t);
    } else {
        // Unsupported sample format
        return false;
    }
    
    // If stereo, average channels (convert to mono)
    if (wav->fmt.num_channels == 2) {
        int16_t sample2;
        
        if (wav->fmt.bits_per_sample == 8) {
            if (fread(&byte_sample, sizeof(uint8_t), 1, wav->file) != 1) {
                return false;
            }
            sample2 = (int16_t)((byte_sample - 128) << 8);
            wav->data_pos += sizeof(uint8_t);
        } else {
            if (fread(&sample2, sizeof(int16_t), 1, wav->file) != 1) {
                return false;
            }
            wav->data_pos += sizeof(int16_t);
        }
        
        // Average the two channels
        *sample = (*sample + sample2) / 2;
    }
    
    return true;
}

void wav_close(wav_file_t *wav) {
    assert(wav);
    
    if (wav->is_open && wav->file) {
        fclose(wav->file);
        wav->is_open = false;
        wav->file = NULL;
    }
}

void wav_reset(wav_file_t *wav) {
    assert(wav);
    
    if (wav->is_open && wav->file) {
        fseek(wav->file, wav->data_offset, SEEK_SET);
        wav->data_pos = 0;
    }
}
