#!/usr/bin/env python3
"""Render example LCD screens with the firmware's glyphs, icons and menu text."""
from pathlib import Path
import re
import struct

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
SKETCH = ROOT / "firmware/Si4732Traditional"
ASSETS = ROOT / "assets"


def array_body(path, name):
    source = re.sub(r"/\*.*?\*/|//[^\n]*", "", path.read_text(), flags=re.S)
    return re.search(rf"\b{name}\[\]\s+PROGMEM\s*=\s*\{{(.*?)\}};", source, re.S)[1]


def hex_values(path, name):
    return [int(token, 16) for token in re.findall(r"0x([0-9a-fA-F]+)", array_body(path, name))]


def rgb565(value):
    return ((value >> 11) * 255 // 31, (value >> 5 & 63) * 255 // 63, (value & 31) * 255 // 31)


class BitmapFont:
    def __init__(self, ascent, descent, glyphs):
        self.ascent = ascent
        self.height = ascent + descent
        self.glyphs = glyphs

    @classmethod
    def from_vlw(cls, blob):
        count, _, _, _, ascent, descent = struct.unpack_from(">6i", blob)
        cursor = 24 + 28 * count
        glyphs = {}
        for index in range(count):
            cp, h, w, advance, dy, dx, _ = struct.unpack_from(">7i", blob, 24 + index * 28)
            size = w * h
            mask = Image.frombytes("L", (w, h), blob[cursor:cursor + size])
            glyphs[chr(cp)] = (mask, advance, dx, -dy)
            if cp > 32 and cp != 127:
                descent = max(descent, h - dy)
            cursor += size
        return cls(ascent, descent, glyphs)

    @classmethod
    def from_gfx(cls, path, name):
        bits = bytes(hex_values(path, name + "Bitmaps"))
        rows = re.findall(r"\{([^}]+)\}", array_body(path, name + "Glyphs"))
        glyphs = {}
        ascent = 0
        for cp, row in enumerate(rows, start=0x20):
            offset, w, h, advance, dx, dy = map(int, row.split(","))
            pixels = bytes(255 * ((bits[offset + n // 8] >> (7 - n % 8)) & 1) for n in range(w * h))
            glyphs[chr(cp)] = (Image.frombytes("L", (w, h), pixels), advance, dx, dy)
            ascent = max(ascent, -dy)
        # TFT_eSPI centers GFX free fonts using the height above the baseline.
        return cls(ascent, 0, glyphs)

    def draw(self, screen, text, x, y, color="white", *, left=False, numeric=False):
        glyphs = [self.glyphs[ch] for ch in text]
        last, advance, dx, _ = glyphs[-1]
        width = sum(g[1] for g in glyphs)
        if not numeric:
            width += dx + last.width - advance
        if not left:
            x -= width // 2
        baseline = y - self.height // 2 + self.ascent
        for mask, advance, dx, dy in glyphs:
            screen.paste(color, (x + dx, baseline + dy), mask)
            x += advance


def draw_icon(screen, name, size, position):
    pixels = hex_values(SKETCH / "icon.h", name)
    assert len(pixels) == size * size, name
    icon = Image.frombytes("RGB", (size, size), bytes(channel for p in pixels for channel in rgb565(p)))
    screen.paste(icon, position)


def tuning_screen(fonts, menu=None):
    """One FM tuning example. Signal, volume and battery values are illustrative."""
    zh, mono, orbit, freq, band = fonts
    screen = Image.new("RGB", (320, 170))
    pen = ImageDraw.Draw(screen)
    freq.draw(screen, "103.9", 158, 60, numeric=True)
    band.draw(screen, "VHF", 160, 12)
    for x in ([241] if menu else [1, 241]):
        for y in range(20, 96, 25):
            pen.rounded_rectangle((x, y, x + 75, y + 21), radius=4, outline="white")
    if menu:
        pen.rounded_rectangle((1, 1, 76, 110), radius=4, outline="red")
        zh.draw(screen, "選單", 38, 14)
        pen.rounded_rectangle((6, 56, 71, 71), radius=2, fill=rgb565(0x105B))
        selected = menu.index("頻寬")
        for offset in range(-2, 3):
            zh.draw(screen, menu[(selected + offset) % len(menu)], 38, 64 + 16 * offset, rgb565(0xBEDF))
    else:
        orbit.draw(screen, "SSB", 1, 11, "#646464", left=True)
        for i, label in enumerate(["步進", "頻寬", "增益", "頻率"]):
            zh.draw(screen, label, 21, 32 + 25 * i)
        for i, value in enumerate(["10", "AUT", "ON"]):
            mono.draw(screen, value, 58, 33 + 25 * i, numeric=i == 0)
        draw_icon(screen, "plus", 18, (51, 97))

    mono.draw(screen, "82%", 268, 11)
    pen.rectangle((292, 4, 311, 14), outline="white")
    pen.rectangle((293, 5, 307, 13), fill="#00ff00")
    pen.rectangle((312, 6, 313, 10), fill="white")
    orbit.draw(screen, "FM", 278, 33)
    pen.line([(244, 48), (252, 48), (248, 53), (244, 48)], fill="white")
    pen.line((248, 48, 248, 64), fill="white")
    for i in range(11):
        pen.rectangle((250 + 2*i, 61 - i, 251 + 2*i, 64), fill=rgb565(0x3526) if i < 10 else "red")
    orbit.draw(screen, "26", 298, 58, numeric=True)
    mono.draw(screen, "SNR", 261, 83)
    orbit.draw(screen, "19", 298, 83, numeric=True)
    draw_icon(screen, "speaker", 18, (250, 97))
    orbit.draw(screen, "35", 298, 108, numeric=True)

    pen.polygon([(156, 120), (160, 130), (164, 120)], fill="red")
    pen.line((160, 122, 160, 169), fill="red")
    for i, value in enumerate(range(1019, 1059)):
        x = i * 8
        y = 146 if value % 10 == 0 else 154 if value % 5 == 0 else 162
        pen.rectangle((x, y, x + int(value % 5 == 0), 169), fill="red" if i == 20 else rgb565(0xC638))
        if value % 10 == 0:
            mono.draw(screen, str(value // 10), x, 138, numeric=True)
    return screen


def main():
    source = (SKETCH / "Si4732Traditional.ino").read_text()
    menu_body = re.search(r'const char \*menu\[\] = \{(.*?)\};', source, re.S)[1]
    menu = re.findall(r'"([^"]+)"', menu_body)
    zh = BitmapFont.from_vlw((ASSETS / "TraditionalChinese16.vlw").read_bytes())
    fonts = [zh]
    for filename, name in [("NotoMono16.h", "NotoMono16"), ("orbitronbold16.h", "orbitronbold16"),
                           ("FREQFONT.h", "FREQFONT1")]:
        fonts.append(BitmapFont.from_vlw(bytes(hex_values(SKETCH / filename, name))))
    gfx = ROOT / "vendor/TFT_eSPI/Fonts/Custom/Orbitron_Light_24.h"
    fonts.append(BitmapFont.from_gfx(gfx, "Orbitron_Light_24"))

    for name, screen in [("fm", tuning_screen(fonts)), ("menu", tuning_screen(fonts, menu))]:
        screen.resize((640, 340), Image.Resampling.NEAREST).save(ASSETS / f"ui-{name}.png")


if __name__ == "__main__":
    main()
