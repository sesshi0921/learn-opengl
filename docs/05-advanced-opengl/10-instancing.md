# インスタンシング（Instancing）

> **目標：** 同一メッシュを大量に描画する際のドローコールのオーバーヘッドを理解し、インスタンシングを使って数万〜数十万のオブジェクトを効率的に描画できるようになる。

---

## ドローコールのオーバーヘッド問題

OpenGL でオブジェクトを描画するたびに `glDrawArrays` や `glDrawElements` を呼び出すと、CPU から GPU への**コマンド発行**が発生します。このコマンドには以下のような処理が含まれます：

```
1回のドローコールで起こること:

CPU 側                          │  GPU 側
────────────────────────────────┼──────────────────────
ユニフォーム設定                │
バッファバインド                │
ステート検証                    │
ドライバがコマンドバッファ構築  │
  ─── コマンド送信 ──────────▶  │  描画実行
                                │
```

1つのオブジェクトの描画はほぼ一瞬ですが、**ドローコール自体のオーバーヘッド**が蓄積すると問題になります。例えば1000本の草を1本ずつ描画すると、草の三角形は少ないのにドローコールのコストが支配的になります。

```
通常描画 (1000個のオブジェクト):
  for (int i = 0; i < 1000; i++) {
      setUniform(model[i]);   // ← 毎回 CPU→GPU 通信
      glDrawArrays(...);      // ← 毎回ドローコール
  }
  合計: 1000回のドローコール → CPUがボトルネック

インスタンシング:
  setupInstanceData();          // ← 1回だけ
  glDrawArraysInstanced(..., 1000);  // ← 1回のドローコール
  合計: 1回のドローコール → GPU が効率的に処理
```

---

## インスタンシングの基本概念

インスタンシングは**1回のドローコールで同じメッシュを複数回描画**する技術です。各インスタンスの違い（位置、色、スケールなど）はシェーダー内で `gl_InstanceID` を使って区別します。

### glDrawArraysInstanced / glDrawElementsInstanced

```cpp
// 通常の描画
glDrawArrays(GL_TRIANGLES, 0, vertexCount);

// インスタンシング描画
glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, instanceCount);

// インデックス付きインスタンシング
glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0, instanceCount);
```

第4引数（または第5引数）の `instanceCount` で描画回数を指定します。GPU は同じメッシュを `instanceCount` 回描画しますが、内部的には高度に並列化されます。

---

## gl_InstanceID ビルトイン変数

頂点シェーダー内で `gl_InstanceID` を参照すると、現在描画中のインスタンスの番号（0から始まる整数）が取得できます。

```glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

uniform vec2 offsets[100];  // 各インスタンスのオフセット

out vec3 fColor;

void main() {
    vec2 offset = offsets[gl_InstanceID];
    gl_Position = vec4(aPos + offset, 0.0, 1.0);
    fColor = aColor;
}
```

**C++ 側でオフセット配列を設定：**

```cpp
vec2 translations[100];
int index = 0;
float offset = 0.1f;
for (int y = -10; y < 10; y += 2) {
    for (int x = -10; x < 10; x += 2) {
        vec2 translation;
        translation.x = (float)x / 10.0f + offset;
        translation.y = (float)y / 10.0f + offset;
        translations[index++] = translation;
    }
}

// ユニフォーム配列として設定
for (int i = 0; i < 100; i++) {
    std::string uniformName = "offsets[" + std::to_string(i) + "]";
    glUniform2f(glGetUniformLocation(shader.ID, uniformName.c_str()),
                translations[i].x, translations[i].y);
}

// 100個のインスタンスを1回のドローコールで描画
glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 100);
```

> **制限事項：** ユニフォーム配列にはサイズ上限があります（通常数百〜数千要素）。数万個のインスタンスにはインスタンス属性を使います。

---

## インスタンス属性（Instanced Array）

大量のインスタンス（数千〜数十万）に対応するには、インスタンスデータを**頂点属性**として設定し、`glVertexAttribDivisor` で更新頻度を制御します。

### glVertexAttribDivisor の仕組み

```cpp
glVertexAttribDivisor(index, divisor);
```

