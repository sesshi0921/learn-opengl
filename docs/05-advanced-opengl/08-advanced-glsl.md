# 高度な GLSL（Advanced GLSL）

> **目標：** GLSL の組み込み変数を活用し、インターフェースブロックとユニフォームバッファオブジェクト（UBO）で効率的なシェーダー間データ共有を実装できるようになる

---

## GLSL の組み込み変数

GLSL には、シェーダーステージ間でデータをやり取りするための特別な変数が用意されています。明示的に宣言しなくても使えるものがあり、レンダリングパイプラインの動作を細かく制御できます。

```
パイプラインと組み込み変数の対応:

  頂点シェーダー                 フラグメントシェーダー
  ┌────────────────────┐       ┌────────────────────┐
  │ 入力:               │       │ 入力:               │
  │   gl_VertexID       │       │   gl_FragCoord      │
  │                     │       │   gl_FrontFacing    │
  │ 出力:               │  ──→  │                     │
  │   gl_Position       │       │ 出力:               │
  │   gl_PointSize      │       │   gl_FragDepth      │
  └────────────────────┘       └────────────────────┘
```

---

## 頂点シェーダーの組み込み変数

### gl_Position

頂点シェーダーの **必須出力**。クリップ空間での頂点位置を指定し、パースペクティブ除算後に NDC（-1〜+1）に変換されます。

```glsl
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
```

### gl_PointSize

`GL_POINTS` 描画時のポイントサイズ（ピクセル単位）。C++ 側で `glEnable(GL_PROGRAM_POINT_SIZE)` が必要です。

```glsl
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_PointSize = gl_Position.z; // 遠いほど大きく（デモ用）
}
```

活用例: パーティクルシステム、ポイントクラウド、星空の表現

### gl_VertexID

現在処理中の頂点インデックス。`glDrawElements` ではインデックスバッファ値、`glDrawArrays` では通し番号。

```glsl
void main() {
    if (gl_VertexID % 2 == 0)
        gl_Position = vec4(aPos * 1.1, 1.0); // 偶数頂点を拡大
    else
        gl_Position = vec4(aPos, 1.0);
}
```

---

## フラグメントシェーダーの組み込み変数

### gl_FragCoord（ウィンドウ座標）

現在のフラグメントのウィンドウ空間座標（ピクセル座標）。`.z` は深度値（0.0〜1.0）。

```
  (0, height) ──── (width, height)
    │                    │
    │  gl_FragCoord.xy   │
    │                    │
  (0, 0) ──────── (width, 0)
```

```glsl
void main() {
    if (gl_FragCoord.y < 400.0)
        FragColor = vec4(1.0, 0.0, 0.0, 1.0); // 下半分: 赤
    else
        FragColor = vec4(0.0, 0.0, 1.0, 1.0); // 上半分: 青
}
```

### gl_FrontFacing（フロントフェイス判定）

表面なら `true`、裏面なら `false` の `bool` 値。表裏で異なるテクスチャを適用できます。

```glsl
void main() {
    if (gl_FrontFacing)
        FragColor = texture(frontTexture, TexCoords);
    else
        FragColor = texture(backTexture, TexCoords);
}
```

> **注意:** `glEnable(GL_CULL_FACE)` が有効だと裏面は描画されず、常に `true` になります。

---

## gl_FragDepth（深度の手動書き換え）

`gl_FragDepth` に値を書き込むと深度値を上書きできますが、**Early Depth Testing**（FS 実行前に深度テストする最適化）が無効になります。

```glsl
gl_FragDepth = 0.0; // 最前面に強制
```

OpenGL 4.2+ ではレイアウト修飾子で制約を宣言し、Early Depth Test を部分的に維持できます:

```glsl
layout (depth_greater) out float gl_FragDepth; // 元の値以上にのみ変更
```

| 修飾子 | 意味 | Early Depth Test |
|---|---|---|
| `depth_any` | 任意（デフォルト） | 無効 |
| `depth_greater` | 元の値以上 | 部分的に有効 |
| `depth_less` | 元の値以下 | 部分的に有効 |
| `depth_unchanged` | 変更しない | 完全に有効 |

---

## インターフェースブロック

頂点シェーダーからフラグメントシェーダーに複数の変数を渡す際、`in/out` を1つずつ宣言する代わりに **ブロック** としてまとめることができます。

