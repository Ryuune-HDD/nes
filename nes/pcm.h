#ifndef PCM_H
#define PCM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {



#endif

int pcm_init(uint32_t sample_rate);
int pcm_submit_buffer(const uint16_t* buffer, size_t sample_count);
int pcm_play_file(const char* filename);
void pcm_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // PCM_H
