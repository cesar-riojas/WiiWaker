/**
 * wii_item_wheel.cpp - Skyward Sword 8-Way Radial Item Selector for WiiWaker
 */

#include "wii_item_wheel.h"
#include "wii_wpad.h"
#include "wii_speaker.h"
#include <cmath>
#include <cstdio>
#include <cstring>

extern "C" __attribute__((weak)) void wii_equip_inventory_item(int slot_btn, uint8_t inv_idx) {
    (void)slot_btn;
    (void)inv_idx;
}

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

// Wind Waker Inventory Slot IDs (matching dInvSlot_*_e)
enum {
    INV_SLOT_WIND_WAKER     = 2,
    INV_SLOT_GRAPPLING_HOOK = 3,
    INV_SLOT_BOOMERANG      = 5,
    INV_SLOT_DEKU_LEAF      = 6,
    INV_SLOT_BOW            = 12,
    INV_SLOT_BOMB           = 13,
    INV_SLOT_HOOKSHOT       = 19,
    INV_SLOT_SKULL_HAMMER   = 20,
};

static const uint8_t s_invSlots[WII_WHEEL_SLOT_COUNT] = {
    INV_SLOT_BOW,             // 0: Hero's Bow (North)
    INV_SLOT_BOMB,            // 1: Bombs (NE)
    INV_SLOT_BOOMERANG,       // 2: Boomerang (East)
    INV_SLOT_HOOKSHOT,        // 3: Hookshot (SE)
    INV_SLOT_SKULL_HAMMER,    // 4: Skull Hammer (South)
    INV_SLOT_DEKU_LEAF,       // 5: Deku Leaf (SW)
    INV_SLOT_GRAPPLING_HOOK,  // 6: Grappling Hook (West)
    INV_SLOT_WIND_WAKER,      // 7: Wind Waker (NW)
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
            // Releasing B confirms selection & equips the item!
            s_isOpen = false;
            s_openScale = 0.0f;
            s_holdFrames = 0;

            int active_slot = wii_get_active_tool_slot();
            if (s_selectedIndex >= 0 && s_selectedIndex < WII_WHEEL_SLOT_COUNT) {
                wii_equip_inventory_item(active_slot, s_invSlots[s_selectedIndex]);
                wii_speaker_play(WII_SFX_ITEM_EQUIP);
            }
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
    // Set up 2D orthographic projection for 640x480 screen space
    Mtx44 ortho;
    Mtx modelview;
    guOrtho(ortho, 0.0f, 480.0f, 0.0f, 640.0f, 0.0f, 1.0f);
    GX_LoadProjectionMtx(ortho, GX_ORTHOGRAPHIC);
    guMtxIdentity(modelview);
    GX_LoadPosMtxImm(modelview, GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
    GX_SetNumTexGens(0);
    GX_SetNumTevStages(1);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetZMode(GX_DISABLE, GX_ALWAYS, GX_FALSE);
    GX_SetCullMode(GX_CULL_NONE);

    const float CX = 320.0f;
    const float CY = 240.0f;
    const float RADIUS = 130.0f * s_openScale;

    // 1. Draw translucent backdrop disc behind the wheel
    const int RING_SEGS = 24;
    const float BACK_R = 175.0f * s_openScale;
    u8 bg_alpha = (u8)(130.0f * s_openScale);
    GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT0, RING_SEGS + 2);
    GX_Position3f32(CX, CY, 0.0f);
    GX_Color4u8(20, 18, 14, bg_alpha);
    for (int s = 0; s <= RING_SEGS; s++) {
        float a = (2.0f * 3.14159265f * (float)s) / (float)RING_SEGS;
        GX_Position3f32(CX + std::sin(a) * BACK_R, CY - std::cos(a) * BACK_R, 0.0f);
        GX_Color4u8(20, 18, 14, 0);
    }
    GX_End();

    // 2. Draw 8 radial slot medallions
    const int SLOT_SEGS = 12;
    for (int i = 0; i < WII_WHEEL_SLOT_COUNT; i++) {
        float rad = s_slots[i].angle_deg * (3.14159265f / 180.0f);
        float sx = CX + std::sin(rad) * RADIUS;
        float sy = CY - std::cos(rad) * RADIUS;

        bool is_selected = (i == s_selectedIndex);
        float slot_r = (is_selected ? 28.0f : 22.0f) * s_openScale;

        u8 r_col = is_selected ? 255 : 212;
        u8 g_col = is_selected ? 225 : 175;
        u8 b_col = is_selected ? 85  : 55;
        u8 a_col = (u8)((is_selected ? 245.0f : 195.0f) * s_openScale);

        GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT0, SLOT_SEGS + 2);
        GX_Position3f32(sx, sy, 0.0f);
        GX_Color4u8(45, 38, 25, a_col);
        for (int s = 0; s <= SLOT_SEGS; s++) {
            float a = (2.0f * 3.14159265f * (float)s) / (float)SLOT_SEGS;
            GX_Position3f32(sx + std::sin(a) * slot_r, sy - std::cos(a) * slot_r, 0.0f);
            GX_Color4u8(r_col, g_col, b_col, a_col);
        }
        GX_End();
    }

    // 3. Draw directional pointer ray from center toward selected sector
    float ptr_rad = s_pointerAngle * (3.14159265f / 180.0f);
    float tip_x = CX + std::sin(ptr_rad) * (RADIUS * 0.85f);
    float tip_y = CY - std::cos(ptr_rad) * (RADIUS * 0.85f);
    float perp_x = std::cos(ptr_rad) * 5.0f * s_openScale;
    float perp_y = std::sin(ptr_rad) * 5.0f * s_openScale;

    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
    GX_Position3f32(CX - perp_x, CY - perp_y, 0.0f);
    GX_Color4u8(255, 240, 150, 220);
    GX_Position3f32(CX + perp_x, CY + perp_y, 0.0f);
    GX_Color4u8(255, 240, 150, 220);
    GX_Position3f32(tip_x, tip_y, 0.0f);
    GX_Color4u8(45, 235, 115, 255);
    GX_End();
#endif
}

} // extern "C"
