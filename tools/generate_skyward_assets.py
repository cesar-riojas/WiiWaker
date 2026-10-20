#!/usr/bin/env python3
"""
Generates high-definition, transparent Skyward Sword UI sprites:
1. Ornate Golden Gear / Sunburst Ring
2. 3D Rounded-Rect [B] Button Keycap
3. 3D Domed (A) Button Keycap
4. 3D Cross D-Pad (+) Key
5. Circular (-) and (+) Buttons
6. White Cursor Hand / Pointer Ray
"""

import os
import math
from PIL import Image, ImageDraw, ImageFont, ImageFilter

OUT_DIR = "/Users/cesarriojas/Games/WiiWaker/wii/assets/skyward_sword"
os.makedirs(OUT_DIR, exist_ok=True)

def draw_golden_gear_ring(size=128):
    """Ornate notched golden sunburst ring as seen in Skyward Sword item slots."""
    scale = 4
    s = size * scale
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    cx, cy = s // 2, s // 2
    outer_r = int(s * 0.46)
    inner_r = int(s * 0.38)
    notch_count = 12

    # Draw notched cog outer ring
    points = []
    for i in range(notch_count * 2):
        angle = (i * math.pi) / notch_count
        r = outer_r if (i % 2 == 0) else int(outer_r * 0.90)
        px = cx + r * math.cos(angle)
        py = cy + r * math.sin(angle)
        points.append((px, py))

    # Golden gradient colors
    gold_outer = (212, 175, 85, 230)
    gold_border = (160, 125, 45, 255)
    gold_fill = (45, 38, 25, 180)

    draw.polygon(points, fill=gold_outer, outline=gold_border, width=scale * 2)
    # Inner dark translucent dish
    draw.ellipse([cx - inner_r, cy - inner_r, cx + inner_r, cy + inner_r], fill=gold_fill, outline=gold_border, width=scale)

    res = img.resize((size, size), Image.Resampling.LANCZOS)
    res.save(os.path.join(OUT_DIR, "ss_golden_gear_ring.png"))
    print("Generated ss_golden_gear_ring.png")

def draw_b_button(size=64):
    """3D White/Ivory rounded rectangular [B] keycap."""
    scale = 4
    w, h = int(size * 0.75 * scale), int(size * scale)
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    pad = 4 * scale
    # Outer shadow
    draw.rounded_rectangle([pad, pad + 2 * scale, w - pad, h - pad + 2 * scale], radius=8 * scale, fill=(40, 40, 40, 160))
    # Beveled body
    draw.rounded_rectangle([pad, pad, w - pad, h - pad], radius=8 * scale, fill=(245, 245, 240, 255), outline=(130, 130, 125, 255), width=scale * 2)

    # Embossed 'B'
    try:
        font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial Bold.ttf", 26 * scale)
    except:
        font = ImageFont.load_default()

    bbox = draw.textbbox((0, 0), "B", font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    tx = (w - tw) // 2
    ty = (h - th) // 2 - int(scale * 1.5)

    # Subtle text emboss shadow
    draw.text((tx + scale, ty + scale), "B", fill=(255, 255, 255, 200), font=font)
    draw.text((tx, ty), "B", fill=(95, 95, 95, 255), font=font)

    res = img.resize((int(size * 0.75), size), Image.Resampling.LANCZOS)
    res.save(os.path.join(OUT_DIR, "ss_b_button.png"))
    print("Generated ss_b_button.png")

def draw_a_button(size=64):
    """3D Domed circular (A) keycap."""
    scale = 4
    s = size * scale
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    pad = 4 * scale
    cx, cy = s // 2, s // 2
    r = s // 2 - pad

    # Beveled shadow
    draw.ellipse([cx - r, cy - r + 3 * scale, cx + r, cy + r + 3 * scale], fill=(40, 40, 40, 150))
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(248, 248, 242, 255), outline=(140, 140, 135, 255), width=scale * 2)

    try:
        font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial Bold.ttf", 24 * scale)
    except:
        font = ImageFont.load_default()

    bbox = draw.textbbox((0, 0), "A", font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    tx = (s - tw) // 2
    ty = (s - th) // 2 - int(scale * 1.5)

    draw.text((tx + scale, ty + scale), "A", fill=(255, 255, 255, 200), font=font)
    draw.text((tx, ty), "A", fill=(90, 90, 90, 255), font=font)

    res = img.resize((size, size), Image.Resampling.LANCZOS)
    res.save(os.path.join(OUT_DIR, "ss_a_button.png"))
    print("Generated ss_a_button.png")

def draw_dpad(size=64):
    """3D Cross D-Pad with directional hashes."""
    scale = 4
    s = size * scale
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    cx, cy = s // 2, s // 2
    arm_w = int(s * 0.28)
    arm_l = int(s * 0.44)

    # Cross shape
    draw.rectangle([cx - arm_w // 2, cy - arm_l, cx + arm_w // 2, cy + arm_l], fill=(225, 225, 220, 255), outline=(130, 130, 125, 255), width=scale * 2)
    draw.rectangle([cx - arm_l, cy - arm_w // 2, cx + arm_l, cy + arm_w // 2], fill=(225, 225, 220, 255), outline=(130, 130, 125, 255), width=scale * 2)

    # Direction ticks
    tick_col = (110, 110, 105, 255)
    # Up
    draw.line([cx, cy - arm_l + 4 * scale, cx, cy - arm_l + 10 * scale], fill=tick_col, width=scale * 2)
    # Down
    draw.line([cx, cy + arm_l - 10 * scale, cx, cy + arm_l - 4 * scale], fill=tick_col, width=scale * 2)
    # Left
    draw.line([cx - arm_l + 4 * scale, cy, cx - arm_l + 10 * scale, cy], fill=tick_col, width=scale * 2)
    # Right
    draw.line([cx + arm_l - 10 * scale, cy, cx + arm_l - 4 * scale, cy], fill=tick_col, width=scale * 2)

    res = img.resize((size, size), Image.Resampling.LANCZOS)
    res.save(os.path.join(OUT_DIR, "ss_dpad.png"))
    print("Generated ss_dpad.png")

if __name__ == "__main__":
    draw_golden_gear_ring(96)
    draw_b_button(48)
    draw_a_button(48)
    draw_dpad(56)
