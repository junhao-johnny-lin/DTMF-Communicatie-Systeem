#include "goertzel.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float goertzel(int16_t *data, int N, float freq, float sample_rate)
{
    float s_prev = 0.0f;
    float s_prev2 = 0.0f;

    float normalized_freq = freq / sample_rate;
    float coeff = 2.0f * cosf(2.0f * (float)M_PI * normalized_freq);

    for (int i = 0; i < N; i++)
    {
        float s = (float)data[i] + coeff * s_prev - s_prev2;
        s_prev2 = s_prev;
        s_prev = s;
    }

    return (s_prev2 * s_prev2) + (s_prev * s_prev) - (coeff * s_prev * s_prev2);
}