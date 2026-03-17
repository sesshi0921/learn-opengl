# 📘 ブレンディング（Blending）

> **目標：** アルファ値による透過処理を理解し、半透明オブジェクトの正しい描画順序を含むブレンディングを実装できるようになる。

---

## 📖 透過テクスチャの必要性

3Dシーンでは、完全に不透明でないテクスチャが頻繁に登場します。

- **草**: スプライト（板ポリゴン）に貼り付けた草のテクスチャ
- **窓**: 半透明のガラス
- **フェンス**: 金網のように部分的に透明
- **パーティクル**: 煙、炎、魔法エフェクト

これらを表現するために **アルファ値** と **ブレンディング** が必要です。

```
草のテクスチャ例：
┌─────────────┐
│░░░░░█░░░░░░░│  █ = 不透明（草の部分）α=1.0
│░░░░███░░░░░░│  ░ = 完全透明（背景）  α=0.0
│░░░█████░░░░░│
│░░███████░░░░│
│░█████████░░░│
│░░░░░█░░░░░░░│  ← 茎
│░░░░░█░░░░░░░│
└─────────────┘
 四角形のポリゴンだが、
 透明部分は見えない
```

---

## 📖 アルファ値とは

テクスチャの各ピクセル（テクセル）は RGBA の4成分を持ちます。

| 成分 | 意味 | 範囲 |
|------|------|------|
| R | 赤 | 0.0〜1.0 |
| G | 緑 | 0.0〜1.0 |
| B | 青 | 0.0〜1.0 |
| **A（アルファ）** | **不透明度** | **0.0（完全透明）〜 1.0（完全不透明）** |

```
アルファ値の効果：

α = 1.0    α = 0.7    α = 0.3    α = 0.0
████████   ▓▓▓▓▓▓▓▓   ░░░░░░░░   （見えない）
完全不透明   やや透明   かなり透明   完全透明
```

PNG画像は透明度（アルファチャンネル）をサポートしています。テクスチャ読み込み時に4チャンネルで読み込む必要があります。

```cpp
// stb_image でアルファチャンネル付き画像を読み込む
int width, height, nrChannels;
unsigned char *data = stbi_load("grass.png", &width, &height, &nrChannels, 0);

// RGBA テクスチャとして設定
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
             GL_RGBA, GL_UNSIGNED_BYTE, data);
```

---

## 📖 discard によるアルファテスト

最もシンプルなアプローチは、**完全に透明なピクセルを捨てる** ことです。ブレンディングではなく、フラグメントシェーダで `discard` を使います。

```glsl
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
uniform sampler2D texture1;

void main()
{
    vec4 texColor = texture(texture1, TexCoords);
    // アルファ値が閾値以下なら、フラグメントを完全に破棄
    if (texColor.a < 0.1)
        discard;
    FragColor = texColor;
}
```

**`discard` はフラグメントを完全に捨てます。** 深度バッファにもステンシルバッファにも書き込まれません。

### テクスチャラッピングの注意

透明テクスチャでは、テクスチャのラッピングモードに注意が必要です。`GL_REPEAT` だとエッジ付近で隣の色が混ざり込み、半透明の縁が見えてしまうことがあります。

```cpp
// 透明テクスチャには GL_CLAMP_TO_EDGE を使う
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
```

---

## 📖 ブレンディングの有効化

半透明（0 < α < 1）を表現するには、ブレンディングを有効化します。

```cpp
glEnable(GL_BLEND);
```

### ブレンド方程式

ブレンディングの基本式：

$$\vec{C}_{result} = \vec{C}_{source} \times F_{source} + \vec{C}_{destination} \times F_{destination}$$

| 要素 | 意味 |
|------|------|
| $\vec{C}_{source}$ | 新しいフラグメントの色 |
| $\vec{C}_{destination}$ | カラーバッファに既にある色 |
| $F_{source}$, $F_{destination}$ | それぞれの係数 |

### glBlendFunc

```cpp
glBlendFunc(GLenum sfactor, GLenum dfactor);
```

最も一般的な設定：

