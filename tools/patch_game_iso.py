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
    from dol import DOL
    from fs_helpers import read_u32, write_u32, write_bytes
except ImportError:
    from wwlib.rarc import RARC
    from wwlib.bti import BTIFile, ImageFormat, PaletteFormat
    from wwlib.gcm import GCM
    from wwlib.dol import DOL
    from wwlib.fs_helpers import read_u32, write_u32, write_bytes

FINAL_ASSETS_DIR = os.path.join(REPO_ROOT, "wii", "assets", "skyward_sword_native", "final")

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
    "baat": (-500, -500, 0, 0),  # HIDE duplicate B button square

    # --- ROW 1 (TOP): Solid Golden Circular Medallion Socket + Single [B] Key Below It ---
    "ba01": (550,  50, 44, 44),   # Item 1 socket (Solid Gold Medallion, centered at 572)
    "xitk": (550,  50, 44, 44),   # Item icon frame
    "xitm": (555,  55, 34, 34),   # Equipped Tool Icon (centered inside medallion)
    "x10":  (566,  78, 12, 12),   # Ammo tens
    "x01":  (578,  78, 12, 12),   # Ammo ones
    "bab":  (560,  96, 24, 34),   # Official clean sans-serif Nintendo [B] button key (24x34 centered on 572)

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
    "bawp": (545, 266, 80, 20),   # True (A) Button Contextual Action Text ("Talk", "Open", "Roll", "Check")

    # --- ROW 4 (BOTTOM): (-) on Left, (+) on Right (Symmetric around 572) ---
    "baz":  (543, 305, 26, 26),   # (-) button (clean, no item overlap)
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


