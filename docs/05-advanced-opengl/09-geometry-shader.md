# ジオメトリシェーダー（Geometry Shader）

> **目標：** ジオメトリシェーダーの仕組みを理解し、プリミティブの生成・変形・可視化デバッグに活用できるようになる。

---

## レンダリングパイプラインにおける位置づけ

ジオメトリシェーダーは、**頂点シェーダーの後**、**ラスタライザの前**に実行されるオプションのシェーダーステージです。頂点シェーダーが個々の頂点を処理するのに対し、ジオメトリシェーダーは**プリミティブ全体**（点・線・三角形）を受け取り、新しいプリミティブを生成・変形・破棄できます。

```
パイプライン: 頂点データ → [頂点シェーダー] → [ジオメトリシェーダー] → [ラスタライザ] → [フラグメントシェーダー]
                       (per-vertex)   (per-primitive)                (per-fragment)

ジオメトリシェーダーはプリミティブ1つを受け取り、0個以上のプリミティブを出力する
```

**なぜジオメトリシェーダーを使うのか？**

- GPU 上でプリミティブの数や形を動的に変更できる
- 法線の可視化などデバッグ描画をシェーダーだけで実現できる
- 爆発エフェクトやワイヤーフレーム表示をシンプルに実装できる
- ただし**パフォーマンスコストが高い**ため、用途は限定的

---

## 入出力プリミティブタイプ

### 入力（layout 修飾子で指定）

ジオメトリシェーダーが受け取るプリミティブの種類を `layout(xxx) in;` で宣言します。

| 入力タイプ | 対応するドローモード | gl_in[] の要素数 |
|--------------------------|----------------------------------------|------------------|
| `points` | `GL_POINTS` | 1 |
| `lines` | `GL_LINES`, `GL_LINE_STRIP` | 2 |
| `triangles` | `GL_TRIANGLES`, `GL_TRIANGLE_STRIP` | 3 |
| `lines_adjacency` | `GL_LINES_ADJACENCY` | 4 |
| `triangles_adjacency` | `GL_TRIANGLES_ADJACENCY` | 6 |

### 出力（layout 修飾子で指定）

生成するプリミティブの種類と、1回の呼び出しで出力できる最大頂点数を指定します。

| 出力タイプ | 説明 |
|--------------------|----------------------------------------|
| `points` | 点の集合 |
| `line_strip` | 連続する線分 |
| `triangle_strip` | 連続する三角形 |

```glsl
// 入力: 三角形、出力: 三角形ストリップ（最大3頂点）
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
```

> **max_vertices** を大きくしすぎるとパフォーマンスが低下します。必要最小限に設定しましょう。

---

## gl_in[] とビルトイン変数

`gl_in[]` 配列で入力頂点情報にアクセスします。

```glsl
in gl_PerVertex {
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];
```

さらに `gl_Position`、`gl_PointSize` を出力として設定できます。

---

## EmitVertex() と EndPrimitive()

ジオメトリシェーダーの核心となる2つの関数：

- **`EmitVertex()`** — 現在の `gl_Position` と out 変数で頂点を出力ストリームに追加
- **`EndPrimitive()`** — それまでに追加された頂点群を1つのプリミティブとして確定

`triangle_strip` 出力で n 頂点を出力すると (n-2) 個の三角形が生成されます。`EndPrimitive()` でストリップを確定し、次の `EmitVertex()` から新しいストリップが始まります。

---

## 基本例：パススルージオメトリシェーダー

まずは入力をそのまま出力するだけの最小限のジオメトリシェーダーです。

**頂点シェーダー：**

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out VS_OUT {
    vec3 color;
} vs_out;

void main() {
    gl_Position = vec4(aPos, 1.0);
    vs_out.color = aColor;
}
```

**ジオメトリシェーダー（パススルー）：**

```glsl
#version 330 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec3 color;
} gs_in[];

out vec3 fColor;

