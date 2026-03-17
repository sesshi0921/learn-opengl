# 📘 マテリアル（Materials）

> **目標：** 物体の素材ごとに異なるライティング特性をGLSLのstructで定義し、さまざまなマテリアルを切り替えて表示できるようになる。また、光源の強度をアンビエント・ディフューズ・スペキュラーで分離管理する方法を学ぶ。

---

## 📖 マテリアルとは何か

### 素材によって変わる光の見え方

前のチャプターでは、1つの `objectColor` と固定の係数（`ambientStrength = 0.1` など）だけでライティングを計算していました。しかし現実世界では、金属、木、プラスチック、ゴムなどの素材ごとに光の反射特性は大きく異なります。

```
   ☀️ 同じ光
    │         │         │
    ▼         ▼         ▼
┌──────┐ ┌──────┐ ┌──────┐
│ 金属 │ │木材  │ │ ゴム │
│ ✦✦✦ │ │      │ │      │
│鋭い光沢│ │やや光沢│ │光沢なし│
└──────┘ └──────┘ └──────┘
shininess  shininess  shininess
 = 128      = 16       = 4
```

マテリアルシステムでは、各素材に対して以下の4つの属性を定義します：

| 属性 | 意味 |
|------|------|
| **ambient** | 環境光をどの色で反射するか（通常は物体の色と同じ） |
| **diffuse** | 拡散光をどの色で反射するか（物体の主要な色） |
| **specular** | 鏡面反射の色（金属は素材色、非金属は白に近い） |
| **shininess** | 鏡面反射の鋭さ（小さい＝鈍い、大きい＝鋭い） |

---

## 📖 GLSLでの Material 構造体

### struct による定義

GLSLでは `struct` を使い、マテリアルのプロパティをまとめます：

```glsl
#version 330 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform Material material;
```

これにより、1つの uniform 名前空間の下にすべてのマテリアル属性がまとまります。CPU側からは `material.ambient` のようにドット記法でアクセスします。

### 前のコードからの変更点

前のチャプターのPhongシェーダーでは：

```glsl
// 前: 定数と objectColor で計算
vec3 ambient  = ambientStrength * lightColor;
vec3 diffuse  = diff * lightColor;
vec3 specular = specularStrength * spec * lightColor;
vec3 result   = (ambient + diffuse + specular) * objectColor;
```

マテリアル導入後：

```glsl
// 後: マテリアル構造体を使用
vec3 ambient  = lightColor * material.ambient;
vec3 diffuse  = lightColor * (diff * material.diffuse);
vec3 specular = lightColor * (spec * material.specular);
vec3 result   = ambient + diffuse + specular;
```

`objectColor` は不要になりました。物体の色はマテリアルの `ambient` と `diffuse` で表現されます。

---

## 📖 実際のマテリアルデータ

以下は OpenGL/DirectX で広く使われるクラシックなマテリアル値です。これらの値は **devernay.free.fr** のテーブルが有名で、LearnOpenGL でも参照されています。

### 代表的な素材テーブル

| 素材 | ambient | diffuse | specular | shininess |
|------|---------|---------|----------|-----------|
| **金 (Gold)** | (0.24725, 0.1995, 0.0745) | (0.75164, 0.60648, 0.22648) | (0.628281, 0.555802, 0.366065) | 51.2 |
| **翡翠 (Jade)** | (0.135, 0.2225, 0.1575) | (0.54, 0.89, 0.63) | (0.316228, 0.316228, 0.316228) | 12.8 |
| **ゴム (Rubber)** | (0.05, 0.05, 0.0) | (0.5, 0.5, 0.4) | (0.7, 0.7, 0.04) | 10.0 |
| **真珠 (Pearl)** | (0.25, 0.20725, 0.20725) | (1.0, 0.829, 0.829) | (0.296648, 0.296648, 0.296648) | 11.264 |
| **クロム (Chrome)** | (0.25, 0.25, 0.25) | (0.4, 0.4, 0.4) | (0.774597, 0.774597, 0.774597) | 76.8 |
| **銅 (Copper)** | (0.19125, 0.0735, 0.0225) | (0.7038, 0.27048, 0.0828) | (0.256777, 0.137622, 0.086014) | 12.8 |
| **エメラルド** | (0.0215, 0.1745, 0.0215) | (0.07568, 0.61424, 0.07568) | (0.633, 0.727811, 0.633) | 76.8 |
| **銀 (Silver)** | (0.19225, 0.19225, 0.19225) | (0.50754, 0.50754, 0.50754) | (0.508273, 0.508273, 0.508273) | 51.2 |

