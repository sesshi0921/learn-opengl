# 📘 アンチエイリアシング（Anti Aliasing）

> **目標：** ラスタライゼーションで発生するエイリアシング（ジャギー）の原因を理解し、MSAA の仕組みと実装方法（オンスクリーン・オフスクリーン）を習得する。

---

## 📖 エイリアシングとは

ラスタライザは連続的な三角形のエッジを**離散的なピクセルグリッド**に変換します。この過程で、斜めの辺が階段状のギザギザ（**ジャギー / エイリアシング**）として見えてしまいます。

```
理想的なエッジ:         ラスタライズ後:
      /                 ┌──┐
     /                  │██│
    /                 ┌─┤██│
   /                  │██├─┘
  /                 ┌─┤██│
 /                  │██├─┘
/                   │██│
                    └──┘
                    階段状のジャギーが発生
```

**なぜ起こるのか？**

各ピクセルの中心1点だけで「三角形の内側かどうか」を判定するためです。ピクセルの中心が三角形の内側なら色が塗られ、外側なら塗られません。この二値判定が不連続なエッジを生みます。

```
ピクセルグリッドと三角形のエッジ:

  ┌───┬───┬───┬───┬───┐
  │   │   │   │ · │   │   · = ピクセル中心
  ├───┼───┼──/┼───┼───┤
  │   │   │/· │ · │   │   / = 三角形のエッジ
  ├───┼──/┼───┼───┼───┤
  │   │/· │ · │ · │   │
  ├──/┼───┼───┼───┼───┤
  │/· │ · │ · │ · │   │
  └───┴───┴───┴───┴───┘

中心が三角形内 → ██  (塗る)
中心が三角形外 → □  (塗らない)
→ 境界がギザギザに見える
```

---

## 📖 スーパーサンプリング（SSAA）

最も単純なアンチエイリアシング手法は **SSAA (Super Sample Anti-Aliasing)** です。

**仕組み：** 表示解像度の数倍（例: 4倍）の解像度でレンダリングし、最終的にダウンサンプリング（平均化）して表示解像度に縮小します。

```
SSAA 4x: 表示解像度の4倍でレンダリングし、平均化して縮小
通常 800×600 → SSAA 4x: 1600×1200 → 各ピクセルを4サンプル平均
```

**利点：** 最高品質のアンチエイリアシング
**欠点：** レンダリングコストが N 倍（4x SSAA ならシェーダー実行量が4倍、メモリも4倍）→ **実用的ではない**

---

## 📖 マルチサンプリング（MSAA）

**MSAA (Multi-Sample Anti-Aliasing)** は SSAA の効率化版です。SSAA のように全サンプルでフラグメントシェーダーを実行するのではなく、**フラグメントシェーダーは1回だけ実行**し、複数のサンプルポイントでカバレッジ（三角形が覆っているか）だけを判定します。

### サンプルポイントとカバレッジ

MSAA 4x の場合、各ピクセル内に4つのサンプルポイントが配置されます。

```
1ピクセル内のサンプルポイント (MSAA 4x):

  ┌─────────────────┐
  │  ·           ·   │   · = サンプルポイント
  │       ●          │   ● = ピクセル中心（シェーダー実行位置）
  │  ·           ·   │
  └─────────────────┘

三角形のエッジがピクセルを横切る場合:

  ┌─────────────────┐
  │  ■     ╱    □   │   ■ = カバーされたサンプル
  │       ●╱         │   □ = カバーされていないサンプル
  │  ■  ╱       □   │
  └────╱────────────┘

カバレッジ: 2/4 = 50%
最終色 = フラグメントシェーダーの色 × 50% + 背景色 × 50%
```

### MSAA の処理手順

1. **カバレッジテスト**: 各サンプルポイントが三角形内かどうか判定
2. **フラグメントシェーダー実行**: ピクセル中心で **1回だけ** 実行
3. **色の書き込み**: カバーされたサンプルにのみシェーダーの出力色を書き込む
4. **リゾルブ**: 全サンプルの色を平均して最終ピクセル色を決定

