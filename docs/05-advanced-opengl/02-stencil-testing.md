# ステンシルテスト（Stencil Testing）

> **目標：** ステンシルバッファの仕組みを理解し、オブジェクトアウトライン（枠線エフェクト）をステップバイステップで実装できるようになる。

---

## ステンシルバッファとは

ステンシルバッファは、各ピクセルに対応する **8ビット整数値（0〜255）** を格納するバッファです。「ステンシル（stencil）」は「型紙」という意味で、特定のピクセルに対する描画の許可・禁止を制御する **マスク** として機能します。

```
画面上の各ピクセル：

┌─────┬─────┬─────┬─────┐
│ R G │ R G │ R G │ R G │  カラーバッファ（色）
│ B A │ B A │ B A │ B A │
├─────┼─────┼─────┼─────┤
│ Z   │ Z   │ Z   │ Z   │  深度バッファ（奥行き）
├─────┼─────┼─────┼─────┤
│ S   │ S   │ S   │ S   │  ステンシルバッファ（マスク）
│ 0   │ 1   │ 1   │ 0   │  ← 8ビット整数（0〜255）
└─────┴─────┴─────┴─────┘
```

**直感的に言えば：** ステンシルバッファは画面上に「見えない型紙」を置くようなものです。穴が開いている場所（テスト通過）にだけ描画し、穴のない場所（テスト不合格）では描画をスキップします。

**テスト順序：** ステンシルテスト → 深度テスト → カラー書き込み の順で実行されます。ステンシルテストに落ちたフラグメントは深度テストにも進みません。

---

## ステンシルテストの有効化

```cpp
// ステンシルテストを有効化
glEnable(GL_STENCIL_TEST);

// 毎フレームのクリア（カラー + 深度 + ステンシル）
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
```

ステンシルバッファのクリア値はデフォルトで `0` です。

---

## glStencilFunc — テスト条件の設定

```cpp
glStencilFunc(GLenum func, GLint ref, GLuint mask);
```

**3つのパラメータ：**

| パラメータ | 意味 | 例 |
|-----------|------|-----|
| `func` | 比較関数（深度テスト関数と同じ種類） | `GL_EQUAL`, `GL_NOTEQUAL` 等 |
| `ref` | 比較に使う参照値 | `1` |
| `mask` | 比較前に両方の値にANDされるマスク | `0xFF`（全ビット有効） |

**テスト式：** `(ref & mask) func (stencil & mask)`

```cpp
// 例：ステンシル値が1のピクセルにのみ描画を許可
glStencilFunc(GL_EQUAL, 1, 0xFF);

// 例：ステンシル値が0でないピクセルにのみ描画
glStencilFunc(GL_NOTEQUAL, 0, 0xFF);

// 例：常にテスト通過（ステンシル値を書き込むだけの目的）
glStencilFunc(GL_ALWAYS, 1, 0xFF);
```

### 利用可能な比較関数

| 関数 | テスト通過条件 |
|------|--------------|
| `GL_NEVER` | 常に不合格 |
| `GL_ALWAYS` | 常に通過 |
| `GL_LESS` | `(ref & mask) < (stencil & mask)` |
| `GL_LEQUAL` | `(ref & mask) <= (stencil & mask)` |
| `GL_GREATER` | `(ref & mask) > (stencil & mask)` |
| `GL_GEQUAL` | `(ref & mask) >= (stencil & mask)` |
| `GL_EQUAL` | `(ref & mask) == (stencil & mask)` |
| `GL_NOTEQUAL` | `(ref & mask) != (stencil & mask)` |

---

## glStencilOp — テスト結果に応じた更新操作

```cpp
glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
```

**3つの状況に対してそれぞれアクションを指定：**

| パラメータ | 状況 |
|-----------|-----|
| `sfail` | **ステンシルテスト不合格** |
| `dpfail` | ステンシルテスト通過、**深度テスト不合格** |
| `dppass` | ステンシルテスト通過、**深度テスト通過**（両方成功） |

### 6つのアクション