| divisor の値 | 意味 |
|-------------|---------------------------------------|
| 0 | 頂点ごとに属性が進む（通常の頂点属性）|
| 1 | インスタンスごとに属性が進む |
| 2 | 2インスタンスごとに属性が進む |
| N | Nインスタンスごとに属性が進む |

```
divisor=0 (頂点属性):          divisor=1 (インスタンス属性):
 Instance0: v0→d[0] v1→d[1]   Instance0: v0→d[0] v1→d[0]
 Instance1: v0→d[0] v1→d[1]   Instance1: v0→d[1] v1→d[1]
```

### セットアップコード

```cpp
// インスタンス用のオフセットデータを VBO に格納
unsigned int instanceVBO;
glGenBuffers(1, &instanceVBO);
glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * 100, &translations[0], GL_STATIC_DRAW);

// 頂点属性として設定（location = 2）
glEnableVertexAttribArray(2);
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
glVertexAttribDivisor(2, 1);  // ← インスタンスごとに更新
```

対応するシェーダー：

```glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aOffset;  // インスタンス属性

out vec3 fColor;

void main() {
    gl_Position = vec4(aPos + aOffset, 0.0, 1.0);
    fColor = aColor;
}
```

---

## mat4 をインスタンス属性として渡す

各インスタンスに異なるモデル行列を適用したい場合、`mat4` を頂点属性として渡す必要があります。しかし頂点属性の最大サイズは `vec4` なので、**mat4 は4つの vec4 に分割**して設定します。

```cpp
// mat4 は 4つの vec4 (4列) で構成される
// location 3, 4, 5, 6 を使用
std::size_t vec4Size = sizeof(glm::vec4);
glEnableVertexAttribArray(3);
glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
glEnableVertexAttribArray(4);
glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
glEnableVertexAttribArray(5);
glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
glEnableVertexAttribArray(6);
glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

// 4つすべてのインスタンス属性に divisor を設定
glVertexAttribDivisor(3, 1);
glVertexAttribDivisor(4, 1);
glVertexAttribDivisor(5, 1);
glVertexAttribDivisor(6, 1);
```

対応するシェーダー：

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in mat4 aInstanceMatrix;  // location 3,4,5,6 を自動消費

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * aInstanceMatrix * vec4(aPos, 1.0);
}
```

mat4 は location 3〜6 の4列に分割され、stride は 64 bytes、各列のオフセットは `列番号 × sizeof(vec4)` です。

---

## 実用例：小惑星フィールド

数万個の小惑星を描画するシーンで、通常描画とインスタンシングのパフォーマンスを比較します。

### ランダム変換行列の生成

```cpp
unsigned int amount = 100000;
std::vector<glm::mat4> modelMatrices(amount);

srand(static_cast<unsigned int>(glfwGetTime()));
float radius = 150.0f;
float offset = 25.0f;

for (unsigned int i = 0; i < amount; i++) {
    glm::mat4 model = glm::mat4(1.0f);

    // 1. リング状の位置にランダム配置
    float angle = (float)i / (float)amount * 360.0f;
    float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float x = sin(angle) * radius + displacement;
    displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float y = displacement * 0.4f;  // y軸は薄く
    displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float z = cos(angle) * radius + displacement;
    model = glm::translate(model, glm::vec3(x, y, z));

    // 2. ランダムスケール (0.05 ～ 0.25)
    float scale = static_cast<float>((rand() % 20) / 100.0 + 0.05);
    model = glm::scale(model, glm::vec3(scale));

    // 3. ランダム回転
    float rotAngle = static_cast<float>(rand() % 360);
    model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

    modelMatrices[i] = model;
}
```

### インスタンスバッファの設定と描画

```cpp
unsigned int instanceBuffer;
glGenBuffers(1, &instanceBuffer);
glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4),
             &modelMatrices[0], GL_STATIC_DRAW);

// モデルの各メッシュの VAO に mat4 インスタンス属性を追加する
// (location 3〜6 に4つの vec4 を設定、各 divisor=1)
// …前述の mat4 属性設定コードと同様…

