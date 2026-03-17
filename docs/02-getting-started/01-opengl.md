# 📘 入門編 1：OpenGL とは

> **目標：** OpenGL の歴史・仕組み・拡張機能を理解する

---

## 📖 OpenGL の歴史

OpenGL は 1992 年、**Silicon Graphics Inc.（SGI）** によって公開されました。当時は高性能ワークステーション向けの API でしたが、現在では PC・モバイル・Web など幅広いプラットフォームで使われています。

```
1992 年  OpenGL 1.0 リリース（SGI）
   ↓
2004 年  OpenGL 2.0（シェーダー言語 GLSL 導入）
   ↓
2008 年  OpenGL 3.0（コアプロファイル登場、古い機能を deprecated に）
   ↓
2009 年  OpenGL 3.2（ジオメトリシェーダー）
   ↓
2010 年  OpenGL 4.0（テッセレーション）
   ↓
2017 年  OpenGL 4.6（最新版、現在もメンテナンス中）
```

> 💡 現在 OpenGL の仕様管理は **Khronos Group** が行っています。

---

## 📖 コアプロファイルの重要性

OpenGL 3.2 以降、2 つのプロファイルが存在します。

### 互換プロファイル（Compatibility Profile）
```cpp
// 古い書き方（非推奨）
glBegin(GL_TRIANGLES);
  glVertex3f(0.0f, 0.5f, 0.0f);
  glVertex3f(-0.5f, -0.5f, 0.0f);
  glVertex3f(0.5f, -0.5f, 0.0f);
glEnd();
```
→ 動くが、GPU の性能を活かせない。学習にも不向き。

### コアプロファイル（Core Profile）← 本教材
```cpp
// 現代的な書き方（VBO・VAO・シェーダーを使う）
glBindVertexArray(VAO);
glDrawArrays(GL_TRIANGLES, 0, 3);
```
→ GPU の並列処理を最大限活用。業界標準。

---

## 📖 拡張機能（Extensions）

OpenGL は **拡張機能（Extension）** という仕組みで機能を追加できます。GPU メーカー（NVIDIA・AMD など）が独自機能を提供する際も拡張機能として提供します。

```cpp
// 拡張機能が使えるか確認する
if (GL_ARB_extension_name) {
    // 拡張機能を使ったコード
} else {
    // 通常のコード
}
```

拡張機能のプレフィックス：

| プレフィックス | 意味 |
|----------------|------|
| `GL_ARB_` | Architecture Review Board 承認（安定） |
| `GL_EXT_` | 複数ベンダー合意 |
| `GL_NV_` | NVIDIA 独自 |
| `GL_AMD_` | AMD 独自 |
| `GL_INTEL_` | Intel 独自 |

---

## 📖 ステートマシンの詳細

OpenGL のすべての操作は **現在のコンテキスト（Context）** に対して行われます。

```
OpenGL Context
┌──────────────────────────────────┐
│ 現在バインドされているテクスチャ  │
│ 現在バインドされているシェーダー  │
│ 現在バインドされているバッファ    │
│ ビューポートの設定               │
│ ブレンディング設定               │
│ 深度テスト設定                   │
│ ...（数百の状態変数）            │
└──────────────────────────────────┘
```

> ⚠️ **注意：** コンテキストは 1 スレッドに 1 つ。マルチスレッドで OpenGL を使う場合は注意が必要です。

---

## 📖 GLAD：関数ポインタの管理

OpenGL の関数は、実行時に GPU ドライバから **動的にロード** する必要があります。これを手動で行うのは非常に煩雑なため、**GLAD** というツールを使います。

```cpp
// GLAD の初期化（ウィンドウ作成後に呼ぶ）
if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "GLAD の初期化に失敗しました" << std::endl;
    return -1;
}
```