```cpp
// 標準的なアルファブレンディング
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

**これは何をしているのか？**

```
Source（新しいフラグメント）: 赤色、α=0.6
Destination（既存の色）:     青色

sfactor = GL_SRC_ALPHA         = 0.6
dfactor = GL_ONE_MINUS_SRC_ALPHA = 1.0 - 0.6 = 0.4

result = source * 0.6 + destination * 0.4
       = 赤 * 0.6 + 青 * 0.4
       = やや赤寄りの紫
```

### 具体的な計算例

```
Source:      (1.0, 0.0, 0.0, 0.6)  ← 赤、60%不透明
Destination: (0.0, 0.0, 1.0, 1.0)  ← 青、不透明

結果:
R = 1.0 * 0.6 + 0.0 * 0.4 = 0.6
G = 0.0 * 0.6 + 0.0 * 0.4 = 0.0
B = 0.0 * 0.6 + 1.0 * 0.4 = 0.4
A = 0.6 * 0.6 + 1.0 * 0.4 = 0.76

結果: (0.6, 0.0, 0.4, 0.76)  ← 赤みがかった紫
```

### 主なブレンド係数

| 係数 | 値 |
|------|-----|
| `GL_ZERO` | 0 |
| `GL_ONE` | 1 |
| `GL_SRC_COLOR` | ソースの色成分 |
| `GL_SRC_ALPHA` | ソースのα値 |
| `GL_DST_COLOR` | デスティネーションの色成分 |
| `GL_DST_ALPHA` | デスティネーションのα値 |
| `GL_ONE_MINUS_SRC_ALPHA` | 1 - ソースのα値 |
| `GL_ONE_MINUS_DST_ALPHA` | 1 - デスティネーションのα値 |

---

## 📖 glBlendFuncSeparate

RGB成分とアルファ成分で **別々の** ブレンド係数を設定できます。

```cpp
glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                    GL_ONE, GL_ZERO);
// RGB: 通常のアルファブレンド
// Alpha: ソースのアルファ値をそのまま使用
```

**なぜ必要か？** RGB成分は透過合成したいが、結果のアルファ値は別の計算にしたい場合に使います。

---

## 📖 glBlendEquation

デフォルトでは**加算**ですが、変更できます。

```cpp
glBlendEquation(GLenum mode);
```

| モード | 方程式 |
|--------|--------|
| `GL_FUNC_ADD` | $result = src \times F_s + dst \times F_d$（**デフォルト**） |
| `GL_FUNC_SUBTRACT` | $result = src \times F_s - dst \times F_d$ |
| `GL_FUNC_REVERSE_SUBTRACT` | $result = dst \times F_d - src \times F_s$ |
| `GL_MIN` | $result = min(src, dst)$ |
| `GL_MAX` | $result = max(src, dst)$ |

---

## 📖 半透明オブジェクトの描画順序問題

ブレンディングにおいて最も重要かつ厄介な問題が **描画順序** です。

### なぜ順序が重要なのか？

深度テストは「奥のフラグメントを破棄する」ため、半透明オブジェクトの背後にあるものが描画されなくなります。

```
問題のある描画順序（手前から描画）:

カメラ → [窓A(手前)] → [窓B(奥)]

1. 窓A を描画。深度バッファに窓Aの深度を書き込む。
2. 窓B を描画しようとするが、深度テストで不合格。
   → 窓B は描画されない！
   → 窓A の後ろに窓B が見えるべきなのに、消えてしまう。
```

```
正しい描画順序（奥から描画）:

1. 不透明オブジェクトをすべて描画（順序不問）
2. 半透明オブジェクトを奥から手前の順で描画

カメラ ← [窓B(奥)] ← [窓A(手前)]

1. 窓B を描画。背景とブレンドされる。
2. 窓A を描画。窓B の結果とブレンドされる。
   → 正しい透過表現が得られる！
