#!/usr/bin/env python3
"""
Extracts and isolates Wind Waker cel-shaded UI elements from the Gemini-generated
artwork sheets using connected boundary flood-fill alpha transparency masking.
"""

import os
from collections import deque
from PIL import Image, ImageFilter, ImageEnhance

HUD_SHEET = "/Users/cesarriojas/.gemini/antigravity/brain/4d285130-c2cc-4ff2-b2c7-916fbfe3eb1f/wind_waker_vertical_controller_hud_1792520520946.jpg"
WHEEL_SHEET = "/Users/cesarriojas/.gemini/antigravity/brain/4d285130-c2cc-4ff2-b2c7-916fbfe3eb1f/wind_waker_radial_item_selector_1792520538774.jpg"
OUT_DIR = "/Users/cesarriojas/Games/WiiWaker/wii/assets/wind_waker_cel_shaded"
os.makedirs(OUT_DIR, exist_ok=True)

def boundary_flood_fill_alpha(crop_img, tol=235):
    """
    Flood fills from image boundaries inward to convert only background white pixels
    to transparent alpha (0), preserving interior white highlights.
    """
    img = crop_img.convert("RGBA")
    w, h = img.size
    visited = [[False]*h for _ in range(w)]
    q = deque()

    # Seed all four borders
    for x in range(w):
        for y in (0, h - 1):
            r, g, b, _ = img.getpixel((x, y))
            if r >= tol and g >= tol and b >= tol:
                q.append((x, y))
                visited[x][y] = True
    for y in range(h):
        for x in (0, w - 1):
            if not visited[x][y]:
                r, g, b, _ = img.getpixel((x, y))
                if r >= tol and g >= tol and b >= tol:
                    q.append((x, y))
                    visited[x][y] = True

    while q:
        cx, cy = q.popleft()
        img.putpixel((cx, cy), (0, 0, 0, 0))
        for dx, dy in ((-1,0), (1,0), (0,-1), (0,1)):
            nx, ny = cx + dx, cy + dy
            if 0 <= nx < w and 0 <= ny < h and not visited[nx][ny]:
                r, g, b, _ = img.getpixel((nx, ny))
                if r >= tol and g >= tol and b >= tol:
                    visited[nx][ny] = True
                    q.append((nx, ny))

    # Crop to non-transparent bounding box
    bbox = img.getbbox()
    if bbox:
        # Add 4px margin if possible
        pad_bbox = (
            max(0, bbox[0] - 4),
            max(0, bbox[1] - 4),
            min(w, bbox[2] + 4),
            min(h, bbox[3] + 4)
        )
        return img.crop(pad_bbox)
    return img

def extract_all():
    print(f"Loading HUD sheet from {HUD_SHEET}...")
    hud = Image.open(HUD_SHEET)

    # 1. Golden Gear [B] button (Y=61..356)
    print("Extracting WW Golden Gear [B]...")
    b_gear = boundary_flood_fill_alpha(hud.crop((0, 60, hud.width, 360)))
    b_gear.save(os.path.join(OUT_DIR, "ww_golden_gear_b.png"))
    print(f"  -> Saved ww_golden_gear_b.png {b_gear.size}")

    # 2. Cel-shaded D-Pad (Y=395..745)
    print("Extracting WW Cel-shaded D-Pad...")
    dpad = boundary_flood_fill_alpha(hud.crop((0, 390, hud.width, 750)))
    dpad.save(os.path.join(OUT_DIR, "ww_dpad.png"))
    print(f"  -> Saved ww_dpad.png {dpad.size}")

    # 3. Cel-shaded (A) Button (Y=781..1054)
    print("Extracting WW (A) Button...")
    a_btn = boundary_flood_fill_alpha(hud.crop((0, 780, hud.width, 1055)))
    a_btn.save(os.path.join(OUT_DIR, "ww_a_button.png"))
    print(f"  -> Saved ww_a_button.png {a_btn.size}")

    # 4. Cel-shaded (-) / (+) Buttons (Y=1085..1224)
    print("Extracting WW (-) & (+) Medallions...")
    minus_plus = boundary_flood_fill_alpha(hud.crop((0, 1080, hud.width, 1230)))
    minus_plus.save(os.path.join(OUT_DIR, "ww_minus_plus.png"))
    print(f"  -> Saved ww_minus_plus.png {minus_plus.size}")

    # 5. Radial Item Selector Wheel
    print(f"Loading Wheel sheet from {WHEEL_SHEET}...")
    wheel = Image.open(WHEEL_SHEET)
    wheel_alpha = boundary_flood_fill_alpha(wheel)
    wheel_alpha.save(os.path.join(OUT_DIR, "ww_radial_wheel_ring.png"))
    print(f"  -> Saved ww_radial_wheel_ring.png {wheel_alpha.size}")

    print("\nAll Wind Waker cel-shaded sprites extracted with alpha transparency!")

if __name__ == "__main__":
    extract_all()
