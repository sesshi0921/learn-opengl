# モデル（Model）

> **目標：** Assimpで読み込んだ3Dモデルデータをメッシュに変換し、OpenGLで描画するModelクラスを実装する。ノードツリーの再帰走査、テクスチャのキャッシュ機構、エラーハンドリングを習得し、任意の3Dモデルを表示できるようになる。

---

## Modelクラスの全体像

Modelクラスは前章のMeshクラスを**束ねる上位クラス**です。1つのModelは複数のMeshを持ち、Assimpのデータ構造からOpenGLで使える形に変換する役割を担います。

```
Model
├── meshes[]          ... Meshオブジェクトの配列
├── textures_loaded[] ... 読み込み済みテクスチャの配列（キャッシュ）
├── directory         ... モデルファイルのディレクトリパス
│
├── loadModel()       ... Assimpでファイル読み込み
├── processNode()     ... ノードツリーの再帰走査
├── processMesh()     ... aiMesh → Mesh変換
├── loadMaterialTextures() ... マテリアルからテクスチャ読み込み
└── Draw()            ... 全メッシュの描画
```

### 処理の流れ

```
Model("path/to/model.obj")
  │
  └─→ loadModel()
        │
        ├─→ Assimp::Importer.ReadFile()   ... ファイル読み込み
        │     └─→ aiScene取得
        │
        └─→ processNode(rootNode, scene)   ... 再帰走査開始
              │
              ├─→ processMesh(mesh, scene)  ... 各メッシュを変換
              │     ├─→ 頂点データ抽出
              │     ├─→ インデックス抽出
              │     └─→ loadMaterialTextures()  ... テクスチャ読み込み
              │
              └─→ processNode(child, scene) ... 子ノードへ再帰
```

---

## Modelクラスのヘッダ定義

```cpp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include "shader.h"

#include <string>
#include <vector>

class Model
{
public:
    // 読み込み済みテクスチャ（キャッシュ用）
    std::vector<Texture> textures_loaded;
    // モデルを構成する全メッシュ
    std::vector<Mesh> meshes;
    // モデルファイルのディレクトリ
    std::string directory;

    // コンストラクタ
    Model(const std::string &path);

    // 全メッシュを描画
    void Draw(Shader &shader);

private:
    void loadModel(const std::string &path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadMaterialTextures(
        aiMaterial *mat, aiTextureType type, const std::string &typeName);
};
```

---

## loadModel()の実装

`loadModel()` はAssimpを使ってファイルを読み込み、再帰処理を開始するエントリーポイントです。

```cpp
Model::Model(const std::string &path)
{
    loadModel(path);
}

void Model::loadModel(const std::string &path)
{
    // 1. Assimpでファイルを読み込む
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate      |
        aiProcess_FlipUVs          |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace
    );

    // 2. エラーチェック
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }

    // 3. ディレクトリパスを保存（テクスチャの相対パス解決用）
    directory = path.substr(0, path.find_last_of('/'));

    // 4. ルートノードから再帰処理を開始
    processNode(scene->mRootNode, scene);
}
```

### ディレクトリパスの重要性

```
モデルファイル: "resources/models/nanosuit/nanosuit.obj"
                                          ↑
directory = "resources/models/nanosuit"

テクスチャパス（モデル内）: "glass_dif.png"
                             ↓
実際のパス: "resources/models/nanosuit/glass_dif.png"
            = directory + "/" + テクスチャパス
```

テクスチャファイルはモデルファイルからの**相対パス**で記録されていることが多いため、`directory` を保存しておく必要があります。

---

## processNode()の再帰処理

`processNode()` はノードツリーを再帰的に走査し、各ノードのメッシュを処理します。

```cpp
void Model::processNode(aiNode *node, const aiScene *scene)
{
    // 1. 現在のノードが持つ全メッシュを処理
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // ノードのmMeshes[i]はシーンのメッシュ配列へのインデックス
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    // 2. 子ノードに対して再帰呼び出し
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}
```

### 再帰処理の可視化

```
processNode(RootNode)
├── RootNodeのメッシュを処理 (0個)
├── processNode(Body)
│   ├── Bodyのメッシュを処理 (1個 → meshes[0])
│   ├── processNode(LeftArm)
│   │   └── LeftArmのメッシュを処理 (1個 → meshes[1])
│   └── processNode(RightArm)
│       └── RightArmのメッシュを処理 (1個 → meshes[2])
├── processNode(Head)
│   └── Headのメッシュを処理 (2個 → meshes[3], meshes[4])
└── processNode(Weapon)
    └── Weaponのメッシュを処理 (1個 → meshes[5])

結果: meshes配列に6つのメッシュが格納される
```

