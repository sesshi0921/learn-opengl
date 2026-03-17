# Assimp（Open Asset Import Library）

> **目標：** 3Dモデルファイルの基本概念を理解し、Assimpライブラリのデータ構造・読み込みフラグ・CMake統合方法を習得する。自分のOpenGLプロジェクトでAssimpを使って任意の3Dモデルを読み込む準備ができるようになる。

---

## なぜ3Dモデルファイルが必要なのか

これまでのチュートリアルでは、立方体や平面の頂点データを手書きで配列に定義してきました。しかし、現実のアプリケーションではキャラクターや建物、武器など、何千・何万もの三角形で構成される複雑なオブジェクトを扱います。

```cpp
// 立方体ですら36頂点 × 8属性 = 288個の浮動小数点数…
float vertices[] = {
    // positions // normals // texture coords
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
     0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
    // ... これが数万行になったら？
};
```

これを手動で定義するのは**非現実的**です。3Dアーティストは Blender・Maya・3ds Max などのモデリングツールでモデルを作成し、**ファイルに書き出し**ます。プログラマはそのファイルを読み込んで描画するだけでよいのです。

### モデルファイルに含まれる情報

```
┌─────────────────────────────────┐
│ 3Dモデルファイル │
├─────────────────────────────────┤
│ 頂点座標 (Positions) │
│ 法線ベクトル (Normals) │
│ テクスチャ座標 (UV) │
│ 面 (Faces / Indices) │
│ マテリアル (Materials) │
│ テクスチャパス (Texture paths) │
│ ボーン / アニメーション │
│ ノード階層 (Scene graph) │
└─────────────────────────────────┘
```

---

## 3Dモデルのファイル形式

世の中には数十種類の3Dファイル形式が存在します。主要なものを見てみましょう。

| 形式 | 拡張子 | 特徴 | テキスト/バイナリ |
|------|--------|------|-------------------|
| **Wavefront OBJ** | `.obj` + `.mtl` | シンプルで広く対応。テキストベースで可読性が高い | テキスト |
| **FBX** | `.fbx` | Autodesk製。アニメーション対応が強力 | バイナリ（テキスト版もある） |
| **glTF 2.0** | `.gltf` / `.glb` | Khronos標準。「3DのJPEG」と呼ばれる。PBR対応 | JSON + バイナリ |
| **COLLADA** | `.dae` | XMLベース。情報量が多いが重い | テキスト(XML) |
| **3DS** | `.3ds` | 古い形式だがレガシー資産が多い | バイナリ |
| **STL** | `.stl` | 3Dプリント向け。色・テクスチャ情報なし | テキスト/バイナリ |
| **PLY** | `.ply` | 3Dスキャンデータ向け。頂点カラー対応 | テキスト/バイナリ |

### OBJ + MTL形式の例

OBJはテキストベースなので中身を読むことができます：

```
# cube.obj
mtllib cube.mtl # マテリアルファイル参照

v -1.0 1.0 1.0 # 頂点座標 (vertex)
v 1.0 1.0 1.0
v 1.0 -1.0 1.0
v -1.0 -1.0 1.0

vt 0.0 1.0 # テクスチャ座標 (vertex texture)
vt 1.0 1.0
vt 1.0 0.0
vt 0.0 0.0

vn 0.0 0.0 1.0 # 法線 (vertex normal)

usemtl FrontMaterial # マテリアル指定
f 1/1/1 2/2/1 3/3/1 # 面 (face): 頂点/UV/法線 のインデックス
f 1/1/1 3/3/1 4/4/1
```

対応するMTLファイル：

```
# cube.mtl
newmtl FrontMaterial
Ka 0.1 0.1 0.1 # アンビエント色
Kd 0.8 0.2 0.2 # ディフューズ色
Ks 1.0 1.0 1.0 # スペキュラ色
Ns 32.0 # 光沢度
map_Kd textures/wall.png # ディフューズテクスチャ
map_Ks textures/spec.png # スペキュラテクスチャ
```

> 💡 OBJ形式は「v/vt/vn/f」という単純な構文なので、パーサを自作することも可能です。しかし、**すべてのフォーマットに対応する**ために Assimp を使います。

---

## Assimpとは

**Assimp**（Open Asset Import Library）は、**40種類以上**の3Dファイル形式を統一的なデータ構造に変換してくれるオープンソースライブラリです。

```
   OBJ ──┐
   FBX ──┤ ┌──────────────┐
  glTF ──┼──→ [ Assimp ] ──→│ 統一された │──→ OpenGL描画
   DAE ──┤ │ データ構造 │
   3DS ──┘ └──────────────┘
```

Assimpがやってくれること：
- ファイル形式の差異を吸収
- ポストプロセス（三角形分割、法線生成、UV反転など）
- 統一的な `aiScene` オブジェクトとしてデータを提供

