# 入門編 2：ウィンドウの作成（Creating a Window）

> **目標：** GLFW と GLAD を使って OpenGL コンテキスト付きのウィンドウを作成する

---

## 必要なライブラリ

OpenGL でウィンドウを作るには、OS のウィンドウシステムを抽象化するライブラリが必要です。

| ライブラリ | 役割 |
|-----------|------|
| **GLFW** | ウィンドウ作成・入力管理・OpenGL コンテキスト作成 |
| **GLAD** | OpenGL 関数のロード |
| **GLM** | 数学演算（行列・ベクトル） |

---

## GLFW のセットアップ

### CMakeLists.txt の例

```cmake
cmake_minimum_required(VERSION 3.10)
project(OpenGLTutorial)

set(CMAKE_CXX_STANDARD 17)

# GLFW
find_package(glfw3 3.3 REQUIRED)

# ソースファイル
add_executable(tutorial
    src/main.cpp
    src/glad.c # GLAD のソース
)

# インクルードパス
target_include_directories(tutorial PRIVATE
    include/ # glad.h などを置く場所
)

# リンク
target_link_libraries(tutorial glfw)

# Linux では追加で必要
if(UNIX)
    target_link_libraries(tutorial GL dl pthread)
endif()
```

---

## main.cpp の骨格

以下が最小限のウィンドウ作成コードです。

```cpp
#include <glad/glad.h> // GLAD は GLFW より先にインクルード！
#include <GLFW/glfw3.h>
#include <iostream>

// ウィンドウサイズ変更時のコールバック
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// 入力処理
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    // ========================================
    // 1. GLFW の初期化
    // ========================================
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // OpenGL 3.x
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // OpenGL x.3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // コアプロファイル

    // macOS の場合は追加で必要
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // ========================================
    // 2. ウィンドウの作成
    // ========================================
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cerr << "ウィンドウの作成に失敗しました" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // このウィンドウのコンテキストをカレントに

    // ========================================
    // 3. GLAD の初期化（ウィンドウ作成後！）
    // ========================================
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD の初期化に失敗しました" << std::endl;
        return -1;
    }

    // ========================================
    // 4. ビューポートの設定
    // ========================================
    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // ========================================
    // 5. レンダリングループ
    // ========================================
    while (!glfwWindowShouldClose(window)) {
        // 入力処理
        processInput(window);

        // 画面クリア
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // 暗いシアン色
        glClear(GL_COLOR_BUFFER_BIT);

        // バッファを交換（ダブルバッファリング）
        glfwSwapBuffers(window);

        // イベント処理
        glfwPollEvents();
    }

    // ========================================
    // 6. 後処理
    // ========================================
    glfwTerminate();
    return 0;
}
```

---

## 重要概念の解説

### `glfwWindowHint`：バージョン指定

```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // メジャーバージョン: 3
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // マイナーバージョン: 3
// → OpenGL 3.3 のコンテキストを要求する
```

### `glViewport`：描画領域の設定

```
ウィンドウ（800x600）
┌──────────────────────────┐
│ │
│ ビューポート │
│ (0,0) から (800,600) │
│ │
└──────────────────────────┘
     ↑ (0,0) = 左下

glViewport(x, y, width, height)
```

### ダブルバッファリング

```
フロントバッファ → モニターに表示中
バックバッファ → 描画中（非表示）

glfwSwapBuffers() → 2 つを交換
```

ダブルバッファリングにより、描画の途中の状態が画面に見えてしまう「ちらつき」を防ぎます。

### `glfwPollEvents` vs `glfwWaitEvents`

| 関数 | 動作 | 用途 |
|------|------|------|
| `glfwPollEvents()` | イベントがあれば処理して即座に返る | **リアルタイムゲーム** |
| `glfwWaitEvents()` | イベントが来るまでブロック | エディタ・静止画アプリ |

---

## インクルード順序の注意

```cpp
// 正しい順序
#include <glad/glad.h> // GLAD を先にインクルード
#include <GLFW/glfw3.h> // GLFW はあと

// 間違い：GLFW が OpenGL ヘッダをロードしてしまい衝突する
#include <GLFW/glfw3.h>
#include <glad/glad.h>
```

---

## 💡 ポイントまとめ