| アクション | 動作 |
|-----------|------|
| `GL_KEEP` | 現在のステンシル値をそのまま保持 |
| `GL_ZERO` | ステンシル値を `0` に設定 |
| `GL_REPLACE` | ステンシル値を `glStencilFunc` の `ref` 値に置換 |
| `GL_INCR` | ステンシル値を +1（最大255でクランプ） |
| `GL_DECR` | ステンシル値を -1（最小0でクランプ） |
| `GL_INVERT` | 全ビットを反転（ビット単位NOT） |

```cpp
// デフォルト設定（何も更新しない）
glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

// 一般的な設定（テスト通過時にref値を書き込む）
glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
```

---

## glStencilMask — 書き込みマスク

ステンシルバッファへの書き込みをビット単位で制御します。

```cpp
// 全ビット書き込み可能（デフォルト）
glStencilMask(0xFF);

// 書き込み完全禁止（読み取り専用）
glStencilMask(0x00);
```

**`glDepthMask` のステンシル版** と考えてください。テスト自体は行われますが、バッファへの更新だけを制御します。

---

## オブジェクトアウトラインの実装

ステンシルテストの最も人気のある応用が **オブジェクトアウトライン** です。選択されたオブジェクトに色付きの枠線を表示するエフェクトです。

### アルゴリズムの概要

```
ステップ1: ステンシル書き込み有効      ステップ2: 拡大した単色描画
                                    （ステンシル外のみ）
┌─────────────────┐               ┌─────────────────┐
│                 │               │                 │
│   ┌─────────┐  │               │  ╔═══════════╗  │
│   │ステンシル│  │    ───→      │  ║  ■■■■■■■ ║  │
│   │  =1     │  │               │  ║  ■CUBE ■ ║  │
│   │(CUBE描画)│  │               │  ║  ■■■■■■■ ║  │
│   └─────────┘  │               │  ╚═══════════╝  │
│                 │               │  ↑ アウトライン  │
└─────────────────┘               └─────────────────┘
 ステンシル値: 0=背景, 1=CUBE         ステンシル≠1の部分だけ描画
```

### ステップバイステップ手順

```
手順1: ステンシルテストを有効化
手順2: 通常のオブジェクトを描画（ステンシルに1を書き込む）
手順3: ステンシル書き込みを無効化
手順4: オブジェクトを少しスケールアップして単色で描画
       （ステンシル値が1でない部分のみ）
手順5: ステンシルを復元
```

### 完全な実装コード

#### 単色シェーダ（アウトライン用）

```glsl
// outline_vertex.glsl
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
```

```glsl
// outline_fragment.glsl
#version 330 core
out vec4 FragColor;

uniform vec3 outlineColor;

void main()
{
    FragColor = vec4(outlineColor, 1.0);
}
```

#### C++ 描画コード

```cpp
// ====== 初期設定 ======
glEnable(GL_DEPTH_TEST);
glEnable(GL_STENCIL_TEST);
// ステンシルテスト不合格時 → 何もしない
// 深度テスト不合格時 → 何もしない
// 両方通過時 → ref値を書き込む
glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

// ====== 描画ループ ======
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

// --- 手順1: 床を描画（ステンシルに書き込まない） ---
glStencilMask(0x00);  // ステンシル書き込み禁止
drawFloor();

// --- 手順2: オブジェクトを通常描画（ステンシルに1を書き込む） ---
glStencilFunc(GL_ALWAYS, 1, 0xFF);  // 常に通過、ref=1
glStencilMask(0xFF);                // 書き込み有効
drawCubes();  // 通常シェーダで描画

// --- 手順3: アウトラインを描画 ---
glStencilFunc(GL_NOTEQUAL, 1, 0xFF);  // ステンシル値≠1の部分のみ
glStencilMask(0x00);                   // 書き込み禁止
glDisable(GL_DEPTH_TEST);              // 深度テスト無効（常に前面に）

outlineShader.use();
outlineShader.setVec3("outlineColor", glm::vec3(1.0f, 0.5f, 0.0f)); // オレンジ

// 少しスケールアップして描画
float scale = 1.05f;
for (auto& cube : cubes)
{
    glm::mat4 model = cube.modelMatrix;
    model = glm::scale(model, glm::vec3(scale));
    outlineShader.setMat4("model", model);
    cube.draw();
}

// --- 手順4: 状態を復元 ---
glStencilMask(0xFF);
glStencilFunc(GL_ALWAYS, 0, 0xFF);
glEnable(GL_DEPTH_TEST);
```

