# 入門編 9：カメラ（Camera）

> **目標：** FPS スタイルのカメラシステムをゼロから実装する。マウスとキーボードで自由に 3D シーンを探索できるようにする

---

## カメラとは

OpenGL には「カメラ」というオブジェクトは存在しません。  
カメラを表現するには **View 行列** でワールド全体を逆方向に動かします。

```
カメラが右に動く = ワールドが左に動く
カメラが前に進む = ワールドが後ろに動く
```

---

## カメラのベクトル

カメラの姿勢は 3 つのベクトルで表現します。

```
         Camera Up（↑）
              │
              │
              │
              ●─────── Camera Right（→）
             ╱
            ╱
           ╱
    Camera Front（前方向）
```

### 計算方法

```cpp
// カメラの位置と向き
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // -Z が前方向
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); // Y 軸が上

// lookAt で View 行列を生成
glm::mat4 view = glm::lookAt(
    cameraPos,
    cameraPos + cameraFront, // 注視点 = カメラ位置 + 前方向
    cameraUp
);
```

---

## キーボード入力でカメラを移動

```cpp
float cameraSpeed = 2.5f * deltaTime; // フレームレート依存にしない！

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront; // 前進

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront; // 後退

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(
            glm::cross(cameraFront, cameraUp)) * cameraSpeed; // 左移動

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(
            glm::cross(cameraFront, cameraUp)) * cameraSpeed; // 右移動
}
```

> 💡 **`glm::cross(cameraFront, cameraUp)`** = 前方と上方の外積 = **右方向ベクトル**

---

## デルタタイム（Delta Time）

フレームレートに依存しない移動速度を実現するための仕組みです。

```cpp
float deltaTime = 0.0f; // 前フレームからの経過時間
float lastFrame = 0.0f;

// レンダリングループ内
float currentFrame = static_cast<float>(glfwGetTime());
deltaTime = currentFrame - lastFrame;
lastFrame = currentFrame;

// 速度にデルタタイムを掛ける
float cameraSpeed = 2.5f * deltaTime;
```

```
60 FPS: deltaTime ≈ 0.016秒 → speed = 2.5 * 0.016 = 0.04
30 FPS: deltaTime ≈ 0.033秒 → speed = 2.5 * 0.033 = 0.08
1秒間の移動量 = 2.5 × 1.0 = 2.5（どちらも同じ！）
```

---

## マウスでカメラを回転（オイラー角）

マウスの動きをカメラの回転（Yaw・Pitch）に変換します。

```
Yaw（ヨー） ：Y 軸周りの回転 → 左右を向く
Pitch（ピッチ）：X 軸周りの回転 → 上下を向く
Roll（ロール） ：Z 軸周りの回転 → FPS では通常使わない
```

```cpp
float yaw = -90.0f; // X 軸方向（-Z 方向）を向くために -90 度
float pitch = 0.0f;
float sensitivity = 0.1f; // マウス感度

// マウスコールバック
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static float lastX = 400, lastY = 300;
    static bool firstMouse = true;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Y は上が正（スクリーンは逆）
    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Pitch の制限（真上・真下を向きすぎないように）
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // カメラ前方ベクトルを更新
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}
```

---

## マウスカーソルを隠す

FPS スタイルでは、マウスカーソルを隠してウィンドウ内に固定します。

```cpp
// カーソルを隠して固定
glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

// コールバックを登録
glfwSetCursorPosCallback(window, mouse_callback);
```

---

## スクロールでズーム（FOV 変更）

```cpp
float fov = 45.0f;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    fov -= (float)yoffset;
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 45.0f) fov = 45.0f;
}

// Projection 行列を fov で更新
glm::mat4 projection = glm::perspective(
    glm::radians(fov),
    (float)width / height, 0.1f, 100.0f
);
```

---

## カメラクラス

```cpp
// Camera.h
#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    Camera(glm::vec3 position = glm::vec3(0, 0, 0),
           glm::vec3 up = glm::vec3(0, 1, 0),
           float yaw = -90.0f, float pitch = 0.0f)
        : Front(glm::vec3(0, 0, -1)), MovementSpeed(2.5f),
          MouseSensitivity(0.1f), Zoom(45.0f) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD) Position += Front * velocity;
        if (direction == BACKWARD) Position -= Front * velocity;
        if (direction == LEFT) Position -= Right * velocity;
        if (direction == RIGHT) Position += Right * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset,
                               bool constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;
        Yaw += xoffset;
        Pitch += yoffset;
        if (constrainPitch) {
            if (Pitch > 89.0f) Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }
        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset) {
        Zoom -= yoffset;
        if (Zoom < 1.0f) Zoom = 1.0f;
        if (Zoom > 45.0f) Zoom = 45.0f;
    }

private:
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

#endif
```

