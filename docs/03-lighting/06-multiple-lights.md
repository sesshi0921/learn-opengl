# 複数の光源（Multiple Lights）

> **目標：** 平行光源・点光源・スポットライトを1つのシーンに統合し、各光源の寄与を加算して最終的なライティングを計算するフルライティングシステムを構築できるようになる。

---

## 複数光源の考え方

現実世界と同様に、各光源の寄与を**足し合わせて**最終色を求めます（重ね合わせの原理）。

```
 太陽 → 寄与A ─┐
💡 電球1 → 寄与B ─┤
💡 電球2 → 寄与C ─┼→ 最終色 = A + B + C + D + E
💡 電球3 → 寄与D ─┤
 懐中電灯 → 寄与E┘
```

今まで学んだ `CalcDirLight`、`CalcPointLight`、`CalcSpotLight` の結果を**すべて足せばよい**のです。

### アーキテクチャ

```
main()
├── CalcDirLight(dirLight) → vec3
├── for (i < NR_POINT_LIGHTS)
│ CalcPointLight(pointLights[i]) → vec3 ×4
├── CalcSpotLight(spotLight) → vec3
└── FragColor = vec4(全部足す, 1.0)
```

---

## GLSL関数の活用

3種類の光源を1つの main() にすべて書くと膨大になるため、**光源タイプごとに計算関数を分離**します。可読性・再利用性・保守性が向上します。

```glsl
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
```

---

## 構造体の定義

```glsl
#version 330 core

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

#define NR_POINT_LIGHTS 4

uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;
uniform vec3 viewPos;
```

`#define NR_POINT_LIGHTS 4` で点光源の数を定義し、配列サイズとループ回数を一致させます。

---

## 各関数の完全な実装

### CalcDirLight

```glsl
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    return (ambient + diffuse + specular);
}
```

### CalcPointLight

```glsl
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant
                              + light.linear * distance
                              + light.quadratic * distance * distance);

    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    return (ambient + diffuse + specular) * attenuation;
}
```

### CalcSpotLight

```glsl
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant
                              + light.linear * distance
                              + light.quadratic * distance * distance);

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    return (ambient + diffuse + specular) * attenuation * intensity;
}
```

---

## main() 関数の完全実装

```glsl
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
out vec4 FragColor;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // フェーズ1: 平行光源
    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    // フェーズ2: 点光源（4つ）
    for (int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);

    // フェーズ3: スポットライト
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir);

    FragColor = vec4(result, 1.0);
}
```

---

## CPU側のuniform設定コード

### 全光源パラメータの設定

```cpp
// ===== 平行光源 =====
lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
lightingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
lightingShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
lightingShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

// ===== 点光源（ヘルパー関数でループ）=====
glm::vec3 pointLightPositions[] = {
    glm::vec3( 0.7f, 0.2f, 2.0f),
    glm::vec3( 2.3f, -3.3f, -4.0f),
    glm::vec3(-4.0f, 2.0f, -12.0f),
    glm::vec3( 0.0f, 0.0f, -3.0f)
};

for (int i = 0; i < 4; i++) {
    std::string base = "pointLights[" + std::to_string(i) + "]";
    lightingShader.setVec3(base + ".position", pointLightPositions[i]);
    lightingShader.setVec3(base + ".ambient", 0.05f, 0.05f, 0.05f);
    lightingShader.setVec3(base + ".diffuse", 0.8f, 0.8f, 0.8f);
    lightingShader.setVec3(base + ".specular", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat(base + ".constant", 1.0f);
    lightingShader.setFloat(base + ".linear", 0.09f);
    lightingShader.setFloat(base + ".quadratic", 0.032f);
}

// ===== スポットライト（カメラ懐中電灯）=====
lightingShader.setVec3("spotLight.position", camera.Position);
lightingShader.setVec3("spotLight.direction", camera.Front);
lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
lightingShader.setFloat("spotLight.constant", 1.0f);
lightingShader.setFloat("spotLight.linear", 0.09f);
lightingShader.setFloat("spotLight.quadratic", 0.032f);
lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
lightingShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
lightingShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
```

---

## シーン設定例

光の色と強度を変えるだけで、まったく異なる雰囲気を作れます。

### 砂漠（Desert）

```cpp
dirLight.ambient = vec3(0.3f, 0.24f, 0.14f);
dirLight.diffuse = vec3(0.7f, 0.42f, 0.26f); // 暖色の太陽
pointDiffuse = vec3(0.8f, 0.4f, 0.1f); // オレンジの電球
```

### 工場（Factory）

```cpp
dirLight.ambient = vec3(0.05f, 0.05f, 0.1f);
dirLight.diffuse = vec3(0.2f, 0.2f, 0.7f); // 寒色
pointDiffuse = vec3(0.5f, 0.5f, 0.5f); // 白色蛍光灯
```

### ホラー（Horror）

```cpp
dirLight.ambient = vec3(0.0f); // 環境光なし
dirLight.diffuse = vec3(0.05f); // ほぼ暗闇
pointDiffuse[0] = vec3(0.6f, 0.0f, 0.0f); // 赤い光が1つだけ
pointDiffuse[1] = vec3(0.0f); // 残りは消灯
```

### 生化学ラボ（Biochemical Lab）

```cpp
dirLight.ambient = vec3(0.05f, 0.1f, 0.05f);
dirLight.diffuse = vec3(0.1f, 0.4f, 0.1f); // 緑
pointDiffuse = vec3(0.2f, 0.8f, 0.2f); // 毒々しい緑の蛍光灯
```