### 素材データの特徴

テーブルを観察すると重要なパターンが見えてきます：

- **金属**（金、銅、クロム、銀）→ specular の色が素材の色に近い
- **非金属**（翡翠、真珠、ゴム）→ specular がグレーに近い（均一な白いハイライト）
- **shininess が高い**（クロム 76.8、エメラルド 76.8）→ 表面がなめらかで鋭いハイライト
- **shininess が低い**（ゴム 10.0）→ 表面がざらついてぼんやりしたハイライト

---

## 📖 CPU側からのマテリアル設定

### uniform のドット記法

GLSL struct の各メンバーは個別に設定します：

```cpp
// 金 (Gold) マテリアルの設定
objectShader.setVec3("material.ambient",  0.24725f, 0.1995f, 0.0745f);
objectShader.setVec3("material.diffuse",  0.75164f, 0.60648f, 0.22648f);
objectShader.setVec3("material.specular", 0.628281f, 0.555802f, 0.366065f);
objectShader.setFloat("material.shininess", 51.2f);
```

### マテリアル切り替えの実装例

```cpp
struct MaterialData {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

MaterialData gold = {
    glm::vec3(0.24725f, 0.1995f, 0.0745f),
    glm::vec3(0.75164f, 0.60648f, 0.22648f),
    glm::vec3(0.628281f, 0.555802f, 0.366065f),
    51.2f
};

MaterialData jade = {
    glm::vec3(0.135f, 0.2225f, 0.1575f),
    glm::vec3(0.54f, 0.89f, 0.63f),
    glm::vec3(0.316228f, 0.316228f, 0.316228f),
    12.8f
};

// マテリアルを切り替える関数
void setMaterial(Shader& shader, const MaterialData& mat) {
    shader.setVec3("material.ambient",   mat.ambient);
    shader.setVec3("material.diffuse",   mat.diffuse);
    shader.setVec3("material.specular",  mat.specular);
    shader.setFloat("material.shininess", mat.shininess);
}

// 使用例
setMaterial(objectShader, gold);  // 金に設定
// setMaterial(objectShader, jade); // 翡翠に切り替え
```

---

## 📖 Light 構造体の導入

### なぜ光も分離するのか？

これまで `lightColor` は1つの `vec3` でした。しかし現実の光源は、環境光・拡散光・鏡面光に対して**異なる強度**を持つことがあります。例えば：

- 環境光は弱く → シーンが暗すぎないように
- 拡散光は中程度 → 主要な照明
- 鏡面光は強く → くっきりしたハイライト

### GLSL での Light 構造体

```glsl
struct Light {
    vec3 position;

    vec3 ambient;   // 環境光の強度・色
    vec3 diffuse;   // 拡散光の強度・色
    vec3 specular;  // 鏡面光の強度・色
};

uniform Light light;
```

### フラグメントシェーダーの更新

```glsl
// lightColor の代わりに light.xxx を使用
vec3 ambient  = light.ambient * material.ambient;
vec3 diffuse  = light.diffuse * (diff * material.diffuse);
vec3 specular = light.specular * (spec * material.specular);
vec3 result   = ambient + diffuse + specular;
```

### 典型的な Light の設定

```cpp
objectShader.setVec3("light.position", lightPos);
objectShader.setVec3("light.ambient",  0.2f, 0.2f, 0.2f); // 弱い環境光
objectShader.setVec3("light.diffuse",  0.5f, 0.5f, 0.5f); // 中程度の拡散光
objectShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f); // 全力のハイライト
```

```
光の強度設定:
ambient  (0.2, 0.2, 0.2) ████░░░░░░ 20%
diffuse  (0.5, 0.5, 0.5) █████░░░░░ 50%
specular (1.0, 1.0, 1.0) ██████████ 100%
```

---

## 📖 時間経過で光の色を変えるアニメーション

光の色を時間で変化させることで、マテリアルが異なる照明条件でどう見えるかを確認できます：

```cpp
// レンダリングループ内
float time = glfwGetTime();

glm::vec3 lightColor;
lightColor.x = sin(time * 2.0f);
lightColor.y = sin(time * 0.7f);
lightColor.z = sin(time * 1.3f);

// diffuse は少し暗めに
glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);
// ambient はさらに暗く
glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f);

objectShader.setVec3("light.ambient",  ambientColor);
objectShader.setVec3("light.diffuse",  diffuseColor);
objectShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);
```