```

### 描画の正しい手順

```
┌─────────────────────────────────────┐
│ 1. 不透明オブジェクトをすべて描画       │
│    （深度テスト有効、書き込み有効）      │
│    → 順序は問わない                   │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│ 2. 半透明オブジェクトをソート           │
│    （カメラからの距離で降順ソート）      │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│ 3. 半透明オブジェクトを奥から手前に描画  │
│    深度テスト: 有効（読み取り）         │
│    深度書き込み: 無効が望ましい         │
│    ブレンディング: 有効                │
└─────────────────────────────────────┘
```

### カメラ距離によるソート

```cpp
// std::map を使えば距離で自動ソート（昇順）される
std::map<float, glm::vec3> sortedTransparent;

for (unsigned int i = 0; i < transparentPositions.size(); i++)
{
    float distance = glm::length(camera.Position - transparentPositions[i]);
    sortedTransparent[distance] = transparentPositions[i];
}

// 降順（奥から）に描画するために reverse_iterator を使う
for (auto it = sortedTransparent.rbegin();
     it != sortedTransparent.rend(); ++it)
{
    model = glm::mat4(1.0f);
    model = glm::translate(model, it->second);
    shader.setMat4("model", model);
    drawTransparentQuad();
}
```

> ⚠️ **`std::map` の制限**: 同じ距離のオブジェクトはキーが衝突します。より堅牢な実装では `std::vector` + `std::sort` を使います。

---

## 📖 OIT（Order-Independent Transparency）概要

ソートベースの手法には限界があります：

- **相互に貫通するオブジェクト** はオブジェクト単位のソートでは正しくない
- **毎フレームのソートコスト** が発生する

**OIT（順序非依存透過）** は、描画順序に関係なく正しい透過表現を行う高度な手法です。

| 手法 | 概要 |
|------|------|
| Depth Peeling | 複数パスで「次に手前のレイヤー」を剥がす |
| Weighted Blended OIT | 深度に基づく重み付き平均（1パス、近似） |
| Per-Pixel Linked Lists | ピクセルごとにフラグメントのリストを構築 |

これらは応用的なテクニックで、基本的なブレンディングを理解した上で学ぶと良いでしょう。

---

## 💡 ポイントまとめ

| 項目 | 内容 |
|------|------|
| アルファ値 | RGBA の A 成分。0.0=透明、1.0=不透明 |
| `discard` | フラグメントを完全に破棄（αテスト） |
| テクスチャラップ | 透明テクスチャには `GL_CLAMP_TO_EDGE` |
| ブレンド有効化 | `glEnable(GL_BLEND)` |
| 標準ブレンド | `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)` |
| ブレンド方程式 | $C_{result} = C_{src} \times F_s + C_{dst} \times F_d$ |
| `glBlendFuncSeparate` | RGBとαで別の係数を設定 |
| `glBlendEquation` | 加算・減算・min・max を選択 |
| 描画順序 | 不透明→半透明（奥→手前）の順で描画 |
| ソート | カメラからの距離で降順ソート |
| OIT | ソート不要の高度な透過手法 |

---

## ✏️ ドリル問題

### 問題1: 穴埋め
標準的なアルファブレンディングの設定を完成させてください。

```cpp
glEnable(______);
glBlendFunc(______, ______);
```

<details><summary>📝 解答</summary>

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

</details>

---

### 問題2: 計算問題
以下の条件でブレンド結果のRGB値を計算してください。

- Source: `(1.0, 0.0, 0.0)`, α = 0.5
- Destination: `(0.0, 1.0, 0.0)`
- `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)`

<details><summary>📝 解答</summary>

$$R = 1.0 \times 0.5 + 0.0 \times 0.5 = 0.5$$
$$G = 0.0 \times 0.5 + 1.0 \times 0.5 = 0.5$$
$$B = 0.0 \times 0.5 + 0.0 \times 0.5 = 0.0$$

結果: `(0.5, 0.5, 0.0)` — **黄色** になります。赤と緑が半々で混ざった色です。

</details>

---

### 問題3: 選択問題
半透明オブジェクトの正しい描画順序は？

- (A) 手前から奥に描画
- (B) 奥から手前に描画
- (C) 描画順序は関係ない
- (D) ランダムな順序で描画

<details><summary>📝 解答</summary>

**(B) 奥から手前に描画**

奥のオブジェクトを先に描画してカラーバッファに書き込んでおき、手前のオブジェクトを描画する際に、既にある奥の色とブレンドします。手前から描画すると、深度テストにより奥のオブジェクトが破棄されてしまいます。

</details>

---

### 問題4: 穴埋め（GLSL）
アルファ値が 0.1 未満のフラグメントを破棄するシェーダコードを完成させてください。

```glsl
void main()
{
    vec4 texColor = texture(texture1, TexCoords);
    if (______)
        ______;
    FragColor = texColor;
}
```

<details><summary>📝 解答</summary>

```glsl
void main()
{
    vec4 texColor = texture(texture1, TexCoords);
    if (texColor.a < 0.1)
        discard;
    FragColor = texColor;
}
```

`discard` はフラグメントを完全に破棄します。深度バッファやステンシルバッファにも書き込まれません。

</details>

---

### 問題5: 選択問題
透明テクスチャのラッピングモードに `GL_REPEAT` を使うと何が起きますか？

- (A) テクスチャが正しく繰り返される
- (B) テクスチャが引き伸ばされる
- (C) エッジ付近で隣の色が混入し、半透明の縁が見える
- (D) テクスチャが黒くなる

<details><summary>📝 解答</summary>

**(C) エッジ付近で隣の色が混入し、半透明の縁が見える**

テクスチャフィルタリングにより、テクスチャの端で反対側のテクセルの色が補間されます。透明な領域の隣に不透明な色があると、半透明の帯が見えます。`GL_CLAMP_TO_EDGE` で防げます。

</details>

---

### 問題6: 記述問題
`glBlendFunc(GL_ONE, GL_ONE)` を設定するとどのような効果が得られますか？どのような場面で使われますか？

<details><summary>📝 解答</summary>

**加算ブレンディング** になります。方程式は：

$$C_{result} = C_{source} \times 1 + C_{destination} \times 1$$

ソースとデスティネーションの色がそのまま加算されるため、明るくなる効果があります。**パーティクルエフェクト**（炎、光の粒子、グロー効果）でよく使われます。加算されるため、多くの粒子が重なると白くなり、発光しているように見えます。

</details>

---

## 🔨 実践課題

### 課題1: 草テクスチャの描画 ⭐
`discard` を使って、四角形ポリゴンに貼り付けた草テクスチャの透明部分を除去して表示してください。

**チェックポイント:**
- [ ] RGBA のPNGテクスチャを `GL_RGBA` で読み込み
- [ ] フラグメントシェーダで `discard` を実装
- [ ] 複数の草を異なる位置に配置
- [ ] `GL_CLAMP_TO_EDGE` を設定して縁のアーティファクトを防止

---

### 課題2: 半透明ウィンドウの正しい描画 ⭐⭐
異なる距離に配置した半透明の窓を、正しいブレンディングで描画してください。

**チェックポイント:**
- [ ] 不透明オブジェクト（床・キューブ）を先に描画
- [ ] 半透明の窓をカメラからの距離でソート
- [ ] 奥から手前の順に描画
- [ ] カメラを移動しても正しい透過順序が維持される
- [ ] ソート順を逆にして（手前→奥）描画し、問題を視覚的に確認

---

### 課題3: ブレンドモードの比較デモ ⭐⭐⭐
1つのシーンで複数のブレンドモードの効果を同時に比較表示してください。

**チェックポイント:**
- [ ] 画面を4分割し、それぞれ異なるブレンドモードで描画
  - 標準アルファブレンド（`GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA`）
  - 加算ブレンド（`GL_ONE, GL_ONE`）
  - 乗算ブレンド（`GL_DST_COLOR, GL_ZERO`）
  - スクリーンブレンド（`GL_ONE, GL_ONE_MINUS_SRC_COLOR`）
- [ ] 各モードの違いが視覚的に分かるシーン設計
- [ ] キー入力でモードを切り替えるUIも用意

---

## 🔗 ナビゲーション

⬅️ [ステンシルテスト](./02-stencil-testing.md) | ➡️ [フェイスカリング →](./04-face-culling.md)