```glsl
// 頂点シェーダー                        // フラグメントシェーダー
out VS_OUT {                          in VS_OUT {        // ブロック名は同じ
    vec2 TexCoords;                       vec2 TexCoords;
    vec3 FragPos;                         vec3 FragPos;
    vec3 Normal;                          vec3 Normal;
} vs_out;                             } fs_in;           // インスタンス名は異なってOK
```

**ルール:** ブロック型名（`VS_OUT`）とメンバーの型・名・順序は一致必須。インスタンス名は異なってよい。

---

## ユニフォームバッファオブジェクト（UBO）

### 問題: uniform の重複設定

複数のシェーダーで同じ `projection` と `view` 行列を使う場合、各シェーダーに対して個別に `setMat4` を呼ぶ必要があります。

```cpp
// シェーダーが3つあると…
shaderA.setMat4("projection", projection);
shaderA.setMat4("view", view);
shaderB.setMat4("projection", projection);
shaderB.setMat4("view", view);
shaderC.setMat4("projection", projection);
shaderC.setMat4("view", view);
// 冗長！
```

### 解決: UBO で共有

UBO を使えば、1つのバッファに uniform データを格納し、複数のシェーダーから参照できます。

```
UBO の仕組み:

  ┌─────────────────────────┐
  │    Uniform Buffer       │
  │  ┌───────────────────┐  │
  │  │ projection (mat4) │  │   バインディングポイント 0
  │  │ view       (mat4) │  │ ──────────────┐
  │  └───────────────────┘  │               │
  └─────────────────────────┘               │
                                            ▼
                              ┌──────────────────────┐
                              │ GL Binding Point 0   │
                              └───┬──────┬──────┬───┘
                                  │      │      │
                                  ▼      ▼      ▼
                              ShaderA ShaderB ShaderC
                              (block  (block  (block
                               idx 0)  idx 0)  idx 0)
```

### GLSL 側: uniform ブロック宣言

```glsl
#version 330 core
layout (std140) uniform Matrices {
    mat4 projection;  // オフセット: 0
    mat4 view;        // オフセット: 64
};
// 合計: 128 bytes
```

### std140 レイアウト規則

`std140` はメモリ上の配置を厳密に定義するレイアウトです。C++ 側から正しいオフセットでデータを書き込むために、この規則を理解する必要があります。

| データ型 | サイズ | アラインメント |
|---|---|---|
| `int`, `float`, `bool` | 4 byte | 4 byte |
| `vec2` | 8 byte | 8 byte |
| `vec3` | 12 byte | **16 byte**（注意！） |
| `vec4` | 16 byte | 16 byte |
| `mat4` | 64 byte | 16 byte（各列が vec4） |
| 配列の各要素 | 要素サイズ | **16 byte** に切り上げ |

**最大の落とし穴:** `vec3` は 12 byte ですが、アラインメントが **16 byte** です。つまり次のメンバーは 16 の倍数のオフセットから始まります。

### std140 オフセット計算例

```glsl
layout (std140) uniform ExampleBlock {
    float value;      // オフセット: 0   (サイズ 4,  アライン 4)
    vec3  vector;     // オフセット: 16  (サイズ 12, アライン 16)
    mat4  matrix;     // オフセット: 32  (サイズ 64, アライン 16)
    float values[3];  // オフセット: 96  (各 16 byte にパディング)
    bool  boolean;    // オフセット: 144 (サイズ 4,  アライン 4)
    int   integer;    // オフセット: 148 (サイズ 4,  アライン 4)
};
// 合計: 152 bytes
```

**ポイント:** `value`(4byte) の後、`vector`(vec3, アライン16) までに 12byte のパディングが入ります。配列 `values[3]` の各要素も 16byte にパディングされるため、float 1個に 12byte の空きが生じます。

### C++ 側: UBO の作成と設定

```cpp
// ステップ1: UBO の作成
unsigned int uboMatrices;
glGenBuffers(1, &uboMatrices);
glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4),
             NULL, GL_STATIC_DRAW);
glBindBuffer(GL_UNIFORM_BUFFER, 0);

// ステップ2: バインディングポイントに紐づけ
glBindBufferRange(GL_UNIFORM_BUFFER,
                  0,                    // バインディングポイント
                  uboMatrices,
                  0,                    // オフセット
                  2 * sizeof(glm::mat4)); // サイズ

// ステップ3: 各シェーダーの uniform ブロックをバインディングポイントに接続
unsigned int blockIndexA = glGetUniformBlockIndex(shaderA.ID, "Matrices");
glUniformBlockBinding(shaderA.ID, blockIndexA, 0); // →ポイント0

unsigned int blockIndexB = glGetUniformBlockIndex(shaderB.ID, "Matrices");
glUniformBlockBinding(shaderB.ID, blockIndexB, 0); // →ポイント0
```

