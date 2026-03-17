# キューブマップ（Cubemaps）

> **目標：** キューブマップの仕組みを理解し、スカイボックスと環境マッピング（反射・屈折）を実装できるようになる

---

## キューブマップとは

キューブマップとは、立方体の6面それぞれにテクスチャを貼り付けたものです。通常の 2D テクスチャが (u, v) の2成分でサンプリングするのに対し、キューブマップは **3D の方向ベクトル** でサンプリングします。

```
方向ベクトルによるサンプリング:

         ┌──────────┐
         │   +Y     │  (上面)
         │  (Top)   │
    ┌────┼──────────┼────┬──────────┐
    │ -X │   +Z     │ +X │   -Z     │
    │Left│ (Front)  │Right│ (Back)  │
    └────┼──────────┼────┴──────────┘
         │   -Y     │  (下面)
         │ (Bottom) │
         └──────────┘

    方向ベクトル (x, y, z) → 最も大きい成分の面を選択
    → その面上の2D座標でテクスチャをサンプリング
```

**直感的な理解:** 立方体の中心から矢印を飛ばし、当たった面の色を取得する、と考えましょう。

---

## 6面のテクスチャターゲット

OpenGL では以下の6つのターゲットに順番にテクスチャを割り当てます。

| ターゲット | 方向 | 軸 | インデックス |
|---|---|---|---|
| `GL_TEXTURE_CUBE_MAP_POSITIVE_X` | 右 | +X | 0 |
| `GL_TEXTURE_CUBE_MAP_NEGATIVE_X` | 左 | -X | 1 |
| `GL_TEXTURE_CUBE_MAP_POSITIVE_Y` | 上 | +Y | 2 |
| `GL_TEXTURE_CUBE_MAP_NEGATIVE_Y` | 下 | -Y | 3 |
| `GL_TEXTURE_CUBE_MAP_POSITIVE_Z` | 前 | +Z | 4 |
| `GL_TEXTURE_CUBE_MAP_NEGATIVE_Z` | 後 | -Z | 5 |

重要な特性として `GL_TEXTURE_CUBE_MAP_POSITIVE_X + i` でインデックス `i` の面を指定できます（連番のため）。

---

## キューブマップの読み込み

6枚の画像を順番にロードしてキューブマップを作成する関数です。

```cpp
#include <stb_image.h>
#include <vector>
#include <string>

unsigned int loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(),
                                        &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0,
                         GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cout << "Cubemap tex failed to load: "
                      << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
                    GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
                    GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
                    GL_CLAMP_TO_EDGE);

    return textureID;
}
```

呼び出し例:

```cpp
std::vector<std::string> faces {
    "right.jpg",   // +X
    "left.jpg",    // -X
    "top.jpg",     // +Y
    "bottom.jpg",  // -Y
    "front.jpg",   // +Z
    "back.jpg"     // -Z
};
unsigned int cubemapTexture = loadCubemap(faces);
```

**`GL_CLAMP_TO_EDGE` を使う理由:** 面の継ぎ目でテクスチャがリピートしてしまうのを防ぎ、面同士がシームレスにつながるようにするためです。

---

## スカイボックスの実装

スカイボックスとは、シーンを囲む巨大な立方体にキューブマップを貼ったものです。カメラがどこにいても「背景」として見えます。

### スカイボックスの頂点データ

```cpp
float skyboxVertices[] = {
    // 位置のみ（36頂点 = 6面 × 2三角形 × 3頂点）
    -1.0f,  1.0f, -1.0f,   -1.0f, -1.0f, -1.0f,    1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,    1.0f,  1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,   -1.0f, -1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,    1.0f, -1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,    1.0f,  1.0f, -1.0f,    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,    1.0f, -1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,    1.0f,  1.0f, -1.0f,    1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f,  1.0f
};
```

### スカイボックスのシェーダー

```glsl
// 頂点シェーダー
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main() {
    TexCoords = aPos;  // 頂点位置がそのまま方向ベクトル
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;  //  深度値を常に最大に
}
```

```glsl
// フラグメントシェーダー
#version 330 core
out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube skybox;

void main() {
    FragColor = texture(skybox, TexCoords);
}
```

### 3つの重要テクニック

#### 1. ビュー行列から平行移動を除去

```cpp
glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
```

**なぜ？** `mat4` を `mat3` に変換すると平行移動成分（4列目）が消えます。スカイボックスはカメラがどこに移動しても常に同じ距離に見えるべきだからです。回転だけを残すことで「無限遠」の背景を表現します。

```
mat4 のビュー行列:           mat3 にすると:
┌─────────────────┐         ┌──────────┐
│ R00 R01 R02 Tx  │         │ R00 R01 R02 │  ← 回転のみ
│ R10 R11 R12 Ty  │   →     │ R10 R11 R12 │
│ R20 R21 R22 Tz  │         │ R20 R21 R22 │
│  0   0   0   1  │         └──────────┘
└─────────────────┘         Tx, Ty, Tz が消える！
```

