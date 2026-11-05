#!/usr/bin/env python3
"""Recreate the Traditional Chinese sketch from the pinned upstream checkout."""
from pathlib import Path
import json
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
UPSTREAM = ROOT / "upstream"
SKETCH = ROOT / "firmware/Si4732Traditional"
TRANSLATIONS = {
    "步进": "步進", "拍频": "拍頻", "带宽": "頻寬",
    "降噪": "軟靜音", "静音": "靜音", "关于": "關於",
    "存储电台": "儲存電台", "读取电台": "讀取電台",
    "自动熄屏": "自動熄屏", "菜单": "選單", "衰减": "衰減",
    "搜索": "搜台", "频率": "頻率",
    "嘉立创 你别失望 开源 ": "原始硬體：Sunnygold",
    "软件 汉化 改进 :zooc": "繁體中文漢化與改進：＠maxxtang(Github)",
}


def once(text, old, new, count=1):
    assert text.count(old) == count, (old, text.count(old), count)
    return text.replace(old, new)


def main():
    SKETCH.mkdir(parents=True, exist_ok=True)
    for p in UPSTREAM.iterdir():
        if p.suffix in {".h", ".cpp"} and p.name != "HanYiZhongYuanJian16.h":
            shutil.copy2(p, SKETCH / p.name)
    text = (UPSTREAM / "ALL_IN_ONE_T-Display_S3.ino").read_text()
    # Translate string literals only; preserve upstream comments and attribution.
    def translate(match):
        s = match.group()
        for old, new in TRANSLATIONS.items():
            s = s.replace(old, new)
        return s
    text = re.sub(r'"(?:[^"\\]|\\.)*"', translate, text)
    text = text.replace('"HanYiZhongYuanJian16.h"', '"TraditionalChinese16.h"')
    text = text.replace("HanYiZhongYuanJian16", "TraditionalChinese16")
    text = once(text, '  spr.drawString(" ", 1, 70);\n'
                      '  spr.drawString("QQ群:854656412", 1, 90);',
                '  spr.drawString("繁體中文漢化：0.2.1", 1, 70);')
    text = once(text, 'spr.drawString("On", 38, 72);',
                'spr.loadFont(TraditionalChinese16);\n'
                '        spr.drawString("開啟", 38, 72);\n'
                '        spr.unloadFont();')
    text = once(text, 'spr.drawString("Loading SSB", 160, 62, 4);',
                'spr.loadFont(TraditionalChinese16);\n'
                '        spr.drawString("載入 SSB 中", 160, 62);\n'
                '        spr.unloadFont();', count=2)
    text = once(text, 'tft.drawString("no station saved!", 110, 90, 2);',
                'tft.loadFont(TraditionalChinese16);\n'
                '        tft.drawString("尚未儲存電台", 110, 90);\n'
                '        tft.unloadFont();', count=2)
    text = once(text, 'tft.println("Si4735 not detected");',
                'tft.setTextSize(1);\n'
                '    tft.loadFont(TraditionalChinese16);\n'
                '    tft.drawString("未偵測到 Si4732", 160, 85);\n'
                '    tft.unloadFont();')
    # Core 3.x uses pin-based LEDC APIs, including every brightness write.
    text = once(text,
                'ledcSetup(0, 15000, 8);        //频率提高后背光底噪消失 原频率2000\n'
                '  ledcAttachPin(PIN_LCD_BL, 0);  //esp 2.x',
                'ledcAttach(PIN_LCD_BL, 15000, 8); // Arduino ESP32 3.x, 15 kHz backlight')
    text = text.replace("ledcWrite(0,", "ledcWrite(PIN_LCD_BL,")
    # Upstream has uncovered RSSI values (HF=2, FM=1): return the adjacent bin.
    text = once(text, '(rssi > 1) and (rssi <= 1)', '(rssi > 1) and (rssi <= 2)')
    text = once(text, 'if (rssi < 1) return 1;', 'if (rssi <= 1) return 1;')
    text = once(text, '\n}\n\nvoid drawMenu() {', '\n  return 1;\n}\n\nvoid drawMenu() {')
    # USB diagnostics are kept off the product display. '?' reports cached state.
    text = once(text, 'void setup() {',
                'void setup() {\n'
                '  Serial.begin(115200);\n'
                '  Serial.printf("[ZH-TW 0.1] boot flash=%u psram=%u\\n",\n'
                '                ESP.getFlashChipSize(), ESP.getPsramSize());')
    text = once(text, '  spr.createSprite(320, 170);',
                '  spr.createSprite(320, 170);\n'
                '  Serial.println("[ZH-TW 0.1] display_init_done configured=ST7789_parallel8_320x170");')
    marker = '  if (si4735Addr == 0) {'
    text = once(text, marker,
                '  Serial.printf("[ZH-TW 0.1] radio_i2c=0x%02X\\n", si4735Addr);\n\n' + marker)
    text = once(text, '  batteryLevel = getBatteryChargeLevel(CONV_FACTOR);\n}',
                '  batteryLevel = getBatteryChargeLevel(CONV_FACTOR);\n'
                '  Serial.printf("[ZH-TW 0.1] ready mode=%s freq=%u volume=%u\\n",\n'
                '                bandModeDesc[currentMode], currentFrequency, rx.getVolume());\n}')
    text = once(text, 'void loop() {',
                'void loop() {\n'
                '  if (Serial.available() && Serial.read() == \'?\') {\n'
                '    Serial.printf("[ZH-TW 0.1] status uptime_ms=%lu mode=%s freq=%u rssi=%u psram=%u\\n",\n'
                '                  millis(), bandModeDesc[currentMode], currentFrequency, rssi, ESP.getPsramSize());\n'
                '  }')
    # Apply the receiver's display fixes to the translated v0.1 baseline.
    with tempfile.TemporaryDirectory() as temp_dir:
        stage = Path(temp_dir) / "Si4732Traditional.ino"
        stage.write_text(text)
        subprocess.run(["patch", "-s", "-F", "0", str(stage),
                        str(ROOT / "tools/display-power.patch")], check=True)
        (SKETCH / stage.name).write_text(stage.read_text())
    battery = (SKETCH / "Battery.cpp").read_text()
    battery = once(battery, '\n}\n\n// void stopADC', '\n  return 0.0; // No ADC sample when disabled.\n}\n\n// void stopADC')
    (SKETCH / "Battery.cpp").write_text(battery)
    old_font = SKETCH / "HanYiZhongYuanJian16.h"
    if old_font.exists():
        old_font.unlink()
    (ROOT / "translations.json").write_text(json.dumps(TRANSLATIONS, ensure_ascii=False, indent=2) + "\n")
    print("Prepared", SKETCH)


if __name__ == "__main__":
    main()