`sin` は `-1.0〜1.0` の範囲を返しますが、OpenGLは負の色を0にクランプするので、色が「消えて」また「点く」ような効果が生まれます。

---

## 📖 完全なシェーダーコード

### バーテックスシェーダー（materials.vs）

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
```

### フラグメントシェーダー（materials.fs）

```glsl
#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
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
    // アンビエント
    vec3 ambient = light.ambient * material.ambient;

    // ディフューズ
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);

    // スペキュラー
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);

    // 合成
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
```

### CPU側の完全なセットアップ

```cpp
// シェーダーの使用
objectShader.use();

// マテリアルの設定（例: 金）
objectShader.setVec3("material.ambient",  0.24725f, 0.1995f, 0.0745f);
objectShader.setVec3("material.diffuse",  0.75164f, 0.60648f, 0.22648f);
objectShader.setVec3("material.specular", 0.628281f, 0.555802f, 0.366065f);
objectShader.setFloat("material.shininess", 51.2f);

// 光源の設定
objectShader.setVec3("light.position", lightPos);
objectShader.setVec3("light.ambient",  0.2f, 0.2f, 0.2f);
objectShader.setVec3("light.diffuse",  0.5f, 0.5f, 0.5f);
objectShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

// カメラ位置
objectShader.setVec3("viewPos", camera.Position);
```

---

## 💡 ポイントまとめ

| 項目 | 説明 |
|------|------|
| Material struct | ambient, diffuse, specular (vec3) + shininess (float) |
| Light struct | position, ambient, diffuse, specular (すべてvec3) |
| 金属のspecular | 素材の色に近い（金色のハイライト etc.） |
| 非金属のspecular | 白〜グレーに近い（一般的な白いハイライト） |
| ambient の一般的な値 | diffuse と同じ色、ただし弱い (例: ×0.2) |
| objectColor の廃止 | material.ambient/diffuse が物体の色を担当する |
| uniform の設定 | `material.ambient` のようにドット記法で個別設定 |
| 光の色アニメーション | sin/cos で色を変え、diffuse/ambient に反映 |
| shininess の範囲 | 素材テーブルでは 10〜77 程度が一般的 |

---

## ✏️ ドリル問題

### 問1: struct の穴埋め

以下のGLSLコードの空欄を埋めよ：

```glsl
struct Material {
    vec3 ______;     // 環境光での反射色
    vec3 ______;     // 拡散光での反射色
    vec3 ______;     // 鏡面反射の色
    float ______;    // 鏡面反射の鋭さ
};
```

<details><summary>📝 解答</summary>

```glsl
struct Material {
    vec3 ambient;     // 環境光での反射色
    vec3 diffuse;     // 拡散光での反射色
    vec3 specular;    // 鏡面反射の色
    float shininess;  // 鏡面反射の鋭さ
};
```

</details>

### 問2: 素材の特徴

以下の特徴に最も合う素材をテーブルから選べ：

1. shininess が最も高く、specular がグレーに近い金属 → ______
2. 緑がかった diffuse で shininess が低い非金属 → ______
3. specular に色がつき（黄色っぽく）、中程度の shininess → ______

<details><summary>📝 解答</summary>

1. **クロム (Chrome)** — shininess = 76.8、specular = (0.774, 0.774, 0.774) でグレー/白寄り
2. **翡翠 (Jade)** — diffuse = (0.54, 0.89, 0.63) が緑、shininess = 12.8
3. **金 (Gold)** — specular = (0.628, 0.556, 0.366) が黄色っぽい、shininess = 51.2

</details>

### 問3: uniform 設定コード

銅 (Copper) マテリアルをセットするC++コードを書け。ambient = `(0.19125, 0.0735, 0.0225)`, diffuse = `(0.7038, 0.27048, 0.0828)`, specular = `(0.256777, 0.137622, 0.086014)`, shininess = `12.8`。

<details><summary>📝 解答</summary>

```cpp
objectShader.setVec3("material.ambient",  0.19125f, 0.0735f, 0.0225f);
objectShader.setVec3("material.diffuse",  0.7038f, 0.27048f, 0.0828f);
objectShader.setVec3("material.specular", 0.256777f, 0.137622f, 0.086014f);
objectShader.setFloat("material.shininess", 12.8f);
```

</details>

### 問4: ライティング計算

Light の ambient が `(0.2, 0.2, 0.2)`、Material の ambient が `(0.25, 0.20725, 0.20725)`（真珠）のとき、最終的なアンビエント成分を計算せよ。

<details><summary>📝 解答</summary>

```
ambient = light.ambient * material.ambient
        = (0.2 × 0.25,  0.2 × 0.20725,  0.2 × 0.20725)
        = (0.05, 0.04145, 0.04145)
