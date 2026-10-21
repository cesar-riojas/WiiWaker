#!/usr/bin/env python3
"""
WiiWaker - Game Asset & ISO Patcher
Applies the Skyward Sword HUD layout and authentic Wii Remote & MotionPlus
visual assets to any retail GameCube Wind Waker ISO or extracted filesystem.

Usage:
  python3 tools/patch_game_iso.py --input path/to/WindWaker.iso --output path/to/WiiWaker_Playable.iso
  python3 tools/patch_game_iso.py --menures path/to/menures.arc
"""

import os
import sys
import argparse
import struct
from io import BytesIO
from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))
sys.path.append(os.path.join(SCRIPT_DIR, "wwlib"))

try:
    from rarc import RARC
    from bti import BTIFile, ImageFormat, PaletteFormat
    from gcm import GCM
except ImportError:
    from wwlib.rarc import RARC
    from wwlib.bti import BTIFile, ImageFormat, PaletteFormat
    from wwlib.gcm import GCM

FINAL_ASSETS_DIR = os.path.join(REPO_ROOT, "wii", "assets", "skyward_sword_native", "final")

PANE_LAYOUT = {
    # --- UNWANTED ELEMENTS TO HIDE (Off-screen) ---
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

    # --- ROW 1 (TOP): Centerline X = 572 ---
    "baat": (552, 90, 48, 48),    # Circular gold medallion socket (48x48 centered at 572)
    "xitm": (554, 94, 36, 36),    # Top slot item icon
    "bab":  (560, 96, 24, 34),    # Official [B] button key (24x34 authentic aspect ratio)

    # --- ROW 2 (MIDDLE): Centerline X = 572 ---
    "ba02": (554, 155, 36, 36),   # D-Pad (+) Cross key
    "yitm": (554, 155, 36, 36),   # D-Pad item slot

    # --- ROW 3 (LOWER): Centerline X = 572 ---
    "bawe": (552, 220, 40, 40),   # Domed (A) button (40x40 centered at 572)
    "bat2": (552, 220, 40, 40),   # A button active enlarged ring
    "bawp": (545, 266, 80, 20),   # Contextual Action Text ("Talk", "Open", "Roll", etc.)

    # --- ROW 4 (BOTTOM): (-) on Left, (+) on Right (Symmetric around 572) ---
    "baz":  (543, 305, 26, 26),   # (-) button
    "ba0r": (575, 305, 26, 26),   # (+) button
}

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

