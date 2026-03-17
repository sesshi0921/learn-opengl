# 📘 色（Colors）

> **目標：** 現実世界の光と色の関係を理解し、OpenGLでの色の表現方法と計算を学び、ライティングの基礎となるシーンをセットアップできるようになる。

---

## 📖 現実世界の色と光

### 色はどうやって見えるのか？

私たちが日常で「色」と呼んでいるものは、実は**光の反射**によって生まれます。太陽やライトから出た光（白色光）は、あらゆる色の波長を含んでいます。物体に光が当たると、物体はその素材に応じて**特定の波長を吸収**し、**残りを反射**します。反射された光が私たちの目に届き、「色」として認識されるのです。

```
白色光（全波長）
    │
    ▼
┌──────────┐
│  赤い物体  │  ← 赤以外の波長を吸収
└──────────┘
    │
    ▼ 赤い光だけ反射
   👁️ 「赤く見える！」
```

例えば：
- **赤いリンゴ** → 赤い波長を反射、他を吸収
- **白い壁** → ほぼすべての波長を反射（何も吸収しない）
- **黒い服** → ほぼすべての波長を吸収（何も反射しない）

### 反射率という考え方

物体の「色」は、各色成分の**反射率**として表現できます。赤いリンゴの場合：

| 色成分 | 入射光 | 反射率 | 反射光 |
|--------|--------|--------|--------|
| 赤 (R) | 1.0 | 0.9 | 0.9 |
| 緑 (G) | 1.0 | 0.1 | 0.1 |
| 青 (B) | 1.0 | 0.1 | 0.1 |

結果として `(0.9, 0.1, 0.1)` の色が目に届く → 赤っぽく見えます。

---

## 📖 OpenGLでの色の表現

### RGB ベクトル

OpenGLでは色を **0.0〜1.0 の範囲**の3成分ベクトルで表現します：

```cpp
glm::vec3 coral(1.0f, 0.5f, 0.31f);  // サンゴ色
glm::vec3 white(1.0f, 1.0f, 1.0f);   // 白
glm::vec3 black(0.0f, 0.0f, 0.0f);   // 黒
glm::vec3 red(1.0f, 0.0f, 0.0f);     // 純粋な赤
```

一般的に使われる 0〜255 の整数表現とは異なり、OpenGLでは浮動小数点で正規化された値を使います。変換は単純に 255 で割るだけです：

```
RGB(255, 128, 79) → vec3(1.0, 0.502, 0.310)
```

### 色の成分ごとの乗算

OpenGLでのライティング計算の核心は**色の成分ごとの乗算（component-wise multiplication）**です。光の色と物体の色を掛け合わせることで、実際に見える色が計算されます：

```
見える色 = 光の色 × 物体の色
```

これを数式で表すと：

```
結果.R = 光.R × 物体.R
結果.G = 光.G × 物体.G
結果.B = 光.B × 物体.B
```

---

## 📖 光と色の計算 — 具体例

### 例1: 白い光 × サンゴ色の物体

```
光の色:   (1.0, 1.0, 1.0)  ← 白色光
物体の色: (1.0, 0.5, 0.31) ← サンゴ色

結果 = (1.0×1.0, 1.0×0.5, 1.0×0.31)
     = (1.0, 0.5, 0.31)    ← サンゴ色のまま
```

白い光はすべてを同じ割合で照らすので、物体本来の色が見えます。

### 例2: 緑の光 × サンゴ色の物体

```
光の色:   (0.0, 1.0, 0.0)  ← 緑色の光
物体の色: (1.0, 0.5, 0.31) ← サンゴ色

結果 = (0.0×1.0, 1.0×0.5, 0.0×0.31)
     = (0.0, 0.5, 0.0)     ← 暗い緑色！
```

緑の光には赤と青の成分がないため、物体の赤と青の反射はゼロになります。

### 例3: 暗いオリーブ色の光 × サンゴ色の物体

```
光の色:   (0.33, 0.42, 0.18) ← 暗いオリーブ色
物体の色: (1.0,  0.5,  0.31) ← サンゴ色

結果 = (0.33×1.0, 0.42×0.5, 0.18×0.31)
     = (0.33, 0.21, 0.06)    ← 暗い茶色っぽい色
```

### GLSL での計算

GLSLではベクトル同士の乗算が成分ごとに行われるため、コードは非常にシンプルです：

```glsl
vec3 lightColor  = vec3(1.0, 1.0, 1.0);
vec3 objectColor = vec3(1.0, 0.5, 0.31);
vec3 result = lightColor * objectColor; // (1.0, 0.5, 0.31)
```

---

## 📖 ライティングシーンのセットアップ

### 基本構成

ライティングを学ぶために、最小限のシーンを構築します：

