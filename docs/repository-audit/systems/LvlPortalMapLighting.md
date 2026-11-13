# Lvl_PortalMap 밝기와 색 대비 조사

2026-11-12 · Unreal Engine 5.6 · `/Game/QuakeLike_Base/Lvl_PortalMap`

맵 전체의 후광은 활성 Post Process Volume의 **Bloom Intensity `2.0`**부터 확인할 수 있습니다. 전체 밝기는 **고정 `1 EV100`과 Exposure Compensation `+0.5`**의 조합이 결정합니다. 특정 위치에서만 밝다면 국소 Spot Light와 기구의 발광이 우선 후보입니다. 배치된 Directional Light와 SkyLight는 둘 다 비표시 상태이며 PIE에서도 유지됐습니다.

에디터에서 로드된 배치 액터 2,573개와 컴포넌트를 조사하고, PIE에서 일반 1인칭 카메라와 광원 상태를 확인했습니다. 같은 시점의 휘도 A/B와 패키징 빌드의 렌더 결과는 검증 범위에 포함하지 않았습니다.

## 1. 밝기와 색에 영향을 주는 요소 전수조사

### 직접광과 환경광

배치 광원은 **55개 컴포넌트**입니다. Movable 52개 중 Directional·SkyLight 2개가 비표시이고, 나머지 3개는 Stationary입니다. `Allow Static Lighting=0`과 `Force No Precomputed Lighting=true`이므로 베이크된 조명 기여는 제외됩니다. 표의 단위는 각 컴포넌트에서 확인했으며, cd·lm·lux·Unitless의 숫자를 서로 직접 비교할 수 없습니다. [Epic의 조명 단위 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-physical-lighting-units-in-unreal-engine?application_version=5.6)

| 광원 | 수량 | 현재값 |
| --- | ---: | --- |
| Movable Spot | 30 | `5000 cd` 7개, `2000 cd` 4개, `1600 cd` 1개, `1500 cd` 13개, `1400 cd` 2개, `600 cd` 3개. 반경 약 `10.74–64.45 m`, 역제곱 감쇠, Specular Scale `1.0`. 16개에서 Cast Shadows가 켜져 있습니다. |
| Movable Rect | 11 | `3000 lm` 3개, `2010.62 lm` 5개, `400 lm` 2개, `200 lm` 1개. 모두 `3500 K`, 반경 `9.25–20 m`, Cast Shadows 꺼짐. Specular Scale은 9개가 `0.003`, 400 lm 그룹 2개가 `0.5`입니다. |
| Stationary Rect·Point | 3 | `RectLight23` `8.06 lm`·반경 `10 m`와 `PointLight10/11` 각각 `30 lm`·반경 `14.5/16.5 m`. 모두 RGB `(255,176,105)`. |
| `BP_AlarmLight` | 9 | 액터 3개에 `Light01/02` 각 `50000 Unitless`·`2000 K`·반경 `10 m`, `PointLight` 각 `5000 Unitless`·`2500 K`·반경 `3 m`. 모두 Movable이며 Cast Shadows가 꺼져 있습니다. PIE 표본에서 9개 모두 Visible입니다. |
| Directional·SkyLight | 2 | Movable Directional `20 lux`, `3500 K`, Pitch `-50°`. Movable SkyLight `1.0`, Captured Scene, Real Time Capture 켜짐. 두 컴포넌트 모두 `Visible=false`입니다. |

`5000 cd` Spot은 `SpotLight12/16/17/35/45/46/94`입니다. 94의 반경은 `6445.20 cm`, 나머지 6개는 `4000 cm`입니다. 조사한 광원에 할당된 IES 또는 Light Function은 발견되지 않았습니다. Reflection Capture와 Planar Reflection 배치도 확인되지 않았습니다.

### 후처리와 환경 효과

