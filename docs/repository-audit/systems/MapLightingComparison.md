# Lvl_02Map과 Lvl_PortalMap 조명 차이 및 DropItem 조사

2026-11-12 · Unreal Engine 5.6 · 두 맵의 현재 저장본과 초기 PIE 표본

`Lvl_02Map`은 따뜻한 환경광·직접광에 높은 그림자 대비와 색 틴트가 더해집니다. `Lvl_PortalMap`은 고정 노출에서 강한 Bloom과 국소 Spot/Rect가 화면을 만듭니다. **두 맵에서 확인한 공통 DropItem 메시 13종은 슬롯 머티리얼까지 동일합니다.** `Lvl_02Map`에서 유독 쨍해 보인다면, 자체 발광이 해당 맵의 노출·색보정과 어두운 배경을 만나 강조되는 것이 가장 유력합니다. 확인한 저장본에서 아이템 에셋별 Nanite가 꺼져 있어, Nanite를 별도로 켠 재현 조건의 차이는 확인하지 못했습니다.

## 1. 두 맵의 옵션 차이

| 옵션 | Lvl_02Map | Lvl_PortalMap | 화면에서 확인할 차이 |
| --- | --- | --- | --- |
| 적용 볼륨 | `PostProcess` 1개 | `PostProcessVolume2` 활성, `PostProcessVolume1` 비활성 | 두 활성 볼륨은 모두 Unbound, Blend Weight `1.0`, Priority `0`입니다. 두 PIE의 일반 1인칭 카메라에는 별도 오버라이드가 없었습니다. |
| 전역 노출 | Histogram, 기본 `+1.0 EV`, Min/Max `-10 / 20 EV100` | Histogram, 오버라이드 `+0.5 EV`, Min/Max `1 / 1 EV100` | 02는 시야 구성에 따라 노출이 적응하고 Portal은 고정됩니다. 보정값의 차이 `0.5 EV`만 비교하면 같은 측광 조건에서 약 `1.41배`이나, 실제 프레임의 측광 조건은 다릅니다. [Epic의 자동 노출 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine?application_version=5.6) |
| Local Exposure | 기본 Bilateral, Highlight/Shadow Scale `0.8/0.8` | Fusion, Shadow Scale `1.0` 오버라이드 | 어두운 배경과 밝은 픽업이 한 화면에 있을 때 밝기 재분배 방식이 다릅니다. Global Shadow Contrast와 별개입니다. |
| 색과 대비 | Contrast `1.15`, Shadow Contrast `1.67`, Gamma `0.87`, Tint RGB `(1.20,1.0284,0.8568)` | Saturation `1.10`, Gamma `0.88`, Contrast·Shadow Contrast·Tint 기본값 | 02는 배경과 발광부의 대비와 따뜻한 색 편향이 후보입니다. Portal은 명시적인 채도 증가가 있습니다. |
| 톤과 주변부 | Film Toe `0.378571`, Vignette `0.70`, AO Intensity/Power `0.714286/5.140953` | 엔진 기본 Film Toe `0.55`, Vignette `0.40`, AO 오버라이드 꺼짐 | 암부와 모서리의 밝기 차이로 동일한 아이템도 더 도드라져 보일 수 있습니다. |
| Bloom | 기본 Intensity `0.675`, Threshold `-1`. 볼륨의 저장값 `1.295238 / -0.057143`은 미적용 | Intensity `2.0`, Threshold `-1` | Portal의 Intensity 설정은 약 `2.96배`입니다. 픽업 주변 후광만 02에서 더 강하게 보인다면 Bloom 숫자만으로 설명하기 어렵습니다. [Epic의 Bloom 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/bloom-in-unreal-engine?application_version=5.6) |
| 반사 | 볼륨에서 SSR 명시. Intensity/Quality `100/100`, Max Roughness `0.321143` | 프로젝트 Lumen 반사 상속. 품질 그룹 `1`에서는 Lumen 반사가 꺼져 SSR 폴백 가능. SSR 기본 `100/50`, Max Roughness `0.60` | 두 맵의 조사 품질은 `r.SSR.Quality=2`입니다. 높은 품질에서는 Portal의 Lumen 경로가 달라질 수 있습니다. Lit 슬롯의 반짝임은 별도 확인 대상입니다. |
| 하늘·방향광 | 활성 SkyLight `0.50`, 지정 `TC_Interior`, 따뜻한 색. 활성 Directional `13 lux`, Pitch `-77.02°` | SkyLight `1.0`과 Directional `20 lux` 모두 Visible 꺼짐 | 02의 Lit 표면에는 가시적인 방향광과 환경광이 추가됩니다. |
| 국소 광원 | Movable Point `5269.20 Unitless` 4개, 이 중 3개 반경 `41.13 m`. 배럴 Point `500` 8개. Static 17개는 베이크 기여 제외 | Movable Spot 30개, Movable Rect 11개, 경보등 컴포넌트 9개, Stationary Rect/Point 3개 | 02는 넓게 겹치는 따뜻한 Point Light가 있고, Portal은 지향성 광원이 많습니다. Portal Rect 중 9개는 Specular Scale `0.003`인 반면 02의 Movable Point는 `1.0`입니다. |
| PIE Pawn 광원 | 이번 초기 표본에서 AI 5명의 Point Light 25개가 비표시. 이전 02 PIE에서는 플레이어의 5개도 비표시 | 플레이어 1명과 AI 5명의 Point Light 30개가 표본에서 표시. 강도는 각각 `1/1/3/3/10 Unitless` | 캐릭터 주변의 Lit 표면에 영향을 줄 수 있습니다. 매치 진행 중의 Visibility 전환과 Lighting Channels는 별도 확인 범위입니다. |
| 안개 | Density `0.0005`, Start `2190.48 cm`, 녹색 성분 강함. 볼류메트릭 활성 설정이나 조사 품질에서는 비활성 | Density `0.05`, Start `0`, 따뜻한 황색. 볼류메트릭 컴포넌트 비활성 | 원경 배경색과 픽업 윤곽의 체감 대비가 달라질 수 있습니다. |
| 화면 안의 발광원 | 창 Unlit Boost `9.29`, 기구 Unlit Boost `7.85`, 저장조와 배럴 색 강조 | 기구 `MI_Light02` Emissive `30`, 포털 VFX와 조건부 Scene Capture | 맵의 밝은 부분이 화면에서 차지하는 면적과 위치가 다릅니다. 특히 02에서는 화면 구성이 자동 노출에 영향을 줍니다. |

