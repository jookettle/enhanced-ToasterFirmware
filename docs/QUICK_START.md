# Quick Start

이 문서는 빠르게 펌웨어를 빌드하고 장치에 올리는 방법을 정리합니다.

전제조건
- `PlatformIO`가 설치된 Python 3.x (권장: 3.11+)
- USB로 ESP32 보드 연결

빌드

```bash
pip install -U platformio
cd <repo-root>
# 기본 개발 보드 빌드
platformio run -e esp32dev
# 또는 Adafruit MatrixPortal(S3) 대상 빌드
platformio run -e adafruit_matrixportal_esp32s3
```

플래시

```bash
platformio run -e esp32dev -t upload
```

파일 시스템 (데이터/이모션)

프로젝트의 `data/` 디렉터리는 표정(emotions), 설정(config.yaml) 등을 포함합니다. 장치에서 `FFat` 또는 SD를 사용하도록 빌드된 경우, 아래 방법 중 하나로 파일을 올릴 수 있습니다.

- SD 카드: `data/`의 내용을 SD 카드에 복사
- FFat (내장 플래시): PlatformIO의 `uploadfs` 타겟(사용 가능한 경우) 또는 esptool 기반의 외부 스크립트 이용

문서 및 추가 정보
- 하드웨어/설치 세부사항: `docs/hardware.md`, `docs/install.md`, `docs/hardware_en_US.md`, `docs/install_en_US.md`
- 라이선스 및 서드파티 목록: `THIRD_PARTY_LICENSES.md`

문제가 발생하면 `CONTRIBUTING.md`를 참고하여 이슈를 열어 주세요.
