#include "cyhal.h"
#include "cybsp.h"

#include "cy8ckit_028_tft.h"
#include "GUI.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "goertzel.h"

#define FRAME_SIZE                  (1024)
#define SAMPLE_RATE_HZ              (8000u)
#define DECIMATION_RATE             (64u)
#define AUDIO_SYS_CLOCK_HZ          (24576000u)

#define PDM_DATA                    P10_5
#define PDM_CLK                     P10_4

#define AMPLITUDE_THRESHOLD         (20u)
#define GOERTZEL_MIN_ENERGY         (1000000.0f)
#define DOMINANCE_RATIO             (2.5f)

typedef enum
{
    SCREEN_WAITING,
    SCREEN_DETECTED
} screen_state_t;

void pdm_pcm_isr_handler(void *arg, cyhal_pdm_pcm_event_t event);
void clock_init(void);

char detect_dtmf(int16_t *audio_frame, uint32_t frame_size);
char next_digit(char digit);

void tft_init_screen(void);
void tft_show_waiting(void);
void tft_show_detected_and_reply(char detected, char reply);

volatile bool pdm_pcm_flag = false;

cyhal_pdm_pcm_t pdm_pcm;
cyhal_clock_t audio_clock;
cyhal_clock_t pll_clock;

const cyhal_pdm_pcm_cfg_t pdm_pcm_cfg =
{
    .sample_rate     = SAMPLE_RATE_HZ,
    .decimation_rate = DECIMATION_RATE,
    .mode            = CYHAL_PDM_PCM_MODE_LEFT,
    .word_length     = 16,
    .left_gain       = 0,
    .right_gain      = 0,
};

static const float low_freqs[4] =
{
    697.0f, 770.0f, 852.0f, 941.0f
};

static const float high_freqs[4] =
{
    1209.0f, 1336.0f, 1477.0f, 1633.0f
};

static const char dtmf_map[4][4] =
{
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

int main(void)
{
    cy_rslt_t result;
    int16_t audio_frame[FRAME_SIZE] = {0};

    char last_detected = '\0';
    uint32_t stable_count = 0;

    char displayed_digit = '\0';
    screen_state_t screen_state = SCREEN_WAITING;

    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    __enable_irq();

    clock_init();

    tft_init_screen();
    tft_show_waiting();

    cyhal_gpio_init(CYBSP_USER_LED,
                    CYHAL_GPIO_DIR_OUTPUT,
                    CYHAL_GPIO_DRIVE_STRONG,
                    CYBSP_LED_STATE_OFF);

    result = cyhal_pdm_pcm_init(&pdm_pcm,
                                PDM_DATA,
                                PDM_CLK,
                                &audio_clock,
                                &pdm_pcm_cfg);

    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    cyhal_pdm_pcm_register_callback(&pdm_pcm, pdm_pcm_isr_handler, NULL);

    cyhal_pdm_pcm_enable_event(&pdm_pcm,
                               CYHAL_PDM_PCM_ASYNC_COMPLETE,
                               CYHAL_ISR_PRIORITY_DEFAULT,
                               true);

    cyhal_pdm_pcm_start(&pdm_pcm);

    result = cyhal_pdm_pcm_read_async(&pdm_pcm,
                                      audio_frame,
                                      FRAME_SIZE);

    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    for (;;)
    {
        if (pdm_pcm_flag)
        {
            pdm_pcm_flag = false;

            char detected = detect_dtmf(audio_frame, FRAME_SIZE);

            if (detected != '\0')
            {
                if (detected == last_detected)
                {
                    stable_count++;
                }
                else
                {
                    last_detected = detected;
                    stable_count = 1;
                }

                if (stable_count >= 2)
                {
                    char reply = next_digit(detected);

                    cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_ON);

                    if (screen_state != SCREEN_DETECTED ||
                        displayed_digit != detected)
                    {
                        displayed_digit = detected;
                        screen_state = SCREEN_DETECTED;

                        tft_show_detected_and_reply(detected, reply);
                    }
                }
            }
            else
            {
                last_detected = '\0';
                stable_count = 0;
                displayed_digit = '\0';

                cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_OFF);

                if (screen_state != SCREEN_WAITING)
                {
                    screen_state = SCREEN_WAITING;
                    tft_show_waiting();
                }
            }

            result = cyhal_pdm_pcm_read_async(&pdm_pcm,
                                              audio_frame,
                                              FRAME_SIZE);

            if (result != CY_RSLT_SUCCESS)
            {
                CY_ASSERT(0);
            }
        }

        cyhal_syspm_sleep();
    }
}

