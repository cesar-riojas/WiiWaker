#!/usr/bin/env python3
"""
Injects the authentic native Skyward Sword textures extracted directly from SOUE01:
- ss_native_gold_b_gear.png -> ba_base_02.bti, ba_base_00.bti
- transparent -> ba_b.bti
- ss_native_a_button.png    -> ba_a.bti
- ss_native_dpad.png        -> cross_key_00.bti
- ss_native_minus.png       -> ba_base_z.bti
- ss_native_plus.png        -> ba_s.bti, ba_r.bti
- ss_native_cursor_hand.png -> cursor_00_01.bti

Then repacks:
- dist/apps/WiiWaker/files/res/Msg/menures.arc
- WiiWaker_Playable.iso
- SD Card / Nintendont copy
"""

import sys
import os
import shutil
from io import BytesIO
from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))
sys.path.append(os.path.join(REPO_ROOT, "..", "betterww"))
from wwlib.rarc import RARC
from wwlib.bti import BTIFile, ImageFormat, PaletteFormat
from wwlib.gcm import GCM

FINAL_ASSETS_DIR = os.path.join(REPO_ROOT, "wii", "assets", "skyward_sword_native", "final")
MENURES_PATH = os.path.join(REPO_ROOT, "dist", "apps", "WiiWaker", "files", "res", "Msg", "menures.arc")
PLAYABLE_ISO = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/WiiWaker_Playable.iso"
BASE_ISO = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/BetterWindWaker.iso"
SD_ISO = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/SD_Card_Ready/games/The Legend of Zelda The Wind Waker [GZLE01]/game.iso"

def inject_texture(entry, img, target_size, img_fmt=ImageFormat.C8, pal_fmt=PaletteFormat.RGB5A3):
    bti = BTIFile(entry.data)
    bti.image_format = img_fmt
    if img_fmt in (ImageFormat.C4, ImageFormat.C8, ImageFormat.C14X2):
        bti.palette_format = pal_fmt
    img_resized = img.resize(target_size, Image.Resampling.LANCZOS)
    bti.replace_image(img_resized)
    bti.save_changes()
    entry.data = bti.data
    return bti