---

## Assimpのデータ構造

Assimpがモデルを読み込むと、以下の**ツリー構造**でデータが格納されます。これがAssimpを理解するうえで最も重要な部分です。

```
aiScene (シーン全体)
├── mFlags ... シーンのフラグ
├── mRootNode ─────────────→ aiNode (ルートノード)
│ ├── mName
│ ├── mTransformation ... 変換行列
│ ├── mMeshes[] ... メッシュのインデックス配列
│ ├── mChildren[] ──→ aiNode (子ノード)
│ │ ├── mMeshes[]
│ │ └── mChildren[] ──→ ...
│ └── mParent
│
├── mMeshes[] ─────────────→ aiMesh (メッシュ配列)
│ ├── mVertices[] ... 頂点座標
│ ├── mNormals[] ... 法線
│ ├── mTextureCoords[][] ... UV座標(最大8セット)
│ ├── mTangents[] ... タンジェント
│ ├── mFaces[] ────→ aiFace
│ │ └── mIndices[] ... 頂点インデックス
│ └── mMaterialIndex ... マテリアルのインデックス
│
├── mMaterials[] ──────────→ aiMaterial (マテリアル配列)
│ ├── diffuse color / texture
│ ├── specular color / texture
│ ├── ambient / emissive
│ └── shininess, opacity...
│
├── mTextures[] ───────────→ aiTexture (埋め込みテクスチャ)
│ ├── mWidth, mHeight
│ └── pcData (ピクセルデータ)
│
├── mAnimations[] ... アニメーション
└── mLights[] / mCameras[] ... ライト・カメラ
```

### データ構造の核心ポイント

**aiNode はメッシュを「所有」しない。インデックスで参照する。**

```
aiNode.mMeshes[0] = 2 → aiScene.mMeshes[2] を参照
aiNode.mMeshes[1] = 5 → aiScene.mMeshes[5] を参照
```

これにより、複数のノードが同じメッシュを共有できます（インスタンシング）。

**aiMesh.mMaterialIndex は aiScene.mMaterials の添字**

```
aiMesh.mMaterialIndex = 1 → aiScene.mMaterials[1] を参照
```

メッシュとマテリアルが分離されているため、同じマテリアルを複数メッシュで再利用できます。

---

## CMakeでのAssimp導入

### 方法1: サブモジュールとして追加（推奨）

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.14)
project(LearnOpenGL)

# Assimpのビルドオプション
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)

add_subdirectory(external/assimp)

add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE assimp)
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/external/assimp/include)
```

### 方法2: find_packageを使う（システムインストール済の場合）

```cmake
find_package(assimp REQUIRED)
target_link_libraries(main PRIVATE assimp::assimp)
```

### 方法3: vcpkg / Conanを利用

```bash
# vcpkg
vcpkg install assimp

# Conan
conan install assimp/5.3.1@
```

---

## Assimpの読み込みフラグ（Post-Processing）

Assimpの真の力は**ポストプロセスフラグ**にあります。読み込み時にデータを自動変換できます。

```cpp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Assimp::Importer importer;
const aiScene* scene = importer.ReadFile(
    path,
    aiProcess_Triangulate | // 全ポリゴンを三角形に
    aiProcess_FlipUVs | // UVのY軸を反転
    aiProcess_GenSmoothNormals | // 法線がない場合に生成
    aiProcess_CalcTangentSpace // タンジェント計算
);
```

### フラグ詳細一覧

| フラグ | 効果 | いつ使う？ |
|--------|------|-----------|
| `aiProcess_Triangulate` | すべてのポリゴンを三角形に分割 | **ほぼ常に必要**。OpenGLは三角形が基本 |
| `aiProcess_FlipUVs` | テクスチャ座標のY軸を反転 | OpenGLのUV原点（左下）とモデルの原点（左上）が異なる場合 |
| `aiProcess_GenNormals` | 法線が存在しない場合にフラット法線を生成 | モデルに法線データがない場合 |
| `aiProcess_GenSmoothNormals` | スムーズ法線を生成 | 滑らかなシェーディングが必要な場合 |
| `aiProcess_CalcTangentSpace` | タンジェント・バイタンジェントを計算 | ノーマルマッピングを使用する場合 |
| `aiProcess_JoinIdenticalVertices` | 重複頂点を統合しインデックスを最適化 | メモリ削減・描画高速化 |
| `aiProcess_OptimizeMeshes` | 小さいメッシュを結合してドローコール削減 | パフォーマンス最適化 |
| `aiProcess_OptimizeGraph` | ノード階層を簡略化 | 複雑なシーングラフの簡略化 |
| `aiProcess_ValidateDataStructure` | データの整合性チェック | デバッグ時 |
| `aiProcess_SplitLargeMeshes` | 大きなメッシュを分割（頂点数制限対策） | 古いハードウェア対応時 |
| `aiProcess_LimitBoneWeights` | ボーンウェイト数を制限（デフォルト4） | スキニングアニメーション時 |

### エラーチェック

```cpp
const aiScene* scene = importer.ReadFile(path, flags);