```
┌─────────────────────────────┐
│         シーン               │
│                             │
│   ┌───┐          ╔═══╗     │
│   │   │  ←光→    ║   ║     │
│   │   │          ║   ║     │
│   └───┘          ╚═══╝     │
│  物体(サンゴ色)   光源(白)   │
│                             │
└─────────────────────────────┘
```

2つの立方体を配置します：
1. **物体（Object）** — ライティングの影響を受けるサンゴ色の立方体
2. **光源（Light Source）** — 光の位置を可視化する白い立方体

### 頂点データ

両方の立方体は同じ頂点データを共有します（36頂点 = 12三角形 × 3頂点/三角形）：

```cpp
float vertices[] = {
    // 位置データのみ（法線は後のチャプターで追加）
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    // ... 残り5面分（計36頂点）
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    // ... 上下左右面
};
```

---

## 📖 光源用のシェーダーと VAO

### なぜ分けるのか？

物体と光源には**異なるシェーダー**が必要です：
- **物体** → ライティング計算を行うシェーダー（色が光に影響される）
- **光源** → 常に白を出力するシンプルなシェーダー（光源自体は暗くならない）

VAO も分離することで、後からそれぞれの頂点属性設定を独立に変更できます。ただし VBO（頂点データ）は共通のものを使いまわせます。

### 物体用シェーダー

**バーテックスシェーダー（object.vs）：**

```glsl
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

**フラグメントシェーダー（object.fs）：**

```glsl
#version 330 core
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;

void main()
{
    FragColor = vec4(lightColor * objectColor, 1.0);
}
```

この時点では単純に `lightColor * objectColor` を出力するだけです。まだ本格的なライティング計算はしていませんが、光の色が物体の見た目に影響を与える基本原理を確認できます。

### 光源用シェーダー

**バーテックスシェーダー（light.vs）：**

```glsl
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

**フラグメントシェーダー（light.fs）：**

```glsl
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0); // 常に白 — 光源自体は明るいまま
}
```

光源のフラグメントシェーダーは uniform を受け取らず、常に `vec4(1.0)` = 白を出力します。

---

## 📖 VAO とレンダリングの分離

### 光源用 VAO の作成

```cpp
// ----- 物体用 VAO -----
unsigned int cubeVAO;
glGenVertexArrays(1, &cubeVAO);
glBindVertexArray(cubeVAO);
glBindBuffer(GL_ARRAY_BUFFER, VBO); // 共通の VBO
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

// ----- 光源用 VAO -----
unsigned int lightVAO;
glGenVertexArrays(1, &lightVAO);
glBindVertexArray(lightVAO);
glBindBuffer(GL_ARRAY_BUFFER, VBO); // 同じ VBO を再利用
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);
```

VBO のデータは同じですが、VAO を分けることで将来的に頂点属性を独立に変更できます。

### レンダリングループ

```cpp
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

while (!glfwWindowShouldClose(window))
{
    // --- 物体の描画 ---
    objectShader.use();
    objectShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
    objectShader.setVec3("lightColor",  1.0f, 1.0f, 1.0f);

    glm::mat4 model = glm::mat4(1.0f);
    objectShader.setMat4("model", model);
    objectShader.setMat4("view", view);
    objectShader.setMat4("projection", projection);

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // --- 光源の描画 ---
    lightShader.use();
    model = glm::mat4(1.0f);
    model = glm::translate(model, lightPos);
    model = glm::scale(model, glm::vec3(0.2f)); // 小さくする
    lightShader.setMat4("model", model);
    lightShader.setMat4("view", view);
    lightShader.setMat4("projection", projection);

    glBindVertexArray(lightVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

光源は `translate` で位置を設定し、`scale(0.2f)` で小さな立方体として表示します。

---

## 💡 ポイントまとめ

| 項目 | 説明 |
|------|------|
| 色の物理的な仕組み | 物体は特定の波長を吸収し、残りを反射する |
| OpenGLでの色表現 | RGB各成分を0.0〜1.0の `vec3` で表す |
| 見える色の計算 | `光の色 × 物体の色`（成分ごとの乗算） |
| 白い光 | `(1,1,1)` — 物体本来の色がそのまま見える |
| 有色光 | 光に含まれない色成分は反射できない |
| ライティングシーンの構成 | 光を受ける物体 + 光の位置を示す光源オブジェクト |
| シェーダーの分離 | 物体用（ライティング計算あり）と光源用（常に白）|
| VAOの分離 | 同じVBOを共有しつつ、VAOは別々に作成する |
| 光源の表示 | 小さな白い立方体として描画（`scale(0.2f)`） |

---

## ✏️ ドリル問題

### 問1: 色の成分乗算

光の色が `(0.0, 0.5, 1.0)` で物体の色が `(0.8, 0.6, 0.2)` のとき、見える色を計算せよ。

<details><summary>📝 解答</summary>

```
結果.R = 0.0 × 0.8 = 0.0
結果.G = 0.5 × 0.6 = 0.3
結果.B = 1.0 × 0.2 = 0.2

