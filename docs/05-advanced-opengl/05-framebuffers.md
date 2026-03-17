# 📘 フレームバッファ（Framebuffers）

> **目標：** フレームバッファオブジェクト（FBO）を理解し、オフスクリーンレンダリングとポストプロセスエフェクトを実装できるようになる

---

## 📖 フレームバッファとは

これまで私たちが描画してきた先は、実は **デフォルトフレームバッファ** と呼ばれるものです。ウィンドウを作成すると GLFW が自動で用意してくれるバッファで、カラー・深度・ステンシルの各バッファが含まれています。

OpenGL では、自分で **カスタムフレームバッファオブジェクト（FBO）** を作成し、画面ではなくテクスチャに描画できます。これにより「シーンを一度テクスチャに描き、そのテクスチャに加工を施してから画面に表示する」という **ポストプロセス** が可能になります。

```
┌─────────────────────────────────────────────────┐
│              レンダリングパイプライン               │
│                                                   │
│  シーン描画 ──→ カスタムFBO(テクスチャ) ──→ 加工   │
│                                          │        │
│                     画面表示 ←────────────┘        │
│                  (デフォルトFBO)                    │
└─────────────────────────────────────────────────┘
```

**なぜ FBO を使うのか？**
- ポストプロセスエフェクト（ブラー、グレースケール等）
- シャドウマッピング（深度マップの生成）
- 反射・屈折の表現
- マルチパスレンダリング全般

---

## 📖 FBO の構成要素

フレームバッファは複数の **アタッチメント** から構成されます。

```
┌──────────────────────────────────┐
│       フレームバッファ (FBO)       │
│                                  │
│  ┌────────────────────────────┐  │
│  │ カラーアタッチメント (0..n)  │  │  ← 色情報を格納
│  └────────────────────────────┘  │
│  ┌────────────────────────────┐  │
│  │ 深度アタッチメント          │  │  ← 深度テスト用
│  └────────────────────────────┘  │
│  ┌────────────────────────────┐  │
│  │ ステンシルアタッチメント     │  │  ← ステンシルテスト用
│  └────────────────────────────┘  │
└──────────────────────────────────┘
```

各アタッチメントには **テクスチャ** または **レンダーバッファオブジェクト（RBO）** を紐づけます。

| アタッチメント種別 | 用途 | 推奨形式 |
|---|---|---|
| `GL_COLOR_ATTACHMENT0` | 色情報の書き込み先 | テクスチャ |
| `GL_DEPTH_ATTACHMENT` | 深度値の書き込み先 | RBO（読み取り不要時） |
| `GL_STENCIL_ATTACHMENT` | ステンシル値 | RBO |
| `GL_DEPTH_STENCIL_ATTACHMENT` | 深度+ステンシル兼用 | RBO |

---

## 📖 FBO の作成と設定

### ステップ1: FBO の生成

```cpp
unsigned int framebuffer;
glGenFramebuffers(1, &framebuffer);
glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
```

### ステップ2: カラーアタッチメント用テクスチャの作成

シーンの色情報を書き込む先として、空のテクスチャを作ります。

```cpp
unsigned int textureColorbuffer;
glGenTextures(1, &textureColorbuffer);
glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
// 空のテクスチャ（dataにNULLを渡す）
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT,
             0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

// FBO にテクスチャをアタッチ
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                       GL_TEXTURE_2D, textureColorbuffer, 0);
```

**ポイント:** `glTexImage2D` の最後の引数を `NULL` にすることで、メモリだけ確保しデータは空のままにします。GPU がここにレンダリング結果を書き込みます。

### ステップ3: レンダーバッファオブジェクト（RBO）

深度・ステンシル用にはテクスチャではなく RBO が効率的です。RBO は GPU が直接読み書きする最適化されたバッファで、テクスチャとして後からサンプリングする必要がない場合に適しています。

```cpp
unsigned int rbo;
glGenRenderbuffers(1, &rbo);
glBindRenderbuffer(GL_RENDERBUFFER, rbo);
glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                      SCR_WIDTH, SCR_HEIGHT);

// FBO に RBO をアタッチ
glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                          GL_RENDERBUFFER, rbo);
```

| 比較項目 | テクスチャ | RBO |
|---|---|---|
| 後でサンプリング | ✅ 可能 | ❌ 不可 |
| GPU 最適化 | 普通 | 高い |
| 主な用途 | カラー、シャドウマップ | 深度・ステンシル |

### ステップ4: 完全性チェック

