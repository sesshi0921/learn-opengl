# 📘 ライティングマップ（Lighting Maps）

> **目標：** テクスチャを使って物体の各ピクセルごとに異なるマテリアル特性を持たせる「ライティングマップ」を理解し、ディフューズマップ・スペキュラーマップ・エミッションマップを実装できるようになる。

---

## 📖 なぜライティングマップが必要か

前章では Material 構造体に `vec3 diffuse / specular` を設定して物体全体の色を決めていました。しかし現実の物体は**場所ごとに素材が異なり**ます。木製の箱なら木材部分と金属の縁で反射特性が違います：

```
┌─────────────────────┐
│  木材（面全体）       │    木材部分: 光沢なし (specular ≈ 0)
│  ┌───────────────┐  │    金属部分: 光沢あり (specular ≈ 1)
│  │ 金属の縁取り   │  │
│  └───────────────┘  │    → 1つの vec3 では表現不可能！
└─────────────────────┘    → テクスチャで解決 = ライティングマップ
```

ライティングマップとは、**テクスチャでピクセル単位のマテリアル特性を定義する手法**です：

```
従来:  material.diffuse = vec3     → 全ピクセル同じ色
改良:  material.diffuse = sampler2D → ピクセルごとに異なる色
```

---

## 📖 ディフューズマップ（Diffuse Map）

ディフューズマップは物体表面の**基本的な色**をテクスチャで定義します。テクスチャチャプターで学んだ画像テクスチャそのものです。ambient にも同じテクスチャを再利用します（ambient ≈ diffuse の色なので）。

### Material構造体の変更

```glsl
struct Material {
    sampler2D diffuse;   // vec3 → sampler2D に変更
    vec3      specular;  // まだ vec3（後で変更）
    float     shininess;
};
// ※ ambient は削除。diffuse テクスチャで代用する
```

### 頂点データにテクスチャ座標を追加

```cpp
// 頂点データ: 位置(3) + 法線(3) + テクスチャ座標(2) = 8 floats
float vertices[] = {
    // positions          // normals           // texcoords
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    // ... 残りの面も同様
};
// ストライドが 8 * sizeof(float) に変更
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
```

### バーテックスシェーダー

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model, view, projection;

void main()
{
    FragPos   = vec3(model * vec4(aPos, 1.0));
    Normal    = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
```

### フラグメントシェーダーでの使用

```glsl
vec3 diffColor = vec3(texture(material.diffuse, TexCoords));
vec3 ambient   = light.ambient * diffColor;        // ambient にも再利用
float diff     = max(dot(norm, lightDir), 0.0);
vec3 diffuse   = light.diffuse * diff * diffColor;
```

---

## 📖 テクスチャの読み込みとバインド（CPU側）

```cpp
unsigned int diffuseMap = loadTexture("container2.png");

// sampler2D にはテクスチャユニット番号を設定
lightingShader.use();
lightingShader.setInt("material.diffuse", 0);  // ユニット0

// レンダリングループ内
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, diffuseMap);
```

```
CPU → GPU の接続:
glActiveTexture(GL_TEXTURE0)  → テクスチャユニット 0 を選択
glBindTexture(diffuseMap)     → テクスチャをバインド
setInt("material.diffuse", 0) → sampler2D がユニット0を参照
```

---

## 📖 スペキュラーマップ（Specular Map）

スペキュラーマップは**各ピクセルの鏡面反射の強さ**をテクスチャで定義します。黒は光沢なし（木材）、白は最大光沢（金属）です。

### Material構造体を更新

```glsl
struct Material {
    sampler2D diffuse;    // ディフューズマップ
    sampler2D specular;   // スペキュラーマップ（vec3 → sampler2D）
    float     shininess;
};
```

### フラグメントシェーダーでの使用

```glsl
vec3 viewDir    = normalize(viewPos - FragPos);
vec3 reflectDir = reflect(-lightDir, norm);
float spec      = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
vec3 specular   = light.specular * spec * vec3(texture(material.specular, TexCoords));
// 黒(0,0,0) → 反射ゼロ  /  白(1,1,1) → フル反射
```

### CPU側

```cpp
unsigned int specularMap = loadTexture("container2_specular.png");
lightingShader.setInt("material.specular", 1);  // ユニット1

glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, specularMap);
```

---

## 📖 エミッションマップ（Emission Map）

光源からの照明とは無関係に**物体自身が発光する**部分を定義するテクスチャです。ネオンサインや溶岩の模様に使われます。黒は発光なし、色付きはその色で発光します。

```glsl
struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emission;   // 自己発光マップ
    float     shininess;
};
```

シェーダーでは最後に単純加算します：

```glsl
vec3 emission = vec3(texture(material.emission, TexCoords));
vec3 result   = ambient + diffuse + specular + emission;
```

---

## 📖 完全なフラグメントシェーダー

```glsl
#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emission;
    float     shininess;
};
struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Material material;
uniform Light light;
uniform vec3 viewPos;

void main()
{
    vec3 diffColor  = vec3(texture(material.diffuse,  TexCoords));
    vec3 specColor  = vec3(texture(material.specular, TexCoords));
    vec3 emitColor  = vec3(texture(material.emission, TexCoords));

    // アンビエント
    vec3 ambient = light.ambient * diffColor;
    // ディフューズ
    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = light.diffuse * diff * diffColor;
    // スペキュラー
    vec3 viewDir    = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular   = light.specular * spec * specColor;

    FragColor = vec4(ambient + diffuse + specular + emitColor, 1.0);
}
```

### CPU側の完全セットアップ

```cpp
unsigned int diffuseMap  = loadTexture("container2.png");
unsigned int specularMap = loadTexture("container2_specular.png");
unsigned int emissionMap = loadTexture("matrix.jpg");

lightingShader.use();
lightingShader.setInt("material.diffuse",  0);
lightingShader.setInt("material.specular", 1);
lightingShader.setInt("material.emission", 2);
lightingShader.setFloat("material.shininess", 64.0f);