```

各成分ごとに乗算します。結果は非常に暗いピンクがかったグレーで、環境光としては控えめな色になります。

</details>

### 問5: light.diffuse と light.specular

Light 構造体で `diffuse` と `specular` を分離する利点を述べよ。

<details><summary>📝 解答</summary>

- **ambient**: 弱めに設定して、影が真っ暗にならない最低限の光を提供
- **diffuse**: 中程度の強度で、物体表面の基本的な陰影を表現
- **specular**: 最大強度にして、ハイライトをくっきりと表示

もし3成分を1つの `lightColor` で共有すると、ハイライトを強くしたいだけなのに環境光まで強くなってしまうなど、細かな調整ができません。分離することで各成分を独立に制御できます。

</details>

### 問6: アニメーションの仕組み

光の色アニメーションで `diffuseColor = lightColor * vec3(0.5f)` とし、さらに `ambientColor = diffuseColor * vec3(0.2f)` とする理由は？

<details><summary>📝 解答</summary>

ambient は常に diffuse よりも弱くするべきです。もし ambient が diffuse と同じ強さだと、光が当たっていない面も明るくなりすぎて立体感がなくなります。

- `diffuseColor = lightColor × 0.5` → 光の50%の強さで拡散光
- `ambientColor = diffuseColor × 0.2` → 拡散光の20%（= 光の10%）で環境光

この**段階的な減衰**により、ambient < diffuse < specular の関係が自然に保たれます。

</details>

---

## 🔨 実践課題

### 課題1: マテリアルギャラリー ⭐⭐

上記のテーブルから6種類以上のマテリアルを実装し、キー入力（1〜6キー）で切り替えられるようにする。

**手順：**
1. `MaterialData` 構造体の配列を作成
2. テーブルの値を格納
3. キーコールバックで選択中のインデックスを変更
4. レンダーループで選択中のマテリアルをシェーダーに設定

**チェックポイント：**
- [ ] 6種類以上のマテリアルが正しく表示される
- [ ] キーを押すと即座にマテリアルが切り替わる
- [ ] 金属と非金属の見た目の違いが明確にわかる
- [ ] shininess の違いがハイライトに反映されている

### 課題2: 光色アニメーション ⭐⭐

時間経過で光の色を変化させ、同じマテリアルが異なる照明でどう見えるか確認する。

**手順：**
1. `sin(time * freq)` で R/G/B を別々の周波数で変化させる
2. `diffuseColor` と `ambientColor` を導出
3. 光源の立方体の色も `lightColor` に合わせる

**チェックポイント：**
- [ ] 光の色が滑らかに変化する
- [ ] 物体の見た目が光に応じて自然に変わる
- [ ] 光源の立方体の色が実際の光の色と一致している
- [ ] ambient は常に diffuse より暗い

### 課題3: 24個の立方体 ⭐⭐⭐

devernay.free.fr のマテリアルテーブルにある全素材（24種類）をそれぞれ1つの立方体で表示するシーンを作成する。

**手順：**
1. 24種類のマテリアルデータを定義
2. 4行×6列のグリッド状に立方体を配置
3. 各立方体に異なるマテリアルを適用して描画

```cpp
for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 6; col++) {
        int index = row * 6 + col;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
            glm::vec3(col * 1.5f - 3.75f, row * 1.5f - 2.25f, 0.0f));
        setMaterial(objectShader, materials[index]);
        objectShader.setMat4("model", model);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}
```

**チェックポイント：**
- [ ] 24個の立方体がグリッド状に整列して表示される
- [ ] 各立方体の素材が視覚的に区別できる
- [ ] カメラを動かすとスペキュラーの違いが確認できる
- [ ] 金属系と非金属系の質感の違いが明確に見える

---

## 🔗 ナビゲーション

⬅️ [基本的なライティング](./02-basic-lighting.md) | ➡️ [ライティングマップ →](./04-lighting-maps.md)
