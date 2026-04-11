# Issues and Priorities

간단 요약
- 리포지토리를 스캔해 잠재적 문제와 개선 권장사항을 정리했습니다. 이미 CI 워크플로를 추가했고, 외부 라이선스 스냅샷을 수집하는 스크립트를 추가했습니다.

중요(즉시 조치 권장)
- `platformio.ini`의 `lib_deps`가 버전/커밋으로 고정되어 있지 않음 — 빌드가 외부 라이브러리 변경에 민감해짐. 대응: 각 라이브러리를 `owner/repo#<tag|commit>` 또는 `owner/repo@<version>` 형태로 고정하고, CI에서 고정 검증(또는 `lib_deps.lock`)을 추가하세요.
- FFat 마운트 실패 시 부팅이 중단됨 (`FFat.begin()` 실패 시 `begin()`이 false를 반환) — 초기 설정이나 SD/FFat가 없는 상태에서 디바이스가 부팅 불가. 대응: 안전 모드(fallback) 추가 — 기본 내장 설정으로 계속 부팅하거나, 최소한 시리얼/복구 모드로 진입하도록 개선하세요.
- 라이선스 확인 미비(특히 일부 Adafruit 항목 자동 추정 실패) — GPLv3 프로젝트에 포함된 라이브러리들의 라이선스 호환성 확인 필요. 대응: `third_party_licenses/` 스냅샷을 수동 검토하고 `THIRD_PARTY_LICENSES.md`에 정식 기입하세요.

높음(우선 적용 권장)
- CI 보강: 현재 빌드(컴파일)만 수행 — `buildfs` 또는 `build data`(FFat 이미지) 생성 및 업로드/아티팩트 저장 단계를 추가하세요(릴리스 자동화용). 또한 `lib_deps` 고정 유무를 검증하는 체크를 추가하세요.
- 테스트/품질: 단위/통합 테스트가 없음. PlatformIO `unity` 프레임워크 또는 호스트용 C++ 테스트(서브셋) 추가 권장.

중간
- 문서 보강: 하드웨어 요구사항(PSRAM 여부, 권장 보드), FFat/SD 업로드 예시, 릴리스 프로세스 문서화(FFat 이미지 만드는 방법). 일부 문서는 이미 추가했습니다(`docs/QUICK_START.md`, `CONTRIBUTING.md`).
- 로깅·디버그: 시리얼·HUD 로그 레벨 및 원격 디버그 방법 문서화.

낮음(추후)
- 코드 스타일·포맷: `clang-format` 또는 `cpplint` 같은 자동 포매터/검사 적용.
- 성능·프로파일링 도구: 프레임 드롭/메모리 사용량 모니터링 스크립트.

권장 즉시 작업(간단한 체크리스트)
1. CI에서 `platformio run -e <env>` 외에 `platformio run -e <env> -t buildfs`를 실행해 파일시스템 이미지를 생성하고 아티팩트로 업로드. (릴리스 자동화의 기반)
2. `scripts/check_third_party_licenses.py` 실행 결과를 검토해 `THIRD_PARTY_LICENSES.md`를 보강.
3. `platformio.ini`의 `lib_deps`를 우선순위가 높은 라이브러리(특히 `ESP32-HUB75-MatrixPanel-DMA`, `IRremoteESP8266`, `NeoPixelBus`)부터 태그/커밋으로 고정. (원하시면 제가 PR용 패치를 생성해 드립니다.)
4. FFat 실패 시 폴백 동작(내장 기본 설정으로 계속 부팅 또는 복구 모드 진입) 구현 고려.

다음 단계 제안
- 제가 자동으로 `platformio.ini`의 `lib_deps`에 대해 PR용 placeholder(각 항목에 `#<commit>` 표시) 파일을 생성해 드릴까요? 아니면 먼저 `THIRD_PARTY_LICENSES.md`에 전체 라이선스 텍스트를 병합할까요?
