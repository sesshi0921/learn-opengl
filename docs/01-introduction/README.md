# 🎮 はじめに（Introduction）

> **目標：** この教材が何を目指すのか、OpenGL 学習の全体像を掴む

---

## 📖 OpenGL 学習へようこそ

OpenGL（Open Graphics Library）は、2D・3D グラフィックスを描画するための **クロスプラットフォームな API** です。1992 年に Silicon Graphics 社が公開して以来、ゲーム・CAD・映像制作など幅広い分野で使われ続けています。

この教材では、OpenGL の **コア機能（Core Profile）** を一から学びます。「三角形一つ描けない」状態から始めて、最終的にはリアルタイム 3D グラフィックスを自分の手で作れるようになることを目指します。

---

## 📖 この教材の構成

### 入門編（Getting Started）
環境構築から始め、三角形・テクスチャ・カメラまで、OpenGL の基礎を固めます。

### ライティング編（Lighting）
Phong 照明モデル、マテリアル、各種光源を実装し、リアルな光の表現を学びます。

### モデル読み込み編（Model Loading）
Assimp ライブラリを使って、外部 3D モデルファイルを読み込みます。

### 高度な OpenGL 編（Advanced OpenGL）
深度テスト、ステンシル、フレームバッファなど、高度なレンダリング技術を習得します。

### 高度なライティング編（Advanced Lighting）
シャドウ、法線マッピング、HDR、SSAO など、プロ品質の照明技術を実装します。

### PBR 編
物理ベースレンダリングの理論と実装を学びます。

---

## 📖 OpenGL とは何か（概要）

OpenGL は **仕様（Specification）** であり、実装ではありません。実際の処理は GPU ドライバが行います。

```
あなたのプログラム
      ↓  OpenGL API 呼び出し
   GPU ドライバ（OpenGL 実装）
      ↓
     GPU（実際の描画処理）
      ↓
    モニター
```

### コアプロファイル vs 互換プロファイル

| モード | 特徴 |
|--------|------|
| **互換プロファイル（Compatibility Profile）** | 古い API も使える。学習には不向き |
| **コアプロファイル（Core Profile）** | 古い API を廃止。現代的な書き方のみ。本教材はこちら |

---

## 📖 ステートマシンとしての OpenGL

OpenGL は **ステートマシン（状態機械）** です。現在の「状態」に基づいて描画が行われます。

```cpp
// 状態を設定する
glBindTexture(GL_TEXTURE_2D, myTexture);  // 「今使うテクスチャ」を設定

// 状態を使って描画する
glDrawArrays(GL_TRIANGLES, 0, 3);  // 設定した状態で三角形を描く
```

> 💡 **OpenGL の状態は「文脈（Context）」と呼ばれます。**  
> 何かを描画するには、まずその前に必要な状態をすべてセットする必要があります。

---

## 📖 オブジェクトとは

OpenGL では描画に使うリソース（テクスチャ、バッファなど）を **オブジェクト** として管理します。

```cpp
// オブジェクトを生成する
unsigned int objectId;
glGenObject(1, &objectId);

// オブジェクトをバインド（文脈にセット）する
glBindObject(GL_WINDOW_TARGET, objectId);

// オブジェクトに設定を行う
glSetObjectOption(GL_WINDOW_TARGET, GL_OPTION_WINDOW_WIDTH, 800);

// バインドを解除する
glBindObject(GL_WINDOW_TARGET, 0);
```

---

## 💡 ポイントまとめ

| キーワード | 意味 |
|------------|------|
| OpenGL | グラフィックス API の仕様（実装はドライバ） |
| コアプロファイル | 現代的な OpenGL（古い機能を使わない） |
| ステートマシン | 状態を設定してから描画する仕組み |
| コンテキスト | OpenGL の現在の状態全体 |
| オブジェクト | OpenGL リソースを表す ID ベースの構造体 |

---

## ✏️ ドリル問題

### 問題 1：穴埋め問題

次の文章の空欄を埋めなさい。

> OpenGL は グラフィックス API の【　①　】であり、実際の描画処理は【　②　】が行う。  
> 本教材では古い機能を廃止した【　③　】プロファイルを使用する。  
> OpenGL は現在の【　④　】に基づいて動作する【　⑤　】である。

<details>
<summary>📝 解答</summary>

① 仕様（Specification）  
② GPU ドライバ  
③ コア（Core）  
④ 状態（State）  
⑤ ステートマシン（State Machine）

</details>

---

### 問題 2：正誤問題

各文が正しい（○）か誤り（✗）か答えなさい。

1. OpenGL は Microsoft が開発したグラフィックス API である
2. コアプロファイルでは、古い OpenGL 関数を使用できない
3. OpenGL のオブジェクトは整数 ID で管理される
4. 同じプログラムを異なる OS で動かすとき、OpenGL の動作は完全に同一である

<details>
<summary>📝 解答</summary>

1. ✗（Silicon Graphics 社が開発した）
2. ○
3. ○
4. ✗（同じ仕様でも、ドライバによって微妙に動作が異なる場合がある）

</details>

---

### 問題 3：順序並べ替え

OpenGL でオブジェクトを使う手順を正しい順に並べなさい。

```
(A) glDrawArrays で描画する
(B) glGenObject でオブジェクトを生成する
(C) glBindObject でバインドする
(D) glSetObjectOption で設定を行う
```

<details>
<summary>📝 解答</summary>

**B → C → D → A**

生成 → バインド → 設定 → 描画

</details>

---

## 🔨 実践課題

### 課題 1：環境確認チェックリスト

以下の環境が揃っているか確認しなさい。

- [ ] C++17 対応コンパイラがインストールされている
- [ ] CMake 3.x がインストールされている
- [ ] GLFW のソースまたはバイナリを取得している
- [ ] GLAD のファイル（glad.c, glad.h）を用意している
- [ ] GLM ライブラリを取得している

### 課題 2：調べ学習

以下の用語について、自分の言葉で 1〜2 文で説明しなさい。

1. **Vulkan** と OpenGL の違いは何か？
2. **DirectX** は何が違うのか？
3. **WebGL** は OpenGL とどんな関係があるか？

---

## 🔗 次のチャプター

➡️ [OpenGL とは →](./02_01_opengl.md)
