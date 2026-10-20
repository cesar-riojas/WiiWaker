/**
 * WiiWaker - Wii Remote & MotionPlus Controller & Baton Subsystem
 */

#include "wii_wpad.h"
#include <cmath>
#include <algorithm>
#include <cstring>

#if defined(GEKKO) || defined(__wii__)
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <ogc/lwp_watchdog.h>
#else
// Stub types for host compilation / checks
typedef struct {
    float x, y, z;
} vec3w_t;

typedef struct {
    float pitch, roll, yaw;
} orient_t;

typedef struct {
    uint32_t btns_h;
    uint32_t btns_d;
    uint32_t btns_u;
    vec3w_t accel;
    orient_t orient;
    struct {
        int type;
        union {
            struct {
                struct {
                    float ang;
                    float mag;
                } js;
            } nunchuk;
            struct {
                short rx, ry, rz;
            } mp;
        };
    } exp;
} WPADData;

#define WPAD_EXP_NONE         0
#define WPAD_EXP_NUNCHUK      1
#define WPAD_EXP_MOTION_PLUS  5
#define WPAD_CHAN_0           0
#endif

namespace {

static WiiTactMotionState s_tactState = {
    WII_TACT_DIR_CENTER,
    0.8f,
    false,
    WII_TACT_METER_4_4
};

static WiiAimingOffset s_aimingOffset = { 0.0f, 0.0f, 0.0f };

// Baton Gesture Tuning Constants (MotionPlus raw units / deg/sec)
static const float FLICK_THRESHOLD_GYRO   = 140.0f;
static const float FLICK_MAX_GYRO         = 700.0f;
static const float REBOUND_DEADZONE_GYRO  = 50.0f;

// Low-pass filter smoothing factor
static const float GYRO_SMOOTHING         = 0.30f;
static float s_smoothedPitchRate = 0.0f;
static float s_smoothedYawRate   = 0.0f;

#if defined(GEKKO) || defined(__wii__)
static u64 s_rumbleStopTick = 0;
#else
static uint64_t s_rumbleStopTick = 0;
#endif

} // namespace

