/**
 * WiiWaker - Skyward Sword & Twilight Princess Motion Combat Engine
 *
 * Implements 1:1 directional slashing (MotionPlus pitch/yaw/thrust)
 * and Twilight Princess accelerometer shake / Nunchuk spin attack.
 */

#include "wii_motion_sword.h"
#include "wii_wpad.h"
#include "wii_speaker.h"
#include <cmath>
#include <cstring>
#include <algorithm>

#if defined(GEKKO) || defined(__wii__)
#include <gccore.h>
#include <wiiuse/wpad.h>
#endif

namespace {

static WiiSwordAttackType s_latchedAttack = WII_SWORD_NONE;
static uint32_t s_attackCooldownFrames = 0;

// MotionPlus Thresholds (angular velocity in raw units / deg/sec)
static const float SWING_VELOCITY_MIN      = 180.0f;
static const float SWING_VELOCITY_DIAG_RAT = 0.55f;  // Ratio between axes for diagonal
static const float THRUST_ACCEL_Z_MIN      = 1.85f;  // Forward thrust spike

// Accelerometer / Nunchuk thresholds (Twilight Princess style)
static const float WIIMOTE_SHAKE_ACCEL_MIN = 2.2f;
static const float NUNCHUK_SHAKE_ACCEL_MIN = 2.0f;

static float s_prevNunchukMag = 1.0f;
static float s_prevWiimoteMag = 1.0f;

} // namespace

extern "C" {

void wii_sword_init(void) {
    s_latchedAttack = WII_SWORD_NONE;
    s_attackCooldownFrames = 0;
    s_prevNunchukMag = 1.0f;
    s_prevWiimoteMag = 1.0f;
}

void wii_sword_update(void) {
    if (s_attackCooldownFrames > 0) {
        s_attackCooldownFrames--;
        return;
    }

#if defined(GEKKO) || defined(__wii__)
    WPADData* wpad = WPAD_Data(WPAD_CHAN_0);
    if (!wpad || wpad->err != WPAD_ERR_NONE) return;

    // 1. Check Nunchuk Shake for Spin Attack (Twilight Princess style)
    if (wpad->exp.type == EXP_NUNCHUK) {
        // Nunchuk accelerometer vector
        float nx = (float)wpad->exp.nunchuk.accel.x;
        float ny = (float)wpad->exp.nunchuk.accel.y;
        float nz = (float)wpad->exp.nunchuk.accel.z;
        float n_mag = std::sqrt(nx*nx + ny*ny + nz*nz);
        float delta_n = std::abs(n_mag - s_prevNunchukMag);
        s_prevNunchukMag = n_mag;

        if (delta_n > NUNCHUK_SHAKE_ACCEL_MIN) {
            s_latchedAttack = WII_SWORD_SPIN_ATTACK;
            s_attackCooldownFrames = 18; // 300ms cooldown
            wii_haptic_beat_pulse(35);
            return;
        }
    }

    // 2. MotionPlus 1:1 Directional Slashing (Skyward Sword mechanics)
    if (wpad->exp.type == EXP_MOTION_PLUS) {
        float pitch_rate = (float)wpad->exp.mp.rx; // Vertical pitch velocity
        float yaw_rate   = (float)wpad->exp.mp.rz; // Horizontal yaw velocity
        float roll_rate  = (float)wpad->exp.mp.ry;

        float abs_pitch = std::abs(pitch_rate);
        float abs_yaw   = std::abs(yaw_rate);
        float speed     = std::sqrt(pitch_rate * pitch_rate + yaw_rate * yaw_rate);

        // Check Forward Thrust
        float z_accel = (float)wpad->accel.z;
        if (z_accel > THRUST_ACCEL_Z_MIN && speed < SWING_VELOCITY_MIN * 0.7f) {
            s_latchedAttack = WII_SWORD_THRUST;
            s_attackCooldownFrames = 14;
            wii_haptic_beat_pulse(20);
            return;
        }

        // Check Directional Slashing
        if (speed >= SWING_VELOCITY_MIN) {
            float ratio = (abs_pitch > 0.001f) ? (abs_yaw / abs_pitch) : 10.0f;

            if (ratio > SWING_VELOCITY_DIAG_RAT && ratio < (1.0f / SWING_VELOCITY_DIAG_RAT)) {
                // Diagonal slash (Kesa-giri)
                s_latchedAttack = WII_SWORD_DIAGONAL;
            } else if (abs_pitch > abs_yaw) {
                // Vertical chop (Downward or upward)
                s_latchedAttack = WII_SWORD_VERTICAL_CHOP;
            } else {
                // Horizontal slash
                if (yaw_rate > 0.0f) {
                    s_latchedAttack = WII_SWORD_SLASH_RIGHT;
                } else {
                    s_latchedAttack = WII_SWORD_SLASH_LEFT;
                }
            }
            s_attackCooldownFrames = 12; // 200ms swing interval
            wii_speaker_play(WII_SFX_SWORD_SLASH);
            wii_haptic_beat_pulse(22);
            return;
        }
    } else {
        // 3. Fallback: Twilight Princess accelerometer flick
        float wx = (float)wpad->accel.x;
        float wy = (float)wpad->accel.y;
        float wz = (float)wpad->accel.z;
        float w_mag = std::sqrt(wx*wx + wy*wy + wz*wz);
        float delta_w = std::abs(w_mag - s_prevWiimoteMag);
        s_prevWiimoteMag = w_mag;

        if (delta_w > WIIMOTE_SHAKE_ACCEL_MIN) {
            if (std::abs(wy) > std::abs(wx)) {
                s_latchedAttack = WII_SWORD_VERTICAL_CHOP;
            } else if (wx > 0.0f) {
                s_latchedAttack = WII_SWORD_SLASH_RIGHT;
            } else {
                s_latchedAttack = WII_SWORD_SLASH_LEFT;
            }
            s_attackCooldownFrames = 12;
            wii_speaker_play(WII_SFX_SWORD_SLASH);
            wii_haptic_beat_pulse(20);
            return;
        }
    }
#endif
}

WiiSwordAttackType wii_sword_get_attack(void) {
    return s_latchedAttack;
}

void wii_sword_consume_attack(void) {
    s_latchedAttack = WII_SWORD_NONE;
}

bool wii_sword_is_spin_attack(void) {
    return (s_latchedAttack == WII_SWORD_SPIN_ATTACK);
}

} // extern "C"
