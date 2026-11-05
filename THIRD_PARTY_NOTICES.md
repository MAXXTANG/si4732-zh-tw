# 來源與授權

本版基於 zhang-chong/4732mini 的提交 `5334e7bc5e44a7b66bbbbd5183164147ec5f8641`。原始硬體由 Sunnygold 設計，中文軟體與改進署名 zooc，繁體中文漢化與改進由 @maxxtang 整理。上游 README 指定僅供學習交流並禁止商業用途。[固定版本與原文](https://github.com/zhang-chong/4732mini/tree/5334e7bc5e44a7b66bbbbd5183164147ec5f8641)

| 元件 | 本版本所使用的來源 |
| --- | --- |
| TFT_eSPI 2.5.43 | 上游隨附的 TFT_eSPI_DynamicSpeed 版本，含顯示驅動、字型及開發板設定。[原始授權與署名](vendor/TFT_eSPI/license.txt) |
| PU2CLR SI4735 2.1.8 | 上游隨附版本。收音驅動採 MIT 授權。[原始授權與署名](vendor/PU2CLR_SI4735/license.txt) |
| Arduino-ESP32 3.3.12 | ESP32 Arduino Core 的套件標示為 LGPL-2.1-or-later。[對應原始碼](https://github.com/espressif/arduino-esp32/tree/3.3.12) · [授權原文](licenses/Arduino-ESP32-LICENSE.md) |
| ESP-IDF 5.5.5 | Core 隨附的 ESP-IDF 建置版本。各元件依上游個別聲明，專案主授權為 Apache 2.0。[對應原始碼](https://github.com/espressif/esp-idf/tree/v5.5.5) · [授權原文](licenses/ESP-IDF-LICENSE.txt) |
| 繁體中文字型 | 從 Noto Sans Mono CJK TC Regular 裁出 16 像素點陣字型。Copyright © 2014, 2015 Adobe Systems Incorporated。子集沿用 SIL Open Font License 1.1。[字型專案](https://github.com/notofonts/noto-cjk) · [授權全文](licenses/Noto-CJK-OFL.txt) |
| 其他字型、圖示與 SSB patch | 沿用固定版本的原始檔及其署名。各檔案保留原有聲明。 |

Pillow 用於產生字型與 README 的示意圖。Arduino CLI 及 Espressif ESP32 Arduino Core 用於編譯。各工具由使用者依建置說明安裝。
