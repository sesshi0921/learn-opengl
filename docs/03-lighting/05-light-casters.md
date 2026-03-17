# 📘 光源の種類（Light Casters）

> **目標：** 平行光源（Directional Light）、点光源（Point Light）、スポットライト（Spotlight）の3種類の光源をGLSLで実装し、距離減衰やスポットライトのカットオフ角を理解・実装できるようになる。

---

## 📖 3種類の光源の概要

```
1. 平行光源           2. 点光源            3. スポットライト
    ↓ ↓ ↓ ↓ ↓         ─╲ │ ╱─             │
    ↓ ↓ ↓ ↓ ↓        ─── ☀ ───           ╲ │ ╱
    ↓ ↓ ↓ ↓ ↓         ─╱ │ ╲─             ▽
太陽のように遠い    電球のように全方向    懐中電灯のように円錐
方向のみ           位置+全方向+減衰     位置+方向+角度+減衰
```

| 光源タイプ | 位置 | 方向 | 減衰 | カットオフ | 例 |
|-----------|------|------|------|-----------|-----|
| **平行光源** | なし | あり | なし | なし | 太陽 |
| **点光源** | あり | 全方向 | あり | なし | 電球 |
| **スポットライト** | あり | あり | あり | あり | 懐中電灯 |

---

## 📖 平行光源（Directional Light）

太陽のように非常に遠い光源は、すべての光線がほぼ**平行**に降り注ぎます。位置ではなく**方向のみ**で定義します。

```
点光源:  各フラグメントへの方向が異なる     平行光源:  全フラグメントに同じ方向
      ☀                                    ↓ ↓ ↓ ↓ ↓
     ╱│╲                                   ↓ ↓ ↓ ↓ ↓
    A  B  C                                A  B  C
```

### GLSL構造体と計算関数

```glsl
struct DirLight {
    vec3 direction;  // 光の方向（位置ではない！）
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction); // 符号反転！
    float diff    = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec    = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    return (ambient + diffuse + specular);
}
```

なぜ `-direction` か？ `direction` は「光が進む方向」（上→下）で定義しますが、ライティング計算では「フラグメントから光源への方向」（下→上）が必要だからです。

### CPU側の設定

```cpp
lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
lightingShader.setVec3("dirLight.ambient",    0.05f, 0.05f, 0.05f);
lightingShader.setVec3("dirLight.diffuse",    0.4f,  0.4f,  0.4f);
lightingShader.setVec3("dirLight.specular",   0.5f,  0.5f,  0.5f);
```

---

## 📖 点光源（Point Light）と減衰

点光源は空間上の一点から全方向に光を放ちます。重要な特徴は**距離が離れるほど光が弱くなる**（減衰）ことです。

### 減衰（Attenuation）の公式

$$F_{att} = \frac{1.0}{K_c + K_l \cdot d + K_q \cdot d^2}$$

| 係数 | 名前 | 意味 |
|------|------|------|
| $K_c$ | constant | 通常 1.0。分母が1未満にならない保証 |
| $K_l$ | linear | 距離に比例。中距離の減衰を制御 |
| $K_q$ | quadratic | 距離の二乗に比例。遠距離の急激な減衰 |

### 減衰係数テーブル（推奨値）

| 距離 | constant | linear | quadratic |
|------|----------|--------|-----------|
| 7 | 1.0 | 0.7 | 1.8 |
| 13 | 1.0 | 0.35 | 0.44 |
| 20 | 1.0 | 0.22 | 0.20 |
| 32 | 1.0 | 0.14 | 0.07 |
| 50 | 1.0 | 0.09 | 0.032 |
| 65 | 1.0 | 0.07 | 0.017 |
| 100 | 1.0 | 0.045 | 0.0075 |
| 160 | 1.0 | 0.027 | 0.0028 |
| 200 | 1.0 | 0.022 | 0.0019 |
| 325 | 1.0 | 0.014 | 0.0007 |
| 600 | 1.0 | 0.007 | 0.0002 |

### GLSL構造体と計算関数

```glsl
struct PointLight {
    vec3  position;
    float constant;
    float linear;
    float quadratic;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
};

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff    = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec    = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    // 減衰
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant
                              + light.linear * distance
                              + light.quadratic * distance * distance);

    vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    return (ambient + diffuse + specular) * attenuation;
}
```

