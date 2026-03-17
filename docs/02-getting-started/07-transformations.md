# 入門編 7：変換（Transformations）

> **目標：** 行列変換（移動・回転・スケール）を理解し、GLM を使って実装する

---

## なぜ行列変換が必要か

3D グラフィックスでオブジェクトを動かすには、頂点の座標を **数学的に変換** します。  
ゲームで「キャラクターが歩く」「カメラが回る」はすべて行列計算です。

```
変換の種類：
  移動（Translation） → 位置を変える
  回転（Rotation） → 向きを変える
  スケール（Scale） → 大きさを変える
  
これらを 4×4 行列で表現する！
```

---

## ベクトルの復習

### 加算と減算
```
a = (2, 3)
b = (1, 2)
a + b = (3, 5)
a - b = (1, 1)
```

### スカラー倍
```
a = (2, 3)
2 * a = (4, 6)
```

### 内積（Dot Product）
```
a · b = a.x*b.x + a.y*b.y + a.z*b.z
      = |a| * |b| * cos(θ)
```
用途：2 ベクトルがどれだけ同じ向きか（ライティング計算で多用）

### 外積（Cross Product）
```
a × b = ベクトル a と b の両方に垂直なベクトル
```
用途：法線ベクトルの計算、カメラの Right ベクトル計算

---

## 行列とは

行列は線形変換を表現するための数学的な道具です。

```
4×4 の単位行列（Identity Matrix）：
┌1 0 0 0┐
│0 1 0 0│
│0 0 1 0│
└0 0 0 1┘

任意のベクトルに単位行列を掛けても変化しない
```

---

## 移動（Translation）

```
移動行列（x方向に Tx、y方向に Ty、z方向に Tz 移動）：

┌1 0 0 Tx┐
│0 1 0 Ty│
│0 0 1 Tz│
└0 0 0 1 ┘

(x, y, z, 1) に掛けると：
新しい位置 = (x + Tx, y + Ty, z + Tz, 1)
```

> 💡 **なぜ 4×4 なのか？（斉次座標）**  
> 3×3 行列では移動を表現できません。4 番目の成分（w = 1）を追加することで、  
> 移動も行列の掛け算だけで表現できます。これを **斉次座標** と呼びます。

---

## スケール（Scale）

```
スケール行列（X を Sx 倍、Y を Sy 倍、Z を Sz 倍）：

┌Sx 0 0 0┐
│0 Sy 0 0│
│0 0 Sz 0│
└0 0 0 1┘
```

---

## 回転（Rotation）

### Z 軸周りの回転（2D 的な回転）

```
Z 軸周りに角度 θ 回転：

┌cos(θ) -sin(θ) 0 0┐
│sin(θ) cos(θ) 0 0│
│0 0 1 0│
└0 0 0 1┘
```

### GLM での回転

```cpp
// 注意：GLM の rotate は角度をラジアンで指定
glm::mat4 rot = glm::rotate(glm::mat4(1.0f), // 単位行列から開始
                             glm::radians(90.0f), // 90 度 → ラジアン
                             glm::vec3(0, 0, 1)); // Z 軸周り
```

---

## 変換の組み合わせ

複数の変換を組み合わせるには行列を掛け合わせます。

```
最終変換 = Translation × Rotation × Scale

⚠️ 順序が重要！行列の掛け算は交換則が成り立たない。
```

**実際の順序の罠：**
```
// Scale → Rotate → Translate の順に適用したい場合：
// コードでは逆順に書く！（右側から先に適用されるため）

glm::mat4 transform = glm::mat4(1.0f);
transform = glm::translate(transform, glm::vec3(0.5f, -0.5f, 0.0f)); // 3番目
transform = glm::rotate(transform, glm::radians(45.0f), glm::vec3(0, 0, 1)); // 2番目
transform = glm::scale(transform, glm::vec3(0.5f, 0.5f, 0.5f)); // 1番目

// 実際の適用順序：Scale → Rotate → Translate
```

---

## GLM（OpenGL Mathematics）

GLM は OpenGL 専用の数学ライブラリです。

```cpp
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // translate, rotate, scale
#include <glm/gtc/type_ptr.hpp> // value_ptr（行列をGLに渡す）
```

### 基本的な使い方

```cpp
// ベクトル
glm::vec3 pos(1.0f, 2.0f, 3.0f);
glm::vec4 color(1.0f, 0.5f, 0.2f, 1.0f);

// 行列
glm::mat4 identity = glm::mat4(1.0f); // 単位行列

// 移動
glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));

// 回転（Z 軸周りに 90 度）
glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                             glm::radians(90.0f),
                             glm::vec3(0.0f, 0.0f, 1.0f));

// スケール（0.5 倍）
glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));

// 組み合わせ
glm::mat4 transform = trans * rot * scale;

// シェーダーに渡す
int transformLoc = glGetUniformLocation(shader.ID, "transform");
glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
```