見える色 = (0.0, 0.3, 0.2)
```

赤成分は光に含まれないため完全に消え、青緑っぽい暗い色になります。

</details>

### 問2: シェーダー穴埋め

以下の物体用フラグメントシェーダーの空欄を埋めよ：

```glsl
#version 330 core
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;

void main()
{
    FragColor = vec4(______ * ______, 1.0);
}
```

<details><summary>📝 解答</summary>

```glsl
FragColor = vec4(lightColor * objectColor, 1.0);
```

光の色と物体の色を成分ごとに乗算します。GLSLではベクトル同士の `*` が成分ごとの乗算になります。

</details>

### 問3: 概念問題

光源用フラグメントシェーダーが `FragColor = vec4(1.0);` としている理由を説明せよ。

<details><summary>📝 解答</summary>

光源オブジェクトは「光を発している物体」を可視化するためのものです。光源自体がライティングの影響を受けて暗くなってしまっては、光源の位置がわかりにくくなります。そのため常に白（`vec4(1.0)` = `vec4(1.0, 1.0, 1.0, 1.0)`）を出力し、明るい状態を保ちます。

</details>

### 問4: 色の損失

純粋な赤い光 `(1.0, 0.0, 0.0)` で青い物体 `(0.0, 0.0, 1.0)` を照らすと何色に見えるか？ 理由も述べよ。

<details><summary>📝 解答</summary>

```
結果 = (1.0×0.0, 0.0×0.0, 0.0×1.0) = (0.0, 0.0, 0.0)
```

**黒（何も見えない）** になります。赤い光には青の成分がなく、青い物体は赤の成分を反射しないため、どの成分もゼロになり光が全く反射されません。

</details>

### 問5: VAO の設計

物体と光源で同じ VBO を共有しつつ VAO を分ける利点を述べよ。

<details><summary>📝 解答</summary>

- **メモリ効率**: 頂点データ（VBO）を二重に持つ必要がない
- **柔軟性**: 将来、物体に法線やテクスチャ座標を追加しても、光源のVAOには影響しない
- **独立性**: 頂点属性の設定（stride, offset, 有効化するattribute）を物体と光源で独立に管理できる

</details>

### 問6: RGB変換

画像編集ソフトで色が `RGB(51, 178, 255)` と表示されている。GLSLで使える `vec3` に変換せよ。

<details><summary>📝 解答</summary>

```
R = 51  / 255 ≈ 0.2
G = 178 / 255 ≈ 0.698
B = 255 / 255 = 1.0
```

```glsl
vec3 color = vec3(0.2, 0.698, 1.0);
```

</details>

---

## 🔨 実践課題

### 課題1: 光の色を変えてみよう ⭐

光の色を変えて、物体の見た目がどう変化するか観察する。

**手順：**
1. `lightColor` を `(1.0, 0.0, 0.0)` に変更 → 物体の色を確認
2. `lightColor` を `(0.0, 1.0, 0.0)` に変更 → 物体の色を確認
3. `lightColor` を `(0.5, 0.5, 0.0)` に変更 → 物体の色を確認

**チェックポイント：**
- [ ] 赤い光では物体の緑・青成分が消える
- [ ] 緑の光では物体の赤・青成分が消える
- [ ] 黄色っぽい光では青成分のみ消える

### 課題2: 時間で変化する光 ⭐⭐

`glfwGetTime()` と `sin`/`cos` を使って光の色を時間経過で変化させる。

```cpp
float time = glfwGetTime();
glm::vec3 lightColor;
lightColor.x = sin(time * 2.0f);
lightColor.y = sin(time * 0.7f);
lightColor.z = sin(time * 1.3f);
```

**チェックポイント：**
- [ ] フレームごとに光の色が滑らかに変化する
- [ ] 物体の色も光に応じて変化する
- [ ] sin の値が負になる場合の処理を考えた（`max(0, sin(...))` など）

### 課題3: 複数の光源 ⭐⭐⭐

シーンに光源を3つ配置し、それぞれ異なる色（赤・緑・青）の立方体として表示する。

**手順：**
1. `lightPos` を配列に拡張（3箇所）
2. 各光源に対して `lightShader` で描画
3. 光源の色に対応した `vec3` を光源シェーダーに渡す

**チェックポイント：**
- [ ] 3つの異なる色の光源が表示される
- [ ] 各光源が異なる位置に配置されている
- [ ] 光源の描画ループが正しく実装されている

---

## 🔗 ナビゲーション

⬅️ [入門編まとめ](../02-getting-started/10-review.md) | ➡️ [基本的なライティング →](./02-basic-lighting.md)
