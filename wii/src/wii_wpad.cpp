/**
 * WiiWaker - Wii Remote & MotionPlus Controller & Baton Subsystem
 */

#include "wii_wpad.h"
#include "wii_motion_sword.h"
#include "wii_item_wheel.h"
#include "wii_speaker.h"
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

static WiiControllerType s_activeController = WII_CONTROLLER_WIIMOTE;

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

// D-Pad Quick Tool Slot: 0 = Left (X), 1 = Up (Y), 2 = Right (Z)
static int s_activeToolSlot = 2; // Default to Right wing (e.g. Telescope / active tool)
static uint32_t s_bHoldFrames = 0;
static uint32_t s_downHoldFrames = 0;
static bool s_radialWheelOpen = false;
static const uint32_t B_HOLD_RADIAL_THRESHOLD = 12; // ~200ms hold for radial selector

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

    wii_sword_init();
    wii_speaker_init();
}

WiiControllerType wii_get_active_controller(void) {
    return s_activeController;
}

void wii_set_active_controller(WiiControllerType type) {
    s_activeController = type;
}

void wii_wpad_update(void) {
#if defined(GEKKO) || defined(__wii__)
    WPAD_ScanPads();

    // Update motion-based sword tracking (Skyward Sword & TP mechanics)
    wii_sword_update();

    // Stream active speaker audio packets to Wii Remote
    wii_speaker_update();

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

        // Trigger haptic rumble click and speaker click at the ictus (apex of stroke)
        if (!s_tactState.is_ictus_struck) {
            wii_haptic_beat_pulse(18); // 18ms tactile pulse
            wii_speaker_play(WII_SFX_TACT_BEAT);
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

void wii_aiming_recenter(void) {
    s_aimingOffset.delta_pitch = 0.0f;
    s_aimingOffset.delta_yaw   = 0.0f;
    s_aimingOffset.delta_roll  = 0.0f;
    s_smoothedPitchRate        = 0.0f;
    s_smoothedYawRate          = 0.0f;
    wii_speaker_play(WII_SFX_RECENTER);
    wii_haptic_beat_pulse(28); // Distinct double-tick calibration feedback
}

void wii_haptic_beat_pulse(uint32_t duration_ms) {
#if defined(GEKKO) || defined(__wii__)
    WPAD_Rumble(WPAD_CHAN_0, 1);
    s_rumbleStopTick = gettime() + millisecs_to_ticks(duration_ms);
#endif
}

void wii_pad_poll(void* pad_status_array) {
#if defined(GEKKO) || defined(__wii__)
    if (!pad_status_array) return;
    PADStatus* status = (PADStatus*)pad_status_array;

    // 1. Check GameCube Controller activity on Port 0
    if (status[0].err == PAD_ERR_NONE) {
        bool gcActive = (status[0].button != 0) ||
                        (std::abs((int)status[0].stickX) > 18) ||
                        (std::abs((int)status[0].stickY) > 18) ||
                        (std::abs((int)status[0].substickX) > 18) ||
                        (std::abs((int)status[0].substickY) > 18) ||
                        (status[0].triggerL > 30) ||
                        (status[0].triggerR > 30);
        if (gcActive) {
            s_activeController = WII_CONTROLLER_GAMECUBE;
        }
    }

    // 2. Poll Wii Remote + Nunchuk
    WPADData* wpad = WPAD_Data(WPAD_CHAN_0);
    if (!wpad || wpad->err != WPAD_ERR_NONE) return;

    u16 btn = 0;
    bool wiiActive = (wpad->btns_h != 0);

    // 1. Context / Action Button
    if (wpad->btns_h & WPAD_BUTTON_A)     btn |= PAD_BUTTON_A;

    // 2. Skyward Sword [B] Button Logic:
    // Tap [B]: Use currently active tool slot (Slot 0 -> X, Slot 1 -> Y, Slot 2 -> Z)
    // Hold [B] (>= 200ms): Opens 8-Way Radial Item Wheel
    float wheel_stick_x = 0.0f;
    float wheel_stick_y = 0.0f;
#if defined(GEKKO) || defined(__wii__)
    if (wpad->exp.type == EXP_NUNCHUK) {
        float mag = wpad->exp.nunchuk.js.mag;
        if (mag > 0.15f) {
            float rad = wpad->exp.nunchuk.js.ang * (3.14159265f / 180.0f);
            wheel_stick_x = std::sin(rad) * mag;
            wheel_stick_y = std::cos(rad) * mag;
        }
    }
#endif

    if (wpad->btns_h & WPAD_BUTTON_B) {
        s_bHoldFrames++;
        if (s_bHoldFrames >= B_HOLD_RADIAL_THRESHOLD) {
            s_radialWheelOpen = true;
            wii_item_wheel_update(true, wheel_stick_x, wheel_stick_y);
        }
    } else {
        if (s_radialWheelOpen) {
            wii_item_wheel_update(false, wheel_stick_x, wheel_stick_y);
            wii_haptic_beat_pulse(20);
        } else if (s_bHoldFrames > 0 && s_bHoldFrames < B_HOLD_RADIAL_THRESHOLD) {
            // Quick tap on [B] -> Fire active item!
            if (s_activeToolSlot == 0)      btn |= PAD_BUTTON_X;
            else if (s_activeToolSlot == 1) btn |= PAD_BUTTON_Y;
            else                            btn |= PAD_TRIGGER_Z;
        }
        s_bHoldFrames = 0;
        s_radialWheelOpen = false;
    }

    // 3. Menu / Pause
    if (wpad->btns_h & WPAD_BUTTON_PLUS)  btn |= PAD_BUTTON_START;
    if (wpad->btns_h & WPAD_BUTTON_MINUS) btn |= PAD_TRIGGER_Z; // Map / Inventory

    // 4. Skyward Sword D-Pad Quick Tool Arrangement & Gyro Recenter:
    // D-Pad Left:  Select & Equip Tool Slot 0 (X item)
    // D-Pad Up:    Select & Equip Tool Slot 1 (Y item)
    // D-Pad Right: Select & Equip Tool Slot 2 (Z item)
    // D-Pad Down:  Tap -> Sheathe / Put away tool; Hold (>= 200ms) or Tap during aim -> MotionPlus Gyro Recenter!
    if (wpad->btns_d & WPAD_BUTTON_LEFT) {
        s_activeToolSlot = 0;
        btn |= PAD_BUTTON_X; // Trigger equip/draw
        wii_speaker_play(WII_SFX_ITEM_EQUIP);
        wii_haptic_beat_pulse(15);
    }
    if (wpad->btns_d & WPAD_BUTTON_UP) {
        s_activeToolSlot = 1;
        btn |= PAD_BUTTON_Y; // Trigger equip/draw
        wii_speaker_play(WII_SFX_ITEM_EQUIP);
        wii_haptic_beat_pulse(15);
    }
    if (wpad->btns_d & WPAD_BUTTON_RIGHT) {
        s_activeToolSlot = 2;
        btn |= PAD_TRIGGER_Z; // Trigger equip/draw
        wii_speaker_play(WII_SFX_ITEM_EQUIP);
        wii_haptic_beat_pulse(15);
    }

    if (wpad->btns_h & WPAD_BUTTON_DOWN) {
        s_downHoldFrames++;
        if (s_downHoldFrames == 12) {
            // Re-center MotionPlus Gyro / Aiming calibration (Skyward Sword mechanic)
            wii_aiming_recenter();
        }
    } else {
        if (s_downHoldFrames > 0 && s_downHoldFrames < 12) {
            // If currently aiming (first-person), snap crosshair to center
            if (std::abs(s_aimingOffset.delta_pitch) > 0.05f || std::abs(s_aimingOffset.delta_yaw) > 0.05f) {
                wii_aiming_recenter();
            } else {
                // Quick tap: sheathe weapon / put away tool
                btn |= PAD_BUTTON_A;
                wii_haptic_beat_pulse(15);
            }
        }
        s_downHoldFrames = 0;
    }

    // Direct 1 & 2 buttons retain X / Y mapping
    if (wpad->btns_h & WPAD_BUTTON_1)     btn |= PAD_BUTTON_X;
    if (wpad->btns_h & WPAD_BUTTON_2)     btn |= PAD_BUTTON_Y;

    // Nunchuk expansion
    if (wpad->exp.type == EXP_NUNCHUK) {
        if (wpad->btns_h & WPAD_NUNCHUK_BUTTON_Z) {
            btn |= PAD_TRIGGER_L;
            status[0].triggerL = 255;
            wiiActive = true;
        }
        if (wpad->btns_h & WPAD_NUNCHUK_BUTTON_C) {
            btn |= PAD_TRIGGER_R;
            status[0].triggerR = 255;
            wiiActive = true;
        }

        float mag = wpad->exp.nunchuk.js.mag;
        if (mag > 0.05f) {
            float rad = wpad->exp.nunchuk.js.ang * (3.14159265f / 180.0f);
            float sx = std::sin(rad) * mag * 80.0f;
            float sy = std::cos(rad) * mag * 80.0f;
            if (sx > 80.0f) sx = 80.0f; else if (sx < -80.0f) sx = -80.0f;
            if (sy > 80.0f) sy = 80.0f; else if (sy < -80.0f) sy = -80.0f;
            status[0].stickX = (s8)sx;
            status[0].stickY = (s8)sy;
            if (mag > 0.20f) wiiActive = true;
        }

        // Nunchuk forward thrust -> Instant Shield Bash / Parry!
        float nz = (float)wpad->exp.nunchuk.accel.z;
        if (nz > 1.85f) {
            btn |= PAD_TRIGGER_R; // Shield Guard / deflect
            status[0].triggerR = 255;
            wiiActive = true;
            wii_speaker_play(WII_SFX_PARRY_CLANG);
            wii_haptic_beat_pulse(20);
        }
    }

    // 5. Gyro & MotionPlus Fine Aiming (Bow, Boomerang, Hookshot, Telescope)
    WiiAimingOffset aim;
    wii_aiming_get_offset(&aim);
    if (std::abs(aim.delta_pitch) > 0.0001f || std::abs(aim.delta_yaw) > 0.0001f) {
        float new_sx = (float)status[0].stickX + (aim.delta_yaw * 110.0f);
        float new_sy = (float)status[0].stickY - (aim.delta_pitch * 110.0f);
        if (new_sx > 80.0f) new_sx = 80.0f; else if (new_sx < -80.0f) new_sx = -80.0f;
        if (new_sy > 80.0f) new_sy = 80.0f; else if (new_sy < -80.0f) new_sy = -80.0f;
        status[0].stickX = (s8)new_sx;
        status[0].stickY = (s8)new_sy;
    }

    // Motion combat triggers (Skyward Sword / TP swing activates B button)
    WiiSwordAttackType attack = wii_sword_get_attack();
    if (attack != WII_SWORD_NONE) {
        btn |= PAD_BUTTON_B; // Trigger attack
        wiiActive = true;
    }

    if (wiiActive) {
        s_activeController = WII_CONTROLLER_WIIMOTE;
    }

    // Apply buttons to PADStatus
    status[0].button |= btn;
    status[0].err = PAD_ERR_NONE;
#endif
}

int wii_get_active_tool_slot(void) {
    return s_activeToolSlot;
}

void wii_set_active_tool_slot(int slot) {
    if (slot >= 0 && slot <= 2) {
        s_activeToolSlot = slot;
    }
}

bool wii_is_radial_wheel_open(void) {
    return s_radialWheelOpen;
}

} // extern "C"