char detect_dtmf(int16_t *audio_frame, uint32_t frame_size)
{
    float low_energy[4];
    float high_energy[4];

    uint32_t abs_sum = 0;

    for (uint32_t i = 0; i < frame_size; i++)
    {
        abs_sum += abs(audio_frame[i]);
    }

    uint32_t avg_amplitude = abs_sum / frame_size;

    if (avg_amplitude < AMPLITUDE_THRESHOLD)
    {
        return '\0';
    }

    for (int i = 0; i < 4; i++)
    {
        low_energy[i] = goertzel(audio_frame,
                                 (int)frame_size,
                                 low_freqs[i],
                                 (float)SAMPLE_RATE_HZ);
    }

    for (int i = 0; i < 4; i++)
    {
        high_energy[i] = goertzel(audio_frame,
                                  (int)frame_size,
                                  high_freqs[i],
                                  (float)SAMPLE_RATE_HZ);
    }

    int low_index = 0;
    int high_index = 0;

    for (int i = 1; i < 4; i++)
    {
        if (low_energy[i] > low_energy[low_index])
        {
            low_index = i;
        }
    }

    for (int i = 1; i < 4; i++)
    {
        if (high_energy[i] > high_energy[high_index])
        {
            high_index = i;
        }
    }

    float low_max = low_energy[low_index];
    float high_max = high_energy[high_index];

    float low_second = 0.0f;
    float high_second = 0.0f;

    for (int i = 0; i < 4; i++)
    {
        if (i != low_index && low_energy[i] > low_second)
        {
            low_second = low_energy[i];
        }
    }

    for (int i = 0; i < 4; i++)
    {
        if (i != high_index && high_energy[i] > high_second)
        {
            high_second = high_energy[i];
        }
    }

    if (low_max < GOERTZEL_MIN_ENERGY ||
        high_max < GOERTZEL_MIN_ENERGY)
    {
        return '\0';
    }

    if ((low_second > 0.0f &&
         (low_max / low_second) < DOMINANCE_RATIO) ||
        (high_second > 0.0f &&
         (high_max / high_second) < DOMINANCE_RATIO))
    {
        return '\0';
    }

    return dtmf_map[low_index][high_index];
}

char next_digit(char digit)
{
    switch (digit)
    {
        case '1': return '2';
        case '2': return '3';
        case '3': return '4';
        case '4': return '5';
        case '5': return '6';
        case '6': return '7';
        case '7': return '8';
        case '8': return '9';
        case '9': return 'A';
        case 'A': return 'B';
        case 'B': return 'C';
        case 'C': return 'D';
        case 'D': return '#';
        case '#': return '1';
        case '0': return '1';
        case '*': return '1';
        default:  return '5';
    }
}

void tft_init_screen(void)
{
    cy8ckit_028_tft_init(NULL, NULL, NULL, NULL);
    GUI_Init();

    GUI_SetBkColor(GUI_BLACK);
    GUI_Clear();

    GUI_SetFont(&GUI_Font24_ASCII);
    GUI_SetColor(GUI_WHITE);
    GUI_DispStringHCenterAt("DTMF ONTVANGER", 160, 15);

    GUI_SetFont(&GUI_Font16_ASCII);
    GUI_SetColor(GUI_LIGHTGRAY);
    GUI_DispStringHCenterAt("Wacht op DTMF signaal", 160, 65);

    GUI_SetColor(GUI_WHITE);
    GUI_DrawRoundedRect(45, 95, 275, 180, 10);

    GUI_SetFont(&GUI_Font16_ASCII);
    GUI_SetColor(GUI_LIGHTGRAY);
    GUI_DispStringHCenterAt("RX = ontvangen, TX = antwoord", 160, 210);
}

void tft_show_waiting(void)
{
    GUI_SetColor(GUI_BLACK);
    GUI_FillRect(46, 96, 274, 179);

    GUI_SetFont(&GUI_Font24_ASCII);
    GUI_SetColor(GUI_DARKGRAY);
    GUI_DispStringHCenterAt("WACHTEN", 160, 122);

    GUI_SetColor(GUI_BLACK);
    GUI_FillRect(0, 200, 319, 235);

    GUI_SetFont(&GUI_Font16_ASCII);
    GUI_SetColor(GUI_LIGHTGRAY);
    GUI_DispStringHCenterAt("RX = ontvangen, TX = antwoord", 160, 210);
}

void tft_show_detected_and_reply(char detected, char reply)
{
    char rx_text[6];
    char tx_text[6];

    rx_text[0] = 'R';
    rx_text[1] = 'X';
    rx_text[2] = ':';
    rx_text[3] = ' ';
    rx_text[4] = detected;
    rx_text[5] = '\0';

    tx_text[0] = 'T';
    tx_text[1] = 'X';
    tx_text[2] = ':';
    tx_text[3] = ' ';
    tx_text[4] = reply;
    tx_text[5] = '\0';

    GUI_SetColor(GUI_BLACK);
    GUI_FillRect(46, 96, 274, 179);

    GUI_SetFont(&GUI_Font32B_ASCII);
    GUI_SetColor(GUI_YELLOW);
    GUI_DispStringHCenterAt(rx_text, 160, 105);

    GUI_SetFont(&GUI_Font24_ASCII);
    GUI_SetColor(GUI_CYAN);
    GUI_DispStringHCenterAt(tx_text, 160, 150);

    GUI_SetColor(GUI_BLACK);
    GUI_FillRect(0, 200, 319, 235);

    GUI_SetFont(&GUI_Font16_ASCII);
    GUI_SetColor(GUI_GREEN);
    GUI_DispStringHCenterAt("DTMF correct gedetecteerd", 160, 210);
}

void pdm_pcm_isr_handler(void *arg, cyhal_pdm_pcm_event_t event)
{
    (void)arg;
    (void)event;

    pdm_pcm_flag = true;
}

void clock_init(void)
{
    cyhal_clock_reserve(&pll_clock, &CYHAL_CLOCK_PLL[0]);
    cyhal_clock_set_frequency(&pll_clock, AUDIO_SYS_CLOCK_HZ, NULL);
    cyhal_clock_set_enabled(&pll_clock, true, true);

    cyhal_clock_reserve(&audio_clock, &CYHAL_CLOCK_HF[1]);
    cyhal_clock_set_source(&audio_clock, &pll_clock);
    cyhal_clock_set_enabled(&audio_clock, true, true);
}