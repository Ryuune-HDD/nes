#include "pcm.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#pragma comment(lib, "winmm.lib")

#define BUFFER_COUNT 3
#define MAX_SAMPLES_PER_BUF 4096

static HWAVEOUT hWaveOut = NULL;
static WAVEHDR headers[BUFFER_COUNT];
static char* audio_buffers[BUFFER_COUNT];
static volatile int next_free_buf = 0;
static volatile int playing_count = 0;
static int is_initialized = 0;
static int g_sample_rate = 0;

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
                          DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    if (uMsg == WOM_DONE) {
        playing_count--;
    }
}

int pcm_init(uint32_t sample_rate)
{
    if (is_initialized) return 0;

    if (sample_rate == 0) {
        return -1;
    }

    WAVEFORMATEX wfx = {0};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = sample_rate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = 2;
    wfx.nAvgBytesPerSec = sample_rate * 2;
    wfx.cbSize = 0;

    MMRESULT res = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx,
                               (DWORD_PTR)waveOutProc, 0, CALLBACK_FUNCTION);
    if (res != MMSYSERR_NOERROR) {
        return -1;
    }

    for (int i = 0; i < BUFFER_COUNT; ++i) {
        audio_buffers[i] = (char*)malloc(MAX_SAMPLES_PER_BUF * 2);
        if (!audio_buffers[i]) {
            for (int j = 0; j < i; ++j) free(audio_buffers[j]);
            waveOutClose(hWaveOut);
            return -1;
        }
        memset(&headers[i], 0, sizeof(WAVEHDR));
    }

    is_initialized = 1;
    playing_count = 0;
    next_free_buf = 0;
    g_sample_rate = sample_rate;
    return 0;
}

int pcm_submit_buffer(const uint16_t* buffer, size_t sample_count)
{
    if (!is_initialized || !hWaveOut || !buffer) {
        return 1;
    }

    if (sample_count == 0) return 0;

    size_t byte_size = sample_count * sizeof(uint16_t);
    if (byte_size > MAX_SAMPLES_PER_BUF * 2) {
        byte_size = MAX_SAMPLES_PER_BUF * 2;
    }

    while (playing_count >= BUFFER_COUNT) {
        Sleep(1);
    }

    int idx = next_free_buf;
    next_free_buf = (next_free_buf + 1) % BUFFER_COUNT;

    memcpy(audio_buffers[idx], buffer, byte_size);

    WAVEHDR* hdr = &headers[idx];
    if (hdr->dwFlags & WHDR_PREPARED) {
        waveOutUnprepareHeader(hWaveOut, hdr, sizeof(WAVEHDR));
    }
    hdr->lpData = audio_buffers[idx];
    hdr->dwBufferLength = (DWORD)byte_size;
    hdr->dwFlags = 0;

    waveOutPrepareHeader(hWaveOut, hdr, sizeof(WAVEHDR));
    waveOutWrite(hWaveOut, hdr, sizeof(WAVEHDR));
    playing_count++;

    return 0;
}

int pcm_play_file(const char* filename)
{
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uint16_t* buffer = (uint16_t*)malloc(file_size);
    if (!buffer) {
        fclose(fp);
        return -1;
    }

    fread(buffer, 1, file_size, fp);
    fclose(fp);

    size_t sample_count = file_size / sizeof(uint16_t);
    size_t offset = 0;

    while (offset < sample_count) {
        size_t remaining = sample_count - offset;
        size_t chunk = (remaining < MAX_SAMPLES_PER_BUF) ? remaining : MAX_SAMPLES_PER_BUF;

        while (playing_count >= BUFFER_COUNT - 1) {
            Sleep(1);
        }

        pcm_submit_buffer(buffer + offset, chunk);
        offset += chunk;
    }

    free(buffer);
    return 0;
}

void pcm_cleanup(void)
{
    if (hWaveOut) {
        waveOutReset(hWaveOut);
        for (int i = 0; i < BUFFER_COUNT; ++i) {
            if (headers[i].dwFlags & WHDR_PREPARED) {
                waveOutUnprepareHeader(hWaveOut, &headers[i], sizeof(WAVEHDR));
            }
            free(audio_buffers[i]);
        }
        waveOutClose(hWaveOut);
        hWaveOut = NULL;
    }
    is_initialized = 0;
}