void main() {
    for (int i = 0; i < 3; i++) {
        gl_Position = gl_in[i].gl_Position;
        fColor = gs_in[i].color;
        EmitVertex();
    }
    EndPrimitive();
}
```

**フラグメントシェーダー：**

```glsl
#version 330 core
in vec3 fColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(fColor, 1.0);
}
```

> インターフェースブロック(`VS_OUT`)を使うことでデータ受け渡しが明確になります。ジオメトリシェーダー側では配列 `gs_in[]` として受け取る点に注意。

---

## 応用例1：ポイントから家の形を生成

1つの点から小さな家の形を描く例です。

```glsl
#version 330 core
layout(points) in;
layout(triangle_strip, max_vertices = 5) out;

out vec3 fColor;

void build_house(vec4 position) {
    float size = 0.1;

    // 左下
    fColor = vec3(1.0, 0.0, 0.0);  // 赤
    gl_Position = position + vec4(-size, -size, 0.0, 0.0);
    EmitVertex();
    // 右下
    fColor = vec3(0.0, 1.0, 0.0);  // 緑
    gl_Position = position + vec4( size, -size, 0.0, 0.0);
    EmitVertex();
    // 左上
    fColor = vec3(0.0, 0.0, 1.0);  // 青
    gl_Position = position + vec4(-size,  size, 0.0, 0.0);
    EmitVertex();
    // 右上
    fColor = vec3(1.0, 1.0, 0.0);  // 黄
    gl_Position = position + vec4( size,  size, 0.0, 0.0);
    EmitVertex();
    // 屋根の頂点
    fColor = vec3(1.0, 1.0, 1.0);  // 白
    gl_Position = position + vec4( 0.0, size * 2.0, 0.0, 0.0);
    EmitVertex();

    EndPrimitive();
}

void main() {
    build_house(gl_in[0].gl_Position);
}
```

```
出力:    /\       頂点順序: 1(左下)→2(右下)→3(左上)→4(右上)→5(屋根)
        /  \      三角形: 1-2-3, 2-3-4, 3-4-5
       +----+
       | 家 |
       +----+
```

---

## 応用例2：法線ベクトルの可視化

3Dモデルのデバッグに非常に便利な手法です。各頂点から法線方向に短い線分を描画します。

```glsl
#version 330 core
layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

in VS_OUT {
    vec3 normal;
} gs_in[];

uniform mat4 projection;

const float MAGNITUDE = 0.2;

void GenerateLine(int index) {
    // 頂点位置
    gl_Position = projection * gl_in[index].gl_Position;
    EmitVertex();
    // 法線方向に移動した位置
    gl_Position = projection * (gl_in[index].gl_Position +
                  vec4(gs_in[index].normal, 0.0) * MAGNITUDE);
    EmitVertex();
    EndPrimitive();
}

void main() {
    GenerateLine(0);  // 頂点0の法線
    GenerateLine(1);  // 頂点1の法線
    GenerateLine(2);  // 頂点2の法線
}
```

> 法線可視化は2パスで描画します：1パス目は通常レンダリング、2パス目は法線表示用シェーダーで描画。

---

## 応用例3：爆発エフェクト

```glsl
#version 330 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec2 texCoords;
} gs_in[];

out vec2 TexCoords;

uniform float time;

vec3 GetNormal() {
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross(a, b));
}

vec4 Explode(vec4 position, vec3 normal) {
    float magnitude = 2.0;
    vec3 direction = normal * ((sin(time) + 1.0) / 2.0) * magnitude;
    return position + vec4(direction, 0.0);
}

void main() {
    vec3 normal = GetNormal();

    for (int i = 0; i < 3; i++) {
        gl_Position = Explode(gl_in[i].gl_Position, normal);
        TexCoords = gs_in[i].texCoords;
        EmitVertex();
    }
    EndPrimitive();
}
```

`GetNormal()` で面法線を外積から計算し、`sin(time)` で時間とともに移動量を振動させ、各頂点を法線方向にオフセットして出力します。

---

## C++ 側の設定

ジオメトリシェーダーをプログラムに追加するには、通常のシェーダーと同様に `glAttachShader` を使います。

```cpp
// シェーダーのコンパイル
unsigned int geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
glShaderSource(geometryShader, 1, &geometryShaderSource, NULL);
glCompileShader(geometryShader);

