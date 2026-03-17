# 📘 高度なデータ操作（Advanced Data）

> **目標：** OpenGL のバッファオブジェクトを柔軟に操作する方法（部分更新、直接マッピング、バッファ間コピー、データレイアウト戦略）を習得する

---

## 📖 バッファの基本おさらい

これまで `glBufferData` でバッファ全体にデータを転送してきました。しかし実際のアプリケーションでは、バッファの一部だけを更新したり、CPU から直接バッファメモリに書き込んだりする場面が多くあります。

```
バッファ操作の全体像:

  glBufferData        → バッファ全体を（再）確保＋データ転送
  glBufferSubData     → バッファの一部を更新
  glMapBuffer         → バッファメモリへのポインタを取得
  glCopyBufferSubData → バッファ間でデータをコピー
```

---

## 📖 glBufferSubData による部分更新

`glBufferData` はバッファ全体を再確保しますが、`glBufferSubData` は既存バッファの一部だけを上書きします。

```cpp
// パターン1: まず空のバッファを確保し、後からデータを埋める
glBufferData(GL_ARRAY_BUFFER, totalSize, NULL, GL_STATIC_DRAW);

// 位置データを先頭に書き込み
glBufferSubData(GL_ARRAY_BUFFER, 0, posSize, positions);

// 法線データを位置データの後に書き込み
glBufferSubData(GL_ARRAY_BUFFER, posSize, normSize, normals);

// UVデータをさらに後ろに書き込み
glBufferSubData(GL_ARRAY_BUFFER, posSize + normSize, uvSize, uvs);
```

```
バッファメモリのレイアウト:

 オフセット: 0          posSize    posSize+normSize
    ┌──────────────┬────────────┬──────────────┐
    │  positions   │  normals   │     uvs      │
    │  (posSize)   │ (normSize) │  (uvSize)    │
    └──────────────┴────────────┴──────────────┘
    ← ───────── totalSize ────────────────────→
```

**なぜ使うのか？** バッファ全体ではなく変更があった部分だけを更新できるため、大きなバッファで一部のデータだけが変わる場合に効率的です。

関数シグネチャ:
```cpp
void glBufferSubData(GLenum target,
                     GLintptr offset,  // 書き込み開始位置（バイト）
                     GLsizeiptr size,  // 書き込みサイズ（バイト）
                     const void *data);
```

---

## 📖 glMapBuffer によるバッファへの直接アクセス

`glBufferSubData` はデータの「コピー」を行いますが、`glMapBuffer` はバッファメモリへの **ポインタ** を直接取得し、C/C++ のポインタ操作で読み書きできます。

```cpp
// バッファをマップ（書き込みモード）
glBindBuffer(GL_ARRAY_BUFFER, vbo);
void *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

// ポインタを使って直接書き込み
memcpy(ptr, data, dataSize);

// マップ解除（これを忘れるとバグの原因！）
glUnmapBuffer(GL_ARRAY_BUFFER);
```

**アクセスモードの種類:**

| モード | 意味 |
|---|---|
| `GL_READ_ONLY` | 読み取り専用 |
| `GL_WRITE_ONLY` | 書き込み専用 |
| `GL_READ_WRITE` | 読み書き両用 |

**`glMapBuffer` が便利なケース:**
- ファイルからバッファに直接読み込む（中間変数不要）
- パーティクルシステムなど、大量の頂点を毎フレーム更新する場合
- GPU からバッファの内容を読み戻す場合

```
glBufferSubData vs glMapBuffer:

  glBufferSubData:
    CPU配列 ──[コピー]──→ ドライババッファ ──[転送]──→ GPU

  glMapBuffer:
    CPU ──[直接ポインタ]──→ ドライババッファ ──[転送]──→ GPU
    （中間コピーが1回減る可能性がある）
```

> **注意:** `glUnmapBuffer` を呼ぶまで OpenGL はそのバッファを使用できません。マップ中にバッファを使って描画するとエラーになります。

---

## 📖 バッファの使い方ヒント（Usage Hints）

`glBufferData` の第4引数は、ドライバに対してバッファの使い方を「ヒント」として伝えます。

```cpp
glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
//                                        ^^^^^^^^^^^^^^^^ ヒント
```

ヒントは2つの部分から構成されます:

| 接頭辞 | 意味 |
|---|---|
| `STATIC` | データはほぼ変更されない |
| `DYNAMIC` | データが頻繁に変更される |
| `STREAM` | データが毎フレーム変更される |

