# 入門編 6：テクスチャ（Textures）

> **目標：** テクスチャを読み込んでオブジェクトに貼り付け、UV マッピングを理解する

---

## テクスチャとは

テクスチャは **画像データを GPU に転送して 3D オブジェクトの表面に貼り付ける** 技術です。

```
画像ファイル（PNG/JPG）
      ↓  stb_image で読み込み
   CPU メモリ（pixel data）
      ↓  glTexImage2D で転送
   GPU テクスチャオブジェクト
      ↓  シェーダーでサンプリング
   フラグメントの色
```

---

## UV 座標（テクスチャ座標）

テクスチャ座標は **0.0〜1.0 の範囲** で表します。

```
テクスチャ座標系

(0,1)──────(1,1)
  │  画像   │
  │         │
(0,0)──────(1,0)
 ↑ (s=0,t=0) が左下（OpenGL の場合）
```

頂点にテクスチャ座標（UV/ST）を追加します：

```cpp
float vertices[] = {
    // 位置               // UV
     0.5f,  0.5f, 0.0f,  1.0f, 1.0f,  // 右上
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f,  // 右下
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,  // 左下
    -0.5f,  0.5f, 0.0f,  0.0f, 1.0f   // 左上
};
```

---

## テクスチャの折り返し（Wrapping）

UV 座標が 0〜1 の範囲を超えたときの動作を設定します。

| 設定値 | 動作 |
|--------|------|
| `GL_REPEAT` | タイル状に繰り返す（デフォルト） |
| `GL_MIRRORED_REPEAT` | 鏡反転しながら繰り返す |
| `GL_CLAMP_TO_EDGE` | 端のピクセルを引き伸ばす |
| `GL_CLAMP_TO_BORDER` | 指定した境界色を表示 |

```cpp
// S 軸（水平）の折り返し設定
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
// T 軸（垂直）の折り返し設定
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
```

---

## テクスチャフィルタリング

テクスチャを拡大・縮小したときのピクセルのサンプリング方法です。

| 設定値 | 動作 | 見た目 |
|--------|------|--------|
| `GL_NEAREST` | 最も近いピクセルを使う | ピクセルアート風（ドット感あり） |
| `GL_LINEAR` | 周囲のピクセルを補間 | 滑らか（ぼやける） |

```cpp
// 縮小時（Minification）
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
// 拡大時（Magnification）
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
```

---

## ミップマップ（Mipmaps）

遠くのオブジェクトに高解像度テクスチャを使うと、**エイリアシング（モアレ）** が発生します。ミップマップはこれを防ぐために、**あらかじめ縮小版のテクスチャを用意** しておきます。

```
オリジナル: 512×512
  ↓ 1/2
ミップ 1:   256×256
  ↓ 1/2
ミップ 2:   128×128
  ↓ 1/2
    ...
```

```cpp
// ミップマップを自動生成
glGenerateMipmap(GL_TEXTURE_2D);
```

### ミップマップフィルタリングオプション

| 設定値 | 動作 |
|--------|------|
| `GL_NEAREST_MIPMAP_NEAREST` | ミップレベル選択：最近傍、サンプリング：最近傍 |
| `GL_LINEAR_MIPMAP_NEAREST` | ミップレベル選択：補間、サンプリング：最近傍 |
| `GL_NEAREST_MIPMAP_LINEAR` | ミップレベル選択：最近傍、サンプリング：線形 |
| `GL_LINEAR_MIPMAP_LINEAR` | ミップレベル選択：補間、サンプリング：線形（最高品質） |

---

## テクスチャの読み込み（stb_image）

```cpp
// main.cpp の先頭（.cpp ファイルでのみ定義する）
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// テクスチャ読み込み
int width, height, nrChannels;
stbi_set_flip_vertically_on_load(true);  // OpenGL は Y 軸が逆なので反転が必要
unsigned char* data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
```

---

## テクスチャオブジェクトの作成

```cpp
// テクスチャオブジェクト生成
unsigned int texture;
glGenTextures(1, &texture);

// バインド
glBindTexture(GL_TEXTURE_2D, texture);

// 折り返し設定
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

// フィルタリング設定
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

// 画像を読み込んで GPU に転送
if (data) {
    glTexImage2D(
        GL_TEXTURE_2D,      // テクスチャのターゲット
        0,                   // ミップマップレベル（0 = ベース）
        GL_RGB,              // GPU で使う内部フォーマット
        width, height,       // テクスチャのサイズ
        0,                   // 常に 0（レガシー）
        GL_RGB,              // 入力データのフォーマット
        GL_UNSIGNED_BYTE,    // 入力データの型
        data                 // 画像データ
    );
    glGenerateMipmap(GL_TEXTURE_2D);  // ミップマップ生成
} else {
    std::cerr << "テクスチャ読み込み失敗" << std::endl;
}

// 画像データは CPU では不要になった
stbi_image_free(data);
```

---

## シェーダーでテクスチャを使う

```glsl
// 頂点シェーダー
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
```

```glsl
// フラグメントシェーダー
#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D ourTexture;  // テクスチャサンプラー

void main() {
    FragColor = texture(ourTexture, TexCoord);
}
```