### レンダリングループでのデータ更新

```cpp
// projection は1度だけ設定
glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
glBufferSubData(GL_UNIFORM_BUFFER, 0,
                sizeof(glm::mat4), glm::value_ptr(projection));

// view は毎フレーム更新
glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4),
                sizeof(glm::mat4), glm::value_ptr(view));
glBindBuffer(GL_UNIFORM_BUFFER, 0);

// 各シェーダーの model だけ個別に設定
shaderA.use();
shaderA.setMat4("model", modelA);
// 描画...

shaderB.use();
shaderB.setMat4("model", modelB);
// 描画...
```

### 接続関係のまとめ図

```
glUniformBlockBinding  →  シェーダーのブロック ←→ バインディングポイント
glBindBufferBase/Range →  UBO ←→ バインディングポイント

  Shader A: "Matrices" ─┐
  Shader B: "Matrices" ─┤── Binding Point 0 ── UBO (uboMatrices)
  Shader C: "Matrices" ─┘
```

---

## 💡 ポイントまとめ

| 項目 | 内容 |
|---|---|
| `gl_Position` | 頂点シェーダーの必須出力。クリップ空間座標 |
| `gl_PointSize` | ポイント描画のサイズ。`GL_PROGRAM_POINT_SIZE` 有効化が必要 |
| `gl_VertexID` | 頂点のインデックス番号 |
| `gl_FragCoord` | フラグメントのウィンドウ座標 (x, y, z) |
| `gl_FrontFacing` | 表面なら true、裏面なら false |
| `gl_FragDepth` | 深度の手動設定。Early Depth Test に影響 |
| インターフェースブロック | `in/out` をブロックにまとめる。ブロック名は一致必須 |
| UBO | 複数シェーダーで共有する uniform バッファ |
| `std140` | メモリレイアウトを厳密に定義。vec3 は 16byte アライン |
| `glUniformBlockBinding` | シェーダーブロック → バインディングポイント |
| `glBindBufferBase/Range` | UBO → バインディングポイント |

---

## ドリル問題

### 問1（穴埋め）
ポイントスプライトのサイズをシェーダーから制御するには、C++ 側で `glEnable(_____)` を呼んでから、頂点シェーダーで `gl_____` に値を代入する。

<details><summary> 解答</summary>

`GL_PROGRAM_POINT_SIZE` と `gl_PointSize`

</details>

### 問2（選択）
`gl_FragDepth` に値を書き込んだ場合、デフォルトで何が起こるか？

A. 深度テストが完全に無効になる  
B. Early Depth Testing が無効になる  
C. フラグメントシェーダーの実行がスキップされる  
D. 深度バッファが読み取り専用になる  

<details><summary> 解答</summary>

**B**。`gl_FragDepth` への書き込みは深度値を変更する可能性があるため、フラグメントシェーダー実行前の深度テスト（Early Depth Testing）が無効になります。

</details>

### 問3（std140 オフセット計算）
以下の uniform ブロックの各メンバーのオフセットを計算せよ。

```glsl
layout (std140) uniform LightBlock {
    vec4  position;    // オフセット: ?
    vec3  color;       // オフセット: ?
    float intensity;   // オフセット: ?
    mat4  lightSpace;  // オフセット: ?
};
```

<details><summary> 解答</summary>

1. `position` (vec4): アライン 16 → オフセット **0**、サイズ 16
2. `color` (vec3): アライン 16 → オフセット **16**、サイズ 12
3. `intensity` (float): アライン 4 → オフセット **28**（16+12=28、4の倍数なのでOK）、サイズ 4
4. `lightSpace` (mat4): アライン 16 → オフセット **32**（28+4=32、16の倍数なのでOK）、サイズ 64

合計: 32 + 64 = **96 bytes**

</details>

### 問4（std140 オフセット計算）
以下のブロックで `flag` のオフセットはいくつか？

```glsl
layout (std140) uniform TestBlock {
    float a;       // 0
    vec3  b;       // ?
    float c;       // ?
    bool  flag;    // ?
};
```