---

## 💡 ポイントまとめ

| 概念 | 説明 |
|------|------|
| デルタタイム | フレームレートに依存しない移動速度を実現 |
| Yaw/Pitch | カメラの左右・上下回転角度 |
| lookAt | 位置・注視点・上方向からView行列を生成 |
| 外積 | 前方×上方 = 右方向ベクトルを計算 |
| FOV | 視野角。小さいほどズームイン |

---

## ドリル問題

### 問題 1：デルタタイム穴埋め

```cpp
float 【 ① 】 = 0.0f;
float 【 ② 】 = 0.0f;

// ループ内
float currentFrame = glfwGetTime();
deltaTime = currentFrame - 【 ③ 】;
lastFrame = 【 ④ 】;

// 速度計算
float cameraSpeed = 2.5f * 【 ⑤ 】;
```

<details>
<summary> 解答</summary>

① `deltaTime`  
② `lastFrame`  
③ `lastFrame`  
④ `currentFrame`  
⑤ `deltaTime`

</details>

---

### 問題 2：移動ベクトル

| 移動方向 | 計算式 |
|---------|--------|
| 前進 | `cameraPos 【+/-】 cameraSpeed * cameraFront` |
| 後退 | `cameraPos 【+/-】 cameraSpeed * cameraFront` |
| 左 | `cameraPos 【+/-】 normalize(cross(cameraFront, cameraUp)) * speed` |
| 右 | `cameraPos 【+/-】 normalize(cross(cameraFront, cameraUp)) * speed` |

<details>
<summary> 解答</summary>

| 方向 | 符号 |
|------|------|
| 前進 | `+=` |
| 後退 | `-=` |
| 左 | `-=` |
| 右 | `+=` |

</details>

---

### 問題 3：Pitch の制限

Pitch を -89 〜 +89 度に制限するのはなぜか？89 度を超えたらどうなるか？

<details>
<summary> 解答</summary>

Pitch が ±90 度以上になると、カメラが真上・真下を向き超えてしまう。  
このとき `lookAt` の計算で Up ベクトルと Front ベクトルが平行になり、  
**右ベクトルが計算できなくなる（ジンバルロック）**。  
89 度で止めることでこの問題を防ぐ。

</details>

---

### 問題 4：Yaw/Pitch から前方ベクトル

Yaw=0, Pitch=0 のときの `cameraFront` の値を計算しなさい。

```cpp
front.x = cos(radians(yaw)) * cos(radians(pitch));
front.y = sin(radians(pitch));
front.z = sin(radians(yaw)) * cos(radians(pitch));
```

<details>
<summary> 解答</summary>

- front.x = cos(0) × cos(0) = 1.0 × 1.0 = **1.0**
- front.y = sin(0) = **0.0**
- front.z = sin(0) × cos(0) = 0.0 × 1.0 = **0.0**

→ `(1, 0, 0)`（X 方向を向く）

なぜ初期値を yaw = -90 にするかというと、  
Yaw = -90 のとき：front.x = cos(-90) × 1 = 0, front.z = sin(-90) × 1 = -1  
→ `(0, 0, -1)` = **-Z 方向を向く**（OpenGL の慣習では前が -Z）

</details>

---

## 実践課題

### 課題 1：基本的なカメラ 

WASD キーで移動できるカメラを実装しなさい。デルタタイムを使うこと。

### 課題 2：マウス操作 

マウス移動でカメラを回転させる機能を追加しなさい。

### 課題 3：Camera クラスを使う 

本章の Camera クラスを別ファイルに実装し、main.cpp から利用できるようにしなさい。

### 課題 4：高度なカメラ 

以下の追加機能を実装しなさい。
- `Shift` キーで走る（速度 2 倍）
- スクロールで FOV 変更（ズーム）
- `F` キーでフライモードとウォークモードを切り替え（ウォークモードでは Y 座標固定）

---

## ナビゲーション

 [座標系](./08-coordinate-systems.md) | [入門編まとめ →](./10-review.md)
