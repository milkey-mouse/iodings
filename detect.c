#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "iodings.h"
#include "detect.h"
#include "levenshtein.h"

static unsigned int nextSourceID;
soundSource *listeners;

unsigned int initSoundSource(double freq, double threshold, char *pattern, char *name)
{
    soundSource *s = malloc(sizeof(soundSource));
    memset(s, 0, sizeof(soundSource));
    s->id = nextSourceID++;
    s->freq = freq;
    s->name = strdup(name);
    s->threshold = threshold;
    s->next = listeners;
    listeners = s;

    while (*pattern == '0')
    {
        pattern++;
    }

    char *zero = strrchr(pattern, '1');
    if (zero != NULL)
    {
        s->patternLength = zero - pattern + 1;
    }
    for (uint8_t i = 0; i < s->patternLength; i++)
    {
        s->canonicalPattern[i] = (pattern[i] == '1');
    }
    return s->id;
}

bool deleteSoundSource(unsigned int s)
{
    soundSource *prev = NULL;
    for (soundSource *c = listeners; c != NULL; c = c->next)
    {
        if (c->id == s)
        {
            if (prev == NULL)
            {
                listeners = c->next;
            }
            else
            {
                prev->next = c->next;
            }
            free(c->name);
            free(c);
            return true;
        }
        prev = c;
    }
    return false;
}

double goertzel(SAMPLE samples[], unsigned int count, double freq)
{
    double coeff = 2 * cos(2 * PI * freq / sampleRate);
    double s;
    double s_prev = 0.0;
    double s_prev2 = 0.0;
    uint64_t totalPower = 0.0;
    for (unsigned int i = 0; i < count; i++)
    {
        s = samples[i] + coeff * s_prev - s_prev2;
        s_prev2 = s_prev;
        s_prev = s;
        totalPower += samples[i] * samples[i];
    }

    return (s_prev2 * s_prev2 + s_prev * s_prev - coeff * s_prev * s_prev2) / totalPower;
}

int detect(const void *inBuf, void *outBuf, long unsigned int frames, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *data)
{
    double r;
    for (soundSource *c = listeners; c != NULL; c = c->next)
    {
        if ((r = goertzel((SAMPLE *)inBuf, frames, c->freq)) > c->threshold)
        {
            c->threshold = c->threshold * (1 - (THRESHOLD_ADAPT * 2)) + r * (THRESHOLD_ADAPT * 2);
            if (c->threshold > r)
            {
                c->threshold = r;
            }
            if (c->patternIndex == 0)
            {
                c->activated = true;
            }
            if (c->patternIndex < c->patternLength)
            {
                c->currentPattern[c->patternIndex++] = true;
            }
            if (c->patternIndex == c->patternLength)
            {
                c->activated = false;
                c->patternIndex = c->patternLength + 1;
                if (levenshtein((char *)&c->canonicalPattern, (char *)&c->currentPattern, c->patternLength) <= LEVENSHTEIN_DIFF)
                {
                    puts(c->name);
                    fprintf(stderr, "Sound source '%s' triggered @ %f (threshold adapt: %lf)\n", c->name, r, c->threshold);
                    fflush(stdout);
                }
                else
                {
                    fputs("levenshtein miss!\ncanonical pattern: ", stderr);
                    for (int i = 0; i < c->patternLength; i++)
                    {
                        fputs(c->canonicalPattern[i] ? "1" : "0", stderr);
                    }
                    fputs("\ncurrent pattern:   ", stderr);
                    for (int i = 0; i < c->patternLength; i++)
                    {
                        fputs(c->currentPattern[i] ? "1" : "0", stderr);
                    }
                    fputs("\n", stderr);
                }
            }
        }
        else
        {
            c->threshold = c->threshold * (1 - THRESHOLD_ADAPT) + r * THRESHOLD_ADAPT;
            if (c->threshold < r)
            {
                c->threshold = r;
            }
            if (c->patternIndex > c->patternLength && !c->activated)
            {
                fputs("reset\n", stderr);
                c->patternIndex = 0;
            }
            else if (c->activated)
            {
                c->currentPattern[c->patternIndex++] = false;
            }
        }
    }
    return paContinue;
}
