# Lvl_02Map 밝기와 색 대비 조사

2026-11-12 · Unreal Engine 5.6 · `/Game/QuakeLike_Base/Lvl_02Map`

화면 전체가 밝다는 피드백에는 **기본 Exposure Compensation `+1.0 EV`**가 가장 먼저 확인할 값입니다. 색과 명암이 쨍하다는 피드백에는 **맵 전체에 적용되는 Shadow Contrast `1.67`과 Global Contrast `1.15`**가 가장 유력합니다. 특정 구역에서만 밝다면 넓은 범위가 겹치는 `PointLight75/76/77`을, 창과 조명 기구만 하얗게 번진다면 발광 머티리얼을 먼저 봅니다.

에디터에서 배치 액터 2,946개와 컴포넌트를 조사했고, PIE에서 생성된 광원과 실제 1인칭 카메라의 후처리 설정을 확인했습니다. 아래 수치는 현재 설정이며, 원인 순위와 시험값은 설정의 적용 범위에 따른 가설입니다. 동일 카메라에서 변경 전후의 프레임 휘도를 비교하거나 패키징 빌드를 검증하는 단계는 남아 있습니다.

## 1. 밝기와 색에 영향을 주는 요소 전수조사

### 직접광과 환경광

배치 광원은 총 **31개 컴포넌트**입니다. Movable 14개와 Static 17개이며, PIE에서도 배치 광원의 Mobility와 강도가 유지됐습니다. 아래 Point와 Rect의 Intensity 단위는 모두 `Unitless`입니다. Directional의 `lux`와 직접 비교할 수 없습니다. [Epic의 조명 단위 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-physical-lighting-units-in-unreal-engine?application_version=5.6)

| 액터 및 컴포넌트 | 개수 | 현재값 및 범위 |
| --- | ---: | --- |
| `DirectionalLight` | 1 | Movable. `13 lux`, RGB `(255, 238, 207)`, Pitch `-77.02°`. 동적 그림자가 켜져 있어 가려지지 않은 면에 직접광과 하이라이트를 만듭니다. Atmosphere Sun Light는 꺼져 있습니다. |
| `SkyLight` | 1 | Movable. Intensity Scale `0.50`, RGB `(255, 248, 191)`, Specified Cubemap `TC_Interior`. Real Time Capture는 꺼져 있고 하반구는 검정입니다. 그림자 안의 밝기와 전체 색조에 영향을 줄 수 있습니다. |
| `PointLight75/76/77` | 3 | Movable. 각각 `5269.20`, 반경 `4113.02 cm`, RGB `(255, 182, 157)`. 역제곱 감쇠와 동적 그림자가 켜져 있습니다. 감쇠 구체가 기하학적으로 겹치며 실제 중첩 조도는 차폐와 표면 방향에 따라 달라집니다. |
| `PointLight68` | 1 | Movable. `5269.20`, 반경 `418.71 cm`, RGB `(255, 123, 0)`. 역제곱 감쇠와 동적 그림자가 켜져 있습니다. 작은 구역의 주황색 하이라이트 후보입니다. |
| `Barrel83/84/87/88/89/90/92/93`의 `PointLight` | 8 | Movable. 각각 `500.0`, 반경 `576.38 cm`, 역제곱 감쇠. 3개는 녹색 RGB `(165, 255, 140)`, 5개는 청록 RGB `(0, 203, 255)`입니다. `Cast Dynamic Shadows=false`입니다. |
| `ReservoirPointLight3` | 1 | Static. `1.0`, 반경 `2381.10 cm`, RGB `(145, 255, 0)`, 역제곱 감쇠 꺼짐. |
| `LightActor64–71`의 `RectLight` | 8 | Static. 64/66/69/70/71은 `88911.39`, 65/67/68은 `300000`. 모두 반경 `1 cm`, Indirect Lighting Intensity `5.0`입니다. |
| `LightActor64–71`의 `AreaLight` | 8 | Static Point Light. 각각 `956.96`, 반경 `685.66 cm`, Indirect Lighting Intensity `5.0`입니다. |