#### 2. 深度値を最大にするトリック

```glsl
gl_Position = pos.xyww;  // z を w で置換 → z/w = w/w = 1.0
```

パースペクティブ除算後に `z/w = w/w = 1.0` となり、深度値が常に最大値になります。これによりスカイボックスは他のすべてのオブジェクトの **奥** に描画されます。

#### 3. 深度関数の変更

```cpp
glDepthFunc(GL_LEQUAL);  // ≦ に変更（デフォルトは GL_LESS）
```

深度バッファの初期値は 1.0 で、スカイボックスの深度も 1.0 です。`GL_LESS` では `1.0 < 1.0` が false なので描画されません。`GL_LEQUAL`（≦）にすることで `1.0 <= 1.0` が true となり描画されます。

### 描画コード

```cpp
// シーンのオブジェクトを先に描画（通常の深度テスト）
glDepthFunc(GL_LESS);
drawScene();

// スカイボックスを最後に描画
glDepthFunc(GL_LEQUAL);
skyboxShader.use();
glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
skyboxShader.setMat4("view", view);
skyboxShader.setMat4("projection", projection);
glBindVertexArray(skyboxVAO);
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
glDrawArrays(GL_TRIANGLES, 0, 36);
glDepthFunc(GL_LESS); // 元に戻す
```

---

## 環境マッピング: 反射（Reflection）

オブジェクトの表面が周囲の環境を鏡のように映す効果です。

```
        視線ベクトル I        法線 N
            ↘               ↑
             ↘              │
    ──────────●──────────── 表面
             ↗
            ↗  反射ベクトル R
           ↗
    R = reflect(I, N)
```

```glsl
// 頂点シェーダー
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 Normal;
out vec3 Position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Position = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * vec4(Position, 1.0);
}
```

```glsl
// フラグメントシェーダー
#version 330 core
out vec4 FragColor;
in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
uniform samplerCube skybox;

void main() {
    vec3 I = normalize(Position - cameraPos);  // 視線ベクトル
    vec3 R = reflect(I, normalize(Normal));     // 反射ベクトル
    FragColor = vec4(texture(skybox, R).rgb, 1.0);
}
```

---

## 環境マッピング: 屈折（Refraction）

光がガラスや水のような媒質に入ると方向が変わります。これがスネルの法則に基づく屈折です。

```
    入射光 I        法線 N
        ↘           ↑
         ↘          │  媒質1（空気: η₁=1.0）
    ──────●──────────── 境界面
          ↘            媒質2（ガラス: η₂=1.52）
           ↘
            R = refract(I, N, η₁/η₂)
```

### 屈折率テーブル

| 媒質 | 屈折率 | 比率（空気→媒質） |
|---|---|---|
| 空気 | 1.00 | 1.00 |
| 水 | 1.33 | 1.00 / 1.33 = 0.75 |
| 氷 | 1.309 | 1.00 / 1.309 = 0.76 |
| ガラス | 1.52 | 1.00 / 1.52 = 0.66 |
| ダイヤモンド | 2.42 | 1.00 / 2.42 = 0.41 |

```glsl
// フラグメントシェーダー（屈折）
#version 330 core
out vec4 FragColor;
in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
uniform samplerCube skybox;

void main() {
    float ratio = 1.00 / 1.52; // 空気 → ガラス
    vec3 I = normalize(Position - cameraPos);
    vec3 R = refract(I, normalize(Normal), ratio);
    FragColor = vec4(texture(skybox, R).rgb, 1.0);
}
```

---

## 動的環境マッピング

これまでの環境マッピングは **静的** なキューブマップ（事前に用意した画像）を使っていました。**動的環境マッピング** では、オブジェクトの位置にカメラを置き、6方向にシーンをレンダリングしてキューブマップをリアルタイムに生成します。

```
動的環境マッピングの流れ:
1. 反射オブジェクトの位置にカメラを配置
2. 6方向（±X, ±Y, ±Z）に向けてシーンをFBOに描画
3. 6枚の画像をキューブマップとして構成
4. そのキューブマップで反射・屈折を計算
```

計算コストが非常に高い（シーンを6回描画）ため、以下の最適化が一般的です：
- 更新頻度を下げる（毎フレームではなく数フレームに1回）
- 解像度を下げる
- 反映するオブジェクトを限定する

---

## 💡 ポイントまとめ

