# Contributing

작은 변경이나 버그 리포트, 문서 개선 모두 환영합니다.

간단한 개발/기여 흐름

1. 저장소 포크/클론
2. 새 브랜치 생성: `git checkout -b fix/your-change`
3. 변경 및 로컬 빌드(아래 참고)
4. 테스트 후 PR 생성

빌드 및 플래시 (빠른 예)

- PlatformIO 설치: `python -m pip install --upgrade pip` 그리고 `pip install -U platformio`
- 빌드:
  - `platformio run -e esp32dev`
  - `platformio run -e adafruit_matrixportal_esp32s3`
- 플래시:
  - `platformio run -e esp32dev -t upload` (장치 연결 필요)

파일시스템(FFat/SD) 업로드

프로젝트는 런타임에 `FFat` 또는 SD 카드를 사용합니다. 파일을 장치에 올리려면:

- SD 사용: `data/` 디렉터리의 내용을 SD 카드에 복사
- FFat 사용(지원하는 경우): PlatformIO의 `uploadfs` 타겟 사용 (`platformio run -e <env> -t uploadfs`) 또는 외부 도구로 FFat 이미지 생성 후 업로드

외부 라이브러리 추가시

1. `platformio.ini`의 `lib_deps`에 항목 추가
2. `scripts/check_third_party_licenses.py` 실행하여 라이선스 수집
3. `THIRD_PARTY_LICENSES.md` 업데이트

코드 스타일

- C++ 코드는 기존 스타일을 따릅니다 (헤더/소스 분리, 네임스페이스 `toaster` 등).

이슈/PR 템플릿

간단한 설명, 재현 방법, 기대 동작을 포함해 주세요.