| 항목 | 적용 기준과 현재값 |
| --- | --- |
| 볼륨과 카메라 | `PostProcessVolume2`가 활성, Unbound, Blend Weight `1.0`, Priority `0`입니다. `PostProcessVolume1`은 비활성이므로 저장된 Compensation `-0.02`가 적용되지 않습니다. PIE의 `BP_PlayerShooter.FP_Camera`와 배치된 `EndMatchCamera` 모두 후처리 오버라이드와 Blendable이 비어 있습니다. |
| 자동 노출 | 프로젝트 기본 Histogram을 상속합니다. 활성 볼륨은 Min/Max EV100을 둘 다 `1.0`으로 설정해 자동 적응 범위를 고정합니다. Compensation은 `+0.5`입니다. [Epic의 자동 노출 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine?application_version=5.6) |
| 로컬 노출과 색보정 | Local Exposure Method `Fusion`, Shadow Contrast Scale `1.0` 오버라이드. Global Saturation RGB `1.10`, Gamma RGB `0.88`. Global Contrast, Scene Color Tint, Film Toe, LUT의 볼륨 오버라이드는 꺼져 있습니다. |
| Bloom과 Lens Flare | Bloom Intensity `2.0` 활성, Threshold `-1` 상속. Threshold `-1`에서는 밝기 문턱 없이 화면의 픽셀이 Bloom에 기여할 수 있습니다. Lens Flare는 프로젝트 기본 비활성입니다. [Epic의 Bloom 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/bloom-in-unreal-engine?application_version=5.6) |
| GI와 반사 | Dynamic GI `None`. 반사는 프로젝트의 `Lumen` 선택을 상속합니다. 조사 품질 `sg.ReflectionQuality=1`에서는 엔진 기본 `r.Lumen.Reflections.Allow=0`, `r.SSR.Quality=2`라서 일반 화면은 SSR로 폴백할 수 있습니다. UE 5.6의 `BaseScalability.ini`와 `ScreenSpaceRayTracing.cpp`에서 조건을 확인했습니다. |
| 그림자와 AO | VSM 활성. 활성 볼륨에서 AO를 오버라이드하지 않아 엔진 기본값을 따릅니다. 그림자가 꺼진 Spot/Rect의 밝기는 배치 방향과 감쇠 범위에 크게 좌우됩니다. |
| 안개와 대기 | Exponential Height Fog Density `0.05`, Height Falloff `0.001`, Start Distance `0`, Inscattering RGB `(1.0,0.79755,0.36998)`. Volumetric Fog는 컴포넌트에서 꺼져 있습니다. Sky Atmosphere와 Volumetric Cloud는 Visible이 꺼져 있고, 레거시 Atmospheric Fog 컴포넌트가 배치되어 있습니다. |
| 출력과 품질 | 작업 색공간 sRGB, 조사 세션의 HDR 출력 꺼짐, Post Process/Shadow/Reflection/GI 품질 그룹 `1`, Bloom Quality `4`입니다. |

### 머티리얼과 포털 화면

에디터에서 숨겨지지 않은 액터의 메시 슬롯에 고유 머티리얼 65종, 배치 데칼에 3종, 두 경로를 합쳐 부모 머티리얼 11종이 있었습니다. 데칼 액터는 30개입니다.

