# メッシュ（Mesh）

> **目標：** OpenGLで描画可能な最小単位であるメッシュを理解し、Vertex/Texture構造体の設計、VAO/VBO/EBOのセットアップ、テクスチャバインドの規則を習得する。効率的で再利用可能なMeshクラスを自分で実装できるようになる。

---

## メッシュとは何か

**メッシュ（Mesh）** とは、OpenGLで**1回のドローコールで描画できる最小単位**です。

1つの3Dモデルは通常、複数のメッシュで構成されます。たとえばキャラクターモデルなら：

```
キャラクターモデル
├── 頭メッシュ       (マテリアル: 肌)
├── 体メッシュ       (マテリアル: 服)
├── 手メッシュ ×2    (マテリアル: 肌)
├── 靴メッシュ ×2    (マテリアル: 革)
└── 武器メッシュ     (マテリアル: 金属)
```

各メッシュは以下の3つの要素を持ちます：

1. **頂点データ**（位置・法線・テクスチャ座標など）
2. **インデックスデータ**（頂点を組み合わせて三角形を定義）
3. **テクスチャデータ**（マテリアルに紐づくテクスチャ画像）

---

## Vertex構造体の設計

まず、1つの頂点が持つすべての属性をまとめた構造体を設計します。

```cpp
struct Vertex {
    glm::vec3 Position;   // 位置座標
    glm::vec3 Normal;     // 法線ベクトル
    glm::vec2 TexCoords;  // テクスチャ座標
};
```

### メモリレイアウト

C++の構造体はメンバがメモリ上に**連続して配置**されます（パディングがない場合）。これがOpenGLにとって非常に重要です。

```
Vertex構造体のメモリレイアウト (計32バイト):

オフセット  0    4    8   12   16   20   24   28   32
           ├────┼────┼────┼────┼────┼────┼────┼────┤
           │ Px │ Py │ Pz │ Nx │ Ny │ Nz │ Tx │ Ty │
           └────┴────┴────┴────┴────┴────┴────┴────┘
           |← Position →|← Normal   →|← TexC →|
           |  (12 bytes) | (12 bytes) |(8 bytes)|

    offsetof(Vertex, Position)  = 0
    offsetof(Vertex, Normal)    = 12
    offsetof(Vertex, TexCoords) = 24
```

### offsetofマクロの重要性

`offsetof` はC/C++標準マクロで、**構造体先頭から特定メンバまでのバイトオフセット**を返します。

```cpp
#include <cstddef>  // offsetof

// 使い方: offsetof(構造体型, メンバ名)
size_t pos_offset  = offsetof(Vertex, Position);   // 0
size_t norm_offset = offsetof(Vertex, Normal);      // 12
size_t tex_offset  = offsetof(Vertex, TexCoords);   // 24
```

なぜ `offsetof` が重要なのか？ OpenGLの `glVertexAttribPointer` に**正確なオフセット値**を渡す必要があるからです。

```cpp
// ハードコードする場合（脆い、バグの温床）
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, (void*)0);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 32, (void*)12);

// offsetofを使う場合（安全、メンバの追加・変更に強い）
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
    sizeof(Vertex), (void*)offsetof(Vertex, Position));
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
    sizeof(Vertex), (void*)offsetof(Vertex, Normal));
```

将来 `Vertex` にタンジェントやボーンウェイトを追加しても、`offsetof` を使っていれば**自動的に正しいオフセットが計算**されます。

---

## Texture構造体の設計

テクスチャは識別子・種類・パスを保持します。

```cpp
struct Texture {
    unsigned int id;    // OpenGLテクスチャID
    std::string type;   // テクスチャの種類 ("texture_diffuse", "texture_specular" 等)
    std::string path;   // ファイルパス（重複読み込み防止用）
};
```

テクスチャの種類一覧：

| type 文字列 | 用途 | シェーダでの変数名例 |
|------------|------|---------------------|
| `"texture_diffuse"` | ディフューズマップ | `material.texture_diffuse1` |
| `"texture_specular"` | スペキュラマップ | `material.texture_specular1` |
| `"texture_normal"` | ノーマルマップ | `material.texture_normal1` |
| `"texture_height"` | ハイトマップ | `material.texture_height1` |

---

## Meshクラスの設計

Meshクラスは**データの保持**と**描画**の2つの責務を持ちます。

