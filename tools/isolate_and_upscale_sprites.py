#!/usr/bin/env python3
"""
Isolates and upscales the Skyward Sword HUD elements from user references:
1. Golden Gear [B] crest
2. 3D D-Pad
3. 3D (A) button
4. (-) and (+) buttons
5. Radial wheel slot ring and hand cursor
"""

import os
from PIL import Image, ImageEnhance, ImageFilter

IMG1_PATH = "/Users/cesarriojas/.gemini/antigravity/brain/4d285130-c2cc-4ff2-b2c7-916fbfe3eb1f/.user_uploaded/media_1792520197157_4fdc17af.png"
IMG2_PATH = "/Users/cesarriojas/.gemini/antigravity/brain/4d285130-c2cc-4ff2-b2c7-916fbfe3eb1f/.user_uploaded/media_1792520263872_76445b3f.png"
OUT_DIR = "/Users/cesarriojas/Games/WiiWaker/wii/assets/isolated_upscaled"
os.makedirs(OUT_DIR, exist_ok=True)

def isolate_and_upscale():
    img1 = Image.open(IMG1_PATH).convert("RGBA")
    w1, h1 = img1.size

    # 1. Golden Gear + [B] Button (around center X=138, Y=80, size ~100x100)
    b_crop = img1.crop((70, 20, 210, 160))
    # 2x upscale with sharpness enhancement
    b_up = b_crop.resize((280, 280), Image.Resampling.LANCZOS)
    enhancer = ImageEnhance.Sharpness(b_up)
    b_up = enhancer.enhance(1.8)
    b_up.save(os.path.join(OUT_DIR, "isolated_b_gear.png"))
    print("Saved isolated_b_gear.png")

    # 2. D-Pad (around center X=138, Y=250, size ~90x90)
    dpad_crop = img1.crop((90, 200, 190, 300))
    dpad_up = dpad_crop.resize((200, 200), Image.Resampling.LANCZOS)
    dpad_up = ImageEnhance.Sharpness(dpad_up).enhance(1.8)
    dpad_up.save(os.path.join(OUT_DIR, "isolated_dpad.png"))
    print("Saved isolated_dpad.png")

    # 3. (A) Button (around center X=138, Y=340, size ~70x70)
    a_crop = img1.crop((100, 305, 175, 380))
    a_up = a_crop.resize((150, 150), Image.Resampling.LANCZOS)
    a_up = ImageEnhance.Sharpness(a_up).enhance(1.8)
    a_up.save(os.path.join(OUT_DIR, "isolated_a_button.png"))
    print("Saved isolated_a_button.png")

    # 4. (-) and (+) Buttons (Y ~ 420 to 510)
    mp_crop = img1.crop((75, 420, 215, 510))
    mp_up = mp_crop.resize((280, 180), Image.Resampling.LANCZOS)
    mp_up = ImageEnhance.Sharpness(mp_up).enhance(1.8)
    mp_up.save(os.path.join(OUT_DIR, "isolated_minus_plus.png"))
    print("Saved isolated_minus_plus.png")

    # From Image 2: Radial Wheel Slot Ring & Hand Cursor
    img2 = Image.open(IMG2_PATH).convert("RGBA")
    # Top slot (Hero's Bow slot at 12 o'clock, around X=240..315, Y=10..105)
    slot_crop = img2.crop((235, 10, 320, 105))
    slot_up = slot_crop.resize((200, 220), Image.Resampling.LANCZOS)
    slot_up = ImageEnhance.Sharpness(slot_up).enhance(1.8)
    slot_up.save(os.path.join(OUT_DIR, "isolated_wheel_slot_bow.png"))
    print("Saved isolated_wheel_slot_bow.png")

    # Hand cursor on Bomb Bag (around X=315..365, Y=70..130)
    hand_crop = img2.crop((315, 75, 360, 125))
    hand_up = hand_crop.resize((120, 130), Image.Resampling.LANCZOS)
    hand_up = ImageEnhance.Sharpness(hand_up).enhance(1.8)
    hand_up.save(os.path.join(OUT_DIR, "isolated_cursor_hand.png"))
    print("Saved isolated_cursor_hand.png")

if __name__ == "__main__":
    isolate_and_upscale()