### なぜこれで動くのか？

1. 通常描画でキューブのピクセルにステンシル値 `1` が書き込まれる
2. スケールアップした描画は `GL_NOTEQUAL, 1` なので、ステンシル値が `1` **でない** 部分だけ描画される
3. スケールアップにより元のキューブより少し大きいので、差分が「枠」として見える
4. 深度テストを無効にすることで、アウトラインが他のオブジェクトに隠れない

```
断面図で見ると：

元のキューブ:     │████████│
スケールアップ:  │▓▓████████▓▓│
アウトライン:    │▓▓│      │▓▓│  ← ステンシル≠1 の部分だけ見える
```

---

## スケーリングの注意点

単純な `glm::scale` だと、オブジェクトの形状によってはアウトラインの太さが均一になりません。

```
均一スケール:                法線方向へのオフセット:
┌───────────────┐           ┌───────────────┐
│ ┌───────────┐ │   vs.     │ ┌───────────┐ │
│ │           │ │           │ │           │ │
│ │   CUBE    │ │           │ │   CUBE    │ │
│ │           │ │           │ │           │ │
│ └───────────┘ │           │ └───────────┘ │
└───────────────┘           └───────────────┘
  太さが均一               より正確（頂点シェーダで）
```

より高品質なアウトラインには、頂点シェーダで **法線方向に頂点を押し出す** 方法があります。

```glsl
// 法線方向へのオフセット（頂点シェーダ）
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float outlineWidth;

void main()
{
    vec3 pos = aPos + aNormal * outlineWidth;
    gl_Position = projection * view * model * vec4(pos, 1.0);
}
```

---

## 💡 ポイントまとめ

| 項目 | 内容 |
|------|------|
| ステンシルバッファ | 各ピクセルに8ビット整数（0〜255）のマスク値 |
| テスト順序 | ステンシルテスト → 深度テスト → カラー書き込み |
| 有効化 | `glEnable(GL_STENCIL_TEST)` |
| クリア | `glClear(GL_STENCIL_BUFFER_BIT)` |
| `glStencilFunc` | テスト条件を設定（func, ref, mask） |
| `glStencilOp` | テスト結果でステンシル値をどう更新するか |
| `glStencilMask` | 書き込みのビットマスク。`0x00`で書き込み禁止 |
| `GL_REPLACE` | ref値でステンシルバッファを上書き |
| アウトライン手順 | ①通常描画(S=1) → ②スケールアップ単色(S≠1のみ) |
| 深度テスト無効 | アウトライン描画時は `glDisable(GL_DEPTH_TEST)` |

---

## ドリル問題

### 問題1: 穴埋め
ステンシルテストを使い、ステンシル値が `1` のピクセルにのみ描画を許可する設定を完成させてください。

```cpp
glStencilFunc(______, ______, 0xFF);
```

<details><summary> 解答</summary>

```cpp
glStencilFunc(GL_EQUAL, 1, 0xFF);
```

`GL_EQUAL` は `(ref & mask) == (stencil & mask)` でテストします。ref=1, mask=0xFF なので、ステンシルバッファの値が `1` のピクセルだけ通過します。

</details>

---

### 問題2: 選択問題
`glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE)` の設定で、ステンシルテストは通過したが深度テストに不合格だった場合、ステンシルバッファはどうなりますか？

- (A) ref値に置き換わる
- (B) 0になる
- (C) 何も変化しない
- (D) ビットが反転する

<details><summary> 解答</summary>

**(C) 何も変化しない**

`glStencilOp` の3つのパラメータは：
1. `sfail`（ステンシル不合格）= `GL_KEEP`
2. `dpfail`（ステンシル通過・深度不合格）= `GL_KEEP` ← **この状況**
3. `dppass`（両方通過）= `GL_REPLACE`