```
砂漠: 暖色オレンジ 工場: 💡寒色ブルー ホラー: 赤が点滅 ラボ: 毒々しい緑
```

---

## 💡 ポイントまとめ

| 項目 | 説明 |
|------|------|
| 複数光源の原則 | 各光源の寄与を加算して最終色を求める |
| CalcDirLight | 平行光源。減衰なし、方向のみ |
| CalcPointLight | 点光源。位置 + 減衰 |
| CalcSpotLight | スポットライト。位置+方向+カットオフ+減衰 |
| NR_POINT_LIGHTS | `#define` で点光源数を定義。配列+ループ |
| uniform配列 | `pointLights[0].position` でアクセス |
| GLSL関数分離 | 可読性・再利用性・保守性が向上 |
| シーンの雰囲気 | 色と強度だけで砂漠/工場/ホラー/ラボを表現 |
| ambient加算の注意 | 複数光源のambientが加算されて明るくなりすぎに注意 |

---

## ドリル問題

### 問1: 関数名の対応

| 光源タイプ | 関数名 |
|-----------|--------|
| 平行光源 | ______ |
| 点光源 | ______ |
| スポットライト | ______ |

<details><summary> 解答</summary>

`CalcDirLight`, `CalcPointLight`, `CalcSpotLight`。各関数は `vec3` を返し、そのフラグメントへの光源の寄与を表します。

</details>

### 問2: main()の穴埋め

```glsl
void main()
{
    vec3 norm = normalize(______);
    vec3 viewDir = normalize(______ - FragPos);
    vec3 result = CalcDirLight(______, norm, viewDir);
    for (int i = 0; i < ______; i++)
        result += CalcPointLight(______[i], norm, FragPos, viewDir);
    result += CalcSpotLight(______, norm, FragPos, viewDir);
    FragColor = vec4(______, 1.0);
}
```

<details><summary> 解答</summary>

`Normal`, `viewPos`, `dirLight`, `NR_POINT_LIGHTS`, `pointLights`, `spotLight`, `result`

</details>

### 問3: uniform配列のアクセス

点光源の2番目（インデックス1）の位置を `(2.3, -3.3, -4.0)` に設定するC++コードを書け。

<details><summary> 解答</summary>

```cpp
lightingShader.setVec3("pointLights[1].position", 2.3f, -3.3f, -4.0f);
```

</details>

### 問4: 光の加算計算

各光源の寄与が以下の場合、最終色を計算せよ：

| 光源 | 寄与 |
|------|------|
| 平行光源 | (0.10, 0.10, 0.08) |
| 点光源0 | (0.15, 0.12, 0.05) |
| 点光源1 | (0.00, 0.00, 0.00) |
| 点光源2 | (0.08, 0.06, 0.03) |
| 点光源3 | (0.05, 0.04, 0.02) |
| スポットライト | (0.30, 0.28, 0.20) |

<details><summary> 解答</summary>

```
result = (0.10+0.15+0.00+0.08+0.05+0.30,
          0.10+0.12+0.00+0.06+0.04+0.28,
          0.08+0.05+0.00+0.03+0.02+0.20)
       = (0.68, 0.60, 0.38)
FragColor = vec4(0.68, 0.60, 0.38, 1.0)
```

</details>

### 問5: シーン設定

ホラーシーンの正しい設定を選べ：

1. 平行光源ambient: A)明るい白 B)ほぼゼロ C)暖色
2. 光の色: A)白 B)赤 C)緑

<details><summary> 解答</summary>

1. **B)** ほぼゼロ — 暗い環境でないと恐怖感がない
2. **B)** 赤 — 警告色・血を連想させ不安感を増幅

</details>

### 問6: パフォーマンス考察

点光源を4個から16個に増やした場合の影響と、改善策を1つ述べよ。

<details><summary> 解答</summary>

**影響:** フラグメントシェーダーのループが4倍になり、全ピクセルで計算量増大。FPS低下の可能性。

**改善策:** 各点光源の影響範囲を距離で制限し、範囲外のフラグメントは計算をスキップする。より根本的には **Deferred Rendering**（遅延レンダリング）で光源の影響範囲のみ計算する手法がある。

</details>

---

## 実践課題

### 課題1: フルライティングシステム 

太陽1 + 点光源4 + スポットライト1のフルシーンを構築する。10個の箱を配置し、全光源タイプの効果を確認する。

**チェックポイント：**
- [ ] 太陽光がシーン全体を均一に照らす
- [ ] 点光源の近くが特に明るく、離れると減衰で暗くなる
- [ ] スポットライトが円錐形に照らす
- [ ] カメラ移動でスポットライトが追従する
- [ ] 全光源の効果が正しく加算されている

### 課題2: シーン雰囲気の切り替え 

キー入力（1〜4）で砂漠・工場・ホラー・ラボの4シーンを切り替える。

**チェックポイント：**
- [ ] 砂漠: 暖色でシーン全体が明るい
- [ ] 工場: 寒色で工業的な雰囲気
- [ ] ホラー: 暗く赤い光が不気味
- [ ] ラボ: 緑の蛍光灯が毒々しい

### 課題3: 動的な点光源 

4つの点光源を sin/cos で円軌道上に周回させ、各光源に異なる色（赤・緑・青・白）を設定する。

**チェックポイント：**
- [ ] 4つの光源がシーンの周りを回転する
- [ ] 各光源の色が区別でき、重なる部分で混色する
- [ ] 光源キューブも対応位置に描画される
- [ ] フレームレートが安定している

---

## ナビゲーション

 [光源の種類](./05-light-casters.md) | [Assimp →](../04-model-loading/01-assimp.md)
