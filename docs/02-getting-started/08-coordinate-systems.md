# 📘 入門編 8：座標系（Coordinate Systems）

> **目標：** OpenGL の 5 つの座標空間と、Model/View/Projection 行列を理解する

---

## 📖 5 つの座標空間

OpenGL で 3D オブジェクトが画面に表示されるまでに、**5 つの座標空間** を経由します。

```
オブジェクト空間（Local Space）
         ↓  Model 行列
  ワールド空間（World Space）
         ↓  View 行列
   ビュー空間（View Space / Eye Space）
         ↓  Projection 行列
   クリップ空間（Clip Space）
         ↓  透視除算（w で割る）
NDC 空間（Normalized Device Coordinates）
         ↓  ビューポート変換
  スクリーン空間（Screen Space）
```

---

## 📖 各座標空間の説明

### 1. オブジェクト空間（Local Space）
オブジェクト自身のローカルな座標系。3D モデルを作成した際の座標のこと。

```
例：キャラクターの足元が (0, 0, 0) で
    頭が (0, 2, 0) など
```

### 2. ワールド空間（World Space）
シーン全体の座標系。**Model 行列** でオブジェクト空間から変換。

```cpp
glm::mat4 model = glm::mat4(1.0f);
model = glm::translate(model, glm::vec3(3.0f, 0.0f, 0.0f));  // ワールドの (3,0,0) に配置
model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0, 1, 0));  // 向きを変える
```

### 3. ビュー空間（View Space / Camera Space）
カメラの視点を原点とした座標系。**View 行列** でワールド空間から変換。  
「カメラを動かす」= 「ワールド全体を逆に動かす」

```cpp
// カメラが (0, 0, 3) にあり、原点を見ている
glm::mat4 view = glm::lookAt(
    glm::vec3(0.0f, 0.0f, 3.0f),   // カメラの位置
    glm::vec3(0.0f, 0.0f, 0.0f),   // 注視点
    glm::vec3(0.0f, 1.0f, 0.0f)    // 上方向（Y 軸が上）
);
```

### 4. クリップ空間（Clip Space）
**Projection 行列** でビュー空間から変換。  
視錘台（View Frustum）内に収まるものだけが残る。

### 5. NDC 空間
w 成分で割る「透視除算」により -1.0〜1.0 の範囲に正規化。

---

## 📖 透視投影（Perspective Projection）

遠いものが小さく見える「遠近感」のある投影方法。

```
視錘台（Frustum）の形：
近クリップ面（近くの切り取り面）
│    ╱ ╲
│   ╱   ╲
│  ╱     ╲
│ ╱       ╲
│╱         ╲
遠クリップ面（遠くの切り取り面）

fov  = 垂直視野角（例：45度）
near = 近クリップ面の距離
far  = 遠クリップ面の距離
```

```cpp
// 透視投影行列
glm::mat4 projection = glm::perspective(
    glm::radians(45.0f),        // 垂直視野角（fov）
    (float)width / (float)height,  // アスペクト比
    0.1f,                        // 近クリップ面
    100.0f                       // 遠クリップ面
);
```

---

## 📖 正射影（Orthographic Projection）

遠近感のない平行投影。2D ゲーム・UI・CAD に使用。

```
正射影の視体（直方体）：
┌──────┐
│      │ ← 遠くでも近くでも同じ大きさ
│      │
└──────┘
```

```cpp
// 正射影行列
glm::mat4 projection = glm::ortho(
    0.0f, (float)width,   // left, right
    0.0f, (float)height,  // bottom, top
    -1.0f, 1.0f           // near, far
);
```

---

## 📖 MVP 行列

最終的な変換式：

```
クリップ座標 = Projection × View × Model × 頂点座標
```

```glsl
// 頂点シェーダー
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
```

```cpp
// CPU 側
shader.setMat4("model",      model);
shader.setMat4("view",       view);
shader.setMat4("projection", projection);

// setMat4 の実装例
void setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()),
                       1, GL_FALSE, glm::value_ptr(mat));
}
```

---

## 📖 3D の立方体を描く

```cpp
// 立方体の 36 頂点（6面 × 2三角形 × 3頂点）
float vertices[] = {
    // 前面
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
    // ... 残り 5 面
};
```

### 複数の立方体を並べる

```cpp
// 10 個の立方体の位置
glm::vec3 cubePositions[] = {
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    // ...
};

// 描画ループ
for (unsigned int i = 0; i < 10; i++) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, cubePositions[i]);
    float angle = 20.0f * i;
    model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
    shader.setMat4("model", model);

    glDrawArrays(GL_TRIANGLES, 0, 36);
}
```

