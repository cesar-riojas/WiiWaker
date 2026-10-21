#!/usr/bin/env python3
"""
Injects the authentic Wind Waker cel-shaded UI textures (extracted from Gemini artwork):
1. ww_golden_gear_b.png -> ba_base_02.bti, ba_base_00.bti
2. Transparent blank -> ba_b.bti (so the ornate B emblem on ba_base_02 shows through cleanly)
3. ww_dpad.png -> cross_key_00.bti
4. ww_a_button.png -> ba_a.bti
5. ww_minus.png -> ba_base_z.bti
6. ww_plus.png -> ba_s.bti, ba_r.bti

Then updates:
- dist/apps/WiiWaker/files/res/Msg/menures.arc
- WiiWaker_Playable.iso
- SD card / Nintendont copy
"""

import sys
import os
import shutil
from io import BytesIO
from PIL import Image

sys.path.append("/Users/cesarriojas/Games/betterww")
from wwlib.rarc import RARC
from wwlib.bti import BTIFile, ImageFormat, PaletteFormat
from wwlib.gcm import GCM

ASSETS_DIR = "/Users/cesarriojas/Games/WiiWaker/wii/assets/wind_waker_cel_shaded"
MENURES_PATH = "/Users/cesarriojas/Games/WiiWaker/dist/apps/WiiWaker/files/res/Msg/menures.arc"
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
    print(f"Opening {MENURES_PATH}...")
    with open(MENURES_PATH, "rb") as f:
        file_bytes = f.read()

    rarc = RARC()
    rarc.read(BytesIO(file_bytes))

    # 1. Golden Gear [B] button
    gear_img = Image.open(os.path.join(ASSETS_DIR, "ww_golden_gear_b.png"))
    for bti_name in ["ba_base_02.bti", "ba_base_00.bti"]:
        entry = rarc.get_file_entry(bti_name)
        if entry:
            inject_texture(entry, gear_img, (48, 48), ImageFormat.C8, PaletteFormat.RGB5A3)
            print(f"  [+] Injected ww_golden_gear_b.png into {bti_name}")

    # 2. Transparent glyph for ba_b so the golden gear's embedded B displays crisp and unshadowed
    b_entry = rarc.get_file_entry("ba_b.bti")
    if b_entry:
        blank_b = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
        inject_texture(b_entry, blank_b, (16, 16), ImageFormat.I4)
        print("  [+] Set ba_b.bti to transparent pass-through")

    # 3. Wind Waker Cel-shaded D-Pad
    dpad_img = Image.open(os.path.join(ASSETS_DIR, "ww_dpad.png"))
    dpad_entry = rarc.get_file_entry("cross_key_00.bti")
    if dpad_entry:
        inject_texture(dpad_entry, dpad_img, (32, 32), ImageFormat.C8, PaletteFormat.RGB5A3)
        print("  [+] Injected ww_dpad.png into cross_key_00.bti")

    # 4. Wind Waker Cel-shaded (A) Button
    a_img = Image.open(os.path.join(ASSETS_DIR, "ww_a_button.png"))
    a_entry = rarc.get_file_entry("ba_a.bti")
    if a_entry:
        inject_texture(a_entry, a_img, (32, 32), ImageFormat.C8, PaletteFormat.RGB5A3)
        print("  [+] Injected ww_a_button.png into ba_a.bti")

    # 5. Wind Waker Nautical (-) Medallion
    minus_img = Image.open(os.path.join(ASSETS_DIR, "ww_minus.png"))
    z_entry = rarc.get_file_entry("ba_base_z.bti")
    if z_entry:
        inject_texture(z_entry, minus_img, (32, 32), ImageFormat.C8, PaletteFormat.RGB5A3)
        print("  [+] Injected ww_minus.png into ba_base_z.bti")

    # 6. Wind Waker Nautical (+) Medallion
    plus_img = Image.open(os.path.join(ASSETS_DIR, "ww_plus.png"))
    for s_name in ["ba_s.bti", "ba_r.bti"]:
        s_entry = rarc.get_file_entry(s_name)
        if s_entry:
            inject_texture(s_entry, plus_img, (24, 24), ImageFormat.C8, PaletteFormat.RGB5A3)
            print(f"  [+] Injected ww_plus.png into {s_name}")

    rarc.save_changes()
    menures_bytes = rarc.data.getvalue()
    with open(MENURES_PATH, "wb") as f:
        f.write(menures_bytes)
    print("Updated menures.arc with cel-shaded Wind Waker UI textures!")

    # Update Playable ISO
    print("Packing updated menures.arc into WiiWaker_Playable.iso...")
    gcm = GCM(BASE_ISO)
    gcm.read_entire_disc()
    gcm.changed_files["files/res/Msg/menures.arc"] = BytesIO(menures_bytes)
    gcm.export_disc_to_iso_with_changed_files(PLAYABLE_ISO)

    shutil.copy2(PLAYABLE_ISO, SD_ISO)
    print(f"Exported to {PLAYABLE_ISO} and updated SD Card / Nintendont copy at {SD_ISO}!")

if __name__ == "__main__":
    main()
