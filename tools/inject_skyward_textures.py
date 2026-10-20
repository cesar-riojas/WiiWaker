#!/usr/bin/env python3
"""
Converts the generated Skyward Sword HUD textures to BTI and injects them
into menures.arc and the playable ISO.
"""

import sys
import os
from io import BytesIO
from PIL import Image

sys.path.append("/Users/cesarriojas/Games/betterww")
from wwlib.rarc import RARC
from wwlib.bti import BTIFile
from wwlib.gcm import GCM

ASSETS_DIR = "/Users/cesarriojas/Games/WiiWaker/wii/assets/skyward_sword"
MENURES_PATH = "/Users/cesarriojas/Games/WiiWaker/dist/apps/WiiWaker/files/res/Msg/menures.arc"

def inject_skyward_textures():
    print(f"Reading {MENURES_PATH}...")
    with open(MENURES_PATH, "rb") as f:
        file_bytes = f.read()

    rarc = RARC()
    rarc.read(BytesIO(file_bytes))

    replacements = {
        "ba_base_02.bti": os.path.join(ASSETS_DIR, "ss_golden_gear_ring.png"),
        "ba_base_00.bti": os.path.join(ASSETS_DIR, "ss_golden_gear_ring.png"),
        "ba_b.bti":       os.path.join(ASSETS_DIR, "ss_b_button.png"),
        "ba_a.bti":       os.path.join(ASSETS_DIR, "ss_a_button.png"),
        "cross_key_00.bti": os.path.join(ASSETS_DIR, "ss_dpad.png"),
    }

    for bti_name, png_path in replacements.items():
        entry = rarc.get_file_entry(bti_name)
        if entry and os.path.isfile(png_path):
            img = Image.open(png_path)
            bti = BTIFile(entry.data)
            # Resize image to match BTI target dimensions
            img_resized = img.resize((bti.width, bti.height), Image.Resampling.LANCZOS)
            bti.replace_image(img_resized)
            bti.save_changes()
            entry.data = bti.data
            print(f"  [+] Injected {os.path.basename(png_path)} into {bti_name} ({bti.width}x{bti.height})")

    rarc.save_changes()
    with open(MENURES_PATH, "wb") as f:
        f.write(rarc.data.getvalue())

    print("Updated menures.arc with Skyward Sword textures!")

    # Update playable ISO
    in_iso = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/BetterWindWaker.iso"
    out_iso = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/WiiWaker_Playable.iso"
    nintendont_path = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/SD_Card_Ready/games/The Legend of Zelda The Wind Waker [GZLE01]/game.iso"

    gcm = GCM(in_iso)
    gcm.read_entire_disc()
    gcm.changed_files["files/res/Msg/menures.arc"] = BytesIO(rarc.data.getvalue())
    gcm.export_disc_to_iso_with_changed_files(out_iso)

    import shutil
    shutil.copy2(out_iso, nintendont_path)
    print("Re-exported WiiWaker_Playable.iso with Skyward Sword textures and layout!")

if __name__ == "__main__":
    inject_skyward_textures()