---

## シェーダーでの変換

```glsl
// 頂点シェーダー
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 transform; // 変換行列

void main() {
    gl_Position = transform * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
```

---

## 時間による回転アニメーション

```cpp
// レンダリングループ内
glm::mat4 transform = glm::mat4(1.0f);
transform = glm::translate(transform, glm::vec3(0.5f, -0.5f, 0.0f));
transform = glm::rotate(transform,
    (float)glfwGetTime(), // 時間で角度を増やす
    glm::vec3(0.0f, 0.0f, 1.0f));

shader.use();
unsigned int transformLoc = glGetUniformLocation(shader.ID, "transform");
glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
```

---

## 💡 ポイントまとめ

| 変換 | GLM 関数 | 主な用途 |
|------|----------|----------|
| 移動 | `glm::translate` | キャラクターの位置 |
| 回転 | `glm::rotate` | 回転アニメーション |
| スケール | `glm::scale` | オブジェクトのサイズ変更 |
| 組み合わせ | 行列の掛け算 | 複合変換（TRS） |

| 概念 | 説明 |
|------|------|
| 斉次座標 | w 成分を加えた 4 次元座標。移動を行列で表現可能に |
| 単位行列 | 変換しない行列（かけても変化なし） |
| 変換順序 | Scale → Rotate → Translate（コードは逆順） |
| value_ptr | GLM の行列を OpenGL に渡す関数 |

---

## ドリル問題

### 問題 1：変換行列の穴埋め

`(2, 3, 0)` だけ移動する 4×4 行列を完成させなさい。

```
┌1 0 0 【①】┐
│0 1 0 【②】│
│0 0 1 【③】│
└0 0 0 【④】┘
```

<details>
<summary> 解答</summary>

① 2, ② 3, ③ 0, ④ 1

</details>

---

### 問題 2：GLM コード穴埋め

```cpp
glm::mat4 transform = glm::mat4(【 ① 】); // 単位行列

// Z 軸周りに 45 度回転
transform = glm::rotate(transform,
    glm::radians(【 ② 】),
    glm::vec3(0.0f, 0.0f, 【 ③ 】));

// (1.0, 0.0, 0.0) に移動
transform = 【 ④ 】(transform, glm::vec3(1.0f, 0.0f, 0.0f));

// シェーダーに渡す
glUniformMatrix4fv(loc, 1, GL_FALSE, 【 ⑤ 】(transform));
```

<details>
<summary> 解答</summary>

① `1.0f`  
② `45.0f`  
③ `1.0f`  
④ `glm::translate`  
⑤ `glm::value_ptr`

</details>

---

### 問題 3：変換の順序

以下のコードで実際に適用される変換の順序を答えなさい。

```cpp
glm::mat4 t = glm::mat4(1.0f);
t = glm::translate(t, glm::vec3(1, 0, 0));
t = glm::rotate(t, glm::radians(90.0f), glm::vec3(0, 0, 1));
t = glm::scale(t, glm::vec3(0.5f));
```

<details>
<summary> 解答</summary>

**Scale → Rotate → Translate**（コードは逆順に適用される）

オブジェクトはまず 0.5 倍にスケールされ、次に 90 度回転し、最後に X 方向に 1 移動する。

</details>

---

### 問題 4：内積の応用

ライティング計算でよく使う内積 `dot(N, L)` の各変数が何を表すか、そしてその値の範囲と意味を説明しなさい。

<details>
<summary> 解答</summary>

- `N`：サーフェスの**法線ベクトル**（表面の向き、単位ベクトル）
- `L`：**光源方向ベクトル**（サーフェスから光源への方向、単位ベクトル）
- `dot(N, L)` の範囲：-1.0〜1.0
  - 1.0 = 法線と光が完全に同じ向き = 最大輝度
  - 0.0 = 垂直 = 照らされていない
  - 負の値 = 裏面から当たっている = 通常 0 にクランプ

</details>

---

## 実践課題

### 課題 1：回転するテクスチャ四角形 

前章のテクスチャ四角形に変換行列を適用し、時間とともに回転するようにしなさい。

### 課題 2：TRS の分離制御 

キーボード入力で以下を制御できるようにしなさい。
- `WASD`：移動
- `Q/E`：回転
- `Z/X`：スケール変更

### 課題 3：惑星の公転 

1 つの「太陽」の周りを「惑星」が公転するアニメーションを作りなさい。  
ヒント：公転 = 太陽を中心に回転する = `translate → rotate` の順

---

## ナビゲーション

 [テクスチャ](./06-textures.md) | [座標系 →](./08-coordinate-systems.md)