| 接尾辞 | 意味 |
|---|---|
| `DRAW` | CPU → GPU（描画用データ） |
| `READ` | GPU → CPU（結果の読み戻し） |
| `COPY` | GPU → GPU（バッファ間コピー） |

### 組み合わせ例

| ヒント | 最適な用途 |
|---|---|
| `GL_STATIC_DRAW` | 3Dモデルの頂点データ（変更なし） |
| `GL_DYNAMIC_DRAW` | UI 要素（位置が頻繁に変わる） |
| `GL_STREAM_DRAW` | パーティクル頂点（毎フレーム更新） |
| `GL_STATIC_READ` | GPU 計算結果の読み戻し（1回） |

**重要:** これは「ヒント」であり、強制ではありません。`GL_STATIC_DRAW` のバッファに `glBufferSubData` で書き込んでも**動作はします**。ただしドライバが最適なメモリ配置を選べなくなり、性能が落ちる可能性があります。

---

## 📖 頂点属性のデータレイアウト

### インターリーブ（Interleaved）方式

これまで使ってきた方式。1つの VBO に位置・法線・UV を交互に格納します。

```
インターリーブ配置:
┌─────────────────────────────────────────────────────┐
│ P0 N0 UV0 │ P1 N1 UV1 │ P2 N2 UV2 │ P3 N3 UV3 │ … │
└─────────────────────────────────────────────────────┘
  1頂点分      1頂点分        …

ストライド = sizeof(位置) + sizeof(法線) + sizeof(UV)
           = 12 + 12 + 8 = 32 bytes
```

```cpp
// インターリーブ方式の属性設定
GLsizei stride = 8 * sizeof(float); // 3+3+2 = 8 floats

// 位置: オフセット 0
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                      (void*)0);
// 法線: オフセット 12 bytes (3 floats)
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                      (void*)(3 * sizeof(float)));
// UV: オフセット 24 bytes (6 floats)
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                      (void*)(6 * sizeof(float)));
```

### 分離（Separate / Batched）方式

属性ごとに別々のブロックにまとめて格納します。

```
分離配置:
┌──────────────────────────────────────────┐
│ P0 P1 P2 P3 … │ N0 N1 N2 N3 … │ UV0 UV1 UV2 … │
└──────────────────────────────────────────┘
 位置ブロック      法線ブロック      UVブロック
```

```cpp
// まず空のバッファを確保
glBufferData(GL_ARRAY_BUFFER,
             posSize + normSize + uvSize, NULL, GL_STATIC_DRAW);

// 各ブロックを別々に書き込み
glBufferSubData(GL_ARRAY_BUFFER, 0, posSize, positions);
glBufferSubData(GL_ARRAY_BUFFER, posSize, normSize, normals);
glBufferSubData(GL_ARRAY_BUFFER, posSize + normSize, uvSize, uvs);

// 属性設定（ストライドは各属性のサイズのみ）
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                      (void*)0);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                      (void*)(posSize));
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                      (void*)(posSize + normSize));
```

### 比較

| 項目 | インターリーブ | 分離 |
|---|---|---|
| メモリ局所性 | 高い（1頂点のデータが連続） | 低い |
| GPU キャッシュ効率 | 一般的に良い | やや劣る |
| 一部属性の更新 | 困難（全体に散らばる） | 容易（ブロック単位） |
| 複数 VBO | 不要 | 属性ごとに分ける場合あり |
| 推奨場面 | 静的メッシュ | 動的更新が必要な場合 |

---

## 📖 glCopyBufferSubData によるバッファ間コピー

あるバッファの内容を別のバッファにコピーできます。

```cpp
void glCopyBufferSubData(GLenum readTarget,  // コピー元
                         GLenum writeTarget, // コピー先
                         GLintptr readOffset,
                         GLintptr writeOffset,
                         GLsizeiptr size);
```

### ターゲットの競合問題

2つの VBO 間でコピーしたい場合、どちらも `GL_ARRAY_BUFFER` にバインドしたいですが、同じターゲットには同時に1つしかバインドできません。

```
問題: 2つの VBO をどこにバインドする？

  GL_ARRAY_BUFFER ← vbo1  ✓
  GL_ARRAY_BUFFER ← vbo2  ✗ （vbo1 を上書きしてしまう）
```

