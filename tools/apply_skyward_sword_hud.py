#!/usr/bin/env python3
"""
Applies Skyward Sword's iconic vertical Wii Remote HUD layout to The Wind Waker:
- Stacks Action (A), Sword (B), Item [1], Item [2], and Item [Z] vertically on the right margin
- Aligns contextual action text to the left of the (A) button
- Re-packs menures.arc and injects it into WiiWaker_Playable.iso
"""

import sys
import os
import struct
from io import BytesIO

sys.path.append("/Users/cesarriojas/Games/betterww")
from wwlib.rarc import RARC
from wwlib.gcm import GCM

def apply_ss_hud_layout():
    menures_path = "/Users/cesarriojas/Games/WiiWaker/dist/apps/WiiWaker/files/res/Msg/menures.arc"
    print(f"Reading {menures_path}...")

    with open(menures_path, "rb") as f:
        file_bytes = f.read()

    rarc = RARC()
    rarc.read(BytesIO(file_bytes))

    entry = rarc.get_file_entry("main_parts1.blo")
    data = bytearray(entry.data.getvalue())

    # Target new positions for Skyward Sword vertical stack:
    # Right column center around X=576
    new_coords = {
        # --- (A) Action Button & Contextual Labels (Row 1: Y ~ 40-90) ---
        "bawe": (550, 40),      # A button outer circle base
        "baa":  (570, 60),      # A button icon
        "bat2": (535, 25),      # A button active enlarged ring
        "baa2": (566, 56),      # A button active enlarged icon
        "bawd": (460, 56),      # Contextual action text ("Open", "Roll", "Grab", etc.)
        "bawp": (460, 56),      # "Put away" action text
        "basv": (460, 56),      # Save / Special action text

        # --- (B) Sword / Cancel Button (Row 2: Y ~ 108-148) ---
        "baat": (558, 108),     # B button base
        "bab":  (570, 120),     # B button icon

        # --- Item Slot [1] (Row 3: Y ~ 160-208) ---
        "ba01": (555, 160),     # Item 1 outer base
        "b001": (558, 171),     # Item 1 inner frame
        "bax1": (536, 175),     # '[1]' button badge
        "xitk": (558, 159),     # Item 1 icon backing
        "xitm": (558, 159),     # Item 1 icon
        "x10":  (576, 192),     # Ammo tens
        "x01":  (589, 192),     # Ammo ones

        # --- Item Slot [2] (Row 4: Y ~ 222-270) ---
        "ba02": (555, 222),     # Item 2 outer base
        "b002": (558, 233),     # Item 2 inner frame
        "bay":  (536, 237),     # '[2]' button badge
        "yitk": (555, 221),     # Item 2 icon backing
        "yitm": (555, 221),     # Item 2 icon
        "y10":  (576, 254),     # Ammo tens
        "y01":  (589, 254),     # Ammo ones

        # --- Item Slot [Z] (Row 5: Y ~ 284-332) ---
        "baz":  (554, 284),     # Item 3 base
        "zitk": (554, 276),     # Item 3 icon backing
        "zitm": (555, 277),     # Item 3 icon
        "z10":  (576, 308),     # Ammo tens
        "z01":  (589, 308),     # Ammo ones

        # --- Nunchuk Shield / C Button (Top Banner) ---
        "ba0r": (586, 5),       # R button base
        "bar1": (602, 17),      # (C) button badge
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
                # pack big-endian s16 at pos+0x10 and pos+0x12
                struct.pack_into(">2h", data, pos + 0x10, nx, ny)
                patched_count += 1
                print(f"  [>] Positioned '{name4}' at ({nx}, {ny})")
            pos += size
        else:
            pos += 4

    print(f"Patched {patched_count} pane coordinates to Skyward Sword vertical layout!")

    # Save modified blo back into entry
    entry.data = BytesIO(data)

    rarc.save_changes()
    with open(menures_path, "wb") as f:
        f.write(rarc.data.getvalue())

    print(f"Saved Skyward Sword HUD layout to {menures_path}!")

    # Inject into game ISO
    in_iso = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/BetterWindWaker.iso"
    out_iso = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/WiiWaker_Playable.iso"
    nintendont_path = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/SD_Card_Ready/games/The Legend of Zelda The Wind Waker [GZLE01]/game.iso"

    print(f"Re-injecting into playable ISO: {out_iso}...")
    gcm = GCM(in_iso)
    gcm.read_entire_disc()
    gcm.changed_files["files/res/Msg/menures.arc"] = BytesIO(rarc.data.getvalue())
    gcm.export_disc_to_iso_with_changed_files(out_iso)

    import shutil
    shutil.copy2(out_iso, nintendont_path)
    print("Skyward Sword HUD injected into WiiWaker_Playable.iso and SD Card copy!")

if __name__ == "__main__":
    apply_ss_hud_layout()
