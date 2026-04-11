# lib_deps 고정(pinning) 제안

설명
- 이 문서는 `platformio.ini`의 `lib_deps` 항목을 안정적으로 고정(pinning)하기 위한 제안입니다.
- 실제 태그/커밋 해시는 각 저장소의 Release/Tags 페이지에서 선택해야 합니다. 아래는 현재 항목과 권장 형식입니다.

현재 `lib_deps` 항목 (platformio.ini 기준)

```
Wire
adafruit/Adafruit BusIO
adafruit/Adafruit GFX Library
https://github.com/deltafish32/ESP32-HUB75-MatrixPanel-DMA.git
adafruit/Adafruit SSD1306
https://github.com/deltafish32/IRremoteESP8266.git
Makuna/NeoPixelBus
rneurink/VL53L1X_ULD
claws/BH1750
https://github.com/adafruit/Adafruit_SHT31.git
https://github.com/deltafish32/JPEGDEC.git
https://github.com/deltafish32/AnimatedGIF.git
https://github.com/jarzebski/Arduino-DS3231.git
```

권장 고정 형식(예시)
- PlatformIO에서는 다음과 같은 형식을 지원합니다:
  - `owner/repo#<tag-or-commit>` (git url) 또는 `owner/repo@<version>`

아래는 안전한 고정 제안(각 `<tag-or-commit>`을 실제 태그/커밋으로 교체하세요):

```
Wire
adafruit/Adafruit_BusIO#<tag-or-commit>
adafruit/Adafruit-GFX-Library#<tag-or-commit>
https://github.com/deltafish32/ESP32-HUB75-MatrixPanel-DMA.git#<tag-or-commit>
adafruit/Adafruit_SSD1306#<tag-or-commit>
https://github.com/deltafish32/IRremoteESP8266.git#<tag-or-commit>
Makuna/NeoPixelBus#<tag-or-commit>
rneurink/VL53L1X_ULD#<tag-or-commit>
claws/BH1750#<tag-or-commit>
https://github.com/adafruit/Adafruit_SHT31.git#<tag-or-commit>
https://github.com/deltafish32/JPEGDEC.git#<tag-or-commit>
https://github.com/deltafish32/AnimatedGIF.git#<tag-or-commit>
https://github.com/jarzebski/Arduino-DS3231.git#<tag-or-commit>
```

권장 절차
1. 각 라이브러리의 GitHub Releases/Tags에서 안정 태그(예: v1.2.3) 또는 커밋 해시를 선택합니다.
2. 선택한 태그/커밋을 위 형식으로 `platformio.ini`의 `lib_deps`에 직접 반영하거나, 별도의 `lib_deps.lock`을 생성해 CI에서 사용하는 방식을 채택합니다.
3. CI에서 고정된 `lib_deps`로 빌드를 수행해 문제가 없는지 확인합니다.

참고: `Wire` 같은 보드 내장(Arduino core) 라이브러리는 별도 고정이 필요 없을 수 있습니다. 대신 외부 레거시 포크(deltafish32 등)를 사용하는 항목은 포크 URL과 태그를 명시적으로 고정하세요.
