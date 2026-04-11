# Third-Party Licenses

이 프로젝트에는 아래와 같은 오픈소스 소프트웨어 및 기술적 자산이 포함되어 있습니다.
This project includes the following open-source software and technical assets.

---

## ESP32-upng
<https://github.com/lagunax/ESP32-upng>

ESP_upng -- derived from LodePNG version 20100808
==========================================

Copyright (c) 2005-2010 Lode Vandevenne
Copyright (c) 2010 Sean Middleditch
Copyright (c) 2019 Vitaliy Novoselov (ESP32 Port)

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

---

## ESP32 ADC Calibrate
<https://github.com/e-tinkers/esp32-adc-calibrate>

Copyright (c) 2017 e-Tinkers

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.

---

## External Libraries (via PlatformIO)

이 프로젝트는 `platformio.ini`에 정의된 다음과 같은 라이브러리를 사용합니다.
The following libraries defined in `platformio.ini` are utilized in this project:

| Library | License | Repository |
| :--- | :--- | :--- |
| **ESP32-HUB75-MatrixPanel-DMA** | MIT | [GitHub](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA) |
| **IRremoteESP8266** | LGPLv2.1 | [GitHub](https://github.com/crankyoldgit/IRremoteESP8266) |
| **NeoPixelBus** | LGPL | [GitHub](https://github.com/Makuna/NeoPixelBus) |
| **JPEGDEC** | MIT | [GitHub](https://github.com/bitbank2/JPEGDEC) |
| **AnimatedGIF** | MIT | [GitHub](https://github.com/bitbank2/AnimatedGIF) |
| **VL53L1X_ULD** | MIT | [GitHub](https://github.com/rneurink/VL53L1X_ULD) |
| **BH1750** | MIT | [GitHub](https://github.com/claws/BH1750) |
| **Adafruit SHT31** | MIT | [GitHub](https://github.com/adafruit/Adafruit_SHT31) |
| **Arduino-DS3231** | MIT | [GitHub](https://github.com/jarzebski/Arduino-DS3231) |

---

## Special Credits & Disclaimers

### Chrome Dinosaur Game Assets
`src/effects/effect_dino.cpp`에 포함된 미니게임의 그래픽 자산 및 메커니즘은 구글 크롬(Google Chrome)의 오프라인 공룡 게임을 기반으로 합니다. 해당 자산은 비상업적 교육 목적으로만 사용되며, 모든 권리는 원저작자에게 있습니다.
The graphics and mechanics of the mini-game in `src/effects/effect_dino.cpp` are based on the Google Chrome offline dinosaur game. These assets are used for non-commercial educational purposes only, and all rights belong to the original creators.

### Dithering Algorithms
`src/lib/dither.cpp` 에 구현된 알고리즘은 deeptronix의 소스 코드를 참고하였습니다 (Public Domain / Unlicense).
The algorithms implemented in `src/lib/dither.cpp` reference source code from deeptronix (Public Domain / Unlicense).

---

## Collected license snapshots

자동 수집 스크립트(`scripts/check_third_party_licenses.py`)로 수집한 라이선스 스냅샷입니다. 원본 저장소의 LICENSE 파일을 반드시 함께 확인하세요.

- `third_party_licenses/claws_BH1750_LICENSE.txt`
- `third_party_licenses/deltafish32_AnimatedGIF_LICENSE.txt`
- `third_party_licenses/deltafish32_ESP32-HUB75-MatrixPanel-DMA_LICENSE.txt`
- `third_party_licenses/deltafish32_IRremoteESP8266_LICENSE.txt`
- `third_party_licenses/deltafish32_JPEGDEC_LICENSE.txt`
- `third_party_licenses/jarzebski_Arduino-DS3231_LICENSE.txt`
- `third_party_licenses/Makuna_NeoPixelBus_LICENSE.txt`
- `third_party_licenses/rneurink_VL53L1X_ULD_LICENSE.txt`

> 주의: 일부 항목(`Wire`, `Adafruit BusIO`, `Adafruit GFX Library`, `Adafruit SSD1306`, `Adafruit_SHT31` 등)은 자동 추정이 불완전하여 수동 확인이 필요합니다. 필요한 경우 `scripts/check_third_party_licenses.py`를 수정하거나 수동으로 라이선스 텍스트를 추가하세요.

