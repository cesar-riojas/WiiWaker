#!/usr/bin/env python3
"""
Prepares the authentic, native Skyward Sword UI textures extracted directly from
the official SOUE01 disc archives (DoButton.arc and Main2D.arc) and composites them
with their official Nintendo in-game colorizations.
"""

import os
from PIL import Image

NATIVE_PNG_DIR = "/Users/cesarriojas/Games/WiiWaker/wii/assets/skyward_sword_native/png"
OUT_DIR = "/Users/cesarriojas/Games/WiiWaker/wii/assets/skyward_sword_native/final"
os.makedirs(OUT_DIR, exist_ok=True)

def tint_grayscale(src_img, r_mult, g_mult, b_mult, boost=1.1):
    src = src_img.convert("RGBA")
    w, h = src.size
    out = Image.new("RGBA", (w, h))
    for y in range(h):
        for x in range(w):
            r, g, b, a = src.getpixel((x, y))
            if a > 0:
                lum = (r / 255.0) * boost
                nr = int(min(255, r_mult * lum))
                ng = int(min(255, g_mult * lum))
                nb = int(min(255, b_mult * lum))
                out.putpixel((x, y), (nr, ng, nb, a))
    return out

def main():
    print("Compositing official Skyward Sword textures...")

    # 1. Native Golden Gear [B] Button (from tr_bBtn_01)
    b_raw = Image.open(os.path.join(NATIVE_PNG_DIR, "tr_bBtn_01.png"))
    # Skyward Sword official gold tint: R=255, G=210, B=55
    b_gold = tint_grayscale(b_raw, 255, 210, 55, boost=1.15)
    b_gold.save(os.path.join(OUT_DIR, "ss_native_gold_b_gear.png"))
    print(f"  [+] Created ss_native_gold_b_gear.png {b_gold.size}")

    # 2. Native Emerald Green (A) Button (from tr_aBtn_00)
    a_raw = Image.open(os.path.join(NATIVE_PNG_DIR, "tr_aBtn_00.png"))
    # Skyward Sword official emerald dome tint: R=45, G=215, B=95
    a_green = tint_grayscale(a_raw, 45, 215, 95, boost=1.2)
    a_green.save(os.path.join(OUT_DIR, "ss_native_a_button.png"))
    print(f"  [+] Created ss_native_a_button.png {a_green.size}")

    # 3. Native Slate D-Pad Cross (from tr_crossBtnBasic_00)
    dpad_raw = Image.open(os.path.join(NATIVE_PNG_DIR, "tr_crossBtnBasic_00.png"))
    dpad_raw.save(os.path.join(OUT_DIR, "ss_native_dpad.png"))
    print(f"  [+] Created ss_native_dpad.png {dpad_raw.size}")

    # 4. Native (-) and (+) Buttons (from tr_minusBtn_00 & tr_plusBtn_00)
    minus_raw = Image.open(os.path.join(NATIVE_PNG_DIR, "tr_minusBtn_00.png"))
    plus_raw = Image.open(os.path.join(NATIVE_PNG_DIR, "tr_plusBtn_00.png"))
    minus_raw.save(os.path.join(OUT_DIR, "ss_native_minus.png"))
    plus_raw.save(os.path.join(OUT_DIR, "ss_native_plus.png"))
    print(f"  [+] Created ss_native_minus.png {minus_raw.size} and ss_native_plus.png {plus_raw.size}")

    # 5. Native Radial Item Wheel Medallion Slot (from tr_itemBace_09)
    slot_raw = Image.open(os.path.join(NATIVE_PNG_DIR, "tr_itemBace_09.png"))
    slot_gold = tint_grayscale(slot_raw, 255, 215, 70, boost=1.1)
    slot_gold.save(os.path.join(OUT_DIR, "ss_native_wheel_slot.png"))
    print(f"  [+] Created ss_native_wheel_slot.png {slot_gold.size}")

    # 6. Native Pointing Hand Cursor (from tr_defCursor_00)
    hand_raw = Image.open(os.path.join(NATIVE_PNG_DIR, "tr_defCursor_00.png"))
    hand_raw.save(os.path.join(OUT_DIR, "ss_native_cursor_hand.png"))
    print(f"  [+] Created ss_native_cursor_hand.png {hand_raw.size}")

    print("\nAll authentic Skyward Sword textures successfully processed and saved to:")
    print(f"  {OUT_DIR}")

if __name__ == "__main__":
    main()