```cpp
if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

// デフォルトフレームバッファに戻す
glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

FBO が「完全」になるための条件:
1. 最低1つのアタッチメントがある
2. すべてのアタッチメントが完全（メモリ確保済み）
3. すべてのアタッチメントのサンプル数が同じ

---

## 📖 2パスレンダリング

FBO を使った描画は2段階で行います。

```
パス1: シーンをFBOに描画      パス2: FBOテクスチャを画面に表示
┌─────────────────┐          ┌─────────────────┐
│ glBindFBO(fbo)  │          │ glBindFBO(0)    │
│ シーン描画       │    →     │ スクリーンクワッド│
│ (3Dオブジェクト) │          │ + ポストプロセス  │
└─────────────────┘          └─────────────────┘
```

```cpp
// ======== パス1: カスタム FBO に描画 ========
glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glEnable(GL_DEPTH_TEST);
drawScene(sceneShader); // 通常のシーン描画

// ======== パス2: デフォルト FBO に表示 ========
glBindFramebuffer(GL_FRAMEBUFFER, 0);
glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT);
glDisable(GL_DEPTH_TEST); // スクリーンクワッドに深度テスト不要

screenShader.use();
glBindVertexArray(quadVAO);
glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
glDrawArrays(GL_TRIANGLES, 0, 6);
```

---

## 📖 スクリーンクワッド

画面全体を覆う四角形（2つの三角形）を定義します。

```cpp
float quadVertices[] = {
    // 位置          // UV
    -1.0f,  1.0f,   0.0f, 1.0f,
    -1.0f, -1.0f,   0.0f, 0.0f,
     1.0f, -1.0f,   1.0f, 0.0f,

    -1.0f,  1.0f,   0.0f, 1.0f,
     1.0f, -1.0f,   1.0f, 0.0f,
     1.0f,  1.0f,   1.0f, 1.0f
};
```

NDC（正規化デバイス座標）で -1 〜 +1 の範囲で四角形を定義するため、MVP行列は不要です。

スクリーンクワッド用シェーダー:

```glsl
// 頂点シェーダー
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    TexCoords = aTexCoords;
}
```

```glsl
// フラグメントシェーダー（そのまま表示）
#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;

void main() {
    FragColor = texture(screenTexture, TexCoords);
}
```

---

## 📖 ポストプロセスエフェクト

FBO テクスチャに対してフラグメントシェーダーで加工を行います。

### 色反転（Inversion）

```glsl
void main() {
    FragColor = vec4(vec3(1.0 - texture(screenTexture, TexCoords)), 1.0);
}
```

### グレースケール

```glsl
void main() {
    FragColor = texture(screenTexture, TexCoords);
    // 人間の目の感度に合わせた加重平均
    float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g
                  + 0.0722 * FragColor.b;
    FragColor = vec4(average, average, average, 1.0);
}
```

---

## 📖 カーネル（畳み込み行列）

カーネルとは、注目ピクセルとその周囲のピクセルに重みをかけて合計する仕組みです。3×3 のカーネルが一般的です。

```
カーネルの適用イメージ:

  隣接ピクセル         カーネル値          結果
┌───┬───┬───┐     ┌───┬───┬───┐
│ a │ b │ c │     │k1 │k2 │k3 │     出力 = a*k1 + b*k2 + c*k3
├───┼───┼───┤  ×  ├───┼───┼───┤          + d*k4 + e*k5 + f*k6
│ d │ e │ f │     │k4 │k5 │k6 │          + g*k7 + h*k8 + i*k9
├───┼───┼───┤     ├───┼───┼───┤
│ g │ h │ i │     │k7 │k8 │k9 │
└───┴───┴───┘     └───┴───┴───┘
```

### カーネルシェーダーの実装

```glsl
#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;

const float offset = 1.0 / 300.0; // テクスチャサイズに応じて調整

void main() {
    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // 左上
        vec2( 0.0f,    offset), // 上
        vec2( offset,  offset), // 右上
        vec2(-offset,  0.0f),   // 左
        vec2( 0.0f,    0.0f),   // 中央
        vec2( offset,  0.0f),   // 右
        vec2(-offset, -offset), // 左下
        vec2( 0.0f,   -offset), // 下
        vec2( offset, -offset)  // 右下
    );

    // シャープネスカーネル
    float kernel[9] = float[](
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1
    );

    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(screenTexture,
                            TexCoords.st + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++) {
        col += sampleTex[i] * kernel[i];
    }
    FragColor = vec4(col, 1.0);
}
```

### 代表的なカーネル

**ブラー（ぼかし）:**
```
1/16  2/16  1/16
2/16  4/16  2/16
1/16  2/16  1/16
```

**エッジ検出:**
```
 1   1   1
 1  -8   1
 1   1   1
