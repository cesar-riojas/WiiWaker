<p align="center">
  <img src="docs/assets/wiiwaker_logo.png" alt="WiiWaker Project Logo" width="600"/>
</p>

<p align="center">
  <strong>Native Nintendo Wii Port & MotionPlus Controls Engine for <em>The Legend of Zelda: The Wind Waker</em></strong>
</p>

<p align="center">
  <img src="docs/assets/wii_channel_banner.png" alt="WiiWaker Wii Channel Banner" width="100%"/>
</p>

<p align="center">
  <a href="#key-features">Features</a> •
  <a href="#control-scheme">Controls</a> •
  <a href="#skyward-sword-hud--visuals">HUD & Visuals</a> •
  <a href="#building">Building</a> •
  <a href="#asset-setup--iso-patching">Asset Setup</a> •
  <a href="#legal--disclaimer">Legal</a>
</p>

---

## Overview

**WiiWaker** is a native Nintendo Wii source port of *The Legend of Zelda: The Wind Waker*. Rather than running in backward-compatible GameCube mode (MIOS / Nintendont), WiiWaker runs natively as full Wii software on the **Broadway 729 MHz CPU** with access to the full **88 MB of unified MEM1 + MEM2 RAM**, native 16:9 480p progressive scan, and an authentic **Skyward Sword-inspired Wii Remote & MotionPlus control scheme**.

---

## Key Features

1. **Native Wii Hardware Utilization:**
   - **729 MHz Broadway CPU:** 50% CPU clock increase over the GameCube's 485 MHz Gekko, eliminating frame drops during heavy ocean particle storms and multi-enemy combat.
   - **88 MB Memory Architecture:** Utilizes 24 MB MEM1 (1T-SRAM) for low-latency rendering and 64 MB MEM2 (GDDR3) to eliminate slow legacy ARAM streaming bottlenecks.
   - **Native Hollywood GX Graphics:** Direct hardware GX rendering with native anamorphic 16:9 480p output.
   - **Yamaha ADPCM Remote Speaker Streaming:** Custom 3000 Hz 4-bit Yamaha ADPCM audio streaming directly to the Wii Remote speaker for sword swings, baton ictus clicks, and bow string draw cues.

2. **Skyward Sword Motion Combat & Gestures:**
   - **Directional MotionPlus Swordplay:** 1:1 real-time gyro slashing (vertical chops, horizontal sweeps, diagonal kesa-giri, and thrust attacks).
   - **Nunchuk Shield Bash / Parry:** Fast forward jab of the Nunchuk to execute Link's shield bash, parry incoming projectile attacks, and stagger armored Darknuts.
   - **Tact Baton Conducting:** Real 6-axis gyroscope angular velocity and acceleration spikes detect downbeats, upbeats, and side sweeps with velocity-sensitive volume ($pp$ to $ff$), haptic feedback, and baton sound effects.
   - **Smooth Gyro Aiming:** First-person and third-person gyro pointing for Hero's Bow, Boomerang, Hookshot, and PictoBox, with instant Skyward Sword-style **D-Pad Down recenter**.

3. **Skyward Sword Controller HUD & 8-Way Radial Item Wheel:**
   - **Vertical Motion Controller HUD:** Centered along the right margin ($X=572$), rendering the authentic circular gold medallion socket, 24x34 official Nintendo [B] button key, domed (A) button with context action prompts ("Talk", "Open", "Put away", "Roll", "Check"), directional D-Pad cross, and (-) / (+) medallions.
   - **Active Button Pressed States & Shadows:** Idle state displays authentic Nintendo soft drop shadow; pressed state renders flush and recessed with zero drop shadow.
   - **8-Way Radial Item Selector:** Hold [B] ($\ge 200\text{ms}$) to open the radial wheel and point the MotionPlus cursor hand or tilt the analog stick to quick-select items in active real time. Tap [B] ($<200\text{ms}$) to instantly fire your active item.
   - **D-Pad Quick Wings:** D-Pad Left, Up, and Right instantly equip favorite tools on the fly.

---

## Control Scheme

| Input | In-Game Action | Context / Motion Mechanics |
| :--- | :--- | :--- |
| **Wii Remote Swing** | Sword Slash | Directional chops, horizontal cuts, diagonal sweeps, and thrusts |
| **Nunchuk Forward Flick** | Shield Bash / Parry | Staggers enemies, deflects projectiles, executes counter attacks |
| **Nunchuk Analog Stick** | Move Link / Conduct Meter | Analog walking/running; tilts tempo/meter in baton mode |
| **(A) Button** | Context Action / Roll | "Talk", "Open", "Check", "Lift", "Roll", "Put away" |
| **[B] Button (Tap < 200ms)** | Use Active Item | Fires Bow, Boomerang, Hookshot, Bomb, Wind Waker |
| **[B] Button (Hold ≥ 200ms)** | 8-Way Radial Item Wheel | Gyro cursor hand / stick to select equipped item |
| **D-Pad Left / Up / Right** | Quick Tool Slots | Instant equip to primary item socket |
| **D-Pad Down** | Recenter Aiming | Instantly zeroes gyro orientation in aiming mode |
| **(C) Button (Nunchuk)** | First-Person View / Look | Gyro-controlled free camera view |
| **(Z) Button (Nunchuk)** | L-Target / Lock-On | Focuses camera onto nearest enemy or NPC |
| **(-) Button** | Sea Chart / World Map | Toggles nautical charts and triforce maps |
| **(+) Button** | Pause / Inventory Menu | Opens Quest Status, Spoils Bag, and Settings |
| **Wii Remote Shake** | Spin Attack | High-angular velocity shake charges Hurricane Spin |