---

## 📖 スポットライト（Spotlight）

スポットライトはある一点から**特定の方向にだけ**光を放つ円錐形の光源です。

```
     [光源] position
       │ direction
     ╱─┼─╲ ← cutOff (内側)
   ╱───┼───╲ ← outerCutOff (外側)
  照らされる範囲
```

### なぜ cosine 値で管理するのか？

`dot(lightDir, -direction)` の結果は cosine 値です。最初から cosine で持てば毎フレームの `cos()` 変換が不要で、直接比較するだけで済みます。

```
cos は角度が大きくなると小さくなる:
cos(12.5°) = 0.9763  (内側: 明るい)
cos(17.5°) = 0.9537  (外側: フェードアウト)
cos(θ) > cos(cutOff) → 内側 → 照らす
```

### ソフトエッジスポットライト

ハードエッジだと円錐の境界がくっきりして不自然なので、内側と外側のカットオフ角で**段階的にフェードアウト**させます：

$$\text{intensity} = \text{clamp}\left(\frac{\theta - \gamma}{\phi - \gamma}, 0, 1\right)$$

$\theta$: フラグメントのcos値、$\phi$: 内側cutOffのcos値、$\gamma$: 外側outerCutOffのcos値

### GLSL構造体と計算関数

```glsl
struct SpotLight {
    vec3  position;
    vec3  direction;
    float cutOff;       // cos(12.5°) = 0.9763
    float outerCutOff;  // cos(17.5°) = 0.9537
    float constant;
    float linear;
    float quadratic;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
};

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff    = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec    = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    // 減衰
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant
                              + light.linear * distance
                              + light.quadratic * distance * distance);
    // スポットライト強度（ソフトエッジ）
    float theta     = dot(lightDir, normalize(-light.direction));
    float epsilon   = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    return (ambient + diffuse + specular) * attenuation * intensity;
}
```

---

## 📖 懐中電灯の実装

カメラの位置と方向をスポットライトに設定すれば、プレイヤーの懐中電灯になります：

```cpp
lightingShader.setVec3("spotLight.position",  camera.Position);
lightingShader.setVec3("spotLight.direction", camera.Front);
lightingShader.setFloat("spotLight.cutOff",      glm::cos(glm::radians(12.5f)));
lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
lightingShader.setFloat("spotLight.constant",  1.0f);
lightingShader.setFloat("spotLight.linear",    0.09f);
lightingShader.setFloat("spotLight.quadratic", 0.032f);
lightingShader.setVec3("spotLight.ambient",  0.0f, 0.0f, 0.0f);
lightingShader.setVec3("spotLight.diffuse",  1.0f, 1.0f, 1.0f);
lightingShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
```

---

## 📖 3種類の光源の比較

```
┌─────────────┬──────────────┬──────────────┬─────────────────────┐
│             │ 平行光源      │ 点光源        │ スポットライト       │
├─────────────┼──────────────┼──────────────┼─────────────────────┤
│ 位置        │ なし          │ あり          │ あり                │
│ 方向        │ あり(固定)    │ なし(全方向)  │ あり(指向性)        │
│ 減衰        │ なし          │ あり          │ あり                │
│ カットオフ  │ なし          │ なし          │ あり(内+外)         │
│ 計算コスト  │ 低い          │ 中程度        │ 高い                │
│ 使用例      │ 太陽・月      │ 電球・ランタン│ 懐中電灯・舞台照明  │
└─────────────┴──────────────┴──────────────┴─────────────────────┘
```

---

## 💡 ポイントまとめ

| 項目 | 説明 |
|------|------|
| 平行光源 | 方向のみ。`lightDir = normalize(-direction)`。減衰なし |
| 点光源 | 位置から全方向。距離による減衰あり |
| スポットライト | 位置+方向+カットオフ角。円錐形の照明 |
| 減衰公式 | $1.0 / (K_c + K_l \cdot d + K_q \cdot d^2)$ |
| constant | 通常 1.0。分母≥1 の保証 |
| linear / quadratic | 中距離/遠距離の減衰制御 |
| cutOff / outerCutOff | cos値で管理。dot結果と直接比較可能 |
| intensity | `clamp((θ - outer) / (inner - outer), 0, 1)` |
| 懐中電灯 | カメラの position/front をスポットライトに設定 |

