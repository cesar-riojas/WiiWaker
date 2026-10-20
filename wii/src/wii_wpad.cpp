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
        float roll, pitch, yaw;
    } motion_plus;
    struct {
        uint8_t type;
        union {
            struct {
                uint32_t btns_h;
                vec3w_t accel;
                float js_mag;
                float js_ang;
            } nunchuk;
        };
    } exp;
} WPADData;

#define WPAD_BUTTON_A     0x0800
#define WPAD_BUTTON_B     0x0400
#define WPAD_BUTTON_1     0x0200
#define WPAD_BUTTON_2     0x0100
#define WPAD_BUTTON_PLUS  0x0010
#define WPAD_BUTTON_MINUS 0x1000
#define WPAD_BUTTON_HOME  0x8000
#define WPAD_BUTTON_Z     0x2000
#define WPAD_BUTTON_C     0x4000
#define WPAD_CHAN_0       0
#endif

namespace {

static WiiTactMotionState s_tactState = {
    WII_TACT_DIR_CENTER,
    0.8f,
    false,
    WII_TACT_METER_4_4
};

static WiiAimingOffset s_aimingOffset = { 0.0f, 0.0f, 0.0f };

// Baton Gesture Tuning Constants
static const float FLICK_THRESHOLD_GYRO   = 120.0f; // deg/sec
static const float FLICK_MAX_GYRO         = 650.0f; // deg/sec (for 100% volume)
static const float REBOUND_DEADZONE_GYRO  = 40.0f;  // deg/sec return to center

// Low-pass filter smoothing factor
static const float GYRO_SMOOTHING         = 0.25f;
static float s_smoothedPitchRate = 0.0f;
static float s_smoothedYawRate   = 0.0f;

static uint32_t s_rumbleStopTick = 0;

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
        // Read MotionPlus angular rates (pitch/yaw in deg/s)
        pitch_rate = pad->motion_plus.pitch;
        yaw_rate   = pad->motion_plus.yaw;

        // Read Nunchuk analog stick X for time signature
        if (pad->exp.type == WPAD_EXP_NUNCHUK) {
            float ang = pad->exp.nunchuk.js_ang;
            float mag = pad->exp.nunchuk.js_mag;
            if (mag > 0.4f) {
                nunchuk_x = mag * std::sin(ang * (3.14159265f / 180.0f));
            }
        }
    }
#endif

    // Apply exponential smoothing to filter noise
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

    // Calculate total flick magnitude
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
    if (pad) {
        // Gyro rates to angular camera deltas
        const float SENSITIVITY = 0.00065f;
        s_aimingOffset.delta_pitch = pad->motion_plus.pitch * SENSITIVITY;
        s_aimingOffset.delta_yaw   = pad->motion_plus.yaw * SENSITIVITY;
        s_aimingOffset.delta_roll  = pad->motion_plus.roll * SENSITIVITY;
    }
#endif

    *out_offset = s_aimingOffset;
}

void wii_haptic_beat_pulse(uint32_t duration_ms) {
#if defined(GEKKO) || defined(__wii__)
    WPAD_Rumble(WPAD_CHAN_0, 1);
    // Convert ms to Wii timer ticks
    s_rumbleStopTick = gettime() + (tb_ticks_per_sec / 1000) * duration_ms;
#endif
}

} // extern "C"