**なぜ効率的か？** SSAA 4x ではフラグメントシェーダーを4回実行しますが、MSAA 4x ではカバレッジテストのみ4回、フラグメントシェーダーは1回で済みます。テクスチャサンプリングや複雑なライティング計算のコストが大幅に削減されます。

---

## 📖 GLFW での MSAA 有効化

最も簡単な MSAA の有効化方法はウィンドウ作成時に設定することです。

```cpp
// ウィンドウ作成前にサンプル数を指定
glfwWindowHint(GLFW_SAMPLES, 4);  // 4x MSAA

// ウィンドウ作成
GLFWwindow* window = glfwCreateWindow(800, 600, "MSAA Demo", NULL, NULL);

// OpenGL 側で MSAA を有効化
glEnable(GL_MULTISAMPLE);
```

これだけでデフォルトフレームバッファに MSAA が適用されます。`glDisable(GL_MULTISAMPLE)` で一時的に無効化することもできます。

> **注意：** この方法はデフォルトフレームバッファ（画面に直接描画する場合）にのみ有効です。オフスクリーンレンダリング（FBO）では別途設定が必要です。

---

## 📖 オフスクリーン MSAA（マルチサンプル FBO）

ポストプロセスなどでオフスクリーン FBO を使う場合、マルチサンプル対応の FBO を自分で構築する必要があります。

### GL_TEXTURE_2D_MULTISAMPLE

通常の `GL_TEXTURE_2D` の代わりに、マルチサンプル用の特殊なテクスチャターゲットを使用します。

```cpp
// マルチサンプルテクスチャの作成
unsigned int msaaTexture;
glGenTextures(1, &msaaTexture);
glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaTexture);
glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
    4,              // サンプル数
    GL_RGB,         // 内部フォーマット
    width, height,
    GL_TRUE         // fixedSampleLocations
);
glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

// FBOにアタッチ
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                       GL_TEXTURE_2D_MULTISAMPLE, msaaTexture, 0);
```

> `glTexImage2DMultisample` はフィルタリングパラメータやミップマップを設定しません。

### マルチサンプル RBO と完全な FBO 構築

深度・ステンシルバッファもマルチサンプル対応が必要です。以下に完全な構築コードを示します。

```cpp
// 1. マルチサンプル FBO の作成
unsigned int msaaFBO;
glGenFramebuffers(1, &msaaFBO);
glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);

// 2. マルチサンプルカラーテクスチャ
unsigned int msaaColorBuffer;
glGenTextures(1, &msaaColorBuffer);
glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaColorBuffer);
glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, width, height, GL_TRUE);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                       GL_TEXTURE_2D_MULTISAMPLE, msaaColorBuffer, 0);

// 3. マルチサンプル深度/ステンシル RBO
unsigned int msaaRBO;
glGenRenderbuffers(1, &msaaRBO);
glBindRenderbuffer(GL_RENDERBUFFER, msaaRBO);
glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);
glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                          GL_RENDERBUFFER, msaaRBO);

// 4. 完全性チェック
if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR: MSAA Framebuffer is not complete!" << std::endl;
glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

---

## 📖 glBlitFramebuffer によるリゾルブ

マルチサンプル FBO の内容は直接テクスチャとしてサンプリングできません。**リゾルブ**（複数サンプルの平均化）を行って通常のテクスチャに転送する必要があります。

```cpp
// リゾルブ先の通常 FBO
unsigned int intermediateFBO;
glGenFramebuffers(1, &intermediateFBO);
glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

