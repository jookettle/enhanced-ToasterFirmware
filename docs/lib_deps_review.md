# lib_deps 리뷰 및 권장 고정안

다음은 현재 `platformio.ini`의 `lib_deps` 항목을 기반으로 한 검토 결과 및 권장 작업입니다.

현재 `platformio.ini`의 `lib_deps` (요약):

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

권장 목표
- 모든 외부 라이브러리는 가능한 한 특정 태그 또는 커밋으로 고정(`git url#<tag|commit>` 또는 `owner/repo@<version>`) 하세요.
- 라이선스 텍스트를 자동 수집해 `THIRD_PARTY_LICENSES.md`를 갱신하세요. 아래 스크립트를 추가했습니다.

권장 `lib_deps` 고정 예시(수동 확인 필요)
- GitHub 저장소가 명확한 경우: `https://github.com/<owner>/<repo>.git#<tag-or-commit>`
- 예시 (사용자 확인 필요, 태그/커밋으로 교체하세요):
  - `https://github.com/adafruit/Adafruit-GFX-Library.git#<tag-or-commit>`
  - `https://github.com/adafruit/Adafruit_BusIO.git#<tag-or-commit>`
  - `https://github.com/adafruit/Adafruit_SSD1306.git#<tag-or-commit>`
  - `https://github.com/Makuna/NeoPixelBus.git#<tag-or-commit>`
  - `https://github.com/rneurink/VL53L1X_ULD.git#<tag-or-commit>`
  - `https://github.com/claws/BH1750.git#<tag-or-commit>`

자동화 스크립트
- `scripts/check_third_party_licenses.py` : `platformio.ini`에서 라이브러리 목록을 읽어 GitHub 저장소를 추정하고 LICENSE 파일을 다운로드해 `third_party_licenses/`에 저장합니다. (로컬에서 실행하면 라이선스 상태를 수집해 `THIRD_PARTY_LICENSES.md` 업데이트에 도움됩니다.)

권장 작업 순서
1. 로컬에서 스크립트 실행해 각 라이브러리 라이선스 수집 (아래 명령 참조).
2. CI에서 빌드(현재 GitHub Actions) 실행해 빌드 통과 확인.
3. 각 라이브러리에 대해 안정 태그 또는 커밋을 골라 `platformio.ini`의 `lib_deps`를 `git url#<commit>` 형태로 교체.
4. `THIRD_PARTY_LICENSES.md`에 수집된 라이선스와 출처를 추가하여 PR 제출.

명령 예시
```bash
python3 scripts/check_third_party_licenses.py
```

참고: 스크립트는 네트워크 접속을 필요로 하며, GitHub API 토큰을 사용하면 rate limit 우회가 가능합니다. 스크립트 사용 후 결과를 검토해 태그/커밋을 수동으로 고정하는 것을 권장합니다.