// 描画（インスタンシング）
instanceShader.use();
instanceShader.setMat4("view", view);
instanceShader.setMat4("projection", projection);
for (unsigned int i = 0; i < rock.meshes.size(); i++) {
    glBindVertexArray(rock.meshes[i].VAO);
    glDrawElementsInstanced(GL_TRIANGLES,
                            rock.meshes[i].indices.size(),
                            GL_UNSIGNED_INT, 0, amount);
}
```

### パフォーマンス比較

| 小惑星の数 | 通常描画 | インスタンシング |
|-----------|----------|------------------|
| 1,000 | ~120 FPS | ~800 FPS |
| 10,000 | ~15 FPS | ~300 FPS |
| 100,000 | <1 FPS | ~60 FPS |

※ 環境により異なります

---

## 💡 ポイントまとめ

| 項目 | 内容 |
|---------------------------------|------------------------------------------------------------|
| ドローコールの問題 | CPU→GPU 間通信がボトルネック、描画回数を減らすことが重要 |
| glDrawArraysInstanced | 同じメッシュを instanceCount 回描画する |
| gl_InstanceID | 頂点シェーダー内で現在のインスタンス番号を取得（0始まり） |
| ユニフォーム配列方式 | 小規模向け（数百個まで）、ユニフォームサイズ制限あり |
| インスタンス属性方式 | 大規模向け（数万〜数十万）、VBO に格納 |
| glVertexAttribDivisor(idx, 1) | 属性 idx をインスタンスごとに更新する |
| mat4 の渡し方 | 4つの連続する vec4 属性（4つの location を消費） |
| パフォーマンス効果 | 10万オブジェクトでも実用的な FPS を維持可能 |

---

## ドリル問題

### 問題1（穴埋め）

インスタンス属性を設定するために、属性の更新頻度を「インスタンスごと」に変更する関数呼び出しを書いてください。

```cpp
glEnableVertexAttribArray(2);
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
______________________________;  // インスタンスごとに更新
```

<details><summary> 解答</summary>

```cpp
glVertexAttribDivisor(2, 1);
```

第1引数は属性インデックス（ここでは2）、第2引数が `1` でインスタンスごとに属性が進みます。`0` だと通常の頂点属性と同じ動作になります。

</details>

---

### 問題2（選択）

`glVertexAttribDivisor(3, 2)` を設定した場合、インスタンス0〜5がそれぞれ参照するデータインデックスはどれですか？

- A) 0, 0, 1, 1, 2, 2
- B) 0, 1, 2, 3, 4, 5
- C) 0, 2, 4, 6, 8, 10
- D) 0, 0, 0, 1, 1, 1

<details><summary> 解答</summary>

**A) 0, 0, 1, 1, 2, 2**

divisor = 2 の場合、2つのインスタンスごとに属性が1つ進みます。
- インスタンス 0, 1 → data[0]
- インスタンス 2, 3 → data[1]
- インスタンス 4, 5 → data[2]

一般に: `data_index = floor(instance_id / divisor)`

</details>

---

### 問題3（穴埋め）

`mat4` をインスタンス属性として渡すとき、location 3 から始めて4つの `vec4` に分割します。2番目の列（location 4）の `glVertexAttribPointer` を完成させてください。

```cpp
std::size_t vec4Size = sizeof(glm::vec4);
glEnableVertexAttribArray(4);
glVertexAttribPointer(4, ___, GL_FLOAT, GL_FALSE, ___ * vec4Size, (void*)(___));
glVertexAttribDivisor(4, 1);
```

<details><summary> 解答</summary>

```cpp
glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
```

- 第2引数 `4`: vec4 なので4要素
- 第5引数 `4 * vec4Size`: stride は mat4 全体 = 4つの vec4 = 64バイト
- 第6引数 `(void*)(1 * vec4Size)`: 2列目なのでオフセットは vec4 1つ分 = 16バイト

</details>

---

### 問題4（記述）

ユニフォーム配列方式とインスタンス属性方式の違いと、それぞれの適用場面を説明してください。

<details><summary> 解答</summary>

| 方式 | データの場所 | サイズ制限 | 適用場面 |
|--------------------|--------------------|-----------------------|-----------------------|
| ユニフォーム配列 | ユニフォームバッファ | `GL_MAX_UNIFORM_LOCATIONS` に制限（数百〜数千） | 小規模（数百個まで） |
| インスタンス属性 | VBO（頂点バッファ） | GPU メモリが許す限り | 大規模（数万〜数十万）|

ユニフォーム配列方式は設定が簡単ですが、ループで `glUniform` を呼ぶオーバーヘッドがあります。インスタンス属性方式は初期設定がやや複雑ですが、一度 VBO にデータを格納すれば GPU が直接読み取るため高速です。

</details>

---

### 問題5（計算）

`mat4` を頂点属性として100,000個のインスタンスに渡す場合、インスタンスバッファに必要なメモリサイズは何バイトですか？

<details><summary> 解答</summary>

```
mat4 = 4 × 4 × sizeof(float) = 16 × 4 = 64 バイト

