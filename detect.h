#pragma once
#include <stdbool.h>
#include "iodings.h"

typedef struct soundSource
{
    unsigned int id;
    char *name;
    double freq;
    double threshold;
    bool canonicalPattern[PATTERN_SIZE];
    uint8_t patternLength;
    bool currentPattern[PATTERN_SIZE];
    bool activated;
    uint8_t patternIndex;
    struct soundSource *next;
} soundSource;

soundSource *listeners;

/* Listen for another frequency when detect()ing audio. */
unsigned int initSoundSource(double freq, double threshold, char *pattern, char *name);

/* Stop listening for a source ID. */
bool deleteSoundSource(unsigned int s);

double goertzel(SAMPLE samples[], unsigned int count, double freq);

/* PortAudio callback to detect the presence of soundSources added by addSoundSource(). */
int detect(const void *inBuf, void *outBuf, long unsigned int frames, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *data);