extern "C" {

void wii_wpad_init(void) {
#if defined(GEKKO) || defined(__wii__)
    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
    WPAD_SetVRes(WPAD_CHAN_0, 640, 480);
    WPAD_SetMotionPlus(WPAD_CHAN_0, 1);
#endif
    std::memset(&s_tactState, 0, sizeof(s_tactState));
    s_tactState.current_direction = WII_TACT_DIR_CENTER;
    s_tactState.stroke_intensity = 0.8f;
    s_tactState.meter = WII_TACT_METER_4_4;
}

void wii_wpad_update(void) {
#if defined(GEKKO) || defined(__wii__)
    WPAD_ScanPads();

    // Handle timed haptic rumble
    if (s_rumbleStopTick > 0) {
        if (gettime() >= s_rumbleStopTick) {
            WPAD_Rumble(WPAD_CHAN_0, 0);
            s_rumbleStopTick = 0;
        }
    }
#endif
}

void wii_tact_gesture_update(float metronome_beat_rate) {
    float pitch_rate = 0.0f;
    float yaw_rate   = 0.0f;
    float nunchuk_x  = 0.0f;

#if defined(GEKKO) || defined(__wii__)
    WPADData* pad = WPAD_Data(WPAD_CHAN_0);
    if (pad) {
        // Read MotionPlus angular rates
        if (pad->exp.type == EXP_MOTION_PLUS) {
            pitch_rate = (float)pad->exp.mp.rx;
            yaw_rate   = (float)pad->exp.mp.rz;
        } else {
            // Fallback: estimate stroke from accelerometer tilt / acceleration
            pitch_rate = (float)pad->accel.y;
            yaw_rate   = (float)pad->accel.x;
        }

        // Read Nunchuk analog stick X for time signature
        if (pad->exp.type == EXP_NUNCHUK) {
            float ang = pad->exp.nunchuk.js.ang;
            float mag = pad->exp.nunchuk.js.mag;
            if (mag > 0.4f) {
                nunchuk_x = mag * std::sin(ang * (3.14159265f / 180.0f));
            }
        }
    }
#endif

    // Apply exponential smoothing to filter sensor noise
    s_smoothedPitchRate = (s_smoothedPitchRate * (1.0f - GYRO_SMOOTHING)) + (pitch_rate * GYRO_SMOOTHING);
    s_smoothedYawRate   = (s_smoothedYawRate * (1.0f - GYRO_SMOOTHING)) + (yaw_rate * GYRO_SMOOTHING);

    // Evaluate Time Signature (Meter) from Nunchuk stick
    if (nunchuk_x < -0.4f) {
        s_tactState.meter = WII_TACT_METER_3_4; // 3/4 time (Left)
    } else if (nunchuk_x > 0.4f) {
        s_tactState.meter = WII_TACT_METER_6_4; // 6/4 time (Right)
    } else {
        s_tactState.meter = WII_TACT_METER_4_4; // 4/4 time (Center/Neutral)
    }

    // Calculate total stroke speed
    float flick_speed = std::sqrt(s_smoothedPitchRate * s_smoothedPitchRate +
                                  s_smoothedYawRate * s_smoothedYawRate);

    // Dynamic Volume: map stroke velocity to 0.35 (pianissimo) - 1.0 (fortissimo)
    if (flick_speed > FLICK_THRESHOLD_GYRO) {
        float dynamicVol = 0.35f + 0.65f * std::clamp(
            (flick_speed - FLICK_THRESHOLD_GYRO) / (FLICK_MAX_GYRO - FLICK_THRESHOLD_GYRO),
            0.0f, 1.0f
        );
        s_tactState.stroke_intensity = dynamicVol;

        // Classify Baton Direction based on predominant axis of motion
        if (std::abs(s_smoothedPitchRate) > std::abs(s_smoothedYawRate)) {
            // Vertical flick (Up / Down)
            if (s_smoothedPitchRate > 0.0f) {
                s_tactState.current_direction = WII_TACT_DIR_UP;    // High Note
            } else {
                s_tactState.current_direction = WII_TACT_DIR_DOWN;  // Downbeat / Low Note
            }
        } else {
            // Horizontal sweep (Left / Right)
            if (s_smoothedYawRate > 0.0f) {
                s_tactState.current_direction = WII_TACT_DIR_RIGHT; // Outward sweep
            } else {
                s_tactState.current_direction = WII_TACT_DIR_LEFT;  // Inward sweep
            }
        }

        // Trigger haptic rumble click at the ictus (apex of stroke)
        if (!s_tactState.is_ictus_struck) {
            wii_haptic_beat_pulse(18); // 18ms tactile pulse
            s_tactState.is_ictus_struck = true;
        }
    } else if (flick_speed < REBOUND_DEADZONE_GYRO) {
        // Hand decelerated back toward center rest
        s_tactState.current_direction = WII_TACT_DIR_CENTER;
        s_tactState.is_ictus_struck = false;
    }
}

WiiTactDirection wii_tact_get_direction(void) {
    return s_tactState.current_direction;
}

float wii_tact_get_volume(void) {
    return s_tactState.stroke_intensity;
}

WiiTactMeter wii_tact_get_meter(void) {
    return s_tactState.meter;
}

void wii_aiming_get_offset(WiiAimingOffset* out_offset) {
    if (!out_offset) return;

#if defined(GEKKO) || defined(__wii__)
    WPADData* pad = WPAD_Data(WPAD_CHAN_0);
    if (pad && pad->exp.type == EXP_MOTION_PLUS) {
        const float SENSITIVITY = 0.00065f;
        s_aimingOffset.delta_pitch = (float)pad->exp.mp.rx * SENSITIVITY;
        s_aimingOffset.delta_yaw   = (float)pad->exp.mp.rz * SENSITIVITY;
        s_aimingOffset.delta_roll  = (float)pad->exp.mp.ry * SENSITIVITY;
    }
#endif

    *out_offset = s_aimingOffset;
}

void wii_haptic_beat_pulse(uint32_t duration_ms) {
#if defined(GEKKO) || defined(__wii__)
    WPAD_Rumble(WPAD_CHAN_0, 1);
    s_rumbleStopTick = gettime() + millisecs_to_ticks(duration_ms);
#endif
}

} // extern "C"
