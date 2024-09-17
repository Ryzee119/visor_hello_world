#ifndef AUDIO_H
#define AUDIO_H

#include "xbox.h"

//https://github.com/XboxDev/nxdk/blob/master/lib/hal/audio.h

typedef void (*XAudioCallback)(void *pac97Device, void *data);

typedef struct
{
    unsigned int bufferStartAddress;
    unsigned short bufferLengthInSamples; // 0=no smaples
    unsigned short bufferControl;         // b15=1=issue IRQ on completion, b14=1=last in stream
} AC97_DESCRIPTOR __attribute__((aligned(8)));

// represents the current ac97 device
typedef struct
{
    volatile AC97_DESCRIPTOR pcmSpdifDescriptor[32];
    volatile AC97_DESCRIPTOR pcmOutDescriptor[32];
    volatile unsigned int *mmio;
    unsigned char nextDescriptor;
    XAudioCallback callback;
    void *callbackData;
    int sampleSizeInBits;
    int numChannels;
} AC97_DEVICE __attribute__((aligned(8)));

void xbox_audio_init(int sampleSizeInBits, int numChannels, XAudioCallback callback, void *data);
void xbox_audio_play(void);
void xbox_audio_pause(void);
void xbox_audio_provide(unsigned char *buffer, unsigned short bufferLength, int isFinal);

#endif