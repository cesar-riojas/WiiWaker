#!/usr/bin/env python3
"""
Applies Skyward Sword's exact pixel-aligned layout to main_parts1.blo:
1. Hides the unwanted [1] and [2] badges (bax1, bay) and white cards (b001, b002, blx1, bly1, blz1).
2. Perfectly aligns all elements along the vertical centerline (X ~ 572).
3. Places:
   - Top: Golden crest medallion (baat/xitm) + white [B] button key (bab) directly beneath it.
   - Middle: D-Pad (+) cross (ba02) + equipped item (yitm).
   - Lower: Domed (A) button (bawe/baa) + contextual text directly beneath it.
   - Bottom: (-) on left, (+) on right.
"""

import sys
import os
import shutil
import struct
from io import BytesIO

sys.path.append("/Users/cesarriojas/Games/betterww")
from wwlib.rarc import RARC
from wwlib.gcm import GCM

MENURES_PATH = "/Users/cesarriojas/Games/WiiWaker/dist/apps/WiiWaker/files/res/Msg/menures.arc"
PLAYABLE_ISO = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/WiiWaker_Playable.iso"
SD_ISO = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/SD_Card_Ready/games/The Legend of Zelda The Wind Waker [GZLE01]/game.iso"

# Dimensions and positions (X, Y, W, H)
PANE_LAYOUT = {
    # --- UNWANTED ELEMENTS TO HIDE (X=-500, Y=-500, W=0, H=0) ---
    "bax1": (-500, -500, 0, 0),  # [1] badge
    "bay":  (-500, -500, 0, 0),  # [2] badge
    "b001": (-500, -500, 0, 0),  # white card 1
    "b002": (-500, -500, 0, 0),  # white card 2
    "blx1": (-500, -500, 0, 0),  # extra shadow card
    "blx2": (-500, -500, 0, 0),  # extra shadow card
    "blx3": (-500, -500, 0, 0),  # extra shadow card
    "bly1": (-500, -500, 0, 0),  # extra shadow card
    "bly2": (-500, -500, 0, 0),  # extra shadow card
    "bly3": (-500, -500, 0, 0),  # extra shadow card
    "blz1": (-500, -500, 0, 0),  # extra shadow card
    "blz2": (-500, -500, 0, 0),  # extra shadow card
    "blz3": (-500, -500, 0, 0),  # extra shadow card
    "cent": (-500, -500, 0, 0),  # old GameCube button cluster center backing
    "witk": (-500, -500, 0, 0),  # old GC item socket 3
    "weit": (-500, -500, 0, 0),  # old GC item socket 3
    "yumk": (-500, -500, 0, 0),  # old GC item socket 2
    "yum":  (-500, -500, 0, 0),  # old GC item socket 2
    "a10":  (-500, -500, 0, 0),  # old GC ammo
    "a01":  (-500, -500, 0, 0),  # old GC ammo
    "bawd": (-500, -500, 0, 0),  # Hide GC B-button action text
    "basv": (-500, -500, 0, 0),  # Hide GC R-button defend text (defend is on Nunchuk)
    "baa":  (-500, -500, 0, 0),  # bawe now renders authentic 3D dome directly
    "baa2": (-500, -500, 0, 0),
    "bar1": (-500, -500, 0, 0),  # ba0r now renders authentic (+) button directly
    "yzr":  (-500, -500, 0, 0),  # hide camera eye prompt overlay
    "r":    (-500, -500, 0, 0),
    "baat": (-500, -500, 0, 0),  # HIDE duplicate B button square!

    # --- ROW 1 (TOP): Solid Golden Circular Medallion Socket + Single [B] Key Below It ---
    "ba01": (550,  50, 44, 44),   # Item 1 socket (Solid Gold Medallion, shifted comfortably from edge)
    "xitk": (550,  50, 44, 44),   # Item icon frame
    "xitm": (555,  55, 34, 34),   # Equipped Tool Icon (perfectly centered inside medallion)
    "x10":  (566,  78, 12, 12),   # Ammo tens
    "x01":  (578,  78, 12, 12),   # Ammo ones
    "bab":  (560,  96, 24, 34),   # Official clean sans-serif Nintendo [B] button key (vertically stretched, centered on 572)

    # --- ROW 2 (MIDDLE): D-Pad (+) Cross + Directional Tool Spots ---
    "ba02": (550, 138, 44, 44),   # D-Pad cross key (centered on 572)
    # Left D-Pad Wing Tool Spot:
    "zitk": (536, 148, 24, 24),   # Left tool frame
    "zitm": (536, 148, 24, 24),   # Left tool icon
    "z10":  (538, 170, 10, 10),
    "z01":  (548, 170, 10, 10),
    # Right D-Pad Wing Tool Spot:
    "yitk": (584, 148, 24, 24),   # Right tool frame (e.g. Telescope)
    "yitm": (584, 148, 24, 24),   # Right tool icon
    "y10":  (588, 170, 10, 10),   # Ammo tens
    "y01":  (598, 170, 10, 10),   # Ammo ones

    # --- ROW 3 (LOWER): Domed White (A) Button + Action Label Below ---
    "bawe": (552, 220, 40, 40),   # White (A) button dome (centered on 572)
    "bat2": (552, 220, 40, 40),   # A button active enlarged ring
    "bawp": (545, 266, 80, 20),   # True (A) Button Contextual Action Text: "Interact", "Talk", "Open", "Put away", "Roll", "Check"!

    # --- ROW 4 (BOTTOM): (-) on Left, (+) on Right (Symmetric around 572) ---
    "baz":  (543, 305, 26, 26),   # (-) button (clean, no item overlap)
    "ba0r": (575, 305, 26, 26),   # (+) button
}

def apply_layout():
    print(f"Reading {MENURES_PATH}...")
    with open(MENURES_PATH, "rb") as f:
        file_bytes = f.read()

    rarc = RARC()
    rarc.read(BytesIO(file_bytes))
    entry = rarc.get_file_entry("main_parts1.blo")
    data = bytearray(entry.data.getvalue())

    pos = 0x20
    patched = 0
    while pos < len(data) - 8:
        tag = data[pos:pos+4].decode("latin1", "ignore")
        size = int.from_bytes(data[pos+4:pos+8], "big")
        if tag in ["PIC1", "PAN1", "TXT1", "WIN1"]:
            name4 = data[pos+12:pos+16].decode("latin1", "ignore").strip("\x00").strip()
            if name4 in PANE_LAYOUT:
                x, y, w, h = PANE_LAYOUT[name4]
                struct.pack_into(">4h", data, pos + 0x10, x, y, w, h)
                patched += 1
            pos += size
        else:
            pos += 4

    print(f"Patched {patched} panes in main_parts1.blo to pixel-perfect Skyward Sword layout!")
    entry.data = BytesIO(data)
    rarc.save_changes()

    menures_bytes = rarc.data.getvalue()
    with open(MENURES_PATH, "wb") as f:
        f.write(menures_bytes)

    # Export to ISO
    print(f"Updating {PLAYABLE_ISO}...")
    base_iso = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/BetterWindWaker.iso"
    gcm = GCM(base_iso)
    gcm.read_entire_disc()
    gcm.changed_files["files/res/Msg/menures.arc"] = BytesIO(menures_bytes)

    gcm.export_disc_to_iso_with_changed_files(PLAYABLE_ISO)

    shutil.copy2(PLAYABLE_ISO, SD_ISO)
    print("Exported WiiWaker_Playable.iso and updated SD Card copy!")

if __name__ == "__main__":
    apply_layout()
