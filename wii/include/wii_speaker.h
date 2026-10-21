/**
 * wii_speaker.h - Wii Remote Speaker Audio Subsystem for WiiWaker
 *
 * Implements authentic Skyward Sword / Twilight Princess Wiimote speaker
 * audio feedback for sword swings, parries, radial item equip, baton conducting,
 * and gyro recentering.
 */

#ifndef WII_SPEAKER_H
#define WII_SPEAKER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WII_SFX_SWORD_SLASH = 0,
    WII_SFX_PARRY_CLANG,
    WII_SFX_ITEM_EQUIP,
    WII_SFX_TACT_BEAT,
    WII_SFX_RECENTER,
} WiiSpeakerSfx;

// Initialize speaker subsystem
void wii_speaker_init(void);

// Play a sound effect through the Wii Remote speaker
void wii_speaker_play(WiiSpeakerSfx sfx);

// Update speaker streaming buffer per frame
void wii_speaker_update(void);

#ifdef __cplusplus
}
#endif

#endif // WII_SPEAKER_H
