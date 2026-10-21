#!/usr/bin/env python3
"""
Applies Skyward Sword's exact in-game HUD hierarchy (from official reference):
- Top: Ornate golden gear emblem with [B] button & "Items/Sword" text below it
- Middle: D-Pad (+) cross-key
- Lower: Domed (A) button with dynamic contextual text ("Dodge/Roll/Open") below it
- Bottom: (-) and (+) buttons
"""

import sys
import os
import struct
from io import BytesIO

sys.path.append("/Users/cesarriojas/Games/betterww")
from wwlib.rarc import RARC
from wwlib.gcm import GCM

def apply_ss_hud_exact():
    menures_path = "/Users/cesarriojas/Games/WiiWaker/dist/apps/WiiWaker/files/res/Msg/menures.arc"
    print(f"Reading {menures_path}...")

    with open(menures_path, "rb") as f:
        file_bytes = f.read()

    rarc = RARC()
    rarc.read(BytesIO(file_bytes))

    entry = rarc.get_file_entry("main_parts1.blo")
    data = bytearray(entry.data.getvalue())

    # Exact Skyward Sword vertical stack:
    # Right column center around X=576
    new_coords = {
        # --- TOP: [B] Button (Items/Sword) inside Golden Gear Ring (Y ~ 35 - 95) ---
        "baat": (552, 35),      # B button ornate golden base/gear
        "bab":  (564, 47),      # [B] button glyph
        "ba01": (552, 35),      # Item 1 frame (co-located inside B gear)
        "b001": (556, 42),      # Item 1 inner frame
        "xitk": (552, 35),      # Item icon backing
        "xitm": (552, 35),      # Item icon
        "bax1": (540, 50),      # Item indicator tab
        "x10":  (570, 75),      # Ammo tens
        "x01":  (583, 75),      # Ammo ones

        # --- MIDDLE: D-Pad (+) Cross Key / Secondary Quick Slots (Y ~ 120 - 170) ---
        "ba02": (556, 125),     # Item 2 / D-Pad slot
        "b002": (558, 130),     # Inner frame
        "bay":  (540, 135),     # '[2]' badge
        "yitk": (556, 125),     # Item 2 icon backing
        "yitm": (556, 125),     # Item 2 icon
        "y10":  (570, 160),     # Ammo tens
        "y01":  (583, 160),     # Ammo ones

        # --- LOWER: Domed (A) Button + Action Label Below (Y ~ 185 - 250) ---
        "bawe": (554, 185),     # A button outer base circle
        "baa":  (566, 197),     # (A) button glyph
        "bat2": (538, 170),     # A button active enlarged ring
        "baa2": (562, 194),     # A button active enlarged glyph
        # In Skyward Sword, the action text ("Dodge", "Roll", "Open") is directly BELOW (A)
        "bawd": (532, 230),     # Contextual action text
        "bawp": (532, 230),     # "Put away" text
        "basv": (532, 230),     # Save / Special action text

        # --- BOTTOM: (-) and (+) Buttons (Y ~ 275 - 315) ---
        # (-) button with metallic ring on the left
        "baz":  (544, 275),     # (-) base / Item 3 slot
        "zitk": (544, 275),     # Item icon
        "zitm": (545, 276),
        "z10":  (558, 295),
        "z01":  (571, 295),

        # (+) button smaller on the right
        "ba0r": (588, 280),     # (+) base
        "bar1": (594, 286),     # (+) glyph
    }

    pos = 0x20
    patched_count = 0
    while pos < len(data) - 8:
        tag = data[pos:pos+4].decode("latin1", "ignore")
        size = int.from_bytes(data[pos+4:pos+8], "big")
        if tag in ["PIC1", "PAN1", "TXT1", "WIN1"]:
            name4 = data[pos+12:pos+16].decode("latin1", "ignore").strip("\x00").strip()
            if name4 in new_coords:
                nx, ny = new_coords[name4]
                struct.pack_into(">2h", data, pos + 0x10, nx, ny)
                patched_count += 1
            pos += size
        else:
            pos += 4

    print(f"Patched {patched_count} pane coordinates to exact Skyward Sword HUD hierarchy!")

    entry.data = BytesIO(data)
    rarc.save_changes()
    with open(menures_path, "wb") as f:
        f.write(rarc.data.getvalue())

    print(f"Saved exact Skyward Sword HUD layout to {menures_path}!")

    # Inject into game ISO
    in_iso = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/BetterWindWaker.iso"
    out_iso = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/WiiWaker_Playable.iso"
    nintendont_path = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/SD_Card_Ready/games/The Legend of Zelda The Wind Waker [GZLE01]/game.iso"

    gcm = GCM(in_iso)
    gcm.read_entire_disc()
    gcm.changed_files["files/res/Msg/menures.arc"] = BytesIO(rarc.data.getvalue())
    gcm.export_disc_to_iso_with_changed_files(out_iso)

    import shutil
    shutil.copy2(out_iso, nintendont_path)
    print("Updated WiiWaker_Playable.iso with exact Skyward Sword layout!")

if __name__ == "__main__":
    apply_ss_hud_exact()
