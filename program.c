#include <portaudio.h>
#include <complex.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fftw3.h>
#include <float.h>
#include <math.h>
#include "iodings.h"
#include "program.h"
#include "detect.h"

static double *in;
static fftw_complex *out;
static fftw_plan *plan = NULL;

static double *out_average;
static unsigned int count = 1;
static sampleBlock *first = NULL;
static sampleBlock *recorded = NULL;

void init_fftw(bool estimate)
{
    in = fftw_malloc(CHUNKSIZE * sizeof(double));
    out = fftw_alloc_complex(CHUNKSIZE / 2 + 1);
    out_average = fftw_malloc(CHUNKSIZE * sizeof(double));
    if (plan == NULL)
    {
        plan = malloc(sizeof(fftw_plan));
        *plan = fftw_plan_dft_r2c_1d(CHUNKSIZE, in, out, (estimate ? FFTW_ESTIMATE : FFTW_MEASURE) | FFTW_DESTROY_INPUT);
    }
    if (first == NULL)
    {
        first = malloc(sizeof(sampleBlock));
        first->next = NULL;
        recorded = first;
        sampleBlock *last = first;
        for (int i = 1; i < (10 * sampleRate) / CHUNKSIZE; i++)
        {
            last->next = malloc(sizeof(sampleBlock));
            memset(first, 0, sizeof(sampleBlock));
            last = first->next;
        }
    }
}

int program(const void *inBuf, void *outBuf, long unsigned int frames, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *data)
{
    if (frames != CHUNKSIZE)
    {
        return paContinue;
    }
    for (int i = 0; i < frames; i++)
    {
        in[i] = ((const SAMPLE *)inBuf)[i];
    }
    fftw_execute(*plan);
    for (int o = 0; o < (CHUNKSIZE / 2 + 1); o++)
    {
        out_average[o] += cabs(out[o]) / CHUNKSIZE;
    }
    memcpy(recorded->data, inBuf, sizeof(recorded->data));
    if (recorded->next == NULL)
    {
        recorded->next = malloc(sizeof(sampleBlock));
    }
    recorded = recorded->next;
    count++;
    return paContinue;
}

double max_freq()
{
    double max = -DBL_MAX;
    int bin = 0;
    for (int o = (int)(FREQ_MIN * CHUNKSIZE / sampleRate); o < (CHUNKSIZE / 2 + 1); o++)
    {
        if (out_average[o] > max)
        {
            max = out_average[o];
            bin = o;
        }
    }
    return bin * sampleRate / CHUNKSIZE; // bin to frequency
}

double max_amplitude(double freq)
{
    double prev = 0;
    SAMPLE prevAmplitude = 0;
    double maxSkip = 0;
    double threshold = 0;
    double activated = 0;
    for (sampleBlock *b = first; b != recorded; b = b->next)
    {
        SAMPLE amplitude = 0;
        SAMPLE a;
        for (int i = 0; i < CHUNKSIZE; i++)
        {
            a = abs(b->data[i]);
            if (a > amplitude)
            {
                amplitude = a;
            }
        }
        double g = goertzel(b->data, CHUNKSIZE, freq) / amplitude;
        double skip = g - prev;
        if (skip > maxSkip)
        {
            maxSkip = skip;
            threshold = prev * prevAmplitude;
            activated = g * amplitude;
        }
        prev = g;
        prevAmplitude = amplitude;
    }
    return threshold + (activated - threshold) * 0.1;
}

void max_pattern(double freq, double threshold, char *pattern)
{
    int i = 0;
    int l = 0;
    bool activated = false;
    for (sampleBlock *b = first; b != recorded && i < PATTERN_SIZE + 1; b = b->next)
    {
        if (goertzel(b->data, CHUNKSIZE, freq) > threshold)
        {
            activated = true;
            pattern[i++] = '1';
            l = i;
        }
        else if (activated)
        {
            pattern[i++] = '0';
        }
    }
    while (l < PATTERN_SIZE + 1)
    {
        pattern[l++] = '\0';
    }
}