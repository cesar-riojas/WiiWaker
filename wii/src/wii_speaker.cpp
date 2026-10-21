/**
 * wii_speaker.cpp - Wii Remote Speaker Audio Synthesis & Streaming
 */

#include "wii_speaker.h"
#include <cmath>
#include <cstring>
#include <algorithm>

#if defined(GEKKO) || defined(__wii__)
#include <gccore.h>
#include <wiiuse/wpad.h>
#endif

namespace {

static const int SAMPLE_RATE = 3000; // Wiimote speaker native rate
static const int MAX_PCM_SAMPLES = 600; // ~200ms maximum SFX duration

static int16_t s_activePcm[MAX_PCM_SAMPLES];
static int s_activeSampleCount = 0;
static int s_playHead = 0;
static bool s_speakerEnabled = false;

#if defined(GEKKO) || defined(__wii__)
static WPADEncStatus s_encStatus;
#endif

void generate_tact_beat(int16_t* out, int* count) {
    // 25ms wooden metronome click (75 samples)
    int n = 75;
    for (int i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float env = std::exp(-t * 140.0f);
        float wave = std::sin(2.0f * 3.14159265f * 1850.0f * t);
        out[i] = (int16_t)(wave * env * 24000.0f);
    }
    *count = n;
}

void generate_sword_slash(int16_t* out, int* count) {
    // 120ms whoosh (360 samples)
    int n = 360;
    uint32_t lfsr = 0xACE1u;
    for (int i = 0; i < n; i++) {
        float progress = (float)i / n;
        float env = std::sin(progress * 3.14159265f); // Hanning-style arch envelope
        // Pseudo-random white noise
        lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
        float noise = ((float)(lfsr & 0xFFFF) / 32768.0f) - 1.0f;
        // Pitch swept carrier
        float freq = 400.0f + 800.0f * (1.0f - progress);
        float tone = std::sin(2.0f * 3.14159265f * freq * ((float)i / SAMPLE_RATE));
        float mix = (noise * 0.7f + tone * 0.3f) * env;
        out[i] = (int16_t)(mix * 22000.0f);
    }
    *count = n;
}

void generate_parry_clang(int16_t* out, int* count) {
    // 100ms metallic ring (300 samples)
    int n = 300;
    for (int i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float env = std::exp(-t * 45.0f);
        float w1 = std::sin(2.0f * 3.14159265f * 920.0f * t);
        float w2 = std::sin(2.0f * 3.14159265f * 1380.0f * t);
        float mix = (w1 * 0.6f + w2 * 0.4f) * env;
        out[i] = (int16_t)(mix * 26000.0f);
    }
    *count = n;
}

void generate_item_equip(int16_t* out, int* count) {
    // 140ms melodic two-tone chime (420 samples): C6 (1046Hz) -> G6 (1568Hz)
    int n = 420;
    int half = n / 2;
    for (int i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float freq = (i < half) ? 1046.5f : 1568.0f;
        float note_t = (i < half) ? t : (float)(i - half) / SAMPLE_RATE;
        float env = std::exp(-note_t * 28.0f);
        float tone = std::sin(2.0f * 3.14159265f * freq * t);
        out[i] = (int16_t)(tone * env * 24000.0f);
    }
    *count = n;
}

void generate_recenter(int16_t* out, int* count) {
    // 80ms double confirmation pip (240 samples)
    int n = 240;
    for (int i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float tone = 0.0f;
        if (i < 80) { // Pip 1: 1318Hz
            float env = std::exp(-t * 50.0f);
            tone = std::sin(2.0f * 3.14159265f * 1318.5f * t) * env;
        } else if (i >= 120 && i < 200) { // Pip 2: 1760Hz
            float pt = (float)(i - 120) / SAMPLE_RATE;
            float env = std::exp(-pt * 50.0f);
            tone = std::sin(2.0f * 3.14159265f * 1760.0f * pt) * env;
        }
        out[i] = (int16_t)(tone * 22000.0f);
    }
    *count = n;
}

} // namespace

extern "C" {

void wii_speaker_init(void) {
    s_activeSampleCount = 0;
    s_playHead = 0;
    s_speakerEnabled = false;

#if defined(GEKKO) || defined(__wii__)
    s32 res = WPAD_ControlSpeaker(WPAD_CHAN_0, 1);
    s_speakerEnabled = (res == WPAD_ERR_NONE);
    std::memset(&s_encStatus, 0, sizeof(s_encStatus));
#endif
}

void wii_speaker_play(WiiSpeakerSfx sfx) {
    s_playHead = 0;
    s_activeSampleCount = 0;

    switch (sfx) {
        case WII_SFX_TACT_BEAT:
            generate_tact_beat(s_activePcm, &s_activeSampleCount);
            break;
        case WII_SFX_SWORD_SLASH:
            generate_sword_slash(s_activePcm, &s_activeSampleCount);
            break;
        case WII_SFX_PARRY_CLANG:
            generate_parry_clang(s_activePcm, &s_activeSampleCount);
            break;
        case WII_SFX_ITEM_EQUIP:
            generate_item_equip(s_activePcm, &s_activeSampleCount);
            break;
        case WII_SFX_RECENTER:
            generate_recenter(s_activePcm, &s_activeSampleCount);
            break;
        default:
            break;
    }
}

void wii_speaker_update(void) {
    if (s_playHead >= s_activeSampleCount) return;

#if defined(GEKKO) || defined(__wii__)
    // Each packet to the Wii Remote speaker typically contains 20 encoded ADPCM bytes (40 PCM samples)
    const int CHUNK_SAMPLES = 40;
    int remaining = s_activeSampleCount - s_playHead;
    int chunk = std::min(CHUNK_SAMPLES, remaining);

    u8 encBuffer[32];
    std::memset(encBuffer, 0, sizeof(encBuffer));

    WPAD_EncodeData(&s_encStatus, 0, &s_activePcm[s_playHead], chunk, encBuffer);
    WPAD_SendStreamData(WPAD_CHAN_0, encBuffer, 20);

    s_playHead += chunk;
#else
    s_playHead += 40;
#endif
}

} // extern "C"
