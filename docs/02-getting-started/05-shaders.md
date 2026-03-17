# 入門編 5：シェーダー（Shaders）

> **目標：** GLSL の文法・型・ユニフォーム・シェーダークラスを習得する

---

## GLSL の基本文法

GLSL（OpenGL Shading Language）は C 言語ベースのシェーダー言語です。

```glsl
#version 330 core // バージョン指定（必須）

// 入力変数
in 型 変数名; // 前のステージからの入力

// 出力変数
out 型 変数名; // 次のステージへの出力

// ユニフォーム変数（全シェーダー呼び出しで共有）
uniform 型 変数名;

void main() {
    // エントリーポイント
}
```

---

## GLSL のデータ型

### スカラー型

| 型 | 説明 |
|----|------|
| `int` | 整数 |
| `uint` | 符号なし整数 |
| `float` | 浮動小数点 |
| `double` | 倍精度浮動小数点 |
| `bool` | 真偽値 |

### ベクトル型

| 型 | float | int | uint | bool |
|----|-------|-----|------|------|
| 2 成分 | `vec2` | `ivec2` | `uvec2` | `bvec2` |
| 3 成分 | `vec3` | `ivec3` | `uvec3` | `bvec3` |
| 4 成分 | `vec4` | `ivec4` | `uvec4` | `bvec4` |

### 行列型

| 型 | 説明 |
|----|------|
| `mat2` | 2×2 行列 |
| `mat3` | 3×3 行列 |
| `mat4` | 4×4 行列 |

---

## ベクトルの操作

### スウィズリング（Swizzling）

ベクトルの成分を自由に組み合わせてアクセスできます。

```glsl
vec4 color = vec4(0.5, 0.1, 0.8, 1.0);

// 成分アクセス（2 種類の名前が使える）
float r = color.r; // = color.x = 0.5（赤/X）
float g = color.g; // = color.y = 0.1（緑/Y）
float b = color.b; // = color.z = 0.8（青/Z）
float a = color.a; // = color.w = 1.0（アルファ/W）

// スウィズリング（組み合わせ自由）
vec3 rgb = color.rgb; // vec3(0.5, 0.1, 0.8)
vec3 bgr = color.bgr; // 並び替え可
vec2 rr = color.rr; // 繰り返し可
vec4 abgr = color.abgr; // 順序変更可
```

### ベクトルの初期化

```glsl
vec2 v1 = vec2(0.5, 1.0);
vec3 v2 = vec3(v1, 0.2); // vec2 + float で vec3
vec4 v3 = vec4(v2, 1.0); // vec3 + float で vec4
vec4 v4 = vec4(1.0); // 全成分 1.0
vec4 v5 = vec4(v1, v1); // vec2 + vec2 で vec4
```

---

## in/out：ステージ間のデータ受け渡し

頂点シェーダーとフラグメントシェーダーの間でデータを渡すには、**同じ名前の out/in 変数** を使います。

```glsl
// 頂点シェーダー
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor; // 頂点カラー属性

out vec3 ourColor; // フラグメントシェーダーへ渡す

void main() {
    gl_Position = vec4(aPos, 1.0);
    ourColor = aColor;
}
```

```glsl
// フラグメントシェーダー
#version 330 core

in vec3 ourColor; // 頂点シェーダーから受け取る（補間済み）
out vec4 FragColor;

void main() {
    FragColor = vec4(ourColor, 1.0);
}
```

> 💡 **ラスタライズ補間（Interpolation）：**  
> 頂点間のフラグメントには、各頂点の値を線形補間した値が渡されます。
> これにより、3 頂点に別の色を設定すると自動的にグラデーションになります。

---

## 頂点カラーの追加

```cpp
// 位置（xyz）+ 色（rgb）を持つ頂点データ
float vertices[] = {
    // 位置 // 色
     0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // 右下：赤
    -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // 左下：緑
     0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f // 上 ：青
};

// 属性 0：位置（offset=0）
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
    6 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

// 属性 1：色（offset = 3 * sizeof(float)）
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
    6 * sizeof(float), (void*)(3 * sizeof(float)));
glEnableVertexAttribArray(1);
```

```
メモリレイアウト（1頂点 = 6 float = 24 bytes）
┌────┬────┬────┬────┬────┬────┐
│ X │ Y │ Z │ R │ G │ B │
└────┴────┴────┴────┴────┴────┘
  ↑属性0(offset=0) ↑属性1(offset=12)
       ←── stride = 24 bytes ───→
```

---

## ユニフォーム（Uniforms）

ユニフォームは CPU から GPU に値を渡す変数です。**すべての頂点・フラグメントで同じ値** が使われます。

```glsl
// フラグメントシェーダー
uniform vec4 ourColor;

void main() {
    FragColor = ourColor;
}
```

```cpp
// CPU 側からユニフォームに値をセット
float timeValue = glfwGetTime();
float greenValue = sin(timeValue) / 2.0f + 0.5f;

// ユニフォームの場所を取得
int colorLocation = glGetUniformLocation(shaderProgram, "ourColor");

// シェーダーを使用中でないといけない！
glUseProgram(shaderProgram);

// ユニフォームに値をセット
glUniform4f(colorLocation, 0.0f, greenValue, 0.0f, 1.0f);
```

### `glUniform` のバリエーション