| 関数 | 役割 |
|------|------|
| `glfwInit()` | GLFW の初期化 |
| `glfwWindowHint()` | ウィンドウ・コンテキストの設定 |
| `glfwCreateWindow()` | ウィンドウを生成 |
| `glfwMakeContextCurrent()` | OpenGL コンテキストをカレントスレッドに設定 |
| `gladLoadGLLoader()` | OpenGL 関数を動的にロード |
| `glViewport()` | 描画領域（ビューポート）の設定 |
| `glfwSwapBuffers()` | フロント・バックバッファを交換 |
| `glfwPollEvents()` | OS イベントを処理 |
| `glfwTerminate()` | GLFW のクリーンアップ |

---

## ドリル問題

### 問題 1：穴埋め（コード）

以下のコードの空欄を埋めなさい。

```cpp
// GLFW の初期化
【 ① 】();

// OpenGL 3.3 コアプロファイルを要求
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 【 ② 】);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 【 ③ 】);
glfwWindowHint(GLFW_OPENGL_PROFILE, 【 ④ 】);

// ウィンドウ作成（800x600、タイトル"Hello"）
GLFWwindow* window = 【 ⑤ 】(800, 600, "Hello", NULL, NULL);

// このウィンドウをカレントコンテキストに
【 ⑥ 】(window);

// GLAD の初期化
if (!gladLoadGLLoader((GLADloadproc)【 ⑦ 】)) {
    return -1;
}
```

<details>
<summary> 解答</summary>

① `glfwInit`  
② `3`  
③ `3`  
④ `GLFW_OPENGL_CORE_PROFILE`  
⑤ `glfwCreateWindow`  
⑥ `glfwMakeContextCurrent`  
⑦ `glfwGetProcAddress`

</details>

---

### 問題 2：レンダリングループの順序

レンダリングループ内の処理を正しい順序に並べなさい。

```
(A) glfwSwapBuffers(window)
(B) glClear(GL_COLOR_BUFFER_BIT)
(C) processInput(window)
(D) glfwPollEvents()
(E) glClearColor(0.2f, 0.3f, 0.3f, 1.0f)
```

<details>
<summary> 解答</summary>

**C → E → B → A → D**

1. 入力処理（processInput）
2. クリアカラーを設定（glClearColor）
3. バッファをクリア（glClear）
4. 描画処理（省略）
5. バッファ交換（glfwSwapBuffers）
6. イベント処理（glfwPollEvents）

</details>

---

### 問題 3：正誤問題

1. `gladLoadGLLoader` は `glfwCreateWindow` より前に呼んでも構わない
2. `glViewport` の座標 `(0, 0)` は左下を指す
3. `glfwWaitEvents` はゲームのメインループに適している
4. ダブルバッファリングはちらつきを防ぐための技術である

<details>
<summary> 解答</summary>

1. ✗（コンテキスト作成後でないと初期化できない）
2. ○（OpenGL の座標系では左下が原点）
3. ✗（リアルタイムゲームには `glfwPollEvents` が適切）
4. ○

</details>

---

### 問題 4：記述問題

`framebuffer_size_callback` はいつ呼ばれ、何のために `glViewport` を再設定するのか説明しなさい。

<details>
<summary> 解答</summary>

ウィンドウサイズが変更されたときに呼ばれる。  
ウィンドウサイズが変わるとビューポートも更新しないと、描画領域が古いサイズのままになってしまうため、新しい `width, height` に合わせて `glViewport` を再設定する。

</details>

---

## 実践課題

### 課題 1：背景色を変える

`glClearColor` の引数を変えて、以下の背景色を表示しなさい。

| 課題 | R | G | B | A |
|------|---|---|---|---|
| 赤 | 1.0 | 0.0 | 0.0 | 1.0 |
| 緑 | 0.0 | 1.0 | 0.0 | 1.0 |
| 黒 | 0.0 | 0.0 | 0.0 | 1.0 |

### 課題 2：動的な背景色

`glfwGetTime()` を使って、時間によって背景色が変化するようにしなさい。

```cpp
// ヒント
float timeValue = glfwGetTime(); // 経過時間（秒）
float greenValue = (sin(timeValue) / 2.0f) + 0.5f; // 0〜1 の範囲に変換
glClearColor(0.0f, greenValue, 0.0f, 1.0f);
```

### 課題 3：ウィンドウリサイズ確認

ウィンドウを手動でリサイズして、描画が正しくスケールされることを確認しなさい。もしスケールされない場合は、コールバックが正しく登録されているか確認しなさい。

---

## ナビゲーション

 [OpenGL とは](./01-opengl.md) | [Hello Window →](./03-hello-window.md)
