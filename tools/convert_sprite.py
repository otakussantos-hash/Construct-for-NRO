#!/usr/bin/env python3
"""Converte assets/sprite.png (250x250) para romfs/sprite.rgba (pixels RGBA crus).
Uso: pip install pillow && python3 tools/convert_sprite.py
"""
from PIL import Image

im = Image.open("assets/sprite.png").convert("RGBA")
if im.size != (250, 250):
    raise SystemExit(f"O sprite precisa ter 250x250, mas tem {im.size}")
open("romfs/sprite.rgba", "wb").write(im.tobytes())
print("romfs/sprite.rgba gerado")
