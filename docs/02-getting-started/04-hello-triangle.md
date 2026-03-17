# 入門編 4：Hello Triangle（最重要チャプター！）

> **目標：** VAO・VBO・シェーダーを使って画面に三角形を描く。OpenGL で「何かを描く」ための全手順を習得する

---

## 三角形を描くために必要なもの

```
必要なもの：
  1. 頂点データ（CPU 側の float 配列）
  2. VBO → GPU にデータを転送するバッファ
  3. VAO → 頂点属性の設定を記憶するオブジェクト
  4. 頂点シェーダー → 頂点の位置を決める
  5. フラグメントシェーダー → 色を決める
  6. シェーダープログラム → 2 つのシェーダーをリンクしたもの
  7. glDrawArrays → 実際に描画する命令
```

---

## 頂点入力（Vertex Input）

```cpp
// 三角形の 3 頂点（NDC 座標）
float vertices[] = {
    -0.5f, -0.5f, 0.0f, // 左下
     0.5f, -0.5f, 0.0f, // 右下
     0.0f, 0.5f, 0.0f // 上
};
```

```
      (0, 0.5)
        △
       / \
      / \
(-0.5,-0.5)─(0.5,-0.5)
```

---

## VAO（Vertex Array Object）

VAO は「頂点属性の設定をまとめて記憶するオブジェクト」です。

```
VAO
├── 頂点属性 0 → VBO_A から stride=12, offset=0, size=3 の float
├── 頂点属性 1 → VBO_A から stride=12, offset=12, size=2 の float
└── 頂点属性 2 → VBO_B から ...

描画するとき：VAO をバインドするだけで全設定が復元される！
```

**なぜ VAO が便利か？**
- 毎回 `glVertexAttribPointer` を呼ぶのは大変
- VAO に記憶させておけば、描画時に `glBindVertexArray(VAO)` するだけ

```cpp
// VAO と VBO の生成
unsigned int VAO, VBO;
glGenVertexArrays(1, &VAO);
glGenBuffers(1, &VBO);

// 1. VAO をバインド（以降の設定が VAO に記録される）
glBindVertexArray(VAO);

// 2. VBO にデータをアップロード
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

// 3. 頂点属性を設定（VAO に記録される）
glVertexAttribPointer(
    0, // 属性インデックス（シェーダーの location と対応）
    3, // 成分数（x, y, z の 3 つ）
    GL_FLOAT, // データ型
    GL_FALSE, // 正規化しない
    3 * sizeof(float), // ストライド（1 頂点のバイト数）
    (void*)0 // オフセット（先頭から 0 バイト）
);
glEnableVertexAttribArray(0); // 属性 0 を有効化

// 4. バインドを解除
glBindBuffer(GL_ARRAY_BUFFER, 0);
glBindVertexArray(0);
```

---

## シェーダーのコンパイルとリンク

### シェーダーソースコード（文字列として保持）

```cpp
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main() {
        gl_Position = vec4(aPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f); // オレンジ
    }
)";
```

### コンパイル

```cpp
// 頂点シェーダー
unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
glCompileShader(vertexShader);

// コンパイルエラーチェック
int success;
char infoLog[512];
glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    std::cerr << "頂点シェーダーコンパイルエラー:\n" << infoLog << std::endl;
}

// フラグメントシェーダー（同様に）
unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
glCompileShader(fragmentShader);
```

### リンク（シェーダープログラムの作成）

```cpp
// シェーダープログラムの作成
unsigned int shaderProgram = glCreateProgram();
glAttachShader(shaderProgram, vertexShader);
glAttachShader(shaderProgram, fragmentShader);
glLinkProgram(shaderProgram);

// リンクエラーチェック
glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cerr << "シェーダーリンクエラー:\n" << infoLog << std::endl;
}

// 不要になったシェーダーを削除
glDeleteShader(vertexShader);
glDeleteShader(fragmentShader);
```

---

## 描画（レンダリングループ内）

```cpp
while (!glfwWindowShouldClose(window)) {
    processInput(window);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // シェーダープログラムを使用
    glUseProgram(shaderProgram);

    // VAO をバインド（頂点属性設定を復元）
    glBindVertexArray(VAO);

    // 描画！
    glDrawArrays(
        GL_TRIANGLES, // プリミティブの種類
        0, // 開始インデックス
        3 // 頂点数
    );

    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

---

## EBO（Element Buffer Object）：四角形を描く

三角形 2 つで四角形を描くとき、頂点の重複を避けるため **EBO（インデックスバッファ）** を使います。

```cpp
// 四角形の 4 頂点
float vertices[] = {
     0.5f, 0.5f, 0.0f, // 右上 [0]
     0.5f, -0.5f, 0.0f, // 右下 [1]
    -0.5f, -0.5f, 0.0f, // 左下 [2]
    -0.5f, 0.5f, 0.0f // 左上 [3]
};

// インデックス（三角形 2 つで四角形）
unsigned int indices[] = {
    0, 1, 3, // 三角形 1（右上・右下・左上）
    1, 2, 3 // 三角形 2（右下・左下・左上）
};

unsigned int EBO;
glGenBuffers(1, &EBO);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