| 項目 | 内容 |
|---|---|
| キューブマップ | 6面テクスチャ。3D方向ベクトルでサンプリング |
| `samplerCube` | GLSL でキューブマップをサンプリングする型 |
| `GL_CLAMP_TO_EDGE` | 面の継ぎ目のアーティファクトを防止 |
| `POSITIVE_X + i` | ターゲットが連番のため i でループ可能 |
| ビュー行列 → mat3 | 平行移動を除去して無限遠を表現 |
| `pos.xyww` | 深度を常に1.0にしてスカイボックスを最奥に |
| `GL_LEQUAL` | 深度1.0のスカイボックスを描画可能にする |
| `reflect(I, N)` | 反射ベクトルの計算 |
| `refract(I, N, η)` | 屈折ベクトルの計算（η = 屈折率比） |
| 動的環境マッピング | 6方向リアルタイム描画。コスト高 |

---

## ドリル問題

### 問1（穴埋め）
キューブマップのサンプリングには 2D テクスチャ座標ではなく、3成分の＿＿＿ベクトルを使う。

<details><summary> 解答</summary>

**方向**（direction）ベクトル

</details>

### 問2（穴埋め）
スカイボックスの頂点シェーダーで `gl_Position = pos.___` とすることで、深度値を常に最大にしている。

<details><summary> 解答</summary>

**xyww**

パースペクティブ除算で z/w = w/w = 1.0 となるため。

</details>

### 問3（選択）
ビュー行列から平行移動を除去するために `glm::mat4(glm::mat3(view))` を使う理由として正しいものはどれか？

A. スカイボックスの回転を防ぐため  
B. スカイボックスをカメラと一緒に移動させないため  
C. スカイボックスの拡大縮小を防ぐため  
D. 投影行列との乗算を簡略化するため  

<details><summary> 解答</summary>

**B**。mat3 に変換することで 4 列目の平行移動成分が消え、カメラがどこに移動してもスカイボックスは同じ位置に見えます。

</details>

### 問4（計算）
空気中からダイヤモンド（屈折率 2.42）に光が入射する場合、`refract` 関数の第3引数に渡す比率はいくつか？小数第3位まで求めよ。

<details><summary> 解答</summary>

η = 1.00 / 2.42 ≈ **0.413**

</details>

### 問5（選択）
スカイボックスの深度テストで `GL_LESS`（デフォルト）のままだと何が起こるか？

A. スカイボックスが手前のオブジェクトを隠す  
B. スカイボックスが全く描画されない  
C. 深度バッファがクリアされる  
D. スカイボックスが点滅する  

<details><summary> 解答</summary>

**B**。深度バッファの初期値が 1.0 で、スカイボックスの深度も 1.0 のため、`1.0 < 1.0` は false となり深度テストに失敗します。

</details>

### 問6（穴埋め）
反射ベクトルの計算には GLSL の組み込み関数 `___` を使い、屈折ベクトルには `___` を使う。

<details><summary> 解答</summary>

`reflect` と `refract`

</details>

---

## 実践課題

### 課題1: スカイボックスの表示 
6枚の画像を読み込み、スカイボックスを表示するプログラムを作成せよ。

**チェックポイント:**
- [ ] `loadCubemap` 関数で正しい順番で6面をロード
- [ ] ビュー行列から平行移動を除去している
- [ ] `gl_Position = pos.xyww` で深度を最大化
- [ ] `glDepthFunc(GL_LEQUAL)` を設定している
- [ ] カメラを回転させてもスカイボックスが移動しない

### 課題2: 反射マッピング 
立方体またはモデルにスカイボックスの反射を適用せよ。

**チェックポイント:**
- [ ] 法線をワールド空間に正しく変換している
- [ ] `reflect` 関数で反射ベクトルを計算
- [ ] `samplerCube` でキューブマップをサンプリング
- [ ] カメラの移動に応じて反射が変化する

### 課題3: 反射と屈折の切り替え 
キー入力でオブジェクトの反射/屈折/通常表示を切り替え、さらに屈折率をリアルタイムに変更できるようにせよ。

**チェックポイント:**
- [ ] 反射・屈折・通常の3モードを切り替え可能
- [ ] 上下キーで屈折率を 1.0〜3.0 の範囲で調整可能
- [ ] 現在の屈折率をコンソールに表示
- [ ] ダイヤモンド・水・ガラスのプリセットをキーで選択可能

### 課題4: フレネル効果の近似 
視線角度に応じて反射と屈折をブレンドするフレネル効果を実装せよ（Schlick の近似式を使用）。

**チェックポイント:**
- [ ] Schlick の近似式: `F = F0 + (1 - F0) * pow(1 - cosθ, 5)` を実装
- [ ] 正面では屈折が強く、斜めから見ると反射が強くなる
- [ ] `mix(refractColor, reflectColor, fresnelFactor)` でブレンド

---

## ナビゲーション

 [フレームバッファ](./05-framebuffers.md) | [高度なデータ操作 →](./07-advanced-data.md)