```cpp
class Mesh {
public:
    // メッシュデータ
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

    // コンストラクタ
    Mesh(std::vector<Vertex> vertices,
         std::vector<unsigned int> indices,
         std::vector<Texture> textures);

    // 描画
    void Draw(Shader &shader);

private:
    // OpenGLオブジェクト
    unsigned int VAO, VBO, EBO;

    // セットアップ
    void setupMesh();
};
```

設計のポイント：
- データ（vertices, indices, textures）は**public** → Modelクラスからアクセス可能
- OpenGLオブジェクト（VAO, VBO, EBO）は**private** → 内部実装の詳細
- `setupMesh()` はコンストラクタ内で呼び出される

---

## setupMesh()の完全実装

コンストラクタで受け取ったデータをOpenGLバッファに転送します。

```cpp
Mesh::Mesh(std::vector<Vertex> vertices,
           std::vector<unsigned int> indices,
           std::vector<Texture> textures)
    : vertices(std::move(vertices)),
      indices(std::move(indices)),
      textures(std::move(textures))
{
    setupMesh();
}

void Mesh::setupMesh()
{
    // 1. バッファオブジェクトを生成
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // 2. VAOをバインド（以降の設定をVAOに記録）
    glBindVertexArray(VAO);

    // 3. VBOにデータを転送
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(Vertex),
                 &vertices[0],
                 GL_STATIC_DRAW);

    // 4. EBOにインデックスデータを転送
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 &indices[0],
                 GL_STATIC_DRAW);

    // 5. 頂点属性ポインタの設定
    // Position (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), (void*)offsetof(Vertex, Position));

    // Normal (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    // TexCoords (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    // 6. VAOをアンバインド
    glBindVertexArray(0);
}
```

### setupMesh()の処理フロー図

```
glGenVertexArrays / glGenBuffers
         │
         ▼
glBindVertexArray(VAO) ── 記録開始
         │
         ├─→ glBindBuffer(VBO) → glBufferData(頂点データ)
         │
         ├─→ glBindBuffer(EBO) → glBufferData(インデックス)
         │
         ├─→ glVertexAttribPointer(0, Position)
         ├─→ glVertexAttribPointer(1, Normal)
         └─→ glVertexAttribPointer(2, TexCoords)
         │
         ▼
glBindVertexArray(0) ── 記録終了
```

VAOはすべての設定を「記録」しています。描画時は `glBindVertexArray(VAO)` だけで全設定が復元されます。

---

## Draw()関数の完全実装

Draw()関数の最も重要な部分は**テクスチャのバインド規則**です。

### テクスチャのネーミング規則

シェーダ内では以下の命名規則でサンプラーを定義します：

```glsl
// フラグメントシェーダ
struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_diffuse2;   // 2枚目のディフューズ
    sampler2D texture_specular1;
    sampler2D texture_specular2;  // 2枚目のスペキュラ
};
uniform Material material;
```

規則は `material.{type}{number}` です。C++側でこの名前を動的に構築してuniformを設定します。

### Draw()の実装

```cpp
void Mesh::Draw(Shader &shader)
{
    unsigned int diffuseNr  = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr   = 1;
    unsigned int heightNr   = 1;

    for (unsigned int i = 0; i < textures.size(); i++)
    {
        // テクスチャユニットをアクティブにする
        glActiveTexture(GL_TEXTURE0 + i);

        // テクスチャ番号を決定
        std::string number;
        std::string name = textures[i].type;

        if (name == "texture_diffuse")
            number = std::to_string(diffuseNr++);
        else if (name == "texture_specular")
            number = std::to_string(specularNr++);
        else if (name == "texture_normal")
            number = std::to_string(normalNr++);
        else if (name == "texture_height")
            number = std::to_string(heightNr++);

        // uniform名を構築: "material.texture_diffuse1" 等
        shader.setInt(("material." + name + number).c_str(), i);

        // テクスチャをバインド
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    // メッシュを描画
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES,
                   static_cast<unsigned int>(indices.size()),
                   GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // テクスチャユニットをリセット
    glActiveTexture(GL_TEXTURE0);
}
```

### テクスチャバインドの流れ

