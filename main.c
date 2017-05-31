#include <portaudio.h>
#include <stdbool.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "iodings.h"
#include "program.h"
#include "detect.h"
#include "wav.h"

double sampleRate;

static volatile bool recording = true;
void sigintHandler(int _)
{
    (void)_;
    recording = false;
}

static void usage()
{
    puts("usage:");
    puts("    iodings listen <config>");
    puts("        listens for the frequencies specified in <config>");
    puts("");
    puts("    iodings program <name> (sample)");
    puts("        create a new entry for use in configs");
    puts("        if used, sample must be a signed 16-bit WAV file");
    puts("        if not, iodings will read from the default microphone");
    puts("");
    puts("        to add a new programmed device to a config, just append it:");
    puts("            iodings program \"sample\" sample.wav >> iodings_config");
}

/*static void readConfig()
{
}*/

static void record(bool programming)
{
    if (Pa_Initialize() != paNoError)
    {
        fputs("Couldn't initialize PortAudio\n", stderr);
        exit(1);
    }

    PaStream *stream;
    PaStreamParameters inputParameters;
    if ((inputParameters.device = Pa_GetDefaultInputDevice()) == paNoDevice)
    {
        fputs("No audio device found\n", stderr);
        exit(1);
    }
    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(inputParameters.device);
    fprintf(stderr, "device name: %s\nsample rate: %i\ninput latency: %f\n", deviceInfo->name, (int)deviceInfo->defaultSampleRate, deviceInfo->defaultLowOutputLatency);
    sampleRate = deviceInfo->defaultSampleRate;
    inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = FORMAT;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    if (Pa_OpenStream(&stream, &inputParameters, NULL, sampleRate, CHUNKSIZE, paNoFlag, programming ? program : detect, NULL) != paNoError)
    {
        fputs("Error opening audio stream\n", stderr);
        exit(1);
    }

    if (Pa_StartStream(stream) != paNoError)
    {
        fputs("Error starting audio stream\n", stderr);
        exit(1);
    }

    signal(SIGINT, sigintHandler);
    fputs(programming ? "Recording\n" : "Listening\n", stderr);
    while (recording && Pa_IsStreamActive(stream))
    {
        Pa_Sleep(100);
    }

    if (Pa_CloseStream(stream) != paNoError)
    {
        fputs("Error closing audio stream\n", stderr);
        exit(1);
    }
}

static void saveSample(char *name)
{
    char pattern[PATTERN_SIZE + 1];
    double freq = max_freq();
    double amplitude = max_amplitude(freq);
    max_pattern(freq, amplitude, pattern);
    printf("%lf %lf %s %s\n", freq, amplitude, pattern, name);
}

static void loadConfig(char *fn)
{
    FILE *fp = strcmp(fn, "-") == 0 ? stdin : fopen(fn, "r");
    if (fp == NULL)
    {
        fputs("could not open config file\n", stderr);
        exit(1);
    }

    double freq;
    double threshold;
    char name[256];
    char pattern[PATTERN_SIZE + 1];
    memset(&pattern, 0, PATTERN_SIZE + 1);
    while (fscanf(fp, "%lf %lf %s %s\n", &freq, &threshold, (char *)&pattern, (char *)&name) > 0)
    {
        initSoundSource(freq, threshold, pattern, name);
    }

    fclose(fp);
}

static void saveConfig(char *fn)
{
    FILE *fp = strcmp(fn, "-") == 0 ? stdout : fopen(fn, "w");
    if (fp == NULL)
    {
        fputs("couldn't save updated config\n", stderr);
    }

    for (soundSource *c = listeners; c != NULL; c = c->next)
    {
        char pattern[c->patternLength + 1];
        for (int i = 0; i < c->patternLength; i++)
        {
            pattern[i] = c->canonicalPattern[i] ? '1' : '0';
        }
        pattern[c->patternLength] = '\0';
        fprintf(fp, "%lf %lf %s %s\n", c->freq, c->threshold, pattern, c->name);
    }

    fclose(fp);
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "program") == 0)
    {
        init_fftw(false);
        record(true);
        saveSample(argv[2]);
        return 0;
    }
    else if (argc == 4 && strcmp(argv[1], "program") == 0)
    {
        if (strlen(argv[2]) > 255)
        {
            fputs("Name must be no longer than 255 characters\n", stderr);
            return 1;
        }
        else if (strpbrk(argv[2], "\n") != 0)
        {
            fputs("Newlines are not allowed in the sample name\n", stderr);
        }
        init_fftw(true);
        processSample(program, argv[3]);
        saveSample(argv[2]);
        return 0;
    }
    else if (argc >= 3 && strcmp(argv[1], "listen") == 0)
    {
        loadConfig(argv[2]);
        if (argc >= 4)
        {
            processSample(detect, argv[3]);
        }
        else
        {
            record(false);
            saveConfig(argv[2]);
        }
        return 0;
    }

    usage();
    return 1;
}