#Assembled PowerPC machine code for wiiwaker_pad_hook (91 instructions / 364 bytes).
# Hooks JUTGamePad::read at 0x802C39AC (replacing `bl PADClamp` at 0x80314C90).
# Implements Skyward Sword HUD gameplay controls directly in sys/main.dol:
#  - D-Pad Left  (0x0001) -> Z Item Button (0x0010, Left Wing slot zitm)
#  - D-Pad Right (0x0002) -> Y Item Button (0x0800, Right Wing slot yitm)
#  - D-Pad Down  (0x0004) -> Sea Chart / Dungeon Map (0x0008, vanilla D-Pad Up)
#  - D-Pad Up    (0x0008) -> Quick-cycle Top Medallion (xitm / Slot X) through owned inventory items
WIIWAKER_PAD_HOOK_WORDS = [
    0x9421FFE0,  # stwu    r1, -32(r1)
    0x7C0802A6,  # mflr    r0
    0x90010024,  # stw     r0, 36(r1)
    0x3D808031,  # lis     r12, 0x8031
    0x618C4C90,  # ori     r12, r12, 0x4C90 (PADClamp)
    0x7D8903A6,  # mtctr   r12
    0x4E800421,  # bctrl
    0x3C60803F,  # lis     r3, 0x803F
    0x3863D818,  # addi    r3, r3, -0x27E8  (r3 = &JUTGamePad::mPadStatus[0] = 0x803ED818)
    0x8803000A,  # lbz     r0, 10(r3)       (mPadStatus[0].err)
    0x7C000775,  # extsb.  r0, r0
    0x4082012C,  # bne     .L_done
    0xA0830000,  # lhz     r4, 0(r3)        (r4 = curr_btn)
    0x48000005,  # bl      .L_get_pc
    0x7D6802A6,  # mflr    r11
    0xA0AB0130,  # lhz     r5, 304(r11)     (r5 = prev_btn)
    0xB08B0130,  # sth     r4, 304(r11)     (prev_btn = curr_btn)
    0x7C862878,  # andc    r6, r4, r5       (r6 = newly_pressed = curr_btn & ~prev_btn)
    0x880D8FB7,  # lbz     r0, -28745(r13)  (dMenu_pause at 0x803EF097)
    0x2C000000,  # cmpwi   r0, 0
    0x408200E8,  # bne     .L_menu_open
    0x3CE0803C,  # lis     r7, 0x803C
    0x60E74C08,  # ori     r7, r7, 0x4C08   (r7 = &g_dComIfG_gameInfo = 0x803C4C08)
    0x88075BE6,  # lbz     r0, 0x5BE6(r7)   (dComIfGp_getOperateWind at 0x803CA7EE)
    0x2C000000,  # cmpwi   r0, 0
    0x408200F4,  # bne     .L_done          (do not remap D-Pad while conducting Wind Waker)
    0x54880036,  # rlwinm  r8, r4, 0, 0, 27 (clear low 4 D-Pad bits 0x000F)
    0x70800001,  # andi.   r0, r4, 0x0001   (D-Pad Left?)
    0x41820008,  # beq     +8
    0x61080010,  # ori     r8, r8, 0x0010   (set PAD_TRIGGER_Z -> Left Wing slot zitm)
    0x70800002,  # andi.   r0, r4, 0x0002   (D-Pad Right?)
    0x41820008,  # beq     +8
    0x61080800,  # ori     r8, r8, 0x0800   (set PAD_BUTTON_Y -> Right Wing slot yitm)
    0x70800004,  # andi.   r0, r4, 0x0004   (D-Pad Down?)
    0x41820008,  # beq     +8
    0x61080008,  # ori     r8, r8, 0x0008   (set PAD_BUTTON_UP -> open Sea Chart / Map)
    0xB1030000,  # sth     r8, 0(r3)        (store remapped buttons to mPadStatus[0].button)
    0x70C00008,  # andi.   r0, r6, 0x0008   (D-Pad Up newly pressed?)
    0x418200C0,  # beq     .L_done
    0x89070009,  # lbz     r8, 9(r7)        (r8 = save.mSelectItem[0] at 0x803C4C11)
    0x2C080015,  # cmpwi   r8, 21
    0x41800008,  # blt     +8
    0x3900FFFF,  # li      r8, -1
    0x39200015,  # li      r9, 21
    0x7D2903A6,  # mtctr   r9
    0x3947003C,  # addi    r10, r7, 0x3C    (r10 = &save.mItems[0] at 0x803C4C44)
    0x39080001,  # addi    r8, r8, 1        (.L_cycle_loop)
    0x2C080015,  # cmpwi   r8, 21
    0x41800008,  # blt     +8
    0x39000000,  # li      r8, 0
    0x7C0A40AE,  # lbzx    r0, r10, r8      (r0 = save.mItems[r8])
    0x280000FF,  # cmplwi  r0, 0xFF
    0x41820060,  # beq     .L_cycle_next
    0x8927000A,  # lbz     r9, 10(r7)       (save.mSelectItem[1] = Y slot)
    0x7C084800,  # cmpw    r8, r9
    0x41820054,  # beq     .L_cycle_next
    0x8927000B,  # lbz     r9, 11(r7)       (save.mSelectItem[2] = Z slot)
    0x7C084800,  # cmpw    r8, r9
    0x41820048,  # beq     .L_cycle_next
    0x99070009,  # stb     r8, 9(r7)        (save.mSelectItem[0] = r8)
    0x98075BD3,  # stb     r0, 0x5BD3(r7)   (play.mSelectItem[0] = r0 at 0x803CA7DB)
    0x806D9630,  # lwz     r3, -27088(r13)  (mDoAud_zelAudio_c::mAudioMgrPtr)
    0x38800817,  # li      r4, 0x817        (JA_SE_ITM_MENU_SET)
    0x38A00000,  # li      r5, 0
    0x38C00000,  # li      r6, 0
    0x38E00000,  # li      r7, 0
    0xC022B2C8,  # lfs     f1, -19768(r2)   (1.0f)
    0xFC400890,  # fmr     f2, f1
    0xC062B2D8,  # lfs     f3, -19752(r2)   (-1.0f)
    0xFC801890,  # fmr     f4, f3
    0x39000000,  # li      r8, 0
    0x3D80802A,  # lis     r12, 0x802A
    0x618C6720,  # ori     r12, r12, 0x6720 (mDoAud_seStart)
    0x7D8903A6,  # mtctr   r12
    0x4E800421,  # bctrl
    0x4800002C,  # b       .L_done
    0x4200FF88,  # bdnz    .L_cycle_loop    (.L_cycle_next)
    0x48000024,  # b       .L_done
    0x880D8FB5,  # lbz     r0, -28747(r13)  (.L_menu_open: dMenu_status at 0x803EF095)
    0x2C000003,  # cmpwi   r0, 3            (Map screen open?)
    0x40820018,  # bne     .L_done
    0x70800004,  # andi.   r0, r4, 0x0004   (D-Pad Down pressed?)
    0x41820010,  # beq     .L_done
    0x548807B8,  # rlwinm  r8, r4, 0, 30, 28
    0x61080008,  # ori     r8, r8, 0x0008   (translate to D-Pad Up to close Map)
    0xB1030000,  # sth     r8, 0(r3)
    0x80010024,  # lwz     r0, 36(r1)       (.L_done)
    0x7C0803A6,  # mtlr    r0
    0x38210020,  # addi    r1, r1, 32
    0x4E800020,  # blr
    0x00000000,  # .long   0x00000000       (.L_prev_btn state storage)
]


