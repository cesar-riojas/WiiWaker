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
    uint16_t x, y, z;
} vec3w_t;

typedef struct {
    float x, y, z;
} gforce_t;

typedef struct {
    float pitch, roll, yaw;
} orient_t;

typedef struct {
    int valid;
    float x, y;
} ir_t;

typedef struct {
    int16_t err;
    uint32_t btns_h;
    uint32_t btns_d;
    uint32_t btns_u;
    ir_t ir;
    vec3w_t accel;
    orient_t orient;
    gforce_t gforce;
    struct {
        int type;
        union {
            struct {
                struct {
                    float ang;
                    float mag;
                } js;
                vec3w_t accel;
                orient_t orient;
                gforce_t gforce;
            } nunchuk;
            struct {
                short rx, ry, rz;
                uint8_t status;
                uint8_t ext;
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

// Latched virtual GameCube pad state updated every frame by wii_wpad_update()
static uint16_t s_latchedButtons  = 0;
static int8_t   s_latchedStickX   = 0;
static int8_t   s_latchedStickY   = 0;
static uint8_t  s_latchedTriggerL = 0;
static uint8_t  s_latchedTriggerR = 0;
static bool     s_wiiInputActive  = false;
static struct nunchuk_t s_lastNunchukState;
static bool     s_hasNunchukState = false;

#if defined(GEKKO) || defined(__wii__)
static u64 s_rumbleStopTick = 0;
#else
static uint64_t s_rumbleStopTick = 0;
#endif

static bool has_nunchuk_data(const WPADData* wpad, struct nunchuk_t* out_nc) {
#if defined(GEKKO) || defined(__wii__)
    if (!wpad) return false;
    if (wpad->exp.type == EXP_NUNCHUK) {
        s_lastNunchukState = wpad->exp.nunchuk;
        s_hasNunchukState = true;
        if (out_nc) *out_nc = wpad->exp.nunchuk;
        return true;
    }
    // When MotionPlus is active with pass-through extension attached (mp.ext & 1)
    if (wpad->exp.type == EXP_MOTION_PLUS && (wpad->exp.mp.ext & 0x01) && s_hasNunchukState) {
        if (out_nc) *out_nc = s_lastNunchukState;
        return true;
    }
#endif
    return false;
}

static void wii_process_controller_state(void) {
#if defined(GEKKO) || defined(__wii__)
    WPADData* wpad = WPAD_Data(WPAD_CHAN_0);
    if (!wpad || wpad->err != WPAD_ERR_NONE) {
        s_wiiInputActive = false;
        return;
    }

    u16 btn = 0;
    s8 stickX = 0;
    s8 stickY = 0;
    u8 trigL = 0;
    u8 trigR = 0;
    bool wiiActive = (wpad->btns_h != 0);

    // 1. Context / Action Button
    if (wpad->btns_h & WPAD_BUTTON_A) btn |= PAD_BUTTON_A;

    // 2. Skyward Sword [B] Button Logic:
    // Tap [B]: Use currently active tool slot (Slot 0 -> X, Slot 1 -> Y, Slot 2 -> Z)
    // Hold [B] (>= 200ms): Opens 8-Way Radial Item Wheel
    float wheel_stick_x = 0.0f;
    float wheel_stick_y = 0.0f;
    struct nunchuk_t nc;
    bool hasNc = has_nunchuk_data(wpad, &nc);
    if (hasNc) {
        float mag = nc.js.mag;
        if (mag > 0.15f) {
            float rad = nc.js.ang * (3.14159265f / 180.0f);
            wheel_stick_x = std::sin(rad) * mag;
            wheel_stick_y = std::cos(rad) * mag;
        }
    } else if (wpad->ir.valid) {
        // Fallback: allow IR pointer or Wiimote tilt to steer the radial wheel
        wheel_stick_x = (wpad->ir.x - 320.0f) / 200.0f;
        wheel_stick_y = -(wpad->ir.y - 240.0f) / 160.0f;
    }

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
    if (wpad->btns_d & WPAD_BUTTON_LEFT) {
        s_activeToolSlot = 0;
        btn |= PAD_BUTTON_X;
        wii_speaker_play(WII_SFX_ITEM_EQUIP);
        wii_haptic_beat_pulse(15);
    }
    if (wpad->btns_d & WPAD_BUTTON_UP) {
        s_activeToolSlot = 1;
        btn |= PAD_BUTTON_Y;
        wii_speaker_play(WII_SFX_ITEM_EQUIP);
        wii_haptic_beat_pulse(15);
    }
    if (wpad->btns_d & WPAD_BUTTON_RIGHT) {
        s_activeToolSlot = 2;
        btn |= PAD_TRIGGER_Z;
        wii_speaker_play(WII_SFX_ITEM_EQUIP);
        wii_haptic_beat_pulse(15);
    }

    if (wpad->btns_h & WPAD_BUTTON_DOWN) {
        s_downHoldFrames++;
        if (s_downHoldFrames == 12) {
            wii_aiming_recenter();
        }
    } else {
        if (s_downHoldFrames > 0 && s_downHoldFrames < 12) {
            if (std::abs(s_aimingOffset.delta_pitch) > 0.05f || std::abs(s_aimingOffset.delta_yaw) > 0.05f) {
                wii_aiming_recenter();
            } else {
                btn |= PAD_BUTTON_A;
                wii_haptic_beat_pulse(15);
            }
        }
        s_downHoldFrames = 0;
    }

    // Direct 1 & 2 buttons retain X / Y mapping
    if (wpad->btns_h & WPAD_BUTTON_1) btn |= PAD_BUTTON_X;
    if (wpad->btns_h & WPAD_BUTTON_2) btn |= PAD_BUTTON_Y;

    // Nunchuk Z / C buttons are reported in upper 16 bits of btns_h regardless of union
    if (wpad->btns_h & WPAD_NUNCHUK_BUTTON_Z) {
        btn |= PAD_TRIGGER_L;
        trigL = 255;
        wiiActive = true;
    }
    if (wpad->btns_h & WPAD_NUNCHUK_BUTTON_C) {
        btn |= PAD_TRIGGER_R;
        trigR = 255;
        wiiActive = true;
    }

    // Nunchuk analog stick & calibrated G-force shield bash
    if (hasNc) {
        float mag = nc.js.mag;
        if (mag > 0.05f) {
            float rad = nc.js.ang * (3.14159265f / 180.0f);
            float sx = std::sin(rad) * mag * 80.0f;
            float sy = std::cos(rad) * mag * 80.0f;
            sx = std::clamp(sx, -80.0f, 80.0f);
            sy = std::clamp(sy, -80.0f, 80.0f);
            stickX = (s8)sx;
            stickY = (s8)sy;
            if (mag > 0.20f) wiiActive = true;
        }

        // Nunchuk forward thrust using calibrated gforce (1.0g neutral -> >1.85g spike)
        float nz = nc.gforce.z;
        if (nz > 1.85f) {
            btn |= PAD_TRIGGER_R; // Shield Guard / deflect
            trigR = 255;
            wiiActive = true;
            wii_speaker_play(WII_SFX_PARRY_CLANG);
            wii_haptic_beat_pulse(20);
        }
    }

    // 5. Gyro & MotionPlus Fine Aiming
    WiiAimingOffset aim;
    wii_aiming_get_offset(&aim);
    if (std::abs(aim.delta_pitch) > 0.0001f || std::abs(aim.delta_yaw) > 0.0001f) {
        float new_sx = (float)stickX + (aim.delta_yaw * 110.0f);
        float new_sy = (float)stickY - (aim.delta_pitch * 110.0f);
        stickX = (s8)std::clamp(new_sx, -80.0f, 80.0f);
        stickY = (s8)std::clamp(new_sy, -80.0f, 80.0f);
    }

    // Motion combat triggers (Skyward Sword / TP swing activates B button)
    WiiSwordAttackType attack = wii_sword_get_attack();
    if (attack != WII_SWORD_NONE) {
        btn |= PAD_BUTTON_B;
        wiiActive = true;
    }

    if (wiiActive) {
        s_activeController = WII_CONTROLLER_WIIMOTE;
    }

    s_latchedButtons  = btn;
    s_latchedStickX   = stickX;
    s_latchedStickY   = stickY;
    s_latchedTriggerL = trigL;
    s_latchedTriggerR = trigR;
    s_wiiInputActive  = true;
#endif
}

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

    // Process Wii Remote + Nunchuk button, D-Pad slot, radial wheel, and aiming state
    wii_process_controller_state();

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
    (void)metronome_beat_rate;
    float pitch_rate = 0.0f;
    float yaw_rate   = 0.0f;
    float nunchuk_x  = 0.0f;

#if defined(GEKKO) || defined(__wii__)
    WPADData* pad = WPAD_Data(WPAD_CHAN_0);
    if (pad && pad->err == WPAD_ERR_NONE) {
        // Read MotionPlus angular rates or calibrated gforce fallback (scaled to gyro deg/s)
        if (pad->exp.type == EXP_MOTION_PLUS) {
            pitch_rate = (float)pad->exp.mp.rx;
            yaw_rate   = (float)pad->exp.mp.rz;
        } else {
            // Calibrated gforce.x/y is 0.0g at rest; scale >0.8g flicks into gyro range
            pitch_rate = pad->gforce.y * 220.0f;
            yaw_rate   = pad->gforce.x * 220.0f;
        }

        // Read Nunchuk analog stick X for time signature
        struct nunchuk_t nc;
        if (has_nunchuk_data(pad, &nc)) {
            float ang = nc.js.ang;
            float mag = nc.js.mag;
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
    if (pad && pad->err == WPAD_ERR_NONE) {
        if (pad->exp.type == EXP_MOTION_PLUS) {
            const float SENSITIVITY = 0.00065f;
            s_aimingOffset.delta_pitch = std::clamp(s_aimingOffset.delta_pitch + (float)pad->exp.mp.rx * SENSITIVITY * 0.15f, -1.0f, 1.0f);
            s_aimingOffset.delta_yaw   = std::clamp(s_aimingOffset.delta_yaw   + (float)pad->exp.mp.rz * SENSITIVITY * 0.15f, -1.0f, 1.0f);
            s_aimingOffset.delta_roll  = (float)pad->exp.mp.ry * SENSITIVITY;
        } else if (pad->ir.valid) {
            s_aimingOffset.delta_yaw   = std::clamp((pad->ir.x - 320.0f) / 320.0f, -1.0f, 1.0f);
            s_aimingOffset.delta_pitch = std::clamp((pad->ir.y - 240.0f) / 240.0f, -1.0f, 1.0f);
            s_aimingOffset.delta_roll  = 0.0f;
        }
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

    // 2. Refresh Wii Remote state and merge into Port 0 PADStatus
    wii_process_controller_state();
    if (!s_wiiInputActive) return;

    status[0].button |= s_latchedButtons;
    if (s_latchedTriggerL > status[0].triggerL) status[0].triggerL = s_latchedTriggerL;
    if (s_latchedTriggerR > status[0].triggerR) status[0].triggerR = s_latchedTriggerR;
    if (std::abs((int)s_latchedStickX) > std::abs((int)status[0].stickX)) status[0].stickX = s_latchedStickX;
    if (std::abs((int)s_latchedStickY) > std::abs((int)status[0].stickY)) status[0].stickY = s_latchedStickY;
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