- `MI_Light02`는 메시 슬롯 123곳에서 사용되며, 발광 텍스처에 적용되는 `Emissive=30`, `Specula=1.0`, `Rough A/B=0.25/0.40`을 확인했습니다. 같은 부모를 쓰는 조사 표본 `MI_BigWall_A/D/E`는 `Emissive=0`입니다.
- `MI_Fog01/02`는 합계 11개 슬롯의 반투명 기구 효과로, 둘 다 `Emissive=0.1`입니다. `MI_Fog02`의 FogColor는 `(1.0,0.55902,0)`입니다. 아이템 스포너 46개는 별도의 발광 리스폰 타이머를 사용합니다.
- 포털 액터 5개의 Niagara와 점프패드 4개의 Niagara가 표시됩니다. 별도로 배치된 One Way Portal 3개의 Niagara는 비표시입니다. 포털 너머 화면은 활성화된 Scene Capture의 HDR/LDR 소스와 Show Flags에 따라 달라질 수 있습니다. C++에서 Spatial View는 메인 카메라 후처리 설정을 상속하고, 레거시 경로는 품질에 따라 Bloom과 Eye Adaptation 플래그를 설정합니다. 초기 PIE 표본에서 활성 포털 캡처의 렌더 결과는 확인하지 않았습니다. [포털 캡처 구현](../../../Source/ShootingArena/Teleport/PortalViewSubsystem.cpp#L494)

## 2. 가장 가능성이 높은 수치와 시험값

| 증상 | 첫 후보 | 비교값 |
| --- | --- | --- |
| 기구·픽업 주변에 후광이 크게 퍼짐 | **Bloom Intensity `2.0`**과 Threshold `-1`, `MI_Light02` 발광 `30` | 볼륨에서 `2.0 → 1.0 → 0.675`를 한 단계씩 비교합니다. 특정 기구만 남으면 해당 기구의 발광을 별도로 봅니다. |
| 화면 전체가 밝음 | **고정 Min/Max `1 EV100`과 Compensation `+0.5`** | Min/Max를 유지하고 `+0.5 → 0.0`을 비교합니다. 같은 조도에서 톤매핑 전 노출 배율은 약 `0.707배`가 됩니다. |
| 특정 통로의 밝은 면만 튐 | **Spot `5000 cd` 7개**, 반경 `40 m` 또는 `64.45 m`, Specular Scale `1.0` | 지목된 위치를 비추는 광원만 `5000 → 3750 cd`로 비교합니다. 반짝임만 문제라면 강도를 복원하고 Specular Scale을 따로 확인합니다. |
| 경보등 주변만 따뜻하고 과하게 밝음 | **`BP_AlarmLight`의 `Light01/02=50000 Unitless`** | 해당 경보등 그룹을 식별한 뒤 강도 `75%` 또는 반경을 하나씩 시험합니다. |
| 화면 색만 쨍함 | **Global Saturation `1.10`** | `1.00`으로 비교합니다. |

## 3. 추가로 검토할 옵션

- 먼 곳까지 누렇게 밝으면 높이 안개의 Density와 Inscattering Color를 우선 분리합니다. 광원에 가까울 때만 안개처럼 번지면 기구 메시의 `MI_Fog01/02`도 따로 봅니다.
- 감마 `0.88`의 영향은 노출 비교 이후 확인합니다. 광원과 암부의 차이가 문제라면 Fusion과 Bilateral을 동일한 고정 EV100에서 비교합니다.
- 그림자 없이 다른 구역까지 광원이 닿으면 Spot의 Outer Cone, 감쇠 반경과 Cast Shadows, Rect의 위치와 방향을 검토합니다. 배경이 정상이고 특정 금속만 뜨면 해당 머티리얼의 Roughness·Specular와 실제 반사 경로를 확인합니다.
- 포털 내부 화면만 다르면 메인 뷰와 포털 뷰를 나란히 비교하고, Scene Capture의 HDR/LDR 소스 및 후처리 상속 경로부터 확인합니다.

## 4. Dynamic에 적용되는 조명 옵션

| 적용 대상 | 조절 옵션 |
| --- | --- |
| Movable Spot·Rect·경보등 | `Intensity`, `Temperature/Light Color`, `Attenuation Radius`, Spot의 Cone, Rect의 Source Width/Height, `Specular Scale`, `Diffuse Scale`. 경보등의 시간 변화는 동일 이벤트 시점에서 확인합니다. |
| 움직이는 캐릭터·무기·픽업의 Lit 슬롯 | 동적 그림자와 VSM, 광원 범위, 머티리얼 Roughness·Specular로 조절합니다. 특정 불투명 오브젝트만 별도 조명할 경우 광원과 메시의 Lighting Channels를 함께 설정합니다. [Epic의 Lighting Channels 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/lighting-channels?application_version=5.6) |
| 픽업의 Unlit 슬롯 및 Niagara | 직접광의 세기와 별도로 자체 Emissive, 노출, 색보정, Bloom이 보이는 강도를 결정합니다. 두 맵에 공통으로 등장한 픽업의 확인값은 [맵 비교 보고서](MapLightingComparison.md#2-lvl_02map의-dropitem이-쨍해-보이는-이유)에 정리했습니다. |
| PIE에서 추가된 Pawn 광원 | 플레이어 1명과 AI 5명에 Movable Point Light가 각 5개 있었습니다. PIE 표본에서 30개 모두 `Visible=true`였고, 강도는 Pawn별 `1/1/3/3/10 Unitless`입니다. 플레이어·AI가 가까울 때 화면이 바뀌면 Visibility, Lighting Channels, 강도와 반경을 확인합니다. |
| SkyLight·Dynamic GI·Volumetric Fog | 현재 비표시 또는 비활성입니다. 별도 실험에서 활성화할 경우 SkyLight Capture·Intensity, GI의 간접광 기여, 광원별 Volumetric Scattering Intensity를 하나씩 확인합니다. |

프로젝트 공통값은 [DefaultEngine.ini](../../../Config/DefaultEngine.ini#L76)에서 확인할 수 있습니다.