unsigned int screenTexture;
glGenTextures(1, &screenTexture);
glBindTexture(GL_TEXTURE_2D, screenTexture);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);
```

**リゾルブ処理（Blit）：**

```cpp
// マルチサンプル FBO → 通常 FBO にブリット
glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO);         // 読み取り元: マルチサンプル
glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);  // 書き込み先: 通常
glBlitFramebuffer(
    0, 0, width, height,    // 読み取り元の矩形
    0, 0, width, height,    // 書き込み先の矩形
    GL_COLOR_BUFFER_BIT,    // コピーするバッファ
    GL_NEAREST              // フィルター（色バッファはGL_NEARESTが推奨）
);
```

```
MSAA FBO (TEXTURE_2D_MULTISAMPLE)  ──glBlitFramebuffer──▶  通常 FBO (TEXTURE_2D)
        直接サンプリング不可            サンプル平均化          サンプリング可能
```

---

## 📖 MSAA とポストプロセスの併用

MSAA FBO で描画した結果にポストプロセス（ブラー、HDR 等）を適用する手順：

```cpp
// 完全なレンダリングループ：
// 1. MSAA FBO にシーンを描画
glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);
glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glEnable(GL_DEPTH_TEST);
// ... シーンの描画コード ...

// 2. MSAA FBO → 中間 FBO にリゾルブ
glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO);
glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                  GL_COLOR_BUFFER_BIT, GL_NEAREST);

// 3. 中間 FBO のテクスチャでポストプロセス → デフォルト FBO
glBindFramebuffer(GL_FRAMEBUFFER, 0);
glClear(GL_COLOR_BUFFER_BIT);
glDisable(GL_DEPTH_TEST);
postProcessShader.use();
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, screenTexture);
renderQuad();
```

---

## 📖 カスタムアンチエイリアシング技法

MSAA 以外にも、ポストプロセスベースのアンチエイリアシング手法があります。

| 手法   | 正式名称                              | 特徴                                                      |
|--------|---------------------------------------|------------------------------------------------------------|
| FXAA   | Fast Approximate Anti-Aliasing        | ポストプロセスのみ。画像全体のエッジを検出・ぼかす。軽量だがやや不鮮明 |
| SMAA   | Subpixel Morphological Anti-Aliasing  | エッジ検出 + パターンマッチング。FXAA より高品質            |
| TAA    | Temporal Anti-Aliasing                | フレーム間のジッタリングと蓄積。動きのある場面で高品質      |

これらは MSAA と異なりジオメトリ段階ではなく**画像処理段階**で動作するため、ディファードレンダリングとも相性が良いです（MSAA はフォワードレンダリング向け）。

---

## 💡 ポイントまとめ

| 項目                           | 内容                                                        |
|--------------------------------|-------------------------------------------------------------|
| エイリアシングの原因           | ピクセル中心1点だけで内外判定する離散化の問題                |
| SSAA                           | 高解像度でレンダリング→縮小。高品質だがコスト N 倍          |
| MSAA                           | カバレッジだけ複数サンプル、シェーダーは1回。効率的          |
| GLFW での有効化                | `glfwWindowHint(GLFW_SAMPLES, 4)` + `glEnable(GL_MULTISAMPLE)` |
| マルチサンプルテクスチャ       | `GL_TEXTURE_2D_MULTISAMPLE` + `glTexImage2DMultisample`     |
| マルチサンプル RBO             | `glRenderbufferStorageMultisample`                          |
| リゾルブ                       | `glBlitFramebuffer` で MSAA → 通常テクスチャに変換          |
| 読み書き FBO の使い分け        | `GL_READ_FRAMEBUFFER` / `GL_DRAW_FRAMEBUFFER`               |
| ポストプロセス併用             | MSAA FBO → リゾルブ → 通常テクスチャ → ポストプロセス       |
| 代替手法                       | FXAA, SMAA, TAA（ポストプロセスベース、ディファード対応）   |

---

## ✏️ ドリル問題

### 問題1（選択）

SSAA 4x と MSAA 4x の最大の違いは何ですか？

- A) SSAA はハードウェア非対応だが、MSAA はハードウェア対応
- B) SSAA はフラグメントシェーダーを4回実行するが、MSAA は1回
- C) SSAA は三角形のみ対応だが、MSAA は全プリミティブ対応
- D) SSAA はポストプロセスベースだが、MSAA はラスタライズベース

<details><summary>📝 解答</summary>

**B) SSAA はフラグメントシェーダーを4回実行するが、MSAA は1回**

SSAA は全サンプルポイントでフラグメントシェーダーを実行するためコストが4倍になります。MSAA はカバレッジテストだけを4サンプルで行い、フラグメントシェーダーは1回だけ実行してその結果をカバーされたサンプルに書き込むため、大幅に効率的です。

</details>

---

### 問題2（穴埋め）

GLFW で MSAA を有効化するコードを完成させてください。

```cpp
// ウィンドウ作成前
glfwWindowHint(______, ______);