| 関数 | 型 |
|------|----|
| `glUniform1f` | float 1 個 |
| `glUniform2f` | float 2 個（vec2） |
| `glUniform3f` | float 3 個（vec3） |
| `glUniform4f` | float 4 個（vec4） |
| `glUniform1i` | int 1 個 |
| `glUniform1fv` | float 配列 |
| `glUniformMatrix4fv` | mat4 |

---

## シェーダークラス

毎回シェーダーをコンパイルするコードを書くのは大変なので、再利用可能なクラスにまとめます。

```cpp
// Shader.h
#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    unsigned int ID; // シェーダープログラム ID

    // コンストラクタ：ファイルから読み込んでコンパイル
    Shader(const char* vertexPath, const char* fragmentPath) {
        // ファイルを読み込む
        std::string vertexCode, fragmentCode;
        std::ifstream vShaderFile, fShaderFile;

        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // コンパイル
        unsigned int vertex, fragment;
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // リンク
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() { glUseProgram(ID); }

    // ユニフォームセッター
    void setBool (const std::string& name, bool v) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)v);
    }
    void setInt (const std::string& name, int v) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), v);
    }
    void setFloat(const std::string& name, float v) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), v);
    }
    void setVec4 (const std::string& name, float x, float y, float z, float w) const {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }

private:
    void checkCompileErrors(unsigned int shader, std::string type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cerr << "シェーダーコンパイルエラー [" << type << "]:\n"
                          << infoLog << std::endl;
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cerr << "シェーダーリンクエラー:\n" << infoLog << std::endl;
            }
        }
    }
};

#endif
```

### 使い方

```cpp
// main.cpp
Shader myShader("shader.vert", "shader.frag");

// レンダリングループ内
myShader.use();
myShader.setFloat("someUniform", 1.0f);
```

---

## 💡 ポイントまとめ

| 概念 | 説明 |
|------|------|
| `in` | 前ステージからの入力変数 |
| `out` | 次ステージへの出力変数 |
| `uniform` | CPU から設定する定数（全呼び出しで共通） |
| スウィズリング | ベクトル成分を自由に組み合わせるアクセス |
| 補間 | ラスタライズ時に頂点間の値を線形補間 |
| `glGetUniformLocation` | ユニフォームの場所を取得 |
| `glUniform*f` | float 型ユニフォームに値をセット |

---

## ドリル問題

### 問題 1：スウィズリング

以下のコードの結果を答えなさい。

```glsl
vec4 v = vec4(1.0, 2.0, 3.0, 4.0);

vec3 a = v.rgb; // a = ?
vec2 b = v.zw; // b = ?
vec4 c = v.bgra; // c = ?
float d = v.g; // d = ?
```

<details>
<summary> 解答</summary>

```
a = vec3(1.0, 2.0, 3.0)
b = vec2(3.0, 4.0)
c = vec4(3.0, 2.0, 1.0, 4.0) // bgra なので b→z, g→y, r→x, a→w
d = 2.0
```

</details>

---

### 問題 2：ストライドとオフセット

以下の頂点データに対して、`glVertexAttribPointer` の引数を答えなさい。

```cpp
float vertices[] = {
    // 位置(xyz) // UV(st)
     0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
    // ...
};
// 属性0：位置（vec3）
// 属性1：UV（vec2）
```

1. 属性 0 の stride と offset は？
2. 属性 1 の stride と offset は？

<details>
<summary> 解答</summary>

1 頂点 = 位置(3) + UV(2) = 5 float = **20 バイト**

1. 属性 0：stride = `5 * sizeof(float)` = **20**、offset = **0**
2. 属性 1：stride = `5 * sizeof(float)` = **20**、offset = `3 * sizeof(float)` = **12**

</details>

---

### 問題 3：ユニフォームのコード穴埋め

```cpp
// 時間に応じて色が変化するユニフォームをセット
float time = glfwGetTime();
float value = sin(time) / 2.0f + 0.5f;

int loc = 【 ① 】(shaderProgram, "ourColor");
【 ② 】(shaderProgram);
【 ③ 】(loc, 0.0f, value, 0.0f, 1.0f);
```

<details>
<summary> 解答</summary>

① `glGetUniformLocation`  
② `glUseProgram`  
③ `glUniform4f`

</details>

---

### 問題 4：in/out の対応

頂点シェーダーの `out vec3 vColor` に対応するフラグメントシェーダーの宣言を書きなさい。

<details>
<summary> 解答</summary>

```glsl
in vec3 vColor;
```

（変数名が同じ・型が同じでなければいけない）

</details>

---

## 実践課題

### 課題 1：虹色三角形 

3 頂点に異なる色（赤・緑・青）を設定して、グラデーション三角形を表示しなさい。

### 課題 2：揺れる色 

ユニフォームを使って、フラグメントシェーダーの色が時間とともに変化するようにしなさい。

### 課題 3：Shader クラスの実装 

本章のシェーダークラスをファイルに分けて実装し、`.vert` と `.frag` ファイルからシェーダーを読み込むようにしなさい。

### 課題 4：頂点を上下に動かす 

頂点シェーダーにユニフォーム `offset` を追加して、三角形が上下に揺れるようにしなさい。

```glsl
// 頂点シェーダーのヒント
uniform float offset;
void main() {
    gl_Position = vec4(aPos.x, aPos.y + offset, aPos.z, 1.0);
}
```

---

## ナビゲーション

 [Hello Triangle](./04-hello-triangle.md) | [テクスチャ →](./06-textures.md)