---

## 📖 深度バッファ（Z-Buffer）

3D では奥にあるオブジェクトを手前のオブジェクトで隠す必要があります。これを **深度テスト** と呼びます。

```cpp
// 初期化時に深度テストを有効化
glEnable(GL_DEPTH_TEST);

// レンダリングループ内でカラーバッファと一緒に深度バッファもクリア
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

---

## 💡 ポイントまとめ

```
MVP 変換の流れ：
オブジェクト空間
    ×Model行列 → ワールド空間（位置・向きを決める）
    ×View行列  → ビュー空間（カメラ視点に変換）
    ×Proj行列  → クリップ空間（遠近感をつける）
    ÷w         → NDC（-1〜1 に正規化）
    →ビューポート → スクリーン座標
```

| 行列 | GLM 関数 | 役割 |
|------|----------|------|
| Model | `translate/rotate/scale` | オブジェクトをワールドに配置 |
| View | `lookAt` | カメラの視点に変換 |
| Projection | `perspective` / `ortho` | 遠近感・投影方法 |

---

## ✏️ ドリル問題

### 問題 1：座標空間の順序

5 つの座標空間を変換の順に並べなさい。

```
(A) ビュー空間
(B) スクリーン空間
(C) ワールド空間
(D) NDC 空間
(E) クリップ空間
(F) オブジェクト空間（Local Space）
```

<details>
<summary>📝 解答</summary>

**F → C → A → E → D → B**

オブジェクト → ワールド → ビュー → クリップ → NDC → スクリーン

</details>

---

### 問題 2：MVP 穴埋め

```glsl
// 頂点シェーダーの MVP 変換
gl_Position = 【 ① 】 * 【 ② 】 * 【 ③ 】 * vec4(aPos, 1.0);
//             投影        ビュー     モデル
```

<details>
<summary>📝 解答</summary>

① `projection`  
② `view`  
③ `model`

</details>

---

### 問題 3：perspective の引数

```cpp
glm::perspective(glm::radians(45.0f), 800.0f/600.0f, 0.1f, 100.0f)
//                      ①                   ②          ③     ④
```

各引数の意味を答えなさい。

<details>
<summary>📝 解答</summary>

① **垂直視野角（fov）**：45 度
② **アスペクト比**：幅/高さ = 800/600
③ **近クリップ面の距離**：0.1
④ **遠クリップ面の距離**：100.0

</details>

---

### 問題 4：深度テスト

深度テストを有効にしないと何が起きるか説明し、有効にするコードを書きなさい。

<details>
<summary>📝 解答</summary>

深度テストなし：後に描画されたオブジェクトが前に描画されたものを上書きするため、奥のオブジェクトが手前に見えたり、描画順序によって結果が変わったりする。

```cpp
// 有効化
glEnable(GL_DEPTH_TEST);

// ループ内でクリア（カラーと一緒に！）
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

</details>

---

### 問題 5：lookAt の引数

```cpp
glm::lookAt(
    glm::vec3(0.0f, 0.0f, 3.0f),  // ①
    glm::vec3(0.0f, 0.0f, 0.0f),  // ②
    glm::vec3(0.0f, 1.0f, 0.0f)   // ③
);
```

①②③ の意味を答えなさい。

<details>
<summary>📝 解答</summary>

① **カメラの位置（eye position）**：Z 軸の 3.0 の位置
② **注視点（target/center）**：原点 (0,0,0) を見ている
③ **上方向ベクトル（up vector）**：Y 軸方向が「上」

</details>

---

## 🔨 実践課題

### 課題 1：回転する立方体 ⭐⭐

立方体を描画し、時間とともに回転するようにしなさい。MVP 行列をすべて実装すること。

### 課題 2：10 個の立方体 ⭐⭐⭐

本章のコード例を参考に、10 個の立方体をシーンに配置しなさい。それぞれ異なる向きに回転させること。

### 課題 3：カメラを動かす ⭐⭐⭐

View 行列（`glm::lookAt`）のカメラ位置を時間とともに円軌道上で動かし、立方体を360度ぐるりと回り込むようなアニメーションを作りなさい。

```cpp
// ヒント：円軌道
float radius = 10.0f;
float camX = sin(glfwGetTime()) * radius;
float camZ = cos(glfwGetTime()) * radius;
glm::mat4 view = glm::lookAt(
    glm::vec3(camX, 0.0f, camZ),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f)
);
```

---

## 🔗 ナビゲーション

⬅️ [変換](./07-transformations.md) | ➡️ [カメラ →](./09-camera.md)
