# 入門編 3：Hello Window

> **目標：** カラフルな背景が表示されるウィンドウを動かす。OpenGL のレンダリングパイプラインの全体像を掴む

---

## レンダリングパイプラインの全体像

OpenGL の最大の特徴は、**プログラマブルなレンダリングパイプライン** です。頂点データが最終ピクセルになるまでの処理の流れを「パイプライン」と呼びます。

```
CPU（あなたのコード）
    ↓ 頂点データ（Vertex Data）送信
┌─────────────────────────────────────────────────────┐
│ OpenGL レンダリングパイプライン │
│ │
│ 1. 頂点シェーダー ← プログラム可能 │
│ ↓ │
│ 2. 形状アセンブリ ← 固定処理 │
│ ↓ │
│ 3. ジオメトリシェーダー ← プログラム可能 （省略可）│
│ ↓ │
│ 4. ラスタライズ ← 固定処理 │
│ ↓ │
│ 5. フラグメントシェーダー← プログラム可能 │
│ ↓ │
│ 6. テスト・ブレンド ← 固定処理（設定可能） │
└─────────────────────────────────────────────────────┘
    ↓
フレームバッファ → モニター
```

---

## 各ステージの役割

### ① 頂点シェーダー（Vertex Shader）
- 1 頂点につき 1 回実行される
- 頂点の **3D 座標 → クリップ座標** への変換を行う
- 頂点属性（色・UV 座標など）を処理する

### ② 形状アセンブリ（Shape Assembly / Primitive Assembly）
- 頂点をプリミティブ（三角形・線・点）に組み立てる

### ③ ジオメトリシェーダー（Geometry Shader）
- プリミティブを入力として新しいプリミティブを生成できる
- 省略可能

### ④ ラスタライズ（Rasterization）
- 3D のプリミティブを 2D のピクセル（フラグメント）に変換

### ⑤ フラグメントシェーダー（Fragment Shader）
- 1 ピクセルにつき 1 回実行される
- **最終的な色を決定する** のが主な役割
- ライティング計算もここで行う

### ⑥ テスト・ブレンド（Tests and Blending）
- 深度テスト（奥のものが手前のもので隠れる処理）
- ステンシルテスト
- ブレンディング（透明度処理）

---

## 正規化デバイス座標（NDC）

OpenGL の頂点シェーダーは最終的に **-1.0〜1.0 の範囲の座標（NDC）** を出力する必要があります。

```
NDC 座標系

        (0, 1)
          │
(-1, 0)──┼──(1, 0)
          │
        (0,-1)

※ 画面の中央が (0, 0)
※ 右上が (1, 1)
※ 左下が (-1, -1)
```

この範囲に収まらない頂点は **クリッピング** されます（画面外に見切れる）。

---

## 頂点バッファオブジェクト（VBO）

CPU から GPU へ頂点データを送るには **VBO（Vertex Buffer Object）** を使います。

```cpp
// 頂点データ（三角形の 3 頂点）
float vertices[] = {
    -0.5f, -0.5f, 0.0f, // 左下
     0.5f, -0.5f, 0.0f, // 右下
     0.0f, 0.5f, 0.0f // 上
};

// VBO の生成
unsigned int VBO;
glGenBuffers(1, &VBO);

// VBO をバインド（以降の操作はこの VBO に対して行われる）
glBindBuffer(GL_ARRAY_BUFFER, VBO);

// GPU にデータをアップロード
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
```

### `glBufferData` の第 4 引数（使用パターン）

| 定数 | 意味 |
|------|------|
| `GL_STATIC_DRAW` | ほとんど変更しない（静的な物体） |
| `GL_DYNAMIC_DRAW` | 頻繁に変更する（動くオブジェクト） |
| `GL_STREAM_DRAW` | 毎フレーム変更する |

---

## シェーダーの最初の一歩

シェーダーは **GLSL（OpenGL Shading Language）** という C 言語に似た言語で書きます。

### 最小限の頂点シェーダー

```glsl
#version 330 core

layout (location = 0) in vec3 aPos; // 頂点属性 0 番：位置

void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    // または: gl_Position = vec4(aPos, 1.0);
}
```

### 最小限のフラグメントシェーダー

```glsl
#version 330 core

out vec4 FragColor; // 出力する色（RGBA）

void main() {
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f); // オレンジ色
}
```

---

## 完全なウィンドウプログラム

```cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    // GLFW 初期化
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // ウィンドウ作成
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hello Window", NULL, NULL);
    if (!window) {
        std::cerr << "ウィンドウ作成失敗" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // GLAD 初期化
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD 初期化失敗" << std::endl;
        return -1;
    }

    // レンダリングループ
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        // 暗いシアン系の背景色
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
```

