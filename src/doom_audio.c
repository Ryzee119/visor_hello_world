#include "PureDOOM/PureDOOM.h"
#include "main.h"
#include "midi/midiplay.h"

static SemaphoreHandle_t doom_audio_semaphore;

static void XAudioCallbackfn(void *pac97Device, void *data)
{
    xSemaphoreGiveFromISR(doom_audio_semaphore, NULL);
}

static int16_t linear_interpolate(int16_t y1, int16_t y2, float t)
{
    return (int16_t)(y1 + t * (y2 - y1));
}

static void resample_audio_stereo(int16_t *input, int input_size, int16_t *output, int output_size)
{
    float ratio = (float)(input_size / 2 - 1) / (output_size / 2 - 1);
    for (int i = 0; i < output_size / 2; i++) {
        float src_index = i * ratio;
        int idx = (int)src_index;
        float frac = src_index - idx;

        int input_idx = idx * 2;

        // Interpolate the left channel
        if (input_idx < input_size - 2) {
            output[i * 2] = linear_interpolate(input[input_idx], input[input_idx + 2], frac);
        } else {
            output[i * 2] = input[input_size - 2];
        }

        // Interpolate the right channel
        if (input_idx + 1 < input_size - 1) {
            output[i * 2 + 1] = linear_interpolate(input[input_idx + 1], input[input_idx + 3], frac);
        } else {
            output[i * 2 + 1] = input[input_size - 1];
        }
    }
}

void dooom_changing_music(void *data, int looping)
{
    if (Midiplay_Load(data, 0xFFFF) == 0)
    {
        printf("Failed to load music\n");
    }
    else
    {
        printf("Loaded music\n");
        Midiplay_Loop(looping);
        Midiplay_Play(1);
    }
}

void dooom_stop_song()
{
    Midiplay_Play(0);
}

void dooom_set_volume(int volume)
{
    Midiplay_SetVolume(volume);
}

void doom_sound_task(void *parameters)
{
    SemaphoreHandle_t *doom_logic_mutex = (SemaphoreHandle_t *)parameters;

    static int16_t resampled_buffer0[2230 * 2]; // 48000/11025 * 512 * 2 channels
    static int16_t resampled_buffer1[2230 * 2]; // 48000/11025 * 512 * 2 channels
    static int16_t midi_buffer[2048];

    Midiplay_Init(DOOM_SAMPLERATE);
    XAudioInit(16, 2, XAudioCallbackfn, NULL);

    memset(resampled_buffer0, 0, sizeof(resampled_buffer0));
    memset(resampled_buffer1, 0, sizeof(resampled_buffer1));
    doom_audio_semaphore = xSemaphoreCreateCounting(2, 0);

    XAudioProvideSamples((unsigned char *)resampled_buffer0, sizeof(resampled_buffer0), 0);
    XAudioProvideSamples((unsigned char *)resampled_buffer1, sizeof(resampled_buffer1), 0);
    XAudioPlay();

    int buffer_index = 0;
    while (1) {
        int16_t *out_buffer = (buffer_index == 0) ? resampled_buffer0 : resampled_buffer1;
        buffer_index ^= 1;

        xSemaphoreTake(doom_audio_semaphore, portMAX_DELAY);

        xSemaphoreTake(*doom_logic_mutex, portMAX_DELAY);
        int16_t *in_buffer = doom_get_sound_buffer();
        xSemaphoreGive(*doom_logic_mutex);

        Midiplay_Output(midi_buffer, 1024);

        // Mix the midi buffer with the audio buffer
        for (uint16_t i = 0; i < 2048; i++) {
            int32_t mixed = (int32_t)in_buffer[i] + (int32_t)midi_buffer[i];
            if (mixed > 32767) {
                mixed = 32767;
            } else if (mixed < -32768) {
                mixed = -32768;
            }
            in_buffer[i] = (int16_t)mixed;
        }

        resample_audio_stereo(in_buffer, 512 * 2, out_buffer, 2230 * 2);
        XAudioProvideSamples((unsigned char *)out_buffer, sizeof(resampled_buffer0), 0);
    }
}