2番目のパラメータ `dpfail` が `GL_KEEP` なので、ステンシル値は変更されません。

</details>

---

### 問題3: 穴埋め
オブジェクトアウトラインの描画で、スケールアップしたオブジェクトを描画する際のステンシル設定を完成させてください。

```cpp
// ステンシル値が1でない場所にだけ描画
glStencilFunc(______, 1, 0xFF);
// ステンシルバッファへの書き込みを禁止
glStencilMask(______);
```

<details><summary> 解答</summary>

```cpp
glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
glStencilMask(0x00);
```

`GL_NOTEQUAL` により、ステンシル値が `1` **でない** ピクセルだけにアウトラインが描画されます。これにより、元のオブジェクトの内側は描画されず、「枠」だけが残ります。

</details>

---

### 問題4: 並べ替え問題
オブジェクトアウトラインを実装する手順を正しい順番に並べてください。

- (a) ステンシル書き込みを無効にして、`GL_NOTEQUAL` で単色描画
- (b) `glStencilFunc(GL_ALWAYS, 1, 0xFF)` で通常オブジェクトを描画
- (c) ステンシルと深度テストの設定を復元
- (d) 深度テストを無効にする

<details><summary> 解答</summary>

**(b) → (d) → (a) → (c)**

1. **(b)** 通常描画でステンシルに `1` を書き込む
2. **(d)** 深度テストを無効にする（アウトラインが隠れないように）
3. **(a)** ステンシル値 ≠ 1 の部分のみスケールアップした単色を描画
4. **(c)** 設定を元に戻す

</details>

---

### 問題5: 選択問題
`glStencilMask(0x00)` を設定した場合、正しい説明はどれですか？

- (A) ステンシルテストが無効化される
- (B) ステンシルバッファの読み取りができなくなる
- (C) ステンシルバッファへの書き込みが禁止される
- (D) ステンシルバッファが0でクリアされる

<details><summary> 解答</summary>

**(C) ステンシルバッファへの書き込みが禁止される**

`glStencilMask` はビットマスクで、書き込み時に AND されます。`0x00` は全ビット 0 なので、どんな値を書き込もうとしても実際には反映されません。テスト自体は引き続き実行されます。

</details>

---

### 問題6: 記述問題
アウトラインの描画時に `glDisable(GL_DEPTH_TEST)` を行う理由を説明してください。

<details><summary> 解答</summary>

深度テストが有効なままだと、アウトラインが他のオブジェクト（例えば床）に隠れてしまう場合があります。アウトラインは常にオブジェクトの周囲に完全に見えるべきなので、深度テストを無効にして、既存の深度値に関係なく描画します。

</details>

---

## 実践課題

### 課題1: 基本アウトライン 
2つのキューブにオレンジ色のアウトラインを描画してください。

**チェックポイント:**
- [ ] ステンシルテストと深度テストを両方有効化
- [ ] 通常描画でステンシルに `1` を書き込み
- [ ] スケールアップ（1.05倍）した単色描画で枠線を表示
- [ ] 床にはアウトラインがつかないことを確認

---

### 課題2: 選択オブジェクトのハイライト 
マウスカーソルに最も近いオブジェクトだけにアウトラインを表示する仕組みを実装してください。

**チェックポイント:**
- [ ] 複数オブジェクトをシーンに配置
- [ ] カメラからのレイキャストまたは距離計算で選択オブジェクトを決定
- [ ] 選択されたオブジェクトのみにアウトラインを適用
- [ ] フレームごとに選択が更新される

---

### 課題3: カスタムステンシルエフェクト 
ステンシルバッファを使って「ポータル（窓）」エフェクトを作ってください。画面上の四角形の中にだけ別のシーンが見えるようにします。

**チェックポイント:**
- [ ] 四角形を描画してステンシルに書き込む（カラーバッファには書かない）
- [ ] ステンシル値 = 1 の場所にだけ「別シーン」を描画
- [ ] ステンシル値 ≠ 1 の場所に通常シーンを描画
- [ ] ポータルの位置を移動できるようにする

---

## ナビゲーション

 [深度テスト](./01-depth-testing.md) | [ブレンディング →](./03-blending.md)