// プログラムにアタッチ
unsigned int shaderProgram = glCreateProgram();
glAttachShader(shaderProgram, vertexShader);
glAttachShader(shaderProgram, geometryShader);   // ← ここで追加
glAttachShader(shaderProgram, fragmentShader);
glLinkProgram(shaderProgram);

// 使い終わったら削除
glDeleteShader(vertexShader);
glDeleteShader(geometryShader);
glDeleteShader(fragmentShader);
```

---

## パフォーマンスに関する注意

ジオメトリシェーダーは出力サイズが可変のため GPU の並列化が難しく、`max_vertices` を大きくするとドライバが最悪ケースでメモリを確保するなど、パフォーマンスコストが高くなりがちです。法線可視化やデバッグ用途では有効ですが、大量のジオメトリ生成にはテッセレーションシェーダーやインスタンシングを検討しましょう。

---

## 💡 ポイントまとめ

| 項目 | 内容 |
|---------------------------|--------------------------------------------------------------|
| パイプライン位置 | 頂点シェーダーの後、ラスタライザの前 |
| 処理単位 | プリミティブ単位（1つの三角形、線、点） |
| 入力宣言 | `layout(triangles) in;` 等 |
| 出力宣言 | `layout(triangle_strip, max_vertices = N) out;` |
| EmitVertex / EndPrimitive | 頂点追加 / プリミティブ確定 |
| ビルトイン入力 | `gl_in[].gl_Position` で入力頂点座標にアクセス |
| C++ 側のシェーダー種別 | `GL_GEOMETRY_SHADER` |
| パフォーマンス | GPU 並列性が低下するため、用途を限定すべき |

---

## ドリル問題

### 問題1（穴埋め）

ジオメトリシェーダーで三角形を入力として受け取り、ラインストリップを最大6頂点まで出力するレイアウト宣言を書いてください。

```glsl
layout(______) in;
layout(______, max_vertices = ___) out;
```

<details><summary> 解答</summary>

```glsl
layout(triangles) in;
layout(line_strip, max_vertices = 6) out;
```

入力 `triangles` で `gl_in[]` は3要素になります。出力 `line_strip` で線分を描画し、`max_vertices = 6` で最大6頂点まで出力可能です。

</details>

---

### 問題2（選択）

ジオメトリシェーダーの入力タイプが `triangles` のとき、`gl_in[]` の要素数はいくつですか？

- A) 1
- B) 2
- C) 3
- D) 6

<details><summary> 解答</summary>

**C) 3**

`triangles` の場合、1つの三角形を構成する3頂点が `gl_in[0]`, `gl_in[1]`, `gl_in[2]` としてアクセスできます。`triangles_adjacency` の場合は6になります。

</details>

---

### 問題3（穴埋め）

以下のコードで、法線方向に頂点をオフセットする `Explode` 関数を完成させてください。

```glsl
vec4 Explode(vec4 position, vec3 normal) {
    float magnitude = 2.0;
    vec3 direction = ______ * ((sin(time) + 1.0) / 2.0) * ______;
    return position + vec4(______, 0.0);
}
```

<details><summary> 解答</summary>

```glsl
vec3 direction = normal * ((sin(time) + 1.0) / 2.0) * magnitude;
return position + vec4(direction, 0.0);
```

`(sin(time) + 1.0) / 2.0` は値が 0〜1 の範囲になるため、法線方向に 0〜`magnitude` の距離だけ移動する滑らかなアニメーションになります。

</details>

---

### 問題4（記述）

`EmitVertex()` と `EndPrimitive()` の違いを説明し、`triangle_strip` 出力で5回 `EmitVertex()` を呼んだ後に `EndPrimitive()` を呼んだ場合、何個の三角形が生成されるか答えてください。

<details><summary> 解答</summary>

- **`EmitVertex()`**：現在設定されている gl_Position と out 変数で1頂点を出力ストリームに追加する
- **`EndPrimitive()`**：それまでに追加された頂点群を1つのプリミティブ（ストリップ）として確定する

`triangle_strip` で5頂点を出力した場合、**3個の三角形** が生成されます。
- 三角形1: 頂点0-1-2
- 三角形2: 頂点1-2-3
- 三角形3: 頂点2-3-4

一般に n 頂点の triangle_strip は (n - 2) 個の三角形を生成します。

</details>

---

### 問題5（穴埋め）

C++ 側でジオメトリシェーダーをコンパイルしてプログラムにアタッチするコードを完成させてください。

```cpp
unsigned int gShader = glCreateShader(______);
glShaderSource(gShader, 1, &gShaderSrc, NULL);
glCompileShader(gShader);