// 描画（glDrawArrays の代わりに glDrawElements を使う）
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
```

---

## ワイヤーフレームモード

デバッグに便利：

```cpp
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // ワイヤーフレーム
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // 通常（デフォルト）
```

---

## 💡 ポイントまとめ

| オブジェクト | 役割 |
|------------|------|
| VBO | GPU へ頂点データを転送するバッファ |
| VAO | 頂点属性の設定をまとめて記憶 |
| EBO | インデックスバッファ（重複頂点を削減） |
| シェーダー | GLSL で書いた GPU プログラム |
| シェーダープログラム | 頂点・フラグメントシェーダーをリンクしたもの |

### 描画の手順まとめ

```
1. glGenVertexArrays / glGenBuffers でオブジェクト生成
2. glBindVertexArray で VAO バインド
3. glBindBuffer + glBufferData でデータ転送
4. glVertexAttribPointer + glEnableVertexAttribArray で属性設定
5. シェーダーをコンパイル・リンク
─── 描画ループ ───
6. glUseProgram でシェーダー選択
7. glBindVertexArray で VAO バインド
8. glDrawArrays / glDrawElements で描画
```

---

## ドリル問題

### 問題 1：穴埋め（VAO/VBO）

```cpp
unsigned int VAO, VBO;
glGenVertexArrays(【 ① 】, &VAO);
glGenBuffers(【 ② 】, &VBO);

glBindVertexArray(【 ③ 】);
glBindBuffer(GL_ARRAY_BUFFER, 【 ④ 】);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, 【 ⑤ 】);

glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(【 ⑥ 】);
```

<details>
<summary> 解答</summary>

① `1`  
② `1`  
③ `VAO`  
④ `VBO`  
⑤ `GL_STATIC_DRAW`  
⑥ `0`

</details>

---

### 問題 2：`glVertexAttribPointer` の引数解説

```cpp
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
// ↑ ↑ ↑ ↑ ↑ ↑
// A B C D E F
```

各引数 A〜F の意味を答えなさい。

<details>
<summary> 解答</summary>

- A: 属性インデックス（シェーダーの `layout(location=0)` に対応）
- B: 成分数（vec3 なので 3）
- C: データ型（float）
- D: 正規化するか（GL_FALSE = しない）
- E: ストライド（1 頂点のバイト数 = 3 × 4 = 12 バイト）
- F: 開始オフセット（先頭から 0 バイト）

</details>

---

### 問題 3：シェーダーコンパイルの手順

シェーダーをコンパイルしてプログラムにリンクするまでの手順を正しく並べなさい。

```
(A) glLinkProgram(shaderProgram)
(B) glCompileShader(vertexShader)
(C) glCreateProgram()
(D) glCreateShader(GL_VERTEX_SHADER)
(E) glShaderSource(vertexShader, ...)
(F) glAttachShader(shaderProgram, vertexShader)
(G) glDeleteShader(vertexShader)
```

<details>
<summary> 解答</summary>

**D → E → B → C → F → A → G**

シェーダー生成 → ソース設定 → コンパイル → プログラム生成 → アタッチ → リンク → 削除

</details>

---

### 問題 4：EBO vs VBO

四角形を EBO なしで描くとき、何頂点のデータが必要か？EBO ありでは何頂点か？

<details>
<summary> 解答</summary>

- **EBO なし：** 三角形 2 つ = **6 頂点**（重複あり）
- **EBO あり：** ユニーク頂点 **4 頂点** + インデックス 6 個

EBO を使うと GPU のメモリを節約できる。複雑なモデルでは効果大。

</details>

---

### 問題 5：GLSLコード穴埋め

```glsl
// 頂点シェーダー
#version 330 【 ① 】

layout (location = 0) 【 ② 】 vec3 aPos;

void main() {
    gl_Position = vec4(【 ③ 】, 1.0);
}
```

```glsl
// フラグメントシェーダー
#version 330 core

【 ④ 】 vec4 FragColor;

void main() {
    FragColor = vec4(1.0f, 0.5f, 0.2f, 【 ⑤ 】); // 不透明なオレンジ
}
```

<details>
<summary> 解答</summary>

① `core`  
② `in`  
③ `aPos`  
④ `out`  
⑤ `1.0f`

</details>

---

## 実践課題

### 課題 1：三角形を描く 

本章のコードをすべて組み合わせて、画面にオレンジ色の三角形を表示しなさい。

**チェックポイント：**
- [ ] ウィンドウが開く
- [ ] 暗いシアンの背景に三角形が見える
- [ ] ESC で終了できる

### 課題 2：別の形にする 

頂点データを変更して以下の形を描きなさい。
1. 逆さ三角形（▽）
2. 四角形（EBO を使う）

### 課題 3：2 つの三角形 

VAO・VBO を 2 つずつ用意して、2 つの三角形を別々のオブジェクトとして描きなさい。

```
ヒント：
unsigned int VAOs[2], VBOs[2];
glGenVertexArrays(2, VAOs);
glGenBuffers(2, VBOs);
```

### 課題 4：別の色で 2 つの三角形 

課題 3 に加えて、2 つの三角形に異なるフラグメントシェーダー（別の色）を適用しなさい。

---

## ナビゲーション

 [Hello Window](./03-hello-window.md) | [シェーダー →](./05-shaders.md)