```cpp
// CPU 側：テクスチャユニットをバインド
glActiveTexture(GL_TEXTURE0);   // テクスチャユニット 0 を選択
glBindTexture(GL_TEXTURE_2D, texture);

// サンプラーにユニット番号を設定
glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);
```

---

## 複数テクスチャのミックス

```glsl
// フラグメントシェーダー（2 つのテクスチャをブレンド）
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform float mixValue;

void main() {
    // mix(a, b, t) = a * (1-t) + b * t
    FragColor = mix(
        texture(texture1, TexCoord),
        texture(texture2, TexCoord),
        mixValue  // 0=texture1のみ、1=texture2のみ
    );
}
```

```cpp
// テクスチャユニット 0 と 1 を使う
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, texture1);
glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, texture2);

shader.setInt("texture1", 0);
shader.setInt("texture2", 1);
```

---

## 💡 ポイントまとめ

| 概念 | 説明 |
|------|------|
| UV 座標 | テクスチャ上の位置（0.0〜1.0） |
| Wrapping | UV が範囲外の時の処理 |
| Filtering | テクスチャの拡大縮小時のピクセル補間 |
| Mipmaps | 縮小テクスチャの事前生成 |
| sampler2D | GLSL でテクスチャをサンプリングする型 |
| texture() | GLSL のテクスチャサンプリング関数 |
| テクスチャユニット | 複数テクスチャを同時に使うためのスロット |

---

## ドリル問題

### 問題 1：穴埋め（テクスチャ設定）

```cpp
unsigned int texture;
glGenTextures(【 ① 】, &texture);
glBindTexture(【 ② 】, texture);

// 繰り返し設定
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 【 ③ 】);

// ミップマップ付き線形フィルタリング
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    GL_LINEAR_MIPMAP_LINEAR);

// 読み込み後
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
【 ④ 】(GL_TEXTURE_2D);  // ミップマップ生成
```

<details>
<summary> 解答</summary>

① `1`  
② `GL_TEXTURE_2D`  
③ `GL_REPEAT`  
④ `glGenerateMipmap`

</details>

---

### 問題 2：UV 座標の設定

四角形の 4 頂点に対して、テクスチャがちょうど 1 枚収まる UV 座標を答えなさい。

| 頂点位置 | U（S） | V（T） |
|---------|--------|--------|
| 右上 | 【①】 | 【②】 |
| 右下 | 【③】 | 【④】 |
| 左下 | 【⑤】 | 【⑥】 |
| 左上 | 【⑦】 | 【⑧】 |

<details>
<summary> 解答</summary>

| 位置 | U | V |
|------|---|---|
| 右上 | 1.0 | 1.0 |
| 右下 | 1.0 | 0.0 |
| 左下 | 0.0 | 0.0 |
| 左上 | 0.0 | 1.0 |

</details>

---

### 問題 3：Wrapping モードを選ぶ

以下の状況に最適な Wrapping モードはどれか。

1. 煉瓦テクスチャを壁全体にタイル状に張りたい
2. 空のテクスチャで、端が見切れないようにしたい
3. 床に市松模様のテクスチャを張りたい

<details>
<summary> 解答</summary>

1. `GL_REPEAT`（タイル状の繰り返し）
2. `GL_CLAMP_TO_EDGE`（端を引き伸ばす）
3. `GL_REPEAT`（タイル繰り返し）

</details>

---

### 問題 4：複数テクスチャ

```cpp
// 以下のコードで何が設定されるか説明しなさい
glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, texture2);
shader.setInt("texture2", 1);
```

<details>
<summary> 解答</summary>

1. テクスチャユニット **1** をアクティブに設定
2. `texture2` をテクスチャユニット 1 にバインド
3. シェーダーのユニフォーム `texture2` にテクスチャユニット **1** を割り当てる

</details>

---

### 問題 5：`stbi_set_flip_vertically_on_load(true)` はなぜ必要か？

<details>
<summary> 解答</summary>

画像ファイルは一般的に **左上** を原点として格納されているが、OpenGL のテクスチャ座標は **左下** が原点のため、そのまま読み込むと上下が逆になる。`stbi_set_flip_vertically_on_load(true)` を呼ぶことで読み込み時に垂直方向を反転する。

</details>

---

## 実践課題

### 課題 1：テクスチャを貼る 

任意の JPG/PNG 画像を読み込んで、四角形に貼り付けなさい。

**チェックポイント：**
- [ ] stb_image で画像が読み込める
- [ ] テクスチャ座標（UV）が頂点データに含まれている
- [ ] シェーダーで `sampler2D` が使われている
- [ ] 正しい向きで表示される

### 課題 2：タイリング 

UV 座標を `0〜2` の範囲に変更して、テクスチャが 2×2 でタイル状に表示されるようにしなさい。

### 課題 3：2 枚のテクスチャをブレンド 

2 枚のテクスチャをブレンドし、キーボードの `↑` `↓` で混合比を変化させなさい。

```cpp
// ヒント
if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    mixValue = std::min(mixValue + 0.01f, 1.0f);
if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    mixValue = std::max(mixValue - 0.01f, 0.0f);
```

---

## ナビゲーション

 [シェーダー](./05-shaders.md) | [変換 →](./07-transformations.md)