<details><summary> 解答</summary>

1. `a` (float): オフセット 0、サイズ 4
2. `b` (vec3): アライン 16 → 次の 16 の倍数 = オフセット **16**、サイズ 12
3. `c` (float): アライン 4 → 16 + 12 = 28、4の倍数なので → オフセット **28**、サイズ 4
4. `flag` (bool): アライン 4 → 28 + 4 = 32、4の倍数なので → オフセット **32**

</details>

### 問5（穴埋め）
UBO をバインディングポイント 2 に接続するには `glBindBuffer___(GL_UNIFORM_BUFFER, 2, ubo)` を呼ぶ。

<details><summary> 解答</summary>

`glBindBufferBase`

完全な呼び出し: `glBindBufferBase(GL_UNIFORM_BUFFER, 2, ubo)`

</details>

### 問6（選択）
インターフェースブロックで、頂点シェーダーとフラグメントシェーダー間で一致させる必要があるのはどれか？

A. ブロックの型名のみ  
B. インスタンス名のみ  
C. ブロックの型名とメンバーの型・名前・順序  
D. すべて（型名、インスタンス名、メンバー）  

<details><summary> 解答</summary>

**C**。ブロックの型名（例: `VS_OUT`）とメンバーの型・名前・順序は一致が必要ですが、インスタンス名（例: `vs_out` / `fs_in`）は異なっていて構いません。

</details>

### 問7（記述）
`gl_FragCoord.y` を使って画面を上下2分割し、上半分では通常のテクスチャ表示、下半分ではグレースケール表示を行うフラグメントシェーダーを書け（画面高さは 600 ピクセルとする）。

<details><summary> 解答</summary>

```glsl
#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D tex;

void main() {
    vec4 color = texture(tex, TexCoords);
    if (gl_FragCoord.y < 300.0) {
        float gray = 0.2126 * color.r + 0.7152 * color.g
                   + 0.0722 * color.b;
        FragColor = vec4(gray, gray, gray, 1.0);
    } else {
        FragColor = color;
    }
}
```

</details>

---

## 実践課題

### 課題1: gl_FragCoord による画面分割エフェクト 
`gl_FragCoord` を使って画面を4分割し、各象限で異なるエフェクト（通常、グレースケール、反転、セピア）を適用せよ。

**チェックポイント:**
- [ ] `gl_FragCoord.x` と `gl_FragCoord.y` で4象限を判定
- [ ] 各エフェクトが正しく適用されている
- [ ] ウィンドウサイズを uniform で渡している（ハードコード回避）
- [ ] ウィンドウリサイズ時にも正しく動作する

### 課題2: UBO によるビュー・プロジェクション共有 
3つの異なるシェーダーで同じビュー行列とプロジェクション行列を UBO で共有し、3つの異なる色の立方体を描画せよ。

**チェックポイント:**
- [ ] `std140` レイアウトで uniform ブロックを宣言
- [ ] `glGetUniformBlockIndex` と `glUniformBlockBinding` で各シェーダーを接続
- [ ] `glBindBufferRange` で UBO をバインディングポイントに紐づけ
- [ ] projection は1度、view はフレームごとに `glBufferSubData` で更新
- [ ] model 行列のみ各シェーダーに個別設定

### 課題3: gl_FrontFacing による両面テクスチャ 
フェイスカリングを無効にし、`gl_FrontFacing` で表面と裏面に異なるテクスチャを表示する立方体を作成せよ。

**チェックポイント:**
- [ ] `glDisable(GL_CULL_FACE)` で両面描画を有効化
- [ ] 2つのテクスチャをバインド
- [ ] `gl_FrontFacing` で適切に切り替え
- [ ] 立方体を回転させると裏面テクスチャが見える

### 課題4: std140 バッファの検証ツール 
任意の uniform ブロック定義を入力すると、各メンバーの `std140` オフセットとパディングを計算して表示する C++ プログラムを作成せよ。

**チェックポイント:**
- [ ] `float`, `vec2`, `vec3`, `vec4`, `mat4`, `int`, `bool` に対応
- [ ] パディングバイト数を表示
- [ ] 配列型（`float[3]` 等）のアラインメントも正しく計算
- [ ] 合計サイズを表示

---

## ナビゲーション

 [高度なデータ操作](./07-advanced-data.md) | [ジオメトリシェーダー →](./09-geometry-shader.md)