> 💡 再帰の代わりにスタック/キューを使った反復的な走査も可能ですが、ノードツリーの深さは通常浅いため再帰で問題ありません。

---

## processMesh()の実装

`processMesh()` はAssimpの `aiMesh` からOpenGLで使える `Mesh` オブジェクトに変換する中核関数です。

```cpp
Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // ─── 1. 頂点データの抽出 ───
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        // 位置
        vertex.Position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        // 法線
        if (mesh->HasNormals())
        {
            vertex.Normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        }

        // テクスチャ座標（最大8セットのうち1セット目を使用）
        if (mesh->mTextureCoords[0])
        {
            vertex.TexCoords = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // ─── 2. インデックスデータの抽出 ───
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    // ─── 3. マテリアル / テクスチャの処理 ───
    // mMaterialIndex は unsigned int なので常に >= 0 だが、慣例として記載
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    {

        // ディフューズマップ
        std::vector<Texture> diffuseMaps = loadMaterialTextures(
            material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // スペキュラマップ
        std::vector<Texture> specularMaps = loadMaterialTextures(
            material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        // ノーマルマップ
        // ※ OBJ/MTL の bump マップは Assimp が aiTextureType_HEIGHT として扱うため、
        //    ノーマルマップにこのタイプを使用する（aiTextureType_NORMALS ではない）
        std::vector<Texture> normalMaps = loadMaterialTextures(
            material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        // ハイトマップ（同様に OBJ/MTL 互換の慣例）
        std::vector<Texture> heightMaps = loadMaterialTextures(
            material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
    }

    return Mesh(vertices, indices, textures);
}
```

### データ変換の対応関係

```
Assimp (aiMesh)              →    OpenGL (Mesh)
──────────────────────────────────────────────────
mesh->mVertices[i]           →    vertex.Position
mesh->mNormals[i]            →    vertex.Normal
mesh->mTextureCoords[0][i]   →    vertex.TexCoords
mesh->mFaces[i].mIndices[j]  →    indices[]
scene->mMaterials[index]     →    textures[]
```

---

## loadMaterialTextures()の実装

テクスチャの**重複読み込みを防ぐ**のがこの関数の最大のポイントです。

```cpp
std::vector<Texture> Model::loadMaterialTextures(
    aiMaterial *mat, aiTextureType type, const std::string &typeName)
{
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        // キャッシュをチェック（同じテクスチャを再度読み込まない）
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if (textures_loaded[j].path == std::string(str.C_Str()))
            {
                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }

        if (!skip)
        {
            Texture texture;
            texture.id   = TextureFromFile(str.C_Str(), directory);
            texture.type = typeName;
            texture.path = std::string(str.C_Str());
            textures.push_back(texture);
            textures_loaded.push_back(texture);  // キャッシュに追加
        }
    }

    return textures;
}
```

### テクスチャキャッシュの仕組み

```
1回目: loadMaterialTextures(mat, DIFFUSE, ...)
  → "wall.png" → 未読み込み → TextureFromFile() → textures_loaded に追加
  → "floor.png" → 未読み込み → TextureFromFile() → textures_loaded に追加

2回目: loadMaterialTextures(other_mat, DIFFUSE, ...)
  → "wall.png" → textures_loaded で発見！ → スキップ（再利用）
  → "metal.png" → 未読み込み → TextureFromFile() → textures_loaded に追加

textures_loaded = [wall.png, floor.png, metal.png]  ← 3回のロードで済む
                   （キャッシュなしなら4回）
```

モデルによっては**数十のメッシュが同じテクスチャを共有**することがあるため、このキャッシュは非常に効果的です。

---

## TextureFromFile()ヘルパー関数

stb_imageでテクスチャファイルを読み込み、OpenGLテクスチャを生成する関数です。

```cpp
unsigned int TextureFromFile(const char *path, const std::string &directory)
{
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height,
                                    &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height,
                     0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // テクスチャパラメータ設定
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cerr << "Texture failed to load at path: " << filename << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
```

### チャンネル数による形式の選択

| nrComponents | format | 説明 |
|-------------|--------|------|
| 1 | `GL_RED` | グレースケール画像 |
| 3 | `GL_RGB` | 通常のカラー画像 |
| 4 | `GL_RGBA` | アルファチャンネル付き画像 |

---

## Draw()とmain()での使用

### Model::Draw()

```cpp
void Model::Draw(Shader &shader)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
    {
        meshes[i].Draw(shader);
    }
}
```

非常にシンプル — 全Meshの `Draw()` を順番に呼ぶだけです。

### main()での使用例

