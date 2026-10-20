#ifndef WII_WPAD_H
#define WII_WPAD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Wind Waker Baton Directions (matching mDoAud_ARM_*_e)
typedef enum {
    WII_TACT_DIR_CENTER = 0,
    WII_TACT_DIR_UP     = 1,
    WII_TACT_DIR_RIGHT  = 2,
    WII_TACT_DIR_LEFT   = 3,
    WII_TACT_DIR_DOWN   = 4,
} WiiTactDirection;

// Time Signature (Meter)
typedef enum {
    WII_TACT_METER_3_4 = 0, // Left
    WII_TACT_METER_4_4 = 1, // Neutral / Center
    WII_TACT_METER_6_4 = 2, // Right
} WiiTactMeter;

// Motion-based baton state
typedef struct {
    WiiTactDirection current_direction;
    float stroke_intensity; // 0.0f to 1.0f (dynamic volume)
    bool is_ictus_struck;   // Triggered at the apex of the flick
    WiiTactMeter meter;     // 3/4, 4/4, or 6/4 time
} WiiTactMotionState;

// First-person gyro aiming offsets
typedef struct {
    float delta_pitch;
    float delta_yaw;
    float delta_roll;
} WiiAimingOffset;

// Initialize Wii Remote and MotionPlus subsystem
void wii_wpad_init(void);

// Poll controllers every frame
void wii_wpad_update(void);

// Baton conducting gesture processing
void wii_tact_gesture_update(float metronome_beat_rate);
WiiTactDirection wii_tact_get_direction(void);
float wii_tact_get_volume(void);
WiiTactMeter wii_tact_get_meter(void);

// First-person gyro aiming for Bow, Boomerang, Hookshot, Telescope
void wii_aiming_get_offset(WiiAimingOffset* out_offset);

// Haptic feedback (trigger a short rumble pulse)
void wii_haptic_beat_pulse(uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif // WII_WPAD_H