// OpenGL コンテキスト作成後
glEnable(______);
```

<details><summary>📝 解答</summary>

```cpp
glfwWindowHint(GLFW_SAMPLES, 4);
glEnable(GL_MULTISAMPLE);
```

`GLFW_SAMPLES` でサンプル数を指定し（一般的に4）、`GL_MULTISAMPLE` でマルチサンプリングを有効化します。この2つがセットで必要です。

</details>

---

### 問題3（穴埋め）

マルチサンプルテクスチャを作成して FBO にアタッチするコードを完成させてください。

```cpp
unsigned int msaaTex;
glGenTextures(1, &msaaTex);
glBindTexture(______, msaaTex);
______(______, 4, GL_RGB, 800, 600, GL_TRUE);

glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                       ______, msaaTex, 0);
```

<details><summary>📝 解答</summary>

```cpp
glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaTex);
glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, 800, 600, GL_TRUE);

glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                       GL_TEXTURE_2D_MULTISAMPLE, msaaTex, 0);
```

通常の `GL_TEXTURE_2D` と異なり、`GL_TEXTURE_2D_MULTISAMPLE` を一貫して使います。`glTexImage2DMultisample` は `glTexImage2D` と引数が異なることに注意してください。

</details>

---

### 問題4（穴埋め）

マルチサンプル FBO から通常 FBO へリゾルブする `glBlitFramebuffer` の呼び出しを完成させてください。

```cpp
glBindFramebuffer(______, msaaFBO);          // 読み取り元
glBindFramebuffer(______, intermediateFBO);   // 書き込み先
glBlitFramebuffer(
    0, 0, width, height,
    0, 0, width, height,
    ______,
    GL_NEAREST
);
```

<details><summary>📝 解答</summary>

```cpp
glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO);
glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
glBlitFramebuffer(
    0, 0, width, height,
    0, 0, width, height,
    GL_COLOR_BUFFER_BIT,
    GL_NEAREST
);
```

- `GL_READ_FRAMEBUFFER`: ブリットのソース
- `GL_DRAW_FRAMEBUFFER`: ブリットのデスティネーション
- `GL_COLOR_BUFFER_BIT`: カラーバッファをコピー（`GL_DEPTH_BUFFER_BIT` 等も指定可）

</details>

---

### 問題5（記述）

MSAA を使用したシーンにポストプロセスエフェクト（例：ブラー）を適用したい場合、なぜ直接マルチサンプルテクスチャをポストプロセスシェーダーで使えないのか説明し、解決策を述べてください。

<details><summary>📝 解答</summary>

**理由：** `GL_TEXTURE_2D_MULTISAMPLE` テクスチャはフラグメントシェーダーで通常の `texture()` 関数でサンプリングできません（`texelFetch` と `sampler2DMS` を使えば個別サンプルにアクセスは可能ですが、通常のテクスチャフィルタリングが効きません）。

**解決策：** `glBlitFramebuffer` を使ってマルチサンプル FBO の内容を通常の `GL_TEXTURE_2D` テクスチャを持つ中間 FBO にリゾルブ（転送・平均化）します。その後、中間 FBO のテクスチャを通常のポストプロセスシェーダーでサンプリングします。

手順: MSAA FBO → (glBlitFramebuffer) → 中間 FBO (通常テクスチャ) → ポストプロセスシェーダー → デフォルト FBO

</details>

---

### 問題6（計算）

MSAA 4x のピクセルで、三角形のエッジが4つのサンプルポイントのうち3つをカバーしています。フラグメントシェーダーが出力した色が `(1.0, 0.0, 0.0)`（赤）で、背景色が `(0.0, 0.0, 0.0)`（黒）の場合、リゾルブ後のピクセル色はいくらですか？

<details><summary>📝 解答</summary>

カバレッジ: 3/4 = 75%

リゾルブ後の色 = カバーされたサンプルの色の平均:

$$\text{color} = \frac{3 \times (1.0, 0.0, 0.0) + 1 \times (0.0, 0.0, 0.0)}{4} = (0.75, 0.0, 0.0)$$

やや暗い赤になります。これがエッジのスムーズなグラデーションを生み出し、ジャギーを軽減します。カバレッジが 4/4 の場合は完全な赤 `(1.0, 0.0, 0.0)` になります。

</details>

---

### 問題7（選択）

ディファードレンダリングで最も使いやすいアンチエイリアシング手法はどれですか？

- A) MSAA
- B) SSAA
- C) FXAA / SMAA / TAA
- D) どれも使えない

<details><summary>📝 解答</summary>

**C) FXAA / SMAA / TAA**

MSAA はラスタライズ段階で動作するため、ディファードレンダリングの G-Buffer 生成時に適用すると複数のターゲットすべてにサンプルが必要になり非効率です。FXAA・SMAA・TAA はポストプロセスベースのため、最終画像に対して適用でき、ディファードレンダリングとの相性が優れています。

</details>

---

## 🔨 実践課題

### 課題1: MSAA オン/オフ切り替え ⭐⭐

回転する立方体を描画し、キー入力で MSAA のオン/オフを切り替えてジャギーの違いを視覚的に確認できるプログラムを作成してください。

**チェックポイント：**
- [ ] `glfwWindowHint(GLFW_SAMPLES, 4)` でウィンドウを作成
- [ ] キー（例: M キー）で `glEnable(GL_MULTISAMPLE)` / `glDisable(GL_MULTISAMPLE)` をトグル
- [ ] 画面に現在の状態（ON/OFF）をタイトルバーに表示
- [ ] エッジの違いが明確に見えるよう、コントラストの高い背景色を使用

---

### 課題2: オフスクリーン MSAA + ポストプロセス ⭐⭐⭐

マルチサンプル FBO を使ってシーンを描画し、リゾルブ後にグレースケール変換またはエッジ検出のポストプロセスを適用してください。

**チェックポイント：**
- [ ] マルチサンプル FBO（カラーテクスチャ + 深度RBO）を構築
- [ ] 中間 FBO（通常テクスチャ）を構築
- [ ] シーンを MSAA FBO に描画 → `glBlitFramebuffer` でリゾルブ
- [ ] 中間テクスチャをスクリーンクワッドに描画（ポストプロセスシェーダー使用）
- [ ] MSAA のエッジスムージングとポストプロセスが両方適用されていることを確認

---

### 課題3: MSAA サンプル数の比較ツール ⭐⭐⭐⭐

画面を4分割し、同じシーンを MSAA 0x（無効）, 2x, 4x, 8x でそれぞれ描画して品質を比較できるツールを作成してください。

**チェックポイント：**
- [ ] 4つのマルチサンプル FBO（0x, 2x, 4x, 8x）を作成
- [ ] 4つの中間 FBO をそれぞれ作成
- [ ] 各 FBO に同じシーンを描画 → リゾルブ
- [ ] `glViewport` で画面を4分割し、各テクスチャをそれぞれのビューポートに表示
- [ ] 各ビューポートにサンプル数のラベルを表示（テキストレンダリングまたはオーバーレイ）
- [ ] `glGetIntegerv(GL_MAX_SAMPLES, ...)` でハードウェアの最大サンプル数を確認

---

## 🔗 ナビゲーション

⬅️ [インスタンシング](./10-instancing.md) | 🏠 [トップへ戻る](../README.md)
