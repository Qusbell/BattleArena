# 텍스처 최적화 적용 보고서

**ShootingArena · UE 5.6.1 · 2026-10-21**

Headroom 컨텍스트 해시: `ec2b06ce0d52070743c187a7`

## 적용

현재 맵에서 확인된 4K 이상 `NoMipmaps` 참조 15개를 모두 처리했다. 월드 emissive 14개는 `FromTextureGroup`으로 바꿔 13 mip을 만들고 스트리밍 가능 상태로 유지했다. 대상은 `T_SpaceShipEngine_Emissive`, `T_SecurityGate_Emissive`, `T_TripodLight_Emissive`, `T_EntranceDoor_Emissive`, `T_LightTower_Emissive`, `T_Column01_A_Emissive`, `T_Column01_B_Emissive`, `T_Light03_Emissive`, `T_AlarmLight_Emissive`, `T_EngineBox01_Emissive`, `T_ControlPanelA_Emissive`, `T_CapsuleScreen_Emissive`, `T_AutoCarriage_Emissive`, `T_cloth_rips`다.

3개 3840×2160 HUD 마스크(`T_DamageMask1/2`, `T_HighlightMask`)는 최대 크기를 2048로 제한했다. 에디터 리소스는 각각 1920×1080, 8MB로 확인됐다. 6144² 3×3 번개 플립북 `T_Lightning01_3x3`은 프레임 사이 밉 섞임을 피하려고 mip은 유지하지 않고 최대 크기 4096을 지정했다. 현재 텍스처 그룹 적용 리소스는 3072² DXT5, 9MB다. 이전 기록의 리소스 크기와 비교하면 HUD 약 71.6MB, 번개 약 27MB 감소다.

## 검증

6개 실내 텍스처의 NavMesh 내부 A/B(3시점×2회, 12구간)에서 상주량은 48MB에서 0.48–0.96MB/시점으로 감소했다. 프레임 평균은 57.97→51.12ms, GPU 평균은 54.56→48.04ms였지만 AI와 이펙트 변동 및 A2 중층 81.92ms 편차가 커 mip 효과로 FPS 향상을 단정하지 않는다. 정적 실내 표면에서 큰 화질 저하는 보이지 않았다. HUD·번개 새 상한 변경의 픽셀 단위 전후 화질 및 고해상도 모니터 확인은 하지 않았다.

모든 변경 에셋은 저장했고, 적용 전 원본은 [백업 및 해시 목록](TextureMipBackups)에 보관했다. PIE는 종료했고 맵/내비게이션은 수정하지 않았다. [실내 비교 화면](TextureMipComparison/comparison_interior.png) · [원시 프레임/메모리 집계](TextureMipComparison/interior_results.json)

**범위:** 위 조치로 현재 맵에서 찾아낸 고해상도 `NoMipmaps`와 큰 HUD/VFX 텍스처를 처리했다. 밉이 이미 있는 나머지 4K/8K 텍스처는 근접 품질·화면 점유율을 확인하지 않고 일괄 축소하지 않았다. 패키징 빌드, 고정 리플레이, 이동 중 팝인 및 전체 GPU 피크는 미검증이다. 추정 절감량은 서로 다른 시점의 측정이므로 합산하지 않는다.
