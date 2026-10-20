# WiiWaker

**WiiWaker** is a native Nintendo Wii homebrew port of *The Legend of Zelda: The Wind Waker*, standing on the complete C++ decompilation foundation of the GameCube game.

Unlike running the game in GameCube compatibility mode (MIOS / Nintendont), WiiWaker runs natively as Wii software, unlocking the full 729 MHz CPU, 88 MB RAM, 16:9 widescreen, and full **Wii Remote + MotionPlus** support.

---

## Key Features

1. **Native Wii Hardware Utilization:**
   - **729 MHz Broadway CPU:** 50% clock boost over the original GameCube 485 MHz.
   - **88 MB Memory Architecture:** Utilizes the Wii's 24 MB MEM1 (1T-SRAM) for critical rendering and 64 MB MEM2 (GDDR3) to eliminate slow legacy ARAM streaming bottlenecks.
   - **Native Hollywood GX Graphics:** Direct hardware GX rendering with native anamorphic 16:9 480p output.
   - **High-Speed SD / USB Storage:** Loads assets and saves directly through `libogc` FAT32 filesystem (`fatfs`).

2. **Wii MotionPlus Baton Conducting Mechanic:**
   - Turn your Wii Remote into Link's conductor's baton.
   - Uses real 6-axis gyroscope angular velocity and acceleration spikes to detect:
     - **Downbeat:** Sharp flick downward (`mDoAud_ARM_DOWN_e`)
     - **Upbeat:** Flick upward (`mDoAud_ARM_UP_e`)
     - **Inward Beat:** Sweep left (`mDoAud_ARM_LEFT_e`)
     - **Outward Beat:** Sweep right (`mDoAud_ARM_RIGHT_e`)
     - **Neutral:** Rebound return to rest (`mDoAud_ARM_CENTER_e`)
   - **Dynamic Volume:** Physical stroke velocity scales note dynamics from pianissimo ($pp$) to fortissimo ($ff$).
   - **Haptic Feedback:** Timed rumble pulse on the exact beat strike (the *ictus*).
   - **Time Signature (Meter):** Tilt Nunchuk stick or flick left/right to set 3/4, 4/4, or 6/4 time.

3. **Motion Aiming & Wii HUD:**
   - Smooth MotionPlus pointer aiming for the Hero's Bow, Boomerang, Hookshot, and Telescope.
   - Wii Remote and Nunchuk button glyphs in the HUD and item equip screens.

---

## Repository Structure

```
WiiWaker/
├── game/                  Full C++ decompiled game logic (zeldaret/tww + snrubrm/tww)
│   ├── d_a_player_tact.inc   Wind Waker baton conducting state machine (patched)
│   ├── m_Do_audio.cpp        Audio metronome and note classifier
│   └── m_Do_controller_pad   Controller inputs and pad conversion
├── wii/                   Native Wii platform layer
│   ├── include/
│   │   ├── wii_main.h        Video, audio, and lifecycle management
│   │   ├── wii_memory.h      MEM1/MEM2 arenas & ARAM translation
│   │   ├── wii_storage.h     FAT32 SD/USB asset loading
│   │   └── wii_wpad.h        Wii Remote + MotionPlus gesture engine
│   └── src/
│       ├── wii_main.cpp      480p progressive framebuffer & video init
│       ├── wii_memory.cpp    Arena allocation & cache coherency
│       ├── wii_storage.cpp   POSIX file bridges
│       └── wii_wpad.cpp      MotionPlus gesture classifier & aiming
├── patches/               Standalone patches and mods
│   └── conducting_motion.patch
└── Makefile               devkitPPC build definition
```

---

## Building

### Requirements
- [devkitPro](https://devkitpro.org) with **devkitPPC** and **libogc**
- `elf2dol` tool

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=$DEVKITPRO/devkitPPC
make
```

Output: `WiiWaker.dol`

---

## SD Card Installation (Homebrew Channel)

Copy the built files to your Wii SD card:
```
sdmc:/
└── apps/
    └── WiiWaker/
        ├── boot.dol           (WiiWaker.dol renamed)
        ├── meta.xml           (Homebrew channel metadata)
        ├── icon.png           (App icon)
        └── files/             (Extracted game assets from GZLE01)
```