def split_pointer_for_addi(ptr):
    high = (ptr & 0xFFFF0000) >> 16
    low = ptr & 0xFFFF
    if low >= 0x8000:
        high += 1
    return high & 0xFFFF, low


def patch_main_dol(dol_bytes):
    """
    Patches sys/main.dol (GZLE01) with custom PowerPC code for Skyward Sword HUD controls:
    - D-Pad Left  -> Z Item Slot (Left D-Pad Wing zitm)
    - D-Pad Right -> Y Item Slot (Right D-Pad Wing yitm)
    - D-Pad Up    -> Quick-Cycle Top Medallion (X Item Slot xitm) through owned inventory
    - D-Pad Down  -> Open/Close Sea Chart & Dungeon Map
    """
    dol = DOL()
    dol.read(BytesIO(dol_bytes))

    hook_site = 0x802C39AC
    existing_inst = dol.read_data(read_u32, hook_site)

    hook_payload = b"".join(struct.pack(">I", w) for w in WIIWAKER_PAD_HOOK_WORDS)
    padded_len = (len(hook_payload) + 3) & ~3

    text2 = dol.sections[2]

    # Check if already patched by a previous run of WiiWaker
    if existing_inst != 0x480512E5:
        # Recover branch target from existing bl instruction
        offset_26 = existing_inst & 0x03FFFFFC
        if offset_26 & 0x02000000:
            offset_26 -= 0x04000000
        custom_addr = (hook_site + offset_26) & 0xFFFFFFFF
        print(f"  [i] Updating existing WiiWaker DOL hook at 0x{custom_addr:08X}...")
        dol.write_data(write_bytes, custom_addr, hook_payload)
        dol.write_data(write_u32, 0x800072AC, 0xC00287B8)
        dol.write_data(write_u32, 0x80007320, 0x60000000)
        dol.save_changes()
        return dol.data.getvalue()

    # Determine where to place custom code in Text2 (dol.sections[2])
    if text2.size == 0:
        # Clean vanilla GZLE01 main.dol
        dol.data.seek(0, 2)
        eof_offset = (dol.data.tell() + 31) & ~31
        text2.offset = eof_offset
        text2.address = 0x803FCFA8
        custom_addr = text2.address
        text2.size = padded_len
    else:
        # Already has Text2 (e.g. BetterWW patches); append after existing Text2 code
        aligned_old_size = (text2.size + 3) & ~3
        custom_addr = text2.address + aligned_old_size
        text2.size = aligned_old_size + padded_len

    # Write custom PPC payload into Text2
    dol.write_data(write_bytes, custom_addr, hook_payload)

    # Update OS arena / default thread stack pointers so our code is never overwritten
    new_thread_start = (text2.address + text2.size + 3) & ~3
    hi_add, lo_add = split_pointer_for_addi(new_thread_start)
    dol.write_data(write_u32, 0x80307954, 0x3C600000 | hi_add)
    dol.write_data(write_u32, 0x8030795C, 0x38030000 | lo_add)

    new_thread_end = new_thread_start + 0x10000
    hi_end, lo_end = split_pointer_for_addi(new_thread_end)
    dol.write_data(write_u32, 0x8030794C, 0x3C600000 | hi_end)
    dol.write_data(write_u32, 0x80307950, 0x38030000 | lo_end)
    dol.write_data(write_u32, 0x80301854, 0x3C600000 | hi_end)
    dol.write_data(write_u32, 0x80301858, 0x38630000 | lo_end)

    hi_ori = (new_thread_end >> 16) & 0xFFFF
    lo_ori = new_thread_end & 0xFFFF
    dol.write_data(write_u32, 0x80003278, 0x3C200000 | hi_ori)
    dol.write_data(write_u32, 0x8000327C, 0x60210000 | lo_ori)

    new_metro_trk = new_thread_start + 0x12000
    hi_trk = (new_metro_trk >> 16) & 0xFFFF
    lo_trk = new_metro_trk & 0xFFFF
    dol.write_data(write_u32, 0x803370A8, 0x3C200000 | hi_trk)
    dol.write_data(write_u32, 0x803370AC, 0x60210000 | lo_trk)

    # Patch JUTGamePad::read at 0x802C39AC to bl custom_addr
    rel_branch = (custom_addr - hook_site) & 0x03FFFFFC
    bl_inst = 0x48000001 | rel_branch
    dol.write_data(write_u32, hook_site, bl_inst)

    # Patch mDoAud_getTactDirection (0x80007268) for responsive Wiimote Pointer / Gyro baton conducting:
    # 1. Lower C-Stick / Main-Stick deadzone threshold from 0.9f (-32736(r2)) to 0.35f (-30792(r2) = 0x803F84B8)
    #    so gentle Wiimote IR pointer or tilt gestures register notes without hitting the extreme screen edge.
    dol.write_data(write_u32, 0x800072AC, 0xC00287B8)  # lfs f0, -30792(r2) [0.35f]
    # 2. Keep full 90-degree directional quadrants (+/-45 deg) when sweeping continuously between notes
    #    instead of shrinking to +/-22.5 deg cones when oldDir != CENTER.
    dol.write_data(write_u32, 0x80007320, 0x60000000)  # nop (was bne 0x80007364)

    dol.save_changes()
    print(f"  [+] Injected custom PowerPC pad hook ({len(hook_payload)} bytes) at 0x{custom_addr:08X}")
    print(f"  [+] Hooked JUTGamePad::read (0x{hook_site:08X} -> bl 0x{custom_addr:08X})")
    print("  [+] Patched mDoAud_getTactDirection (0x800072AC) for Wiimote pointer/tilt Wind Waker conducting (0.35f threshold + full 90-deg quadrants)")
    print(f"  [+] Updated OS thread stack arena start to 0x{new_thread_start:08X}")
    return dol.data.getvalue()

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

    main_dol_path = "sys/main.dol"
    if main_dol_path in gcm.files_by_path:
        print(f"Extracting {main_dol_path} from ISO...")
        dol_data = gcm.read_file_data(main_dol_path).getvalue()
        print("Patching sys/main.dol with Skyward Sword D-Pad & Item controls...")
        patched_dol = patch_main_dol(dol_data)
        gcm.changed_files[main_dol_path] = BytesIO(patched_dol)
    else:
        print(f"Warning: {main_dol_path} not found in ISO!")

    print(f"Exporting patched playable disc to: {out_iso}...")
    gcm.export_disc_to_iso_with_changed_files(out_iso)

    print("==========================================================")
    print(f" SUCCESS! Patched playable game created at: {out_iso}")
    print("==========================================================")

if __name__ == "__main__":
    main()
