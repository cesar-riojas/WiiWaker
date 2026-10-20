/**
 * wii_item_wheel.cpp - Skyward Sword 8-Way Radial Item Selector for WiiWaker
 */

#include "wii_item_wheel.h"
#include <cmath>
#include <cstdio>
#include <cstring>

#if defined(GEKKO) || defined(__wii__)
#include <gccore.h>
#endif

namespace {

static const WiiWheelSlotDef s_slots[WII_WHEEL_SLOT_COUNT] = {
    { 0x27, "Hero's Bow",       0.0f,   true }, // North (12:00)
    { 0x31, "Bombs",            45.0f,  true }, // North-East (1:30)
    { 0x2D, "Boomerang",        90.0f,  true }, // East (3:00)
    { 0x2F, "Hookshot",         135.0f, true }, // South-East (4:30)
    { 0x33, "Skull Hammer",     180.0f, true }, // South (6:00)
    { 0x34, "Deku Leaf",        225.0f, true }, // South-West (7:30)
    { 0x25, "Grappling Hook",   270.0f, true }, // West (9:00)
    { 0x22, "Wind Waker",       315.0f, true }, // North-West (10:30)
};

static bool s_isOpen = false;
static uint32_t s_holdFrames = 0;
static const uint32_t HOLD_THRESHOLD = 12; // ~200ms hold to open radial wheel
static int s_selectedIndex = 0;
static float s_pointerAngle = 0.0f;
static float s_openScale = 0.0f;

} // namespace

extern "C" {

void wii_item_wheel_init(void) {
    s_isOpen = false;
    s_holdFrames = 0;
    s_selectedIndex = 0;
    s_pointerAngle = 0.0f;
    s_openScale = 0.0f;
}

void wii_item_wheel_update(bool b_held, float stick_x, float stick_y) {
    if (b_held) {
        s_holdFrames++;
        if (s_holdFrames >= HOLD_THRESHOLD) {
            s_isOpen = true;
            if (s_openScale < 1.0f) {
                s_openScale += 0.15f;
                if (s_openScale > 1.0f) s_openScale = 1.0f;
            }

            // Calculate stick magnitude & angle
            float mag = std::sqrt(stick_x * stick_x + stick_y * stick_y);
            if (mag > 0.30f) {
                // atan2 with -y so North (up) is 0 deg, East is 90 deg
                float rad = std::atan2(stick_x, -stick_y);
                float deg = rad * (180.0f / 3.14159265f);
                if (deg < 0.0f) deg += 360.0f;

                s_pointerAngle = deg;

                // Pick closest 45-degree sector
                int slot = (int)std::floor((deg + 22.5f) / 45.0f) % 8;
                if (slot >= 0 && slot < WII_WHEEL_SLOT_COUNT) {
                    s_selectedIndex = slot;
                }
            }
        }
    } else {
        if (s_isOpen) {
            // Releasing B confirms selection!
            s_isOpen = false;
            s_openScale = 0.0f;
            s_holdFrames = 0;

            // In game: trigger item equip on slot 0 (X / [1])
            // dComIfGs_setSelectItem(0, s_slots[s_selectedIndex].item_id);
            // dComIfGp_setSelectItem(0);
        } else {
            s_holdFrames = 0;
        }
    }
}

bool wii_item_wheel_is_active(void) {
    return s_isOpen;
}

int wii_item_wheel_get_selected(void) {
    return s_selectedIndex;
}

void wii_item_wheel_draw(void) {
    if (!s_isOpen && s_openScale <= 0.01f) return;

#if defined(GEKKO) || defined(__wii__)
    // 2D Orthographic GX drawing of the 8-way wheel
    const float CX = 320.0f;
    const float CY = 240.0f;
    const float RADIUS = 130.0f * s_openScale;

    // Draw 8 radial slot circles
    for (int i = 0; i < WII_WHEEL_SLOT_COUNT; i++) {
        float rad = s_slots[i].angle_deg * (3.14159265f / 180.0f);
        float sx = CX + std::sin(rad) * RADIUS;
        float sy = CY - std::cos(rad) * RADIUS;

        bool is_selected = (i == s_selectedIndex);
        float slot_scale = (is_selected ? 1.25f : 1.0f) * s_openScale;
        (void)sx; (void)sy; (void)slot_scale;
    }
#endif
}

} // extern "C"
