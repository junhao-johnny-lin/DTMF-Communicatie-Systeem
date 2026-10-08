#ifndef DTMF_H
#define DTMF_H

#include <stdint.h>

#define SAMPLE_RATE    16000
#define DURATION_MS    200

/* aantal mono samples */
#define NUM_SAMPLES ((SAMPLE_RATE * DURATION_MS) / 1000)

/* stereo buffer */
extern int16_t audio_buffer[NUM_SAMPLES * 2];

void generate_dtmf(float f1, float f2);

#endif