100,000 × 64 = 6,400,000 バイト ≒ 6.1 MB
```

約6.1MBで、現代の GPU メモリ（数GB）に対しては十分小さいサイズです。これがインスタンシングが数十万オブジェクトでも実用的な理由の一つです。

</details>

---

### 問題6（穴埋め）

以下のシェーダーで `gl_InstanceID` を使って各インスタンスに異なる色を付けるコードを完成させてください。

```glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aOffset;

out vec3 fColor;

void main() {
    gl_Position = vec4(aPos + aOffset, 0.0, 1.0);
    // インスタンスIDから色を生成（0～99のインスタンスを想定）
    fColor = vec3(
        float(______) / 100.0,       // R: IDに比例
        1.0 - float(______) / 100.0, // G: IDに反比例
        0.5                           // B: 固定
    );
}
```

<details><summary> 解答</summary>

```glsl
fColor = vec3(
    float(gl_InstanceID) / 100.0,
    1.0 - float(gl_InstanceID) / 100.0,
    0.5
);
```

`gl_InstanceID` は 0 から始まる整数で、インスタンスごとに1ずつ増加します。これを浮動小数点に変換し正規化することで、インスタンスごとにグラデーションする色を生成できます。

</details>

---

## 実践課題

### 課題1: 100個の四角形グリッド 

NDC 空間 (-1〜1) に10×10のグリッド状に100個の四角形を描画してください。

**チェックポイント：**
- [ ] 四角形1つ分の頂点データ（6頂点 or インデックス付き4頂点）を作成
- [ ] 100個分のオフセットを計算しインスタンスバッファに格納
- [ ] `glVertexAttribDivisor` でオフセットをインスタンス属性に設定
- [ ] `glDrawArraysInstanced` で1回のドローコールで全描画
- [ ] `gl_InstanceID` を使って各四角形の色を変える

---

### 課題2: 草原シミュレーション 

地面テクスチャの上に、数千本の草（ビルボード四角形 + 草テクスチャ）をインスタンシングで描画してください。

**チェックポイント：**
- [ ] 草1本分はテクスチャ付き四角形（アルファで切り抜き）
- [ ] ランダムな位置・スケール・Y軸回転を `mat4` でインスタンス属性として渡す
- [ ] 4つの `vec4` に分割して location を設定
- [ ] `gl_InstanceID` やインスタンス属性で色のバリエーションを追加
- [ ] 5,000本以上で60FPSを維持できることを確認

---

### 課題3: 小惑星リングの実装 

中央に惑星モデル、周囲に数万個の小惑星モデルをリング状に配置するシーンを実装してください。

**チェックポイント：**
- [ ] 小惑星用のランダム `mat4`（位置・回転・スケール）を生成
- [ ] 惑星の周囲にリング状に分布（半径と厚みを調整）
- [ ] インスタンスバッファに全行列を格納し、モデルの各メッシュの VAO に属性を追加
- [ ] `glDrawElementsInstanced` で描画
- [ ] 通常描画版も実装し、FPS を比較する
- [ ] 10,000個以上のインスタンスで動作確認

---

## ナビゲーション

 [ジオメトリシェーダー](./09-geometry-shader.md) | [アンチエイリアシング →](./11-anti-aliasing.md)