```
textures[0] = {id=3, type="texture_diffuse"}   → GL_TEXTURE0, "material.texture_diffuse1"
textures[1] = {id=7, type="texture_diffuse"}   → GL_TEXTURE1, "material.texture_diffuse2"
textures[2] = {id=5, type="texture_specular"}  → GL_TEXTURE2, "material.texture_specular1"

glActiveTexture(GL_TEXTURE0 + i)  ← テクスチャユニットi番を選択
shader.setInt("material.xxx", i)  ← シェーダのサンプラーにユニット番号iを紐付け
glBindTexture(GL_TEXTURE_2D, id)  ← 実際のテクスチャをユニットにバインド
```

この3ステップが「テクスチャユニット」という中間層を介してシェーダとテクスチャを接続する仕組みです。

> 💡 シェーダ側では `layout (location = N)` とMeshの `glVertexAttribPointer` の第1引数が対応します。また `uniform Material material;` に `sampler2D texture_diffuse1` 等のフィールドを持たせます。

---

## 💡 ポイントまとめ

| 項目 | 内容 |
|------|------|
| メッシュ | OpenGLで1回のドローコールで描画できる最小単位 |
| Vertex構造体 | Position(vec3) + Normal(vec3) + TexCoords(vec2) = 32バイト |
| offsetof | 構造体メンバのバイトオフセットを安全に取得するマクロ |
| Texture構造体 | id（OpenGLテクスチャID）+ type（種類）+ path（ファイルパス） |
| setupMesh() | VAO/VBO/EBO生成 → データ転送 → 頂点属性設定 |
| glVertexAttribPointer | stride = sizeof(Vertex), offset = offsetof(Vertex, Member) |
| Draw() | テクスチャバインド → glDrawElements |
| ネーミング規則 | `material.{type}{number}` (例: material.texture_diffuse1) |
| テクスチャバインド | glActiveTexture → setInt → glBindTexture の3ステップ |
| VAOの役割 | 全頂点属性設定を記録し、描画時に一発で復元 |

---

## ドリル問題

### 問1: offsetof計算（計算問題）
以下のVertex構造体で、各メンバの `offsetof` 値を計算せよ（パディングなしと仮定）。

```cpp
struct Vertex {
    glm::vec3 Position;   // float × 3
    glm::vec3 Normal;     // float × 3
    glm::vec2 TexCoords;  // float × 2
    glm::vec3 Tangent;    // float × 3
    glm::vec3 Bitangent;  // float × 3
};
```

<details><summary> 解答</summary>

| メンバ | サイズ | offsetof値 |
|--------|--------|-----------|
| Position | 12バイト (4×3) | **0** |
| Normal | 12バイト (4×3) | **12** |
| TexCoords | 8バイト (4×2) | **24** |
| Tangent | 12バイト (4×3) | **32** |
| Bitangent | 12バイト (4×3) | **44** |

構造体全体のサイズ: `sizeof(Vertex) = 56バイト`

計算: 各メンバのオフセット = それ以前のメンバの合計サイズ

</details>

### 問2: テクスチャバインド手順（穴埋め）
Draw()関数でテクスチャをシェーダに渡す3ステップを正しい順序で埋めよ。

```cpp
// ステップ1: テクスチャユニットi番を______する
glActiveTexture(GL_TEXTURE0 + i);
// ステップ2: シェーダのサンプラーにユニット番号を______
shader.setInt(uniformName, i);
// ステップ3: テクスチャIDをユニットに______
glBindTexture(GL_TEXTURE_2D, textures[i].id);
```

<details><summary> 解答</summary>

1. **アクティブに**する（選択する）
2. **設定する**（紐付ける）
3. **バインド**する

この3ステップにより、テクスチャユニットという「中間層」を介してシェーダのsamplerと実際のテクスチャが接続されます。

</details>

### 問3: Draw関数の穴埋め（穴埋め）
Meshの描画部分を完成させよ。

```cpp
void Mesh::Draw(Shader &shader)
{
    // テクスチャバインド（省略）...

    // メッシュを描画
    glBindVertexArray(______);
    glDrawElements(______, static_cast<unsigned int>(______.size()),
                   GL_UNSIGNED_INT, 0);
    glBindVertexArray(______);
}
```

<details><summary> 解答</summary>

```cpp
glBindVertexArray(VAO);
glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()),
               GL_UNSIGNED_INT, 0);
glBindVertexArray(0);
```

- `VAO` をバインドして頂点属性設定を復元
- `GL_TRIANGLES` で三角形として描画
- `indices.size()` でインデックス数を指定
- `0` をバインドしてVAOをアンバインド

</details>

