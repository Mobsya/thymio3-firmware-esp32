#ifndef __AUDIO_TONEURI_H__
#define __AUDIO_TONEURI_H__

extern const char* tone_uri[];

typedef enum {
    TONE_TYPE_A3,
    TONE_TYPE_ALARM,
    TONE_TYPE_B3,
    TONE_TYPE_BAD,
    TONE_TYPE_BEEP,
    TONE_TYPE_BLOP,
    TONE_TYPE_BYE,
    TONE_TYPE_C3,
    TONE_TYPE_D3,
    TONE_TYPE_DETECT,
    TONE_TYPE_E3,
    TONE_TYPE_F3,
    TONE_TYPE_FALL,
    TONE_TYPE_G3,
    TONE_TYPE_GOOD,
    TONE_TYPE_MAGIC,
    TONE_TYPE_NOTIFY,
    TONE_TYPE_PING,
    TONE_TYPE_TICK,
    TONE_TYPE_MAX,
} tone_type_t;

int get_tone_uri_num();

#endif
