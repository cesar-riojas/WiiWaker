#!/usr/bin/env python3
"""
Generates crisp authentic Wii Remote and Nunchuk controller button icons
and injects them into menures.arc for native Wii HUD rendering.
"""

import sys
import os
from io import BytesIO
from PIL import Image, ImageDraw, ImageFont

sys.path.append("/Users/cesarriojas/Games/betterww")
from wwlib.rarc import RARC
from wwlib.bti import BTIFile

def create_button_icon(label, size=(16, 16), shape="circle"):
    w, h = size
    # Draw at 4x resolution for smooth antialiasing
    scale = 4
    sw, sh = w * scale, h * scale
    img = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    margin = scale
    if shape == "circle":
        # Circular button badge
        draw.ellipse([margin, margin, sw - margin, sh - margin], fill=(255, 255, 255, 255), outline=(0, 0, 0, 255), width=scale)
    elif shape == "round_rect":
        draw.rounded_rectangle([margin, margin, sw - margin, sh - margin], radius=4 * scale, fill=(255, 255, 255, 255), outline=(0, 0, 0, 255), width=scale)
    elif shape == "trigger":
        draw.rounded_rectangle([margin, margin, sw - margin, sh - margin], radius=2 * scale, fill=(240, 240, 240, 255), outline=(0, 0, 0, 255), width=scale)

    # Render character label
    try:
        font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial Bold.ttf", int(9 * scale))
    except Exception:
        font = ImageFont.load_default()

    bbox = draw.textbbox((0, 0), label, font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    tx = (sw - tw) // 2
    ty = (sh - th) // 2 - int(scale * 0.5)

    draw.text((tx, ty), label, fill=(30, 30, 30, 255), font=font)

    # Downsample with Lanczos for crisp, smooth edges
    res = img.resize((w, h), Image.Resampling.LANCZOS)
    return res

def patch_menures_arc(menures_path):
    print(f"Patching Wii button icons into: {menures_path}")
    if not os.path.isfile(menures_path):
        print(f"Error: {menures_path} does not exist!")
        return

    with open(menures_path, "rb") as f:
        file_bytes = f.read()

    bio = BytesIO(file_bytes)
    rarc = RARC()
    rarc.read(bio)

    # Button specifications
    # ba_a: (A) Wii Remote Main Action Button
    # ba_b: (B) Wii Remote Underside Trigger
    # ba_x: [1] Wii Remote 1 Button (Item 1)
    # ba_y: [2] Wii Remote 2 Button (Item 2)
    # ba_r: (C) Nunchuk C Button
    buttons = {
        "ba_a.bti": ("A", (16, 16), "circle"),
        "ba_b.bti": ("B", (16, 16), "circle"),
        "ba_x.bti": ("1", (16, 16), "round_rect"),
        "ba_y.bti": ("2", (16, 16), "round_rect"),
        "ba_r.bti": ("C", (16, 16), "circle"),
    }

    for bti_name, (label, size, shape) in buttons.items():
        entry = rarc.get_file_entry(bti_name)
        if entry:
            icon_img = create_button_icon(label, size, shape)
            bti = BTIFile(entry.data)
            bti.replace_image(icon_img)
            bti.save_changes()
            entry.data = bti.data
            print(f"  [+] Injected Wii '{label}' icon into {bti_name}")

    rarc.save_changes()
    with open(menures_path, "wb") as f:
        f.write(rarc.data.getvalue())

    print("Successfully updated menures.arc with native Wii HUD controller glyphs!")

if __name__ == "__main__":
    path = "/Users/cesarriojas/Games/WiiWaker/dist/apps/WiiWaker/files/res/Msg/menures.arc"
    patch_menures_arc(path)