この問題を解決するために `GL_COPY_READ_BUFFER` と `GL_COPY_WRITE_BUFFER` という専用ターゲットがあります。

```cpp
glBindBuffer(GL_COPY_READ_BUFFER, vbo1);  // コピー元
glBindBuffer(GL_COPY_WRITE_BUFFER, vbo2); // コピー先

glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
                    0, 0, dataSize);
```

```
コピーの流れ:

  GL_COPY_READ_BUFFER  ← vbo1（ソース）
  GL_COPY_WRITE_BUFFER ← vbo2（デスト）

  vbo1: ┌───────────────┐
        │ ABCDEF        │ ── readOffset: 0, size: 6 ──→
        └───────────────┘
  vbo2: ┌───────────────┐
        │ ______ABCDEF  │ ← writeOffset: 6
        └───────────────┘
```

> **補足:** 同じターゲットを read と write の両方に指定することも可能です（例: `GL_ARRAY_BUFFER` を両方に使う）。ただし、コピー元と先が異なるバッファオブジェクトであれば `GL_COPY_READ/WRITE_BUFFER` を使うのがクリーンです。

---

## 📖 バッファオブジェクトの内部動作

```
┌──────────────────────────────────────────────────────┐
│                    OpenGL コンテキスト                  │
│                                                      │
│  バインドポイント（ターゲット）:                        │
│   GL_ARRAY_BUFFER ──────→ [Buffer Object #3]         │
│   GL_ELEMENT_ARRAY_BUFFER → [Buffer Object #7]       │
│   GL_COPY_READ_BUFFER ───→ [Buffer Object #3]        │
│   GL_COPY_WRITE_BUFFER ──→ [Buffer Object #12]       │
│                                                      │
│  Buffer Object #3:                                   │
│   ┌──────────────────────────────────┐               │
│   │ メタデータ: size, usage, mapped  │               │
│   │ データストア: [GPU メモリ上]       │               │
│   └──────────────────────────────────┘               │
│                                                      │
│  1つのバッファを複数ターゲットに同時バインド可能        │
└──────────────────────────────────────────────────────┘
```

バッファオブジェクトは単なる「名前付きメモリ領域」であり、ターゲットはバインドポイント（接続口）に過ぎません。同じバッファを `GL_ARRAY_BUFFER` と `GL_COPY_READ_BUFFER` に同時にバインドすることも可能です。

---

## 💡 ポイントまとめ

| 項目 | 内容 |
|---|---|
| `glBufferSubData` | バッファの一部を更新。オフセットとサイズを指定 |
| `glMapBuffer` | バッファメモリへの直接ポインタ取得 |
| `glUnmapBuffer` | マップ解除。忘れるとバグの原因 |
| `GL_STATIC_DRAW` | ほぼ変更しないデータ向け |
| `GL_DYNAMIC_DRAW` | 頻繁に変更されるデータ向け |
| `GL_STREAM_DRAW` | 毎フレーム変更されるデータ向け |
| インターリーブ | 1頂点のデータが連続。キャッシュ効率良好 |
| 分離方式 | 属性ごとにブロック化。部分更新に強い |
| `glCopyBufferSubData` | バッファ間コピー |
| `GL_COPY_READ/WRITE_BUFFER` | コピー専用のバインドターゲット |

---

## ✏️ ドリル問題

### 問1（穴埋め）
既存バッファの一部にデータを書き込むには `gl___(GL_ARRAY_BUFFER, offset, size, data)` を使う。

<details><summary>📝 解答</summary>

`glBufferSubData`

</details>

### 問2（穴埋め）
バッファメモリへの直接ポインタを取得するには `gl___(GL_ARRAY_BUFFER, GL_WRITE_ONLY)` を呼び、使い終わったら `gl___` で解除する。

<details><summary>📝 解答</summary>

`glMapBuffer` と `glUnmapBuffer`

</details>

### 問3（選択）
パーティクルシステムで毎フレーム頂点データを書き換える場合、最適な使い方ヒントはどれか？

A. `GL_STATIC_DRAW`  
B. `GL_DYNAMIC_DRAW`  
C. `GL_STREAM_DRAW`  
D. `GL_STATIC_READ`  

<details><summary>📝 解答</summary>

**C. `GL_STREAM_DRAW`**