---

## 💡 ポイントまとめ

| 概念 | 説明 |
|------|------|
| パイプライン | 頂点データ → ピクセルまでの一連の処理 |
| 頂点シェーダー | 座標変換を行う GPU プログラム |
| フラグメントシェーダー | 色を決める GPU プログラム |
| GLSL | OpenGL のシェーダー言語 |
| NDC | -1.0〜1.0 の正規化デバイス座標 |
| VBO | GPU 上の頂点データバッファ |
| ラスタライズ | 3D→2D ピクセル変換 |

---

## ドリル問題

### 問題 1：パイプラインの順序

レンダリングパイプラインのステージを正しい順に並べなさい。

```
(A) ラスタライズ
(B) 形状アセンブリ
(C) フラグメントシェーダー
(D) 頂点シェーダー
(E) テスト・ブレンド
```

<details>
<summary> 解答</summary>

**D → B → A → C → E**

頂点シェーダー → 形状アセンブリ → ラスタライズ → フラグメントシェーダー → テスト・ブレンド

</details>

---

### 問題 2：穴埋め（NDC）

NDC（正規化デバイス座標）の各点の座標を答えなさい。

| 位置 | X | Y |
|------|---|---|
| 画面中央 | 【　①　】 | 【　②　】 |
| 右上 | 【　③　】 | 【　④　】 |
| 左下 | 【　⑤　】 | 【　⑥　】 |

<details>
<summary> 解答</summary>

① 0.0 ② 0.0 ③ 1.0 ④ 1.0 ⑤ -1.0 ⑥ -1.0

</details>

---

### 問題 3：コード穴埋め

```cpp
// VBO の生成と使用
unsigned int VBO;
glGenBuffers(【 ① 】, &VBO);
glBindBuffer(【 ② 】, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, 【 ③ 】);
```

<details>
<summary> 解答</summary>

① `1`  
② `GL_ARRAY_BUFFER`  
③ `GL_STATIC_DRAW`（データをほぼ変更しない場合）

</details>

---

### 問題 4：GLSL 理解

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;

void main() {
    gl_Position = vec4(aPos, 1.0);
}
```

1. `layout (location = 0)` は何を指定しているか？
2. `gl_Position` の型 `vec4` の 4 成分は何か？
3. `w` 成分を `1.0` にするのはなぜか（ヒント：斉次座標）？

<details>
<summary> 解答</summary>

1. **頂点属性のインデックス（Attribute Location）** を 0 番と指定している。CPU 側の `glVertexAttribPointer` と対応する
2. `x, y, z, w`（w は斉次座標の成分）
3. `w = 1.0` は通常の 3D 点を表す。`w = 0.0` は方向ベクトルを表す。透視変換でも `w = 1.0` が基本

</details>

---

### 問題 5：記述問題

`GL_STATIC_DRAW`・`GL_DYNAMIC_DRAW`・`GL_STREAM_DRAW` の違いを説明し、それぞれどんな用途に使うか答えなさい。

<details>
<summary> 解答</summary>

| 定数 | 使用頻度 | 用途例 |
|------|---------|--------|
| GL_STATIC_DRAW | ほぼ変更なし | 地形、建物など静的な物体 |
| GL_DYNAMIC_DRAW | 時々変更 | アニメーションする物体 |
| GL_STREAM_DRAW | 毎フレーム変更 | パーティクル、動的テキスト |

</details>

---

## 実践課題

### 課題 1：背景色アニメーション

時間経過とともに背景色がグラデーションで変化するようにしなさい。

```cpp
// ヒント
float time = (float)glfwGetTime();
float r = sin(time) * 0.5f + 0.5f;
float g = sin(time + 2.094f) * 0.5f + 0.5f; // 2π/3 ずらす
float b = sin(time + 4.189f) * 0.5f + 0.5f; // 4π/3 ずらす
glClearColor(r, g, b, 1.0f);
```

### 課題 2：ESC キー以外の終了方法を追加

`Q` キーを押しても終了するように `processInput` を改修しなさい。

### 課題 3：FPS 表示

タイトルバーに現在の FPS を表示する機能を追加しなさい。

```cpp
// ヒント
double currentTime = glfwGetTime();
// 1秒ごとにFPSを計算してタイトルを更新
std::string title = "Hello Window | FPS: " + std::to_string(fps);
glfwSetWindowTitle(window, title.c_str());
```

---

## ナビゲーション

 [ウィンドウの作成](./02-creating-a-window.md) | [Hello Triangle →](./04-hello-triangle.md)