---

## ✏️ ドリル問題

### 問1: 構造体の穴埋め

```glsl
struct DirLight {
    vec3 ______;  // 光の方向
    vec3 ______;  // 環境光
    vec3 ______;  // 拡散光
    vec3 ______;  // 鏡面光
};
```

<details><summary>📝 解答</summary>

`direction`, `ambient`, `diffuse`, `specular`。平行光源は position を持たず direction のみ。

</details>

### 問2: 減衰の計算

constant=1.0, linear=0.09, quadratic=0.032 のとき、距離 d=50 での $F_{att}$ を計算せよ。

<details><summary>📝 解答</summary>

$$F_{att} = \frac{1.0}{1.0 + 0.09 \times 50 + 0.032 \times 2500} = \frac{1.0}{1.0 + 4.5 + 80.0} = \frac{1.0}{85.5} \approx 0.0117$$

距離50で約1.2%まで減衰。quadratic項(80.0)が支配的です。

</details>

### 問3: スポットライトの角度計算

cutOff=cos(12.5°)=0.9763, outerCutOff=cos(17.5°)=0.9537 で、フラグメントが15°の角度にある場合の intensity は？

<details><summary>📝 解答</summary>

```
θ = cos(15°) = 0.9659
epsilon = 0.9763 - 0.9537 = 0.0226
intensity = (0.9659 - 0.9537) / 0.0226 = 0.5398
```

15°は内側(12.5°)と外側(17.5°)の中間付近で、約54%の強度。

</details>

### 問4: lightDir の符号

`direction = (0, -1, 0)`（上→下）のとき、`lightDir` を正しく計算するコードは？

<details><summary>📝 解答</summary>

```glsl
vec3 lightDir = normalize(-dirLight.direction); // = (0, 1, 0) 上向き
```
ライティング計算ではフラグメントから光源への方向が必要なので符号を反転します。

</details>

### 問5: 選択問題 — 減衰係数

正しい説明を**すべて**選べ：

- A) constant を 0 にすると距離0で無限の明るさになる
- B) linear は距離の二乗に比例する
- C) quadratic は遠距離で急激に減衰させる

<details><summary>📝 解答</summary>

**A) と C)** が正しい。B は誤り（linear は距離に比例する一次項、二乗は quadratic）。

</details>

### 問6: 懐中電灯の設定

```cpp
lightingShader.setVec3("spotLight.______", camera.Position);
lightingShader.setVec3("spotLight.______", camera.Front);
lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(______)));
```

<details><summary>📝 解答</summary>

```cpp
lightingShader.setVec3("spotLight.position",  camera.Position);
lightingShader.setVec3("spotLight.direction", camera.Front);
lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
```

</details>

---

## 🔨 実践課題

### 課題1: 平行光源のシーン ⭐⭐

10個の箱を配置し、`DirLight` と `CalcDirLight` で太陽に照らされたシーンを構築する。

**チェックポイント：**
- [ ] すべての箱が同じ方向から照らされている
- [ ] 距離が離れても光の強さが変わらない（減衰なし）
- [ ] ディフューズ/スペキュラーマップが正しく動作する

### 課題2: 減衰する点光源 ⭐⭐

点光源を1つ配置し、減衰テーブルの値を変えて効果の違いを体験する。

**チェックポイント：**
- [ ] 近い箱ほど明るく、遠い箱ほど暗い
- [ ] quadratic を大きくすると遠くの箱が急に暗くなる
- [ ] linear を大きくすると中距離の減衰が強まる

### 課題3: 懐中電灯スポットライト ⭐⭐⭐

カメラ追従のソフトエッジスポットライトを暗い環境(ambient=0)で実装する。

**チェックポイント：**
- [ ] 画面中央付近だけが円形に照らされる
- [ ] カメラ移動で照明範囲が追従する
- [ ] 照明の端がソフトにフェードアウトする
- [ ] 近い物体ほど明るい（減衰が機能する）

---

## 🔗 ナビゲーション

⬅️ [ライティングマップ](./04-lighting-maps.md) | ➡️ [複数の光源 →](./06-multiple-lights.md)