두 맵 모두 `r.DynamicGlobalIlluminationMethod=0`, `r.AllowStaticLighting=0`, `Force No Precomputed Lighting=true`, VSM 활성, 조사 세션 HDR 출력 비활성입니다. 비교에 사용한 개별 값은 [Lvl_02Map 조사](Lvl02MapLighting.md)와 [Lvl_PortalMap 조사](LvlPortalMapLighting.md)에 있습니다.

## 2. Lvl_02Map의 DropItem이 쨍해 보이는 이유

### 실제 슬롯과 발광값

초기 PIE에서 픽업 메시를 02에서 **39개**, Portal에서 **45개** 확인했습니다. 양쪽에 공통으로 나온 13개 메시 에셋은 슬롯 구성까지 같았습니다. 예를 들어 02의 `shield_001` 17개와 Portal의 15개 모두 5개 슬롯에 `M_Plasma`를 사용합니다. 02 표본에만 `Buff_03`이 있었고 Portal 표본에만 `MachineGun_02`가 있었습니다. 이 수치는 초기 표본이며 사망 드롭과 이후 재스폰까지 포함한 수량은 아닙니다.

아래 값은 머티리얼의 Emissive 입력에 연결된 상수로 계산한 **톤매핑 전 선형 RGB**입니다. `M_Plasma`, `M_ShotGun`, `M_RailGun`, `M_MachineGun`, `M_RocketLauncher`, `Buff` 모두 `Masked / Unlit`이며 색 벡터에 `50`을 곱합니다.

| 확인한 메시·슬롯 | 실제 머티리얼 | Emissive RGB |
| --- | --- | --- |
| `shield_001` 5슬롯, `Plasma_Pick`, 플라스마 탄약의 발광 슬롯 | `M_Plasma` | `(0, 0.318, 5.990)` |
| `Shotgun_Spawn`, 샷건 탄약의 발광 슬롯 | `M_ShotGun` | `(50.000, 4.363, 0)` |
| `SM_Heal2`, `RailGun_03`, 레일건 탄약의 발광 슬롯 | `M_RailGun` | `(0, 12.240, 0.374)` |
| `Shield_03`, `HealPAck_07`, 머신건 탄약의 발광 슬롯 | `M_MachineGun` | `(5.449, 5.990, 0)` |
| `SM_Rocket_05`, 로켓 탄약의 발광 슬롯 | `M_RocketLauncher` | `(14.063, 0, 0.105)` |
| `Buff_03` | `Buff` | `(5.990, 0.312, 4.408)` |

