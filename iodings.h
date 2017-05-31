#pragma once
#include <portaudio.h>
#include <stdint.h>

// apparently M_PI was removed in C99
#define PI 3.14159265358979323846

// Chunk size must be at least twice the maximum frequency we want to detect (Nyquist frequency)
#define CHUNKSIZE 4096

// Sample width and corresponding PortAudio type
#define SAMPLE int16_t
#define FORMAT paInt16

// Sample rate of input data. Automatically set to the default for the default input device.
double sampleRate;

// don't count low-frequency bins (hz)
#define FREQ_MIN 600

// how much to adapt the original threshold to the observed one over time
#define THRESHOLD_ADAPT 0.001

// Maximum size for pattern. 64 @ 44100 Hz gives ~6.0 seconds.
#define PATTERN_SIZE 64

// Maximum number of bits that can differ in the canonical and observed patterns for the sound to be recognized.
#define LEVENSHTEIN_DIFF 5
