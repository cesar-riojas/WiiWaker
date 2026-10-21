import sys
import os
import base64
import io
from PIL import Image

sys.path.append("/Users/cesarriojas/Games/betterww")
from wwlib.rarc import RARC
from wwlib.bti import BTIFile

def get_b64(path):
    with open(path, "rb") as f:
        ext = path.split(".")[-1]
        mime = "image/png" if ext == "png" else "image/jpeg"
        return f"data:{mime};base64," + base64.b64encode(f.read()).decode("ascii")

# Background
im_bg = Image.open("/tmp/clean_dungeon_canvas.png").resize((800, 483), Image.Resampling.LANCZOS)
buf = io.BytesIO()
im_bg.convert("RGB").save(buf, format="JPEG", quality=85)
bg_b64 = "data:image/jpeg;base64," + base64.b64encode(buf.getvalue()).decode("ascii")

# Assets
ASSETS = "/Users/cesarriojas/Games/WiiWaker/wii/assets/skyward_sword_native/final"
gear_b64 = get_b64(f"{ASSETS}/ss_native_solid_gold_socket.png")
b_btn_b64 = get_b64(f"{ASSETS}/ss_official_clean_b_button.png")
a_btn_b64 = get_b64(f"{ASSETS}/ss_native_a_button.png")
dpad_b64 = get_b64(f"{ASSETS}/ss_native_dpad.png")
minus_b64 = get_b64(f"{ASSETS}/ss_native_minus.png")
plus_b64 = get_b64(f"{ASSETS}/ss_native_plus.png")

# Item icons
r_item = RARC()
with open("/Users/cesarriojas/Games/WiiWaker/dist/apps/WiiWaker/files/res/Msg/itemicon.arc", "rb") as f:
    r_item.read(io.BytesIO(f.read()))

tele_f = r_item.get_file_entry("telescope.bti")
tele_im = BTIFile(tele_f.data).render()
t_buf = io.BytesIO()
tele_im.save(t_buf, format="PNG")
tele_b64 = "data:image/png;base64," + base64.b64encode(t_buf.getvalue()).decode("ascii")

sword_f = r_item.get_file_entry("sword_00.bti")
sword_im = BTIFile(sword_f.data).render()
s_buf = io.BytesIO()
sword_im.save(s_buf, format="PNG")
sword_b64 = "data:image/png;base64," + base64.b64encode(s_buf.getvalue()).decode("ascii")

template = open("/Users/cesarriojas/Games/WiiWaker/tools/hud_studio_template.html", "r").read()

template = template.replace("__BG__", bg_b64)
template = template.replace("__GEAR__", gear_b64)
template = template.replace("__B_BTN__", b_btn_b64)
template = template.replace("__A_BTN__", a_btn_b64)
template = template.replace("__DPAD__", dpad_b64)
template = template.replace("__MINUS__", minus_b64)
template = template.replace("__PLUS__", plus_b64)
template = template.replace("__TELE__", tele_b64)
template = template.replace("__SWORD__", sword_b64)

out_file = "/Users/cesarriojas/.gemini/antigravity/brain/4d285130-c2cc-4ff2-b2c7-916fbfe3eb1f/hud_interactive_preview.html"
with open(out_file, "w") as f:
    f.write(template)

print(f"Generated {out_file} ({len(template)} bytes)")