```cpp
#include "model.h"
#include "shader.h"
#include "camera.h"

int main()
{
    // ウィンドウ・OpenGL初期化（省略）...

    // シェーダをロード
    Shader shader("model_vertex.glsl", "model_fragment.glsl");

    // モデルをロード（コンストラクタ内で全処理が完了）
    Model ourModel("resources/models/nanosuit/nanosuit.obj");

    // レンダリングループ
    while (!glfwWindowShouldClose(window))
    {
        // 入力処理...
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // ビュー・プロジェクション行列
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        // モデル行列（位置・回転・スケール）
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));
        shader.setMat4("model", model);

        // 描画！
        ourModel.Draw(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
```

複数モデルを配置する場合は、それぞれのModelに異なる `model` 行列（translate/rotate/scale）を設定してから `Draw()` を呼ぶだけです。

---

## 💡 ポイントまとめ

| 項目 | 内容 |
|------|------|
| Model | 複数Meshを管理し、Assimpからの変換を行う上位クラス |
| loadModel() | Assimp::Importer.ReadFile() + エラーチェック + processNode()呼び出し |
| processNode() | ノードツリーを再帰走査し、各ノードのメッシュをprocessMesh()で変換 |
| processMesh() | aiMesh → Vertex/Index/Texture → Meshオブジェクト生成 |
| loadMaterialTextures() | テクスチャをロードし、textures_loadedでキャッシュ管理 |
| TextureFromFile() | stb_imageでロード → glTexImage2D → ミップマップ生成 |
| directory | テクスチャの相対パス解決用にモデルファイルのディレクトリを保存 |
| textures_loaded | 同じテクスチャの重複ロードを防ぐキャッシュ配列 |
| Draw() | 全meshesのDraw()を順番に呼ぶだけのシンプルな関数 |
| nrComponents | 画像のチャンネル数（1=RED, 3=RGB, 4=RGBA）で形式を切り替え |

---

## ドリル問題

### 問1: 再帰処理の理解（穴埋め）
`processNode()` で、まず現在のノードの全______を処理し、次に全______に対して再帰呼び出しを行う。

<details><summary> 解答</summary>

- **メッシュ**（`node->mNumMeshes` 個のメッシュ）
- **子ノード**（`node->mNumChildren` 個の子ノード）

`processNode()` は深さ優先でツリーを走査し、すべてのノードが持つメッシュを漏れなく処理します。

</details>

### 問2: テクスチャキャッシュ（計算問題）
モデルが10個のメッシュを持ち、うち7個が "diffuse.png"を、残り3個が "specular.png" を使っている。テクスチャキャッシュなしの場合とありの場合で、`TextureFromFile()` の呼び出し回数はそれぞれ何回か？

<details><summary> 解答</summary>

- **キャッシュなし:** 10回（メッシュごとに毎回ロード）
- **キャッシュあり:** 2回（"diffuse.png"の初回 + "specular.png"の初回）

キャッシュにより`TextureFromFile()`の呼び出しが10回→2回に削減されます。テクスチャの読み込みはディスクI/OとGPU転送を伴う重い処理なので、この最適化は非常に重要です。

</details>

### 問3: ディレクトリパス（穴埋め）
モデルファイルのパスが `"assets/models/house/house.obj"` のとき、`directory` の値は `"______"` となる。テクスチャパスが `"roof.png"` の場合、最終的なファイルパスは `"______"` になる。

<details><summary> 解答</summary>

- directory: `"assets/models/house"`
- 最終パス: `"assets/models/house/roof.png"`

`path.substr(0, path.find_last_of('/'))` でディレクトリ部分を抽出し、テクスチャパスを `directory + '/' + texturePath` で結合します。

</details>

### 問4: エラーハンドリング（選択問題）
`loadModel()` のエラーチェックで確認する3つの条件として**正しい組み合わせ**はどれか？

- A) `scene == nullptr`, `scene->mNumMeshes == 0`, `scene->mRootNode == nullptr`
- B) `scene == nullptr`, `scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE`, `scene->mRootNode == nullptr`
- C) `importer == nullptr`, `scene->mFlags == 0`, `scene->mMeshes == nullptr`
- D) `scene == nullptr`, `path.empty()`, `scene->mRootNode == nullptr`

<details><summary> 解答</summary>

**B) `scene == nullptr`, `scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE`, `scene->mRootNode == nullptr`**

この3条件は LearnOpenGL の標準パターンです：
1. シーンがnull → ファイルが見つからない・形式非対応
2. INCOMPLETEフラグ → データが不完全
3. ルートノードがnull → シーン構造が壊れている

</details>

### 問5: processMesh()の穴埋め
頂点データ抽出部分を完成させよ。