def patch_menures_archive(menures_bytes):
    rarc = RARC()
    rarc.read(BytesIO(menures_bytes))

    # 1. Patch main_parts1.blo layout
    blo_entry = rarc.get_file_entry("main_parts1.blo")
    if blo_entry:
        data = bytearray(blo_entry.data.getvalue())
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
        print(f"  [+] Patched {patched} panes in main_parts1.blo (Skyward Sword layout)")
        blo_entry.data = BytesIO(data)

    # 2. Inject Native Solid Gold Medallion Socket
    gold_socket_path = os.path.join(FINAL_ASSETS_DIR, "ss_native_solid_gold_socket.png")
    if os.path.exists(gold_socket_path):
        gear_img = Image.open(gold_socket_path)
        for bti_name in ["ba_base_02.bti", "ba_base_00.bti"]:
            entry = rarc.get_file_entry(bti_name)
            if entry:
                inject_texture(entry, gear_img, (48, 48), ImageFormat.C8, PaletteFormat.RGB5A3)
        print("  [+] Injected ss_native_solid_gold_socket.png into ba_base_02.bti / ba_base_00.bti")

    # 3. Inject Authentic Nintendo [B] Key (24x34)
    b_path = os.path.join(FINAL_ASSETS_DIR, "ss_official_clean_b_button.png")
    if os.path.exists(b_path):
        b_entry = rarc.get_file_entry("ba_b.bti")
        if b_entry:
            b_img = Image.open(b_path)
            inject_texture(b_entry, b_img, (24, 34), ImageFormat.C8, PaletteFormat.RGB5A3)
            print("  [+] Injected authentic clean Nintendo [B] button (24x34) into ba_b.bti")

    # 4. Inject Native Skyward Sword (A) Button Dome (idle & pressed)
    a_path = os.path.join(FINAL_ASSETS_DIR, "ss_native_a_button.png")
    if os.path.exists(a_path):
        a_entry = rarc.get_file_entry("ba_base_04.bti")
        if a_entry:
            a_img = Image.open(a_path)
            inject_texture(a_entry, a_img, (44, 44), ImageFormat.C8, PaletteFormat.RGB5A3)
            print("  [+] Injected normal ss_native_a_button.png into ba_base_04.bti (idle raised)")

    a_press_path = os.path.join(FINAL_ASSETS_DIR, "ss_native_a_button_pressed.png")
    if os.path.exists(a_press_path):
        a_pressed_entry = rarc.get_file_entry("ba_base_03.bti")
        if a_pressed_entry:
            a_press_img = Image.open(a_press_path)
            inject_texture(a_pressed_entry, a_press_img, (44, 44), ImageFormat.C8, PaletteFormat.RGB5A3)
            print("  [+] Injected pressed ss_native_a_button_pressed.png into ba_base_03.bti (flush recessed)")

    # Blank out ba_a.bti so no duplicate 'A' renders over the dome
    a_entry_raw = rarc.get_file_entry("ba_a.bti")
    if a_entry_raw:
        blank_a = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
        inject_texture(a_entry_raw, blank_a, (16, 16), ImageFormat.I4)
        print("  [+] Blanked duplicate ba_a.bti")

    # 5. Inject D-Pad Cross
    dpad_path = os.path.join(FINAL_ASSETS_DIR, "ss_native_dpad_right.png")
    if os.path.exists(dpad_path):
        dpad_entry = rarc.get_file_entry("cross_key_00.bti")
        if dpad_entry:
            dpad_img = Image.open(dpad_path)
            inject_texture(dpad_entry, dpad_img, (48, 48), ImageFormat.C8, PaletteFormat.RGB5A3)
            print("  [+] Injected native ss_native_dpad.png into cross_key_00.bti")

    # 6. Inject (-) Button
    minus_path = os.path.join(FINAL_ASSETS_DIR, "ss_native_minus.png")
    if os.path.exists(minus_path):
        z_entry = rarc.get_file_entry("ba_base_z.bti")
        if z_entry:
            minus_img = Image.open(minus_path)
            inject_texture(z_entry, minus_img, (36, 36), ImageFormat.C8, PaletteFormat.RGB5A3)
            print("  [+] Injected native ss_native_minus.png into ba_base_z.bti")

    # 7. Inject (+) Button
    plus_path = os.path.join(FINAL_ASSETS_DIR, "ss_native_plus.png")
    if os.path.exists(plus_path):
        plus_img = Image.open(plus_path)
        for plus_base in ["ba_base_05.bti", "ba_s.bti"]:
            entry = rarc.get_file_entry(plus_base)
            if entry:
                inject_texture(entry, plus_img, (36, 36), ImageFormat.C8, PaletteFormat.RGB5A3)
        # Blank ba_r.bti
        r_entry = rarc.get_file_entry("ba_r.bti")
        if r_entry:
            blank_r = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
            inject_texture(r_entry, blank_r, (16, 16), ImageFormat.I4)
        print("  [+] Injected native ss_native_plus.png into ba_base_05.bti / ba_s.bti")

    # 8. Inject Cursor Hand
    hand_path = os.path.join(FINAL_ASSETS_DIR, "ss_native_cursor_hand.png")
    if os.path.exists(hand_path):
        hand_entry = rarc.get_file_entry("cursor_00_01.bti")
        if hand_entry:
            hand_img = Image.open(hand_path)
            inject_texture(hand_entry, hand_img, (28, 28), ImageFormat.C8, PaletteFormat.RGB5A3)
            print("  [+] Injected native ss_native_cursor_hand.png into cursor_00_01.bti")

    rarc.save_changes()
    return rarc.data.getvalue()

def main():
    parser = argparse.ArgumentParser(description="WiiWaker Game Asset & ISO Patcher")
    parser.add_argument("--input", "-i", help="Path to clean input Wind Waker GameCube ISO")
    parser.add_argument("--output", "-o", help="Path to output patched WiiWaker Playable ISO")
    parser.add_argument("--menures", "-m", help="Path to an individual menures.arc file to patch directly")
    args = parser.parse_args()

    print("==========================================================")
    print(" WiiWaker - Skyward Sword HUD & Wii Asset Patcher")
    print("==========================================================")

    if args.menures:
        if not os.path.isfile(args.menures):
            print(f"Error: {args.menures} does not exist.")
            sys.exit(1)
        print(f"Patching archive: {args.menures}")
        with open(args.menures, "rb") as f:
            data = f.read()
        patched = patch_menures_archive(data)
        with open(args.menures, "wb") as f:
            f.write(patched)
        print("Archive patched successfully!")
        return

    if not args.input:
        print("Error: Please provide --input <WindWaker.iso> or --menures <menures.arc>")
        parser.print_help()
        sys.exit(1)

    if not os.path.isfile(args.input):
        print(f"Error: Input ISO not found: {args.input}")
        sys.exit(1)

    out_iso = args.output or "WiiWaker_Playable.iso"

    print(f"Reading input ISO: {args.input}")
    gcm = GCM(args.input)
    gcm.read_entire_disc()

    menures_rel_path = "files/res/Msg/menures.arc"
    if menures_rel_path not in gcm.files_by_path:
        print(f"Error: {menures_rel_path} not found in ISO!")
        sys.exit(1)

    print(f"Extracting {menures_rel_path} from ISO...")
    menures_data = gcm.read_file_data(menures_rel_path).getvalue()

    print("Patching HUD layout and textures...")
    patched_menures = patch_menures_archive(menures_data)

    print(f"Injecting patched {menures_rel_path} into output ISO...")
    gcm.changed_files[menures_rel_path] = BytesIO(patched_menures)

    print(f"Exporting patched playable disc to: {out_iso}...")
    gcm.export_disc_to_iso_with_changed_files(out_iso)

    print("==========================================================")
    print(f" SUCCESS! Patched playable game created at: {out_iso}")
    print("==========================================================")

if __name__ == "__main__":
    main()