### 問4: 構造体レイアウトの理解（選択問題）
`sizeof(Vertex)` が32バイトのとき、`glVertexAttribPointer` の stride パラメータに渡すべき値は？

- A) 0
- B) 12
- C) 32
- D) `sizeof(Vertex)`

<details><summary> 解答</summary>

**D) `sizeof(Vertex)`**

strideは「次の頂点までのバイト数」を表します。Vertex構造体が32バイトなので、`sizeof(Vertex)` を渡すのが正しく、かつ安全です。Cは偶然同じ値ですが、`sizeof(Vertex)` を使うべきです（構造体変更時に自動追随するため）。Aを指定すると「データが密に詰まっている」と解釈され、インターリーブ配列では正しく動作しません。

</details>

### 問5: テクスチャネーミング（穴埋め）
メッシュが以下のテクスチャを持つとき、各テクスチャのuniform名を答えよ。

```
textures[0]: type = "texture_diffuse"
textures[1]: type = "texture_diffuse"
textures[2]: type = "texture_specular"
textures[3]: type = "texture_normal"
textures[4]: type = "texture_specular"
```

<details><summary> 解答</summary>

| インデックス | type | uniform名 | テクスチャユニット |
|-------------|------|-----------|------------------|
| 0 | texture_diffuse | `material.texture_diffuse1` | GL_TEXTURE0 |
| 1 | texture_diffuse | `material.texture_diffuse2` | GL_TEXTURE1 |
| 2 | texture_specular | `material.texture_specular1` | GL_TEXTURE2 |
| 3 | texture_normal | `material.texture_normal1` | GL_TEXTURE3 |
| 4 | texture_specular | `material.texture_specular2` | GL_TEXTURE4 |

各タイプごとに独立した連番カウンタがインクリメントされます。

</details>

### 問6: setupMeshの順序（選択問題）
setupMesh()で以下の操作の正しい順序はどれか？

- A) VBO生成 → VAOバインド → データ転送 → 属性設定
- B) VAOバインド → VBO生成 → データ転送 → 属性設定
- C) VAO/VBO/EBO生成 → VAOバインド → VBOデータ転送 → EBOデータ転送 → 属性設定 → VAOアンバインド
- D) VBO/EBOデータ転送 → VAO生成 → 属性設定

<details><summary> 解答</summary>

**C) VAO/VBO/EBO生成 → VAOバインド → VBOデータ転送 → EBOデータ転送 → 属性設定 → VAOアンバインド**

VAOのバインド後に行われるVBO/EBOのバインドと頂点属性設定はすべてVAOに記録されます。生成（Gen）自体はバインド前でも後でも構いませんが、データ転送と設定はVAOバインド中に行う必要があります。

</details>

---

## 実践課題

### 課題1: Meshクラスの実装 
上記のMeshクラスを完全に実装し、手動で作成した三角形データで動作確認せよ。

**チェックポイント:**
- [ ] Vertex, Texture構造体を定義した
- [ ] Meshクラスのコンストラクタ、setupMesh(), Draw()を実装した
- [ ] 手動で三角形（3頂点、3インデックス）のデータを作成した
- [ ] 画面に三角形が表示された
- [ ] テクスチャなしでもクラッシュしないことを確認した

### 課題2: 立方体メッシュの描画 
Meshクラスを使って、テクスチャ付きの立方体を描画せよ。

**チェックポイント:**
- [ ] 立方体の36頂点（Position, Normal, TexCoords付き）を定義した
- [ ] ディフューズテクスチャとスペキュラテクスチャを読み込んだ
- [ ] Texture構造体のtype設定が正しい
- [ ] シェーダのuniform名がネーミング規則と一致している
- [ ] ライティング付きのテクスチャ立方体が表示された

### 課題3: 複数メッシュの描画 
複数のMeshオブジェクトを生成し、異なる位置・異なるテクスチャで描画せよ。

**チェックポイント:**
- [ ] 3つ以上のMeshオブジェクトを作成した
- [ ] 各メッシュに異なるmodel行列（位置・回転）を適用した
- [ ] 少なくとも2種類の異なるテクスチャセットを使用した
- [ ] Draw()の前にshader.setMat4("model", ...)で変換行列を設定した
- [ ] すべてのメッシュが正しい位置に正しいテクスチャで表示された

---

## ナビゲーション

 [Assimp](./01-assimp.md) | [モデル →](./03-model.md)