```cpp
for (unsigned int i = 0; i < mesh->mNumVertices; i++)
{
    Vertex vertex;
    vertex.Position = glm::vec3(
        mesh->______[i].x,
        mesh->______[i].y,
        mesh->______[i].z
    );

    if (mesh->mTextureCoords[______])
    {
        vertex.TexCoords = glm::vec2(
            mesh->mTextureCoords[______][i].x,
            mesh->mTextureCoords[______][i].y
        );
    }
    vertices.push_back(vertex);
}
```

<details><summary> 解答</summary>

```cpp
vertex.Position = glm::vec3(
    mesh->mVertices[i].x,
    mesh->mVertices[i].y,
    mesh->mVertices[i].z
);

if (mesh->mTextureCoords[0])
{
    vertex.TexCoords = glm::vec2(
        mesh->mTextureCoords[0][i].x,
        mesh->mTextureCoords[0][i].y
    );
}
```

- `mVertices` は頂点座標の配列
- `mTextureCoords[0]` は最初のテクスチャ座標セット（最大8セットまで保持可能だが、通常は0番を使用）

</details>

### 問6: インデックス抽出（穴埋め）
`processMesh()` でインデックスを抽出するコードを完成させよ。

```cpp
for (unsigned int i = 0; i < mesh->______; i++)
{
    aiFace face = mesh->______[i];
    for (unsigned int j = 0; j < face.______; j++)
    {
        indices.push_back(face.______[j]);
    }
}
```

<details><summary> 解答</summary>

```cpp
for (unsigned int i = 0; i < mesh->mNumFaces; i++)
{
    aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++)
    {
        indices.push_back(face.mIndices[j]);
    }
}
```

- `mNumFaces` → 面の総数
- `mFaces[i]` → i番目の面（`aiProcess_Triangulate` 使用時は常に3頂点）
- `mNumIndices` → その面の頂点インデックス数（三角形なら3）
- `mIndices[j]` → 頂点配列へのインデックス

</details>

### 問7: 処理順序（並べ替え問題）
Model読み込みの処理順序を正しく並べ替えよ。

- (a) `processNode()` で子ノードに再帰
- (b) `Assimp::Importer.ReadFile()` でファイル読み込み
- (c) `processMesh()` で aiMesh を Mesh に変換
- (d) `loadMaterialTextures()` でテクスチャ読み込み
- (e) ディレクトリパスを保存
- (f) エラーチェック（3条件）

<details><summary> 解答</summary>

**b → f → e → c & d → a**

詳細:
1. **(b)** `ReadFile()` でAssimpがファイルを解析し `aiScene` を生成
2. **(f)** scene/flags/rootNode のエラーチェック
3. **(e)** `directory` にモデルファイルのディレクトリパスを保存
4. **(c)** `processMesh()` で頂点・インデックスを抽出し、**(d)** その途中で `loadMaterialTextures()` がテクスチャを読み込む
5. **(a)** 子ノードに対して再帰的に同じ処理を繰り返す

cとdはprocessMesh()の中でdが呼ばれるため、同時に行われます。

</details>

---

## 実践課題

### 課題1: 基本的なモデル読み込みと表示 
Modelクラスを実装し、無料の3Dモデル（例: backpack, nanosuit）を読み込んで画面に表示せよ。

**チェックポイント:**
- [ ] Model, Mesh, Vertex, Texture の全クラス/構造体を実装した
- [ ] 無料の3Dモデルをダウンロードし、プロジェクトに配置した
- [ ] コンパイルエラーなくビルドできた
- [ ] モデルが画面に表示された
- [ ] カメラ操作（WASDキーとマウス）で閲覧できた
- [ ] テクスチャが正しく表示されている（真っ白や真っ黒でない）

### 課題2: 複数モデルの配置 
異なるモデルを複数読み込み、シーン内の異なる位置に配置せよ。

**チェックポイント:**
- [ ] 2種類以上のモデルファイルを用意した
- [ ] 各モデルを異なるmodel行列（位置・回転・スケール）で配置した
- [ ] 同じモデルを複数インスタンス配置した（異なる位置に3つ以上）
- [ ] すべてのモデルが正しく表示される
- [ ] フレームレートが許容範囲内である

### 課題3: ライティング付きモデル表示 
Lightingチャプターで学んだPhongシェーディングをモデルに適用せよ。

**チェックポイント:**
- [ ] フラグメントシェーダにアンビエント・ディフューズ・スペキュラ計算を実装した
- [ ] ライト位置・カメラ位置のuniformを設定した
- [ ] ディフューズマップとスペキュラマップが正しく反映されている
- [ ] ライトの位置を変えると陰影が変化することを確認した

---

## ナビゲーション

 [メッシュ](./02-mesh.md) | [深度テスト →](../05-advanced-opengl/01-depth-testing.md)