1. **픽업 자체 발광이 가장 직접적인 원인입니다.** Unlit 슬롯의 광량은 맵의 Point Light Intensity로 조절되지 않습니다. 자동 노출이 어두운 통로를 들어 올리는 구도에서는 발광부도 같이 올라갑니다. 02의 그림자 대비와 비네트는 배경과 픽업 간 대비를 키울 수 있습니다. 적색 채널을 올리는 `Scene Color Tint.R=1.20`의 영향은 특히 샷건·로켓의 따뜻한 발광색에서 비교할 가치가 있습니다.
2. **탄약 상자의 Lit 슬롯에는 국소 광원과 반사가 추가로 작용합니다.** 조사한 탄약 메시들은 발광 슬롯 외에 Default Lit `Sci-if_AmmoBox_01_Ammo_Inst`를 사용합니다. Roughness/Metallic은 전용 ARM 텍스처를 사용하므로 단일 스칼라로 판단할 수 없습니다. 이 슬롯의 Emissive Boost `400`은 `T_Default_Emissive`를 곱한 값이며 텍스처 픽셀을 확인하기 전에는 실제 발광량으로 해석할 수 없습니다. 예를 들어 02의 플라스마 탄약 한 곳은 `PointLight75/76/77` 세 광원 중심에서 각각 약 `35.4 / 23.6 / 27.8 m`로, 모두 `41.13 m` 감쇠 반경 안입니다. 실제 광량은 벽과 메시 법선에 따라 달라집니다.
3. **회전에 따라 깜빡이는 부위는 발광 면적과 Lit 반사를 따로 봅니다.** `BP_DropItem.Spin`은 메시 변환을 매 프레임 갱신합니다. 회전으로 카메라에 드러나는 발광 면적과 윤곽이 달라질 수 있고, Lit 슬롯은 법선 방향과 SSR에 따라 반짝일 수 있습니다. [메시 회전 경로를 설명하는 C++ 주석](../../../Source/ShootingArena/Private/Items/DeathDropCarrier.cpp#L236)

### Nanite가 켜진 조건에서 추가로 확인할 부분

조사 세션에서 두 맵 모두 `r.Nanite=1`이었지만, 확인한 초기 픽업 메시 84개 인스턴스의 원본 Static Mesh는 모두 `Nanite Settings.Enabled=false`였습니다. `Disallow Nanite=false`는 컴포넌트에서 허용한다는 뜻이며, 에셋에서 꺼진 Nanite를 켜지 않습니다. 위 여섯 Unlit 머티리얼과 탄약의 Lit 인스턴스에서 Nanite Override Material의 대상은 모두 비어 있었습니다. 여섯 Unlit의 실제 Emissive 경로는 상수 두 개의 곱이므로 그 경로에서 Nanite Pass Switch도 확인되지 않았습니다. [Epic의 Nanite 머티리얼 및 컴포넌트 설정](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine?application_version=5.6#material-workflows)

재현 대상에서 실제로 Nanite가 활성이라면, Nanite 메시와 Fallback Mesh의 실루엣·삼각형·UV 차이로 작은 발광 면의 화면 점유율이나 마스크 윤곽이 달라질 수 있습니다. **Lit 슬롯**은 추가로 법선과 탄젠트, 노멀맵 및 거칠기에 따른 하이라이트 모양이 달라질 수 있습니다. **Unlit 슬롯**은 위 발광 상수가 그대로이므로, 해당 경로에서는 탄젠트보다 실루엣·발광 면적·마스크 커버리지를 먼저 비교합니다. [Epic의 Nanite Fallback 및 정밀도 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine?application_version=5.6#nanite-fallback-mesh-and-precision-settings)

## 3. 원인을 가르는 비교 순서

1. 같은 메시·슬롯·위치·회전·카메라·품질에서 화면을 고정합니다. Nanite 시각화의 Mask로 **픽업 자체가** Nanite 경로에 들어가는지 먼저 확인합니다. 가능하면 재현한 한 인스턴스의 `Disallow Nanite`만 바꿔 Fallback과 비교합니다. 맵 전체의 Nanite를 끄면 주변 지오메트리와 그림자도 함께 달라집니다.
2. Nanite 상태를 유지한 채 노출을 고정하고, 픽업 Emissive 배수 `50 → 25`와 Bloom `0.675 → 0`을 각각 비교합니다. 발광면 자체의 밝기와 후광의 폭을 따로 기록합니다. 특히 `Shotgun_Spawn`이나 `shield_001`은 슬롯 전체가 Unlit이라 직접광과 구분하기 쉽습니다.
3. 탄약 상자의 몸체만 시야 각도에 따라 번쩍이면 `Roughness`, `World Normal` 시각화를 확인하고 SSR을 임시로 끄는 비교를 진행합니다. 그 상태에서도 반짝이면 근처 Point Light의 `Specular Scale`을 따로 비교합니다.
4. 두 맵 사이의 차이만 보려면 같은 아이템을 두 맵에서 고정 EV100·동일 Compensation·동일 Bloom으로 먼저 비교합니다. 이후 02의 색보정과 기존 자동 노출을 한 항목씩 복원하면 차이가 시작되는 설정을 확인할 수 있습니다.

PIE로 액터·머티리얼 할당과 후처리 설정을 확인했습니다. 별도 Nanite 활성 상태에서의 프레임 비교, 포털 캡처 화면의 휘도 측정, 패키징 빌드 검증은 수행하지 않았습니다.