---

## Skyward Sword HUD & Visuals

WiiWaker includes complete pixel-aligned layout data and custom cel-shaded vector UI textures:

<p align="center">
  <img src="wii/assets/skyward_sword_native/final/ss_native_solid_gold_socket.png" width="48" alt="Gold Medallion Socket"/>
  <img src="wii/assets/skyward_sword_native/final/ss_official_clean_b_button.png" width="24" height="34" alt="B Key"/>
  <img src="wii/assets/skyward_sword_native/final/ss_native_dpad.png" width="48" alt="D-Pad"/>
  <img src="wii/assets/skyward_sword_native/final/ss_native_a_button.png" width="44" alt="A Button Dome"/>
  <img src="wii/assets/skyward_sword_native/final/ss_native_minus.png" width="28" alt="Minus"/>
  <img src="wii/assets/skyward_sword_native/final/ss_native_plus.png" width="28" alt="Plus"/>
</p>

- **Gold Medallion Socket (`baat`):** Centerline $X=572$, $Y=90$, dimensions $48\times 48$.
- **Clean Official Nintendo [B] Key (`bab`):** Centerline $X=572$, $Y=96$, dimensions $24\times 34$, eliminating the squished box look.
- **Directional D-Pad Cross (`ba02`):** Centerline $X=572$, $Y=155$, dimensions $36\times 36$.
- **Domed (A) Action Dome (`bawe`):** Centerline $X=572$, $Y=220$, dimensions $40\times 40$.
- **Dynamic Context Action Label (`bawp`):** $X=545$, $Y=266$, dynamic text centered directly beneath the (A) button.
- **(-) & (+) Medallions (`baz`, `ba0r`):** $Y=305$, positioned symmetrically at $X=543$ and $X=575$.

---

## Building

### Prerequisites
- [devkitPro](https://devkitpro.org) with `wii-dev` (`devkitPPC`, `libogc`, `elf2dol`).
- Python 3.8+ with `Pillow` (`pip install Pillow`).

### Compiling the Wii Homebrew Binary
```bash
git clone https://github.com/WiiWaker/WiiWaker.git
cd WiiWaker

# Build the native Wii executable
./tools/build_wii.sh
```
This generates `WiiWaker.dol` at repo root and updates `dist/apps/WiiWaker/boot.dol`.

---

## Asset Setup & ISO Patching

WiiWaker uses a **"Bring Your Own Game" (BYOG)** asset extraction and patching workflow. In strict accordance with copyright law, no Nintendo copyrighted ISOs, ROMs, or game disc archives are included in this repository.

### Patching Your GameCube Disc Image
To apply the Skyward Sword HUD, Wii visuals, and control mappings to your legally owned Wind Waker GameCube ISO (`GZLE01` / `BetterWindWaker.iso`):

```bash
python3 tools/patch_game_iso.py \
  --input /path/to/WindWaker.iso \
  --output /path/to/WiiWaker_Playable.iso
```

### Direct SD Card Installation (Homebrew Channel / Nintendont)
Copy the `dist/apps/WiiWaker` folder to your Wii SD card:
```
sdmc:/
└── apps/
    └── WiiWaker/
        ├── boot.dol           # Native Wii executable
        ├── meta.xml           # Homebrew Channel XML metadata
        ├── icon.png           # 128x48 Wii Channel icon
        └── files/             # (Optional) Extracted game files for SD root loading
```

---

## Legal & Disclaimer

**The Legend of Zelda: The Wind Waker** is a registered trademark of **Nintendo Co., Ltd.**

WiiWaker is a non-profit, educational open-source reverse engineering and homebrew modification project.
- This repository contains **zero copyrighted Nintendo proprietary game assets, zero ISO disc images, zero copyrighted audio/music archives, and zero compiled game binaries**.
- All code in this repository is original C++ and Python tooling or clean-room reimplementations.
- Users **must legally own an official retail copy** of *The Legend of Zelda: The Wind Waker* on Nintendo GameCube to extract and patch game assets.

---

## License

This project is licensed under the [MIT License](LICENSE).