GLAD なしで関数をロードするとこうなる（やってはいけない例）：
```cpp
// 手動でロードする場合（非常に煩雑）
typedef void (*PFNGLDRAWARRAYSPROC)(GLenum, GLint, GLsizei);
PFNGLDRAWARRAYSPROC glDrawArrays = 
    (PFNGLDRAWARRAYSPROC)wglGetProcAddress("glDrawArrays");
// ... これを 500 個以上の関数に対して行う → 現実的でない
```

---

## 💡 ポイントまとめ

| 概念 | 説明 |
|------|------|
| Khronos Group | OpenGL の仕様を管理する団体 |
| コアプロファイル | 現代的な OpenGL。本教材ではこちらを使用 |
| 拡張機能 | GPU メーカーが追加する機能 |
| コンテキスト | OpenGL の全状態を保持する「文脈」 |
| GLAD | OpenGL 関数を自動ロードするツール |

---

## ✏️ ドリル問題

### 問題 1：穴埋め問題

> OpenGL の仕様を管理しているのは【　①　】という団体である。  
> OpenGL 3.2 から、現代的な書き方のみを使う【　②　】プロファイルが導入された。  
> OpenGL の関数を実行時にロードするためのツールを【　③　】という。  
> NVIDIA 独自の拡張機能には【　④　】というプレフィックスがつく。

<details>
<summary>📝 解答</summary>

① Khronos Group  
② コア（Core）  
③ GLAD  
④ GL_NV_

</details>

---

### 問題 2：選択問題

OpenGL 2.0 で導入された重要な機能はどれか。

- (A) ジオメトリシェーダー
- (B) シェーダー言語（GLSL）
- (C) テッセレーション
- (D) コアプロファイル

<details>
<summary>📝 解答</summary>

**(B) シェーダー言語（GLSL）**

- ジオメトリシェーダー → OpenGL 3.2
- テッセレーション → OpenGL 4.0
- コアプロファイル → OpenGL 3.2

</details>

---

### 問題 3：コード理解

以下のコードについて答えなさい。

```cpp
if (GL_ARB_extension_name) {
    glCoolNewFunction();
} else {
    doNormalThing();
}
```

1. `GL_ARB_` のプレフィックスは何を意味するか？
2. なぜ `if` で確認してから使う必要があるか？

<details>
<summary>📝 解答</summary>

1. **Architecture Review Board** が承認した、安定した拡張機能であることを意味する
2. 拡張機能はすべての GPU・ドライバで使えるとは限らないため、使用前に対応確認が必要

</details>

---

### 問題 4：記述問題

コアプロファイルと互換プロファイルの違いを 2 点挙げなさい。

<details>
<summary>📝 解答</summary>

1. **機能の範囲：** 互換プロファイルは古い API（`glBegin/glEnd` など）が使えるが、コアプロファイルでは使用不可
2. **パフォーマンス：** コアプロファイルは GPU の並列処理を最大限活用できる設計になっている

</details>

---

## 🔨 実践課題

### 課題 1：GLAD ジェネレーターを使う

1. https://glad.dav1d.de/ にアクセスする（または GLAD2 を使う）
2. 以下の設定を選ぶ：
   - Language: **C/C++**
   - Specification: **OpenGL**
   - Profile: **Core**
   - Version: **3.3**
3. 「Generate」をクリックしてファイルをダウンロードする
4. `glad.c` と `glad/glad.h` がプロジェクトに含まれることを確認する

### 課題 2：バージョン確認コード

以下の関数を実装して、OpenGL のバージョンをコンソールに表示するプログラムを作りなさい。

```cpp
// ヒント：以下の関数を使う
const GLubyte* version = glGetString(GL_VERSION);
const GLubyte* vendor  = glGetString(GL_VENDOR);
const GLubyte* renderer = glGetString(GL_RENDERER);
```

**期待される出力例：**
```
OpenGL Version : 4.6.0 NVIDIA 528.49
Vendor         : NVIDIA Corporation
Renderer       : NVIDIA GeForce RTX 3070/PCIe/SSE2
```

---

## 🔗 ナビゲーション

⬅️ [はじめに](../01-introduction/README.md) | ➡️ [ウィンドウの作成 →](./02-creating-a-window.md)