def main():
    print(f"Reading {MENURES_PATH}...")
    with open(MENURES_PATH, "rb") as f:
        file_bytes = f.read()

    rarc = RARC()
    rarc.read(BytesIO(file_bytes))

    # 1. Native Solid Gold Circular Medallion Socket
    gear_img = Image.open(os.path.join(FINAL_ASSETS_DIR, "ss_native_solid_gold_socket.png"))
    for bti_name in ["ba_base_02.bti", "ba_base_00.bti"]:
        entry = rarc.get_file_entry(bti_name)
        if entry:
            inject_texture(entry, gear_img, (48, 48), ImageFormat.C8, PaletteFormat.RGB5A3)
            print(f"  [+] Injected native ss_native_solid_gold_socket.png into {bti_name}")

    # 2. Authentic Clean Official Nintendo Skyward Sword [B] Key
    b_entry = rarc.get_file_entry("ba_b.bti")
    if b_entry:
        b_img = Image.open(os.path.join(FINAL_ASSETS_DIR, "ss_official_clean_b_button.png"))
        inject_texture(b_entry, b_img, (24, 34), ImageFormat.C8, PaletteFormat.RGB5A3)
        print("  [+] Injected authentic clean official Nintendo [B] button (24x34) into ba_b.bti")

    # 3. Native Skyward Sword (A) Button Dome (ba_base_04 = idle raised, ba_base_03 = pressed recessed)
    a_img = Image.open(os.path.join(FINAL_ASSETS_DIR, "ss_native_a_button.png"))
    a_entry = rarc.get_file_entry("ba_base_04.bti")
    if a_entry:
        inject_texture(a_entry, a_img, (44, 44), ImageFormat.C8, PaletteFormat.RGB5A3)
        print("  [+] Injected normal ss_native_a_button.png into ba_base_04.bti (idle raised)")

    a_pressed_img = Image.open(os.path.join(FINAL_ASSETS_DIR, "ss_native_a_button_pressed.png"))
    a_pressed_entry = rarc.get_file_entry("ba_base_03.bti")
    if a_pressed_entry:
        inject_texture(a_pressed_entry, a_pressed_img, (44, 44), ImageFormat.C8, PaletteFormat.RGB5A3)
        print("  [+] Injected pressed ss_native_a_button_pressed.png into ba_base_03.bti (pressed state)")

    # Blank out ba_a.bti so no duplicate 'A' renders over the dome
    a_entry_raw = rarc.get_file_entry("ba_a.bti")
    if a_entry_raw:
        blank_a = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
        inject_texture(a_entry_raw, blank_a, (16, 16), ImageFormat.I4)
        print("  [+] Blanked ba_a.bti (dome already includes embossed A)")

    # 4. Native Skyward Sword D-Pad Cross with Active Wing Highlight
    dpad_img = Image.open(os.path.join(FINAL_ASSETS_DIR, "ss_native_dpad_right.png"))
    dpad_entry = rarc.get_file_entry("cross_key_00.bti")
    if dpad_entry:
        inject_texture(dpad_entry, dpad_img, (48, 48), ImageFormat.C8, PaletteFormat.RGB5A3)
        print("  [+] Injected native ss_native_dpad.png into cross_key_00.bti")

    # 5. Native Skyward Sword (-) Button
    minus_img = Image.open(os.path.join(FINAL_ASSETS_DIR, "ss_native_minus.png"))
    z_entry = rarc.get_file_entry("ba_base_z.bti")
    if z_entry:
        inject_texture(z_entry, minus_img, (36, 36), ImageFormat.C8, PaletteFormat.RGB5A3)
        print("  [+] Injected native ss_native_minus.png into ba_base_z.bti")

    # 6. Native Skyward Sword (+) Button (Replaces ba_base_05 trapezoid!)
    plus_img = Image.open(os.path.join(FINAL_ASSETS_DIR, "ss_native_plus.png"))
    for plus_base in ["ba_base_05.bti", "ba_s.bti"]:
        entry = rarc.get_file_entry(plus_base)
        if entry:
            inject_texture(entry, plus_img, (36, 36), ImageFormat.C8, PaletteFormat.RGB5A3)
            print(f"  [+] Injected native ss_native_plus.png into {plus_base}")
    # Blank out ba_r.bti so no duplicate 'R' renders over the (+) button
    r_entry = rarc.get_file_entry("ba_r.bti")
    if r_entry:
        blank_r = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
        inject_texture(r_entry, blank_r, (16, 16), ImageFormat.I4)
        print("  [+] Blanked ba_r.bti (plus button self-contained)")

    # 7. Native Skyward Sword Cursor Hand
    hand_img = Image.open(os.path.join(FINAL_ASSETS_DIR, "ss_native_cursor_hand.png"))
    hand_entry = rarc.get_file_entry("cursor_00_01.bti")
    if hand_entry:
        inject_texture(hand_entry, hand_img, (28, 28), ImageFormat.C8, PaletteFormat.RGB5A3)
        print("  [+] Injected native ss_native_cursor_hand.png into cursor_00_01.bti")

    rarc.save_changes()
    menures_bytes = rarc.data.getvalue()
    with open(MENURES_PATH, "wb") as f:
        f.write(menures_bytes)
    print("Updated menures.arc with native Skyward Sword UI textures!")

    # Pack into Playable ISO
    print(f"Packing updated menures.arc into {PLAYABLE_ISO}...")
    gcm = GCM(BASE_ISO)
    gcm.read_entire_disc()
    gcm.changed_files["files/res/Msg/menures.arc"] = BytesIO(menures_bytes)

    gcm.export_disc_to_iso_with_changed_files(PLAYABLE_ISO)

    shutil.copy2(PLAYABLE_ISO, SD_ISO)
    print(f"Exported to {PLAYABLE_ISO} and updated SD card at {SD_ISO}!")

if __name__ == "__main__":
    main()
