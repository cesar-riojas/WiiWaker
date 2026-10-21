#ifndef WII_MOTION_SWORD_H
#define WII_MOTION_SWORD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Skyward Sword & Twilight Princess Directional Combat Mechanics
typedef enum {
    WII_SWORD_NONE          = 0,
    WII_SWORD_VERTICAL_CHOP = 1, // Overhead chop (procCutA)
    WII_SWORD_SLASH_RIGHT   = 2, // Right slash   (procCutR)
    WII_SWORD_SLASH_LEFT    = 3, // Left slash    (procCutL)
    WII_SWORD_THRUST        = 4, // Forward stab  (procCutF)
    WII_SWORD_DIAGONAL      = 5, // Kesa-giri     (procCutKesa)
    WII_SWORD_SPIN_ATTACK   = 6, // Spin attack   (procCutTurn) - Nunchuk shake
    WII_SWORD_JUMP_ATTACK   = 7, // Jump attack   (procJumpCut)
} WiiSwordAttackType;

// Initialize sword motion tracking
void wii_sword_init(void);

// Update sword motion tracking per frame (called from wii_wpad_update)
void wii_sword_update(void);

// Query if a motion-based sword swing was initiated this frame
WiiSwordAttackType wii_sword_get_attack(void);

// Clear current attack latch after being consumed by Link's action state
void wii_sword_consume_attack(void);

// Query whether Nunchuk shake (Twilight Princess spin attack) was triggered
bool wii_sword_is_spin_attack(void);

#ifdef __cplusplus
}
#endif

#endif // WII_MOTION_SWORD_H