毎フレーム全データが書き換えられる場合は `STREAM` が最適。`DYNAMIC` はデータが「頻繁に」変わる場合ですが、毎フレーム全更新なら `STREAM` がより適切です。

</details>

### 問4（計算）
インターリーブ方式で、位置（vec3: 12byte）、法線（vec3: 12byte）、UV（vec2: 8byte）を格納する場合、ストライドは何バイトか？ また、UV 属性のバイトオフセットはいくつか？

<details><summary>📝 解答</summary>

- ストライド: 12 + 12 + 8 = **32 バイト**
- UV のオフセット: 12 + 12 = **24 バイト**

</details>

### 問5（選択）
2つの `GL_ARRAY_BUFFER` バッファ間でデータをコピーするためにある、専用のバッファターゲットはどれか？（2つ選べ）

A. `GL_PIXEL_PACK_BUFFER`  
B. `GL_COPY_READ_BUFFER`  
C. `GL_COPY_WRITE_BUFFER`  
D. `GL_TRANSFORM_FEEDBACK_BUFFER`  

<details><summary>📝 解答</summary>

**B と C**。`GL_COPY_READ_BUFFER` と `GL_COPY_WRITE_BUFFER` はバッファ間コピーのための専用ターゲットです。

</details>

### 問6（計算）
分離方式で、100 頂点分の位置（vec3）・法線（vec3）・UV（vec2）を1つの VBO に格納する。法線ブロックの開始オフセットと UV ブロックの開始オフセットを求めよ。

<details><summary>📝 解答</summary>

- 位置ブロック: 100 × 3 × 4byte = 1200 byte（オフセット 0）
- 法線ブロック開始: **1200 byte**
- 法線ブロックサイズ: 100 × 3 × 4byte = 1200 byte
- UV ブロック開始: 1200 + 1200 = **2400 byte**

</details>

### 問7（記述）
`glMapBuffer` 使用中に描画コマンドを発行するとどうなるか説明せよ。

<details><summary>📝 解答</summary>

バッファがマップされている間は OpenGL がそのバッファにアクセスできないため、描画コマンドは未定義動作やエラーを引き起こします。`glUnmapBuffer` でマップを解除してから描画する必要があります。

</details>

---

## 🔨 実践課題

### 課題1: glBufferSubData による段階的データ埋め ⭐⭐
空の VBO を確保し、`glBufferSubData` を使って位置・色・UV のデータを別々のタイミングで書き込むプログラムを作成せよ。

**チェックポイント:**
- [ ] `glBufferData` に NULL を渡して空バッファを確保
- [ ] `glBufferSubData` でオフセットを正しく計算して書き込み
- [ ] `glVertexAttribPointer` のオフセットが書き込み位置と一致
- [ ] 正しく描画される

### 課題2: glMapBuffer によるリアルタイム更新 ⭐⭐⭐
三角形の頂点色を `glMapBuffer` で毎フレーム更新し、虹色アニメーションを実装せよ。

**チェックポイント:**
- [ ] `glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)` でポインタ取得
- [ ] 時間（`glfwGetTime()`）に基づいて色を計算
- [ ] `glUnmapBuffer` を必ず呼んでいる
- [ ] スムーズに色が変化する

### 課題3: インターリーブ vs 分離の比較 ⭐⭐⭐
同じメッシュデータをインターリーブ方式と分離方式の2通りで格納し、両方が同じ描画結果になることを確認せよ。

**チェックポイント:**
- [ ] 同一のメッシュを2つの方式で格納
- [ ] `glVertexAttribPointer` のストライドとオフセットが各方式で異なる
- [ ] 画面を分割して左右に描画し、同じ見た目であることを確認
- [ ] 各方式の `glVertexAttribPointer` 呼び出しをコメントで説明

### 課題4: バッファ間コピー ⭐⭐⭐⭐
VBO の内容を `glCopyBufferSubData` で別の VBO にコピーし、コピー先 VBO を使って描画が正しく行われることを確認せよ。

**チェックポイント:**
- [ ] `GL_COPY_READ_BUFFER` / `GL_COPY_WRITE_BUFFER` を使用
- [ ] コピー元とコピー先のサイズが一致
- [ ] コピー先 VBO に VAO を設定して描画
- [ ] コピー元と同一の描画結果になる

---

## 🔗 ナビゲーション

⬅️ [キューブマップ](./06-cubemaps.md) | ➡️ [高度な GLSL →](./08-advanced-glsl.md)
