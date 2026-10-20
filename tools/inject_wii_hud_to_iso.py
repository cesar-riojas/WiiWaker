#!/usr/bin/env python3
"""
Injects the Wii HUD button textures directly into the game ISO
so the Wii button icons display immediately in Dolphin and Nintendont/Wii.
"""

import sys
import os
import time
from io import BytesIO

sys.path.append("/Users/cesarriojas/Games/betterww")
from wwlib.gcm import GCM

def inject_wii_hud():
    in_iso = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/BetterWindWaker.iso"
    out_iso = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/WiiWaker_Playable.iso"
    menures_path = "/Users/cesarriojas/Games/WiiWaker/dist/apps/WiiWaker/files/res/Msg/menures.arc"

    print(f"Reading base ISO: {in_iso}")
    start = time.time()
    gcm = GCM(in_iso)
    gcm.read_entire_disc()

    print(f"Loading custom Wii button archive: {menures_path}")
    with open(menures_path, "rb") as f:
        custom_menures = BytesIO(f.read())

    gcm.changed_files["files/res/Msg/menures.arc"] = custom_menures

    print(f"Exporting ISO with native Wii button HUD to: {out_iso}")
    gcm.export_disc_to_iso_with_changed_files(out_iso)

    # Also update the Nintendont ready copy
    nintendont_path = "/Users/cesarriojas/Games/Game Cube/Better Wind Waker/SD_Card_Ready/games/The Legend of Zelda The Wind Waker [GZLE01]/game.iso"
    print(f"Updating Nintendont SD card copy at: {nintendont_path}")
    import shutil
    shutil.copy2(out_iso, nintendont_path)

    elapsed = time.time() - start
    print(f"Done in {elapsed:.2f}s! You can now load WiiWaker_Playable.iso in Dolphin to see the Wii button HUD in-game!")

if __name__ == "__main__":
    inject_wii_hud()