```

カーネルの重みの合計が 1 になるようにすると、画像全体の明るさが保たれます。エッジ検出のように合計が 0 のカーネルはエッジ部分だけを抽出します。

---

## 💡 ポイントまとめ

| 項目 | 内容 |
|---|---|
| FBO | GPU が描画先として使うバッファの集合。自分で作れる |
| テクスチャアタッチメント | 後からサンプリング可能。カラー出力向き |
| RBO | サンプリング不可だが高速。深度/ステンシル向き |
| 完全性チェック | `glCheckFramebufferStatus` で必ず確認 |
| 2パスレンダリング | パス1: FBOに描画 → パス2: スクリーンクワッドで表示 |
| スクリーンクワッド | NDC で -1〜+1 の四角形。MVP不要 |
| ポストプロセス | FBOテクスチャをフラグメントシェーダーで加工 |
| カーネル | 3×3 の重み行列で周囲ピクセルを畳み込み |
| 重みの合計 | 1 → 明るさ維持、0 → エッジ抽出 |

---

## ✏️ ドリル問題

### 問1（穴埋め）
FBO を生成するには `glGen___(1, &fbo)` を呼び、バインドには `glBind___(GL_FRAMEBUFFER, fbo)` を使う。

<details><summary>📝 解答</summary>

`glGenFramebuffers` と `glBindFramebuffer`

</details>

### 問2（選択）
FBO に深度・ステンシルバッファを追加する際、後からシェーダーでサンプリングする予定がない場合、最適な選択肢はどれか？

A. テクスチャをアタッチする  
B. レンダーバッファオブジェクト（RBO）をアタッチする  
C. どちらでも性能は同じ  

<details><summary>📝 解答</summary>

**B. RBO をアタッチする**

RBO は GPU の内部フォーマットで最適化されており、サンプリング不要な場合はテクスチャより高速です。

</details>

### 問3（穴埋め）
テクスチャをカラーアタッチメントとして FBO に紐づける関数は `glFramebuffer___(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0)` である。

<details><summary>📝 解答</summary>

`glFramebufferTexture2D`

</details>

### 問4（計算）
以下のブラーカーネルを (0.8, 0.2, 0.4) のピクセルに適用する。中央ピクセルの値が (0.8, 0.2, 0.4) で、周囲8ピクセルがすべて (0.0, 0.0, 0.0) の場合、出力の R 値はいくつか？

```
1/16  2/16  1/16
2/16  4/16  2/16
1/16  2/16  1/16
```

<details><summary>📝 解答</summary>

中央のみ非ゼロなので、R = 0.8 × (4/16) = 0.8 × 0.25 = **0.2**

</details>

### 問5（選択）
FBO が完全（complete）になるための条件として **正しくない** ものはどれか？

A. 最低1つのアタッチメントが必要  
B. すべてのアタッチメントのサンプル数が同じ  
C. カラー・深度・ステンシルの3種類すべてが必須  
D. すべてのアタッチメントにメモリが割り当て済み  

<details><summary>📝 解答</summary>

**C**。3種類すべては必須ではありません。最低1つのアタッチメントがあれば完全になり得ます。

</details>

### 問6（穴埋め）
エッジ検出カーネルは重みの合計が ___ になるため、エッジ以外の平坦な領域は暗く（黒く）なる。

<details><summary>📝 解答</summary>

**0**

</details>

### 問7（記述）
スクリーンクワッドの頂点座標を NDC の -1〜+1 で定義する利点を説明せよ。

<details><summary>📝 解答</summary>

NDC の -1〜+1 はクリッピング空間全体に対応するため、MVP（モデル・ビュー・プロジェクション）行列を乗算する必要がなく、頂点シェーダーでそのまま `gl_Position` に渡せます。これにより計算コストが削減されます。

</details>

---

## 🔨 実践課題

### 課題1: 基本の FBO ポストプロセス ⭐⭐
既存のシーンに FBO を追加し、グレースケールフィルタを適用せよ。

**チェックポイント:**
- [ ] FBO の生成・テクスチャアタッチ・RBOアタッチが正しい
- [ ] `glCheckFramebufferStatus` で完全性を確認している
- [ ] パス1でシーンを FBO に描画できている
- [ ] パス2でスクリーンクワッドにグレースケールシェーダーを適用している

### 課題2: カーネルエフェクト切り替え ⭐⭐⭐
キーボード入力でポストプロセスエフェクトを切り替えられるようにせよ。

| キー | エフェクト |
|---|---|
| 1 | なし（そのまま表示） |
| 2 | 色反転 |
| 3 | ブラー |
| 4 | エッジ検出 |

**チェックポイント:**
- [ ] キー入力のコールバックでエフェクト番号を切り替え
- [ ] uniform 変数またはシェーダーの切り替えで実装
- [ ] 各カーネルの重みが正しい

### 課題3: カスタムカーネル設計 ⭐⭐⭐⭐
エンボス効果のカーネルを自分で設計し、実装せよ。

```
エンボスカーネル例:
-2  -1   0
-1   1   1
 0   1   2
```

**チェックポイント:**
- [ ] カーネル値の合計が1前後になっている（明るさ維持）
- [ ] シェーダー内でカーネル値を正しく適用している
- [ ] 凹凸が浮き出るような見た目になっている
- [ ] 他のカーネル（シャープネス等）との見た目の違いを確認した

---

## 🔗 ナビゲーション

⬅️ [フェイスカリング](./04-face-culling.md) | ➡️ [キューブマップ →](./06-cubemaps.md)