// レンダリングループ内
lightingShader.setVec3("light.position", lightPos);
lightingShader.setVec3("light.ambient",  0.2f, 0.2f, 0.2f);
lightingShader.setVec3("light.diffuse",  0.5f, 0.5f, 0.5f);
lightingShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, diffuseMap);
glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, specularMap);
glActiveTexture(GL_TEXTURE2);
glBindTexture(GL_TEXTURE_2D, emissionMap);
```

---

## 💡 ポイントまとめ

| 項目 | 説明 |
|------|------|
| ディフューズマップ | テクスチャで位置ごとの拡散色を定義。ambient にも再利用 |
| スペキュラーマップ | 位置ごとの鏡面反射の強さ。黒=光沢なし、白=最大光沢 |
| エミッションマップ | 光源に無関係な自己発光。最終結果に加算 |
| Material struct | `sampler2D diffuse/specular/emission` + `float shininess` |
| sampler2D への設定 | `setInt("material.diffuse", 0)` でテクスチャユニット番号を渡す |
| バインド手順 | `glActiveTexture` → `glBindTexture`（setIntは初回のみ） |
| 頂点属性の変更 | UV座標を location=2 で追加、ストライド 8 floats |

---

## ✏️ ドリル問題

### 問1: Material構造体の穴埋め

```glsl
struct Material {
    ______ diffuse;     // ディフューズマップ
    ______ specular;    // スペキュラーマップ
    ______ emission;    // エミッションマップ
    ______ shininess;   // 鏡面反射の鋭さ
};
```

<details><summary>📝 解答</summary>

```glsl
sampler2D diffuse;
sampler2D specular;
sampler2D emission;
float     shininess;
```
テクスチャマップは `sampler2D`、shininess はスカラーなので `float`。

</details>

### 問2: テクスチャバインドの手順

```cpp
______(GL_TEXTURE0);                // (1)
______(GL_TEXTURE_2D, diffuseMap);  // (2)
lightingShader.______(______);      // (3)
```

<details><summary>📝 解答</summary>

```cpp
glActiveTexture(GL_TEXTURE0);                  // (1)
glBindTexture(GL_TEXTURE_2D, diffuseMap);      // (2)
lightingShader.setInt("material.diffuse", 0);  // (3)
```

</details>

### 問3: マップの使い分け

適切なマップを答えよ（diffuse / specular / emission）：

1. 箱の木目の色をピクセルごとに定義 → ______
2. 金属の縁だけ光沢あり、木は光沢なし → ______
3. ネオン文字を光源なしでも光らせる → ______

<details><summary>📝 解答</summary>

1. **diffuse** — 拡散色を位置ごとに定義
2. **specular** — 鏡面反射の強さを位置ごとに定義
3. **emission** — 光源に無関係な自己発光

</details>

### 問4: シェーダーコードの穴埋め

```glsl
vec3 ambient = light.ambient * vec3(______(material.______, ______));
float diff   = max(dot(norm, lightDir), 0.0);
vec3 diffuse = light.diffuse * diff * vec3(______(material.______, ______));
```

<details><summary>📝 解答</summary>

```glsl
vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
```
`texture()` に sampler2D とテクスチャ座標を渡してサンプリングします。

</details>

### 問5: 選択問題 — スペキュラーマップの白黒

スペキュラーマップで「黒い部分」と「白い部分」の正しい説明は？

- A) 黒=最大反射、白=反射なし
- B) 黒=反射なし、白=最大反射
- C) 黒=影、白=光が当たっている

<details><summary>📝 解答</summary>

**B)** `spec * vec3(0,0,0) = 0`（反射ゼロ）、`spec * vec3(1,1,1)`（最大反射）。

</details>

### 問6: エミッションの加算

エミッションを最終結果に**加算**する理由を述べよ。

<details><summary>📝 解答</summary>

エミッションは外部光源に依存しない「自ら放つ光」です。照明計算とは独立した寄与なので**足す**ことで、暗い環境でも発光部分が明るく見えます。乗算だと周囲が暗いとき発光も消えてしまいます。

</details>

---

## 🔨 実践課題

### 課題1: ディフューズ＋スペキュラーマップの箱 ⭐⭐

`container2.png`（ディフューズ）と `container2_specular.png`（スペキュラー）を使い、木材は光沢なし・金属の縁だけ鏡面反射する箱を実装する。

**チェックポイント：**
- [ ] 箱の木目が正しく表示される
- [ ] 金属の縁にだけ鏡面反射が現れる
- [ ] カメラを動かすとハイライトが移動する

### 課題2: エミッションマップの追加 ⭐⭐

上記の箱にエミッションマップを追加し、光源を暗くしても特定部分が発光することを確認する。

**チェックポイント：**
- [ ] エミッション部分が照明に無関係に光る
- [ ] 照明を暗くしてもエミッション部分は明るいまま
- [ ] エミッションがない部分は通常のライティング

### 課題3: 反転スペキュラーマップ ⭐⭐⭐

シェーダーでスペキュラーマップの値を `vec3(1.0) - specColor` で反転し、木材が光沢を持ち金属が持たない不思議な箱を作成。キー入力で通常/反転を切り替えて視覚的にスペキュラーマップの効果を体感する。

**チェックポイント：**
- [ ] 反転時に木の部分が光沢を持つ
- [ ] 通常版と反転版を切り替えて比較できる
- [ ] スペキュラーマップの仕組みを体感できる

---

## 🔗 ナビゲーション

⬅️ [マテリアル](./03-materials.md) | ➡️ [光源の種類 →](./05-light-casters.md)
