#include <portaudio.h>
#include <sndfile.h>
#include <string.h>
#include <stdlib.h>
#include "detect.h"

void processSample(PaStreamCallback callback, char *fn)
{
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(sfinfo));
    SNDFILE *wav;
    if (!(wav = sf_open(fn, SFM_READ, &sfinfo)))
    {
        fprintf(stderr, "Couldn't open sample file: %s\n", sf_strerror(NULL));
        exit(1);
    }
    sampleRate = sfinfo.samplerate;

    PaStreamCallbackFlags statusFlags;
    memset(&statusFlags, 0, sizeof(statusFlags));
    short buf[CHUNKSIZE];
    if (sfinfo.channels == 1)
    {
        while (sf_read_short(wav, buf, CHUNKSIZE) == CHUNKSIZE)
        {
            callback(buf, NULL, CHUNKSIZE, NULL, statusFlags, NULL);
        }
    }
    else
    {
        int i;
        while (1)
        {
            short frame[sfinfo.channels];
            for (i = 0; i < CHUNKSIZE; i++)
            {
                if (sf_readf_short(wav, frame, 1) == 0)
                {
                    break;
                }
                buf[i] = frame[0];
            }
            if (i != CHUNKSIZE)
            {
                break;
            }
            callback(buf, NULL, CHUNKSIZE, NULL, statusFlags, NULL);
        }
    }

    sf_close(wav);
}