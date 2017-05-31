#pragma once
#include <portaudio.h>
#include <stdbool.h>
#include "iodings.h"

typedef struct sampleBlock
{
    SAMPLE data[CHUNKSIZE];
    struct sampleBlock *next;
} sampleBlock;

double max_freq();
double max_amplitude(double freq);
void max_pattern(double freq, double threshold, char *pattern);

void init_fftw(bool estimate);

int program(const void *inBuf, void *outBuf, long unsigned int frames, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *data);