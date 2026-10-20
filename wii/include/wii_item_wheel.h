/**
 * wii_item_wheel.h - Skyward Sword-style 8-Way Radial Item Selector for WiiWaker
 */

#ifndef WII_ITEM_WHEEL_H
#define WII_ITEM_WHEEL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 8 Radial Item Wheel Slots (45 degrees each)
typedef enum {
    WII_WHEEL_SLOT_BOW          = 0, // 0 deg (Top / North)
    WII_WHEEL_SLOT_BOMBS        = 1, // 45 deg (North-East)
    WII_WHEEL_SLOT_BOOMERANG    = 2, // 90 deg (East)
    WII_WHEEL_SLOT_HOOKSHOT     = 3, // 135 deg (South-East)
    WII_WHEEL_SLOT_HAMMER       = 4, // 180 deg (South)
    WII_WHEEL_SLOT_DEKU_LEAF    = 5, // 225 deg (South-West)
    WII_WHEEL_SLOT_GRAPPLE      = 6, // 270 deg (West)
    WII_WHEEL_SLOT_WIND_WAKER   = 7, // 315 deg (North-West)
    WII_WHEEL_SLOT_COUNT        = 8,
    WII_WHEEL_SLOT_NONE         = -1
} WiiWheelSlotId;

typedef struct {
    uint8_t item_id;
    const char* display_name;
    float angle_deg;
    bool is_available;
} WiiWheelSlotDef;

// Initialize the Skyward Sword Radial Item Wheel
void wii_item_wheel_init(void);

// Update wheel state per frame (tracks B hold, stick/pointer ray angle)
void wii_item_wheel_update(bool b_held, float stick_x, float stick_y);

// Check if the wheel is currently active/open
bool wii_item_wheel_is_active(void);

// Get the currently selected slot index (-1 to 7)
int wii_item_wheel_get_selected(void);

// Render the 8-way circular wheel, ray, center badge, and glowing selection
void wii_item_wheel_draw(void);

#ifdef __cplusplus
}
#endif

#endif // WII_ITEM_WHEEL_H