if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
    return;
}
```

3つの条件をすべてチェックする理由：
1. `scene == nullptr` → ファイルが見つからない、形式が非対応
2. `AI_SCENE_FLAGS_INCOMPLETE` → 読み込みは成功したがデータ不完全
3. `mRootNode == nullptr` → シーンは有効だがノード階層がない

---

## 無料の3Dモデル入手先

学習用に無料で3Dモデルを入手できるサイトをいくつか紹介します。

| サイト | URL | 特徴 |
|--------|-----|------|
| Sketchfab | sketchfab.com | 豊富な無料モデル。glTF直接ダウンロード可 |
| TurboSquid | turbosquid.com | 無料・有料混在。OBJ/FBX多数 |
| Free3D | free3d.com | 無料特化。OBJ形式が多い |
| Open Game Art | opengameart.org | ゲーム用素材。CC0ライセンスが多い |
| KhronosGroup glTF-Sample-Models | GitHub | 公式テスト用glTFモデル集 |

> ⚠️ ライセンスを必ず確認しましょう。商用利用不可のモデルもあります。

---

## 💡 ポイントまとめ

| 項目 | 内容 |
|------|------|
| 3Dモデルファイル | 頂点・法線・UV・面・マテリアル・テクスチャ等を格納 |
| OBJ形式 | テキストベースで可読性が高い。v/vt/vn/f構文 |
| MTL形式 | OBJのマテリアル定義ファイル。色・テクスチャパス等 |
| Assimp | 40+フォーマットを統一データ構造に変換するライブラリ |
| aiScene | 全データのルート。メッシュ・マテリアル・ノードを保持 |
| aiNode | ツリー構造のノード。メッシュへのインデックスと変換行列を持つ |
| aiMesh | 頂点・法線・UV・面データの実体 |
| aiFace | 面を構成する頂点インデックスの配列 |
| aiMaterial | ディフューズ・スペキュラ等のマテリアル情報 |
| Triangulate | 最重要フラグ。すべてのポリゴンを三角形に変換 |
| FlipUVs | OpenGLのUV座標系に合わせるためY軸反転 |
| JoinIdenticalVertices | 重複頂点の統合で最適化 |

---

## ドリル問題

### 問1: データ構造の理解（穴埋め）
Assimpでモデルを読み込むと、最上位のオブジェクトは `______` であり、そこから `______` を辿ることでシーン全体のノードツリーにアクセスできる。

<details><summary> 解答</summary>

- `aiScene`
- `mRootNode`

`aiScene` がすべてのデータのルートオブジェクトであり、`mRootNode` からノードの階層構造を再帰的に辿ることができます。

</details>

### 問2: インデックス参照の仕組み（選択問題）
`aiNode` が持つ `mMeshes` 配列に格納されているのは何か？

- A) `aiMesh` オブジェクトそのもの
- B) `aiScene::mMeshes` 配列へのインデックス（整数値）
- C) メッシュのファイルパス
- D) 頂点座標の配列

<details><summary> 解答</summary>

**B) `aiScene::mMeshes` 配列へのインデックス（整数値）**

`aiNode` はメッシュを直接所有しません。`mMeshes[i]` の値を使って `scene->mMeshes[node->mMeshes[i]]` のように間接参照します。これにより複数ノードが同じメッシュを共有できます。

</details>

### 問3: フラグの効果（穴埋め）
OpenGLでは三角形プリミティブが基本であるため、Assimpの読み込み時には `______` フラグを指定して全ポリゴンを三角形に変換する。また、OpenGLとモデルファイルでテクスチャ座標のY軸方向が逆の場合は `______` フラグを使う。

<details><summary> 解答</summary>

- `aiProcess_Triangulate`
- `aiProcess_FlipUVs`

`aiProcess_Triangulate` は四角形やN角形を三角形に分割します。`aiProcess_FlipUVs` はUV座標の v 成分を `1.0 - v` に変換してOpenGLの座標系に合わせます。

</details>

### 問4: エラーチェック（選択問題）
Assimpで読み込んだ後のエラーチェックとして**不適切**なものはどれか？

- A) `scene == nullptr` のチェック
- B) `scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE` のチェック
- C) `scene->mRootNode == nullptr` のチェック
- D) `scene->mNumMeshes == 0` のチェック

<details><summary> 解答</summary>

**D) `scene->mNumMeshes == 0` のチェック**

メッシュが0個でもシーンとして有効な場合があります（カメラやライトのみのシーンなど）。A, B, C の3つが標準的なエラーチェックパターンです。

</details>

### 問5: ファイル形式の特徴（○×問題）
以下の各文が正しければ○、誤りなら×を答えよ。

1. OBJ形式はバイナリ形式であり、テキストエディタでは読めない
2. glTFはKhronos Groupが策定した標準フォーマットである
3. FBX形式はAutodesk社が開発した
4. STL形式はテクスチャ情報を持つことができる
5. Assimpは10種類程度のフォーマットのみ対応している

<details><summary> 解答</summary>

1. **×** — OBJはテキストベースで、テキストエディタで開いて読むことができます
2. **○** — glTF（GL Transmission Format）はKhronos Groupが策定し、「3DのJPEG」とも呼ばれます
3. **○** — FBXはAutodesk社が開発したプロプライエタリ形式です
4. **×** — STLは3Dプリント向けの形式で、頂点座標と法線のみを持ちテクスチャ情報は含みません
5. **×** — Assimpは40種類以上のフォーマットに対応しています

</details>

### 問6: OBJファイルの読解（穴埋め）
OBJファイルにおいて、`v` は______を、`vt` は______を、`vn` は______を、`f` は______を定義する。

<details><summary> 解答</summary>

- `v` → **頂点座標（vertex position）**
- `vt` → **テクスチャ座標（vertex texture coordinate）**
- `vn` → **法線ベクトル（vertex normal）**
- `f` → **面（face）**

`f 1/2/3` の形式は「頂点インデックス/UVインデックス/法線インデックス」を意味します。

</details>

### 問7: マテリアルの参照（計算問題）
あるシーンに5つのメッシュ（インデックス0〜4）と3つのマテリアル（インデックス0〜2）がある。メッシュ0, 1のmMaterialIndexは0、メッシュ2, 3のmMaterialIndexは1、メッシュ4のmMaterialIndexは2である。マテリアル1を使用しているメッシュは何個か？

<details><summary> 解答</summary>

**2個**（メッシュ2とメッシュ3）

`mMaterialIndex` が1であるメッシュは2と3の2つです。このように、複数メッシュが同じマテリアルを共有することでデータを効率的に管理できます。

</details>

---

## 実践課題

### 課題1: OBJファイルの手動解析 
テキストエディタで簡単なOBJファイル（立方体など）を開き、以下を確認せよ。

**チェックポイント:**
- [ ] 頂点数（`v` の行数）を数えられた
- [ ] テクスチャ座標数（`vt` の行数）を数えられた
- [ ] 面数（`f` の行数）を数えられた
- [ ] 対応するMTLファイルの内容を確認できた
- [ ] どの面がどのマテリアルを使っているか特定できた

### 課題2: Assimpのビルドと基本読み込み 
CMakeプロジェクトにAssimpを組み込み、任意のモデルファイルを読み込んでシーン情報を出力せよ。

```cpp
// 期待する出力例:
// Meshes: 3
// Materials: 2
// Textures: 4
// Root node children: 5
```

**チェックポイント:**
- [ ] CMakeでAssimpをリンクしてビルドが通った
- [ ] `Assimp::Importer` でファイルを読み込めた
- [ ] エラーチェック（3条件）を実装した
- [ ] メッシュ数・マテリアル数・テクスチャ数を出力できた
- [ ] ルートノードの子ノード数を出力できた

### 課題3: 複数フォーマットの読み込み比較 
同じモデルをOBJ, FBX, glTFなど異なる形式で入手（または変換）し、Assimpで読み込んだ結果を比較せよ。

**チェックポイント:**
- [ ] 2種類以上の形式でモデルを用意した
- [ ] 各形式でメッシュ数・頂点数・マテリアル数を比較した
- [ ] 読み込みフラグの有無で結果が変わることを確認した
- [ ] `aiProcess_JoinIdenticalVertices` の有無で頂点数の違いを確認した
- [ ] 結果を表にまとめて違いを考察した

### 課題4: ポストプロセスフラグの効果検証 
各ポストプロセスフラグの効果を確認するプログラムを作成せよ。

**チェックポイント:**
- [ ] フラグなしで読み込んだ場合の頂点数・面数を記録した
- [ ] `aiProcess_Triangulate` 適用後の面数の変化を確認した
- [ ] `aiProcess_GenSmoothNormals` で法線が生成されることを確認した
- [ ] `aiProcess_OptimizeMeshes` でメッシュ数の変化を確認した

---

## ナビゲーション

 [複数の光源](../03-lighting/06-multiple-lights.md) | [メッシュ →](./02-mesh.md)