`r.AllowStaticLighting=0`이고 World Settings의 `Force No Precomputed Lighting=true`입니다. 따라서 Static 광원 17개의 베이크 기여는 현재 렌더링 경로에서 제외됩니다. `BP_LightActor`의 발광 메시와 머티리얼은 광원 컴포넌트와 별도로 화면에 나타납니다. 조사한 배치 광원에서 별도 IES 또는 Light Function 할당은 발견되지 않았습니다. 별도로 배치된 Spot Light, Sky Atmosphere, Volumetric Cloud, Reflection Capture, Planar Reflection, Level Sequence 액터도 발견되지 않았습니다. [Epic의 렌더링 설정](https://dev.epicgames.com/documentation/en-us/unreal-engine/rendering-settings-in-the-unreal-engine-project-settings?application_version=5.6)

### 노출과 후처리

맵의 `PostProcess`는 **활성, Unbound, Blend Weight `1.0`, Priority `0`**이며 Post Process Material은 비어 있습니다. PIE에서 실제 View Target인 `BP_PlayerShooter`의 `FP_Camera`와 배치된 `EndMatchCamera`의 카메라 컴포넌트는 모두 후처리 오버라이드와 Blendable이 비어 있었습니다.

| 항목 | 적용 기준과 현재값 |
| --- | --- |
| 자동 노출 | 프로젝트 기본 Histogram, Compensation `+1.0 EV`, EV100 범위 `-10 ~ 20`, Histogram 백분위 `10/90`, Speed Up/Down `3/1`. 확인한 CVar에서 `r.EyeAdaptationQuality=2`, `r.ExposureOffset=0`입니다. 볼륨에 저장된 Compensation `0.0`은 체크박스가 꺼져 있어 기본값을 덮어쓰지 않습니다. [Epic의 자동 노출 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine?application_version=5.6) |
| 로컬 노출 | 프로젝트 기본 Highlight/Shadow Contrast Scale 모두 `0.8`. 볼륨의 해당 오버라이드는 꺼져 있습니다. |
| 색보정 및 톤매핑 | Global Contrast RGB `1.15`, Global Gamma RGB `0.87`, Shadow Contrast RGB `1.67`, Film Toe `0.378571`, Scene Color Tint RGB `(1.20, 1.0284, 0.8568)`, Vignette `0.70`이 활성입니다. Saturation, Gain, Offset, White Balance, LUT, Film Shoulder 및 White Clip은 볼륨 오버라이드가 꺼져 있습니다. [Epic의 색보정과 톤매핑 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/color-grading-and-the-filmic-tonemapper-in-unreal-engine?application_version=5.6) |
| Bloom 및 Lens Flare | Bloom은 켜져 있고 엔진 기본 Intensity `0.675`, Threshold `-1`을 상속합니다. 볼륨에 저장된 `1.295238 / -0.057143`은 두 오버라이드가 꺼져 있습니다. Lens Flare도 볼륨 오버라이드가 꺼져 있고 프로젝트 기본이 비활성입니다. [Epic의 Bloom 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/bloom-in-unreal-engine?application_version=5.6) |
| 반사 | 프로젝트 기본 `r.ReflectionMethod=1` 위에 볼륨의 `Screen Space`가 적용됩니다. 볼륨의 SSR Intensity/Quality는 `100/100`, Max Roughness는 `0.321143`입니다. 조사 세션의 `r.SSR.Quality=2`가 별도로 적용됩니다. SSR은 매끈한 표면의 밝은 반사 후보입니다. [Epic의 SSR 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/screen-space-reflections-in-unreal-engine?application_version=5.6) |
| 간접광과 그림자 | `r.DynamicGlobalIlluminationMethod=0`이고 볼륨에서 GI 방식을 오버라이드하지 않습니다. 볼륨에 Lumen 품질 오버라이드가 일부 남아 있지만 현재 GI·반사 방식에서는 우선순위가 낮습니다. Virtual Shadow Maps는 활성입니다. |
| Ambient Occlusion | 볼륨의 Intensity `0.714286`, Radius `19.14381 cm`(World Space), Power `5.140953`, Quality `100`, Static Fraction `0.523810`, Fade Distance `20000`, Fade Radius `0`, Mip Blend `0.485714`, Mip Scale `8.0`. 주로 접촉부와 모서리의 어두움을 바꿉니다. |
| 안개 | `ExponentialHeightFog` 1개. Density `0.0005`, Height Falloff `1.58116`, Start Distance `2190.48 cm`, Max Opacity `1.0`, Inscattering RGB `(0.857, 1.0, 0.339)`. 액터에서 Volumetric Fog가 켜져 있으나 조사 세션에서 `r.VolumetricFog=0`이었습니다. 일반 높이 안개는 `r.Fog=1`입니다. |
| 출력과 품질 | 작업 색공간은 sRGB입니다. 조사 세션은 HDR 출력 꺼짐, Post Process/Shadow/Reflection/GI 품질 그룹 `1`, Bloom Quality `4`였습니다. 품질 프리셋이 달라지면 특히 볼류메트릭 안개와 반사 결과가 달라질 수 있습니다. 에디터 뷰포트의 Exposure Game Settings/EV100 선택과 실제 플레이 설정도 비교 대상입니다. |

### 머티리얼, 데칼, 이펙트와 카메라

에디터에서 숨겨지지 않은 액터의 메시 슬롯에서 고유 머티리얼 21종, 배치 Decal Component에서 709종을 수집했습니다. 메시·데칼에서 확인된 부모 머티리얼은 총 59종입니다. 직접 연결된 발광 입력과 주요 인스턴스 파라미터를 확인했습니다. 머티리얼 함수 내부의 경로와 모든 데칼 인스턴스의 픽셀별 출력은 별도 확인 범위입니다.

| 요소 | 확인한 값과 영향 |
| --- | --- |
| 창 뒤 하늘 `MIC_SkyMaterial_Inst` | 창 구조물의 메시 슬롯 9곳. Unlit Emissive, Boost `9.28842`, Max `10.24306`, Tint RGB `(1.0, 0.584, 0.36)`. 창의 밝은 면과 Bloom에 기여할 수 있습니다. |
| 조명 기구 `MIC_LightMaterial_Inst` | `LightActor64–71`의 메시 슬롯 8곳. Unlit Emissive, Boost `7.85143`, Color RGB `(0.669883, 1.0, 0.317)`. 기구 자체가 화면에서 발광합니다. |
| 저장조·배럴 액체 | `Reservoir2`, `Reservoir2Glass2`의 `MIC_LiquidMaterial_Inst`는 선형 `ShallowColor=(0.333, 5.0, 0)`, Roughness `0.12381`, Specular `1.0`입니다. 배럴 변형은 녹색 최대 `1.4`, 청록 최대 `2.2`의 ShallowColor를 사용합니다. 액체의 색과 표면 하이라이트는 별도의 국소 후보입니다. |
| 벽·바닥·유리 | `MIC_BiohazardBaseMaterial_MasterInstance`가 메시 슬롯 749곳에서 사용되며 `MinRough=0.1`, `Specular=0.047619`입니다. 창 유리와 배럴 파편 유리는 `Specular=1.0`입니다. 알베도, Roughness, Metallic, Specular, 노멀과 유리 투과 특성에 따라 동일 조명에서 다르게 보입니다. |
| 데칼과 화면 효과 | DecalActor 728개입니다. 알베도·거칠기 등을 바꾸는 데칼과 발광 입력이 연결된 데칼 부모가 있습니다. 배치 Niagara 컴포넌트로 저장조 버블 1개와 점프패드 링 4개가 있으며, 아이템 스포너 42개에 발광 타이머 머티리얼이 있습니다. |
| 런타임 카메라·이펙트 | 일반 플레이 카메라는 위 볼륨의 영향을 받습니다. 사망 카메라에는 C++에서 강조용 Post Process Material을 추가하는 별도 경로가 있습니다. 사망·전투·버프 이벤트의 순간적인 발광과 노출 변화는 별도 프레임 검증이 필요합니다. [사망 카메라 구현](../../../Source/ShootingArena/Private/Camera/DeathCamActor.cpp#L663) |

## 2. 가장 가능성이 높은 수치와 시험값

| 순위 | 현재값 | 피드백과 연결되는 이유 | 첫 비교값 |
| --- | --- | --- | --- |
| 1 | **Exposure Compensation `+1.0 EV`** | 같은 측광 조건에서 `0 EV` 대비 톤매핑 전 노출 배율이 2배입니다. 프레임 전체가 밝거나 시점에 따라 밝기가 바뀌면 첫 후보입니다. | 맵 볼륨에서 오버라이드하고 `+0.5`, 이어서 `0.0`을 비교합니다. |
| 2 | **Shadow Contrast `1.67` / Global Contrast `1.15` / Scene Color Tint `(1.20, 1.0284, 0.8568)`** | 대비가 높으면 그림자와 밝은 면의 차이가 커집니다. 따뜻한 틴트가 전역으로 적용되어 적색·황색 계열이 더 강해 보일 수 있습니다. | Shadow Contrast `1.30`, Global Contrast `1.05`, Tint `(1, 1, 1)`을 각각 비교합니다. |
| 3 | **`PointLight75/76/77`의 `5269.20`, 반경 `41.13 m`** | 따뜻한 광원 세 개가 넓게 겹칩니다. 해당 구역의 벽·바닥·캐릭터가 함께 뜨는 경우에 유력합니다. | 반경을 고정하고 세 광원을 현재 강도의 `75%`인 약 `3952`로 비교합니다. 번지는 범위가 문제라면 강도를 복원한 뒤 반경 `3000 cm`를 따로 비교합니다. |
| 4 | **발광 Boost `9.29`(창), `7.85`(기구)** | 창·조명 기구만 하얗게 날아가거나 주변에 후광이 생기는 경우 직접적인 후보입니다. 상속된 Bloom Threshold `-1`은 전체 화면이 Bloom 입력에 기여할 수 있게 합니다. | 영향이 보이는 인스턴스만 Boost를 현재의 `50–75%`로 비교합니다. 기구의 디테일은 유지되는데 후광만 문제라면 Bloom Intensity `0.35`를 따로 비교합니다. |

동일한 PIE 카메라 위치, 화질, 해상도에서 변수 하나만 바꿉니다. 밝은 개구부를 보는 위치, 어두운 통로, Point Light가 겹치는 구역을 고정 시점으로 잡고, 노출이 안정된 뒤 화면 중앙 중간톤과 하이라이트의 세부 묘사, 캐릭터와 배경의 분리 정도를 비교합니다. 자동 노출의 영향은 `HDR (Eye Adaptation)` 뷰에서 Average Scene EV100과 Exposure Compensation을 함께 확인합니다. [Epic의 노출 시각화 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine?application_version=5.6)

## 3. 추가로 검토할 옵션

| 피드백이 남는 범위 | 비교할 옵션 |
| --- | --- |
| 가려진 곳까지 고르게 밝음 | `SkyLight` Intensity `0.50 → 0.35`를 비교합니다. 밝기보다 노란 기가 문제라면 Intensity를 유지하고 `Light Color` 또는 `TC_Interior`를 한 항목씩 비교합니다. |
| 개구부나 바닥의 직사광만 강함 | `DirectionalLight` `13 → 10 lux`를 비교합니다. 방향과 그림자 범위가 문제라면 Intensity를 복원하고 각도를 따로 시험합니다. |
| 물체 색만 과함 | 맵 볼륨 Global Saturation `0.9–0.95` 또는 문제 색을 내는 머티리얼·광원 자체의 채도를 비교합니다. 저장조의 초록색만 강하면 `ShallowColor.G=5.0`을 먼저 봅니다. |
| 중앙은 밝고 모서리와 그림자가 유독 어두움 | `Vignette 0.70 → 0.40`, `Film Toe 0.378571`의 오버라이드 해제, AO Power 완화를 각각 비교합니다. `Gamma 0.87`을 `1.0`으로 돌리면 중간톤이 밝아질 수 있으므로 노출과 따로 봅니다. |
| 회전할 때 화면이 밝아졌다 어두워짐 | Metering Mask, Histogram 백분위, Speed Up/Down, EV100 범위를 검토합니다. Min/Max EV100을 같은 값으로 고정하는 시험은 자동 적응의 영향을 확인하는 데 사용합니다. |
| 시야 각도에 따라 금속·유리만 번쩍임 | SSR Intensity `100 → 60`을 진단용으로 비교합니다. 특정 표면에 한정되면 해당 머티리얼의 Roughness·Specular를 조정하는 편이 영향 범위가 작습니다. |
| 고품질에서만 녹색 안개 또는 광선이 보임 | Shadow 품질 그룹별 `r.VolumetricFog`를 확인하고 Fog Inscattering과 광원별 Volumetric Scattering Intensity를 비교합니다. |

## 4. Dynamic에 적용되는 조명 옵션

| 적용 대상 | 현재 상태 | 조절 옵션 |
| --- | --- | --- |
| **Movable 직접광** | Directional 1개, 맵 배치 Point 4개, 배럴 Point 8개가 해당합니다. | `Intensity`, `Light Color/Temperature`, Point의 `Attenuation Radius`, 광원별 `Specular Scale`과 `Diffuse Scale`을 사용합니다. 좁은 면의 번쩍임과 공간 전체 밝기는 Specular와 Diffuse/Intensity를 각각 바꿔 확인합니다. `Source Radius/Angle`은 하이라이트 모양과 그림자 경계에 영향을 줍니다. [Epic의 Movable Light 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/movable-light-mobility-in-unreal-engine?application_version=5.6) |
| **움직이는 캐릭터·무기·오브젝트가 받는 빛** | 현재 Movable 직접광, SkyLight, 머티리얼과 후처리의 영향을 받습니다. 프로젝트에 VSM과 `r.FirstPerson.SelfShadow=1`이 설정돼 있습니다. | 광원의 `Cast Dynamic Shadows`와 수광 메시의 그림자 설정으로 차폐를 조정합니다. 특정 캐릭터만 분리 조명할 때는 광원과 메시의 `Lighting Channels`를 맞춥니다. Lighting Channels는 불투명 표면의 동적 직접광에 적용되며, 노출·Bloom·SkyLight를 분리하는 용도로는 사용할 수 없습니다. [Epic의 Lighting Channels 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/lighting-channels?application_version=5.6) |
| **Movable 환경광** | `SkyLight` 1개가 지정된 큐브맵을 사용합니다. | `Intensity Scale`, `Light Color`, 큐브맵과 하반구 색으로 움직이는 오브젝트의 환경광을 조절할 수 있습니다. 지정 큐브맵을 사용하는 현재 구성에서는 밝기·색 조정에 Real Time Capture가 필요하지 않습니다. [Epic의 Sky Light 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/sky-lights-in-unreal-engine?application_version=5.6) |
| **런타임 생성 조명 및 발광** | PIE 시작 시 플레이어 1명과 AI 5명에 Movable Point Light가 각 5개 있었습니다. Intensity는 각각 `1/1/3/3/10`이고 30개 모두 조사 순간 `Visible=false`였습니다. | 이벤트에서 켜지는 경우 `Visibility`, `Intensity`, `Light Color`, `Attenuation Radius`, 활성 시간과 그림자 옵션을 확인합니다. 광원 없이 빛나는 VFX는 머티리얼의 Emissive와 Niagara 색·밝기 쪽에서 조절합니다. |
| **Dynamic GI와 볼류메트릭 산란** | Dynamic GI는 `None`, 볼류메트릭 안개는 조사 품질에서 꺼져 있었습니다. | Dynamic GI를 별도 실험에서 켤 때 광원별 `Indirect Lighting Intensity`, Lumen의 `Diffuse Color Boost`와 `Skylight Leaking`을 검토합니다. 볼류메트릭 안개가 활성인 품질에서는 광원별 `Volumetric Scattering Intensity`로 빛줄기와 안개의 밝기를 조절합니다. [Epic의 Lumen 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-global-illumination-and-reflections-in-unreal-engine?application_version=5.6) |
| **정적 기구를 동적 광원으로 전환하는 경우** | `LightActor64–71`의 Rect/Area Light가 현재 Static입니다. | 필요한 기구만 복제 레벨에서 Movable로 시험합니다. Rect의 `88911–300000`과 Area Light의 `956.96`은 반경, 단위, 그림자 비용과 함께 다시 조정할 값입니다. |

프로젝트 기본값은 [DefaultEngine.ini](../../../Config/DefaultEngine.ini#L76), 매치 맵 등록은 [DefaultGame.ini](../../../Config/DefaultGame.ini#L158)에서 확인할 수 있습니다. 상속된 Bloom 및 Exposure 기본값은 설치된 UE 5.6의 `Scene.cpp`와 `SceneView.cpp`에서도 대조했습니다.