glAttachShader(program, vertexShader);
glAttachShader(program, ______);
glAttachShader(program, fragmentShader);
glLinkProgram(program);
```

<details><summary> 解答</summary>

```cpp
unsigned int gShader = glCreateShader(GL_GEOMETRY_SHADER);
// ...
glAttachShader(program, gShader);
```

`GL_GEOMETRY_SHADER` は OpenGL 3.2 以降で使用でき、`GL_VERTEX_SHADER` や `GL_FRAGMENT_SHADER` と同様にシェーダープログラムにアタッチします。

</details>

---

### 問題6（計算）

`layout(lines) in;` と宣言したジオメトリシェーダーで `GL_LINE_STRIP` モードで4頂点（A-B-C-D）を描画した場合、ジオメトリシェーダーは何回呼び出されますか？

<details><summary> 解答</summary>

**3回** 呼び出されます。

`GL_LINE_STRIP` は4頂点から3本の線分（A-B, B-C, C-D）を生成します。ジオメトリシェーダーには1本の線分（2頂点）ずつ渡されるため、3回の呼び出しになります。

</details>

---

## 実践課題

### 課題1: ポイントスプライト生成 

4つの点を `GL_POINTS` で描画し、ジオメトリシェーダーで各点を四角形（2つの三角形からなる `triangle_strip`）に展開してください。

**チェックポイント：**
- [ ] `layout(points) in;` と `layout(triangle_strip, max_vertices = 4) out;` を設定
- [ ] 各点の位置を中心に、指定サイズの正方形を4頂点で構成
- [ ] テクスチャ座標を付与し、フラグメントシェーダーでテクスチャを貼る
- [ ] ポイントサイズをユニフォームで変更可能にする

---

### 課題2: ワイヤーフレームオーバーレイ 

3Dモデルの通常レンダリングの上に、ジオメトリシェーダーを使ってワイヤーフレームを重ねて表示してください。

**チェックポイント：**
- [ ] 1パス目: 通常のテクスチャ付きレンダリング
- [ ] 2パス目: ジオメトリシェーダーで各三角形の辺を `line_strip` として出力
- [ ] ワイヤーフレームの色を単色（緑など）に設定
- [ ] `glPolygonOffset` や深度バイアスで Z-fighting を回避

---

### 課題3: 爆発エフェクト付きモデル表示 

モデルを読み込み、時間経過で三角形が法線方向に飛び散る爆発エフェクトを実装してください。

**チェックポイント：**
- [ ] 頂点シェーダーでビュー空間の法線を計算し、ジオメトリシェーダーに渡す
- [ ] ジオメトリシェーダーで面法線（外積）を計算し、法線方向に三角形を移動
- [ ] `sin(time)` でアニメーションさせ、元に戻る往復運動にする
- [ ] テクスチャ座標を維持し、飛び散る三角形にもテクスチャを表示
- [ ] 移動量をユニフォームで調整可能にする

## ナビゲーション

 [高度な GLSL](./08-advanced-glsl.md) | [インスタンシング →](./10-instancing.md)
