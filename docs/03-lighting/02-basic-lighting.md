# 📘 基本的なライティング（Basic Lighting）

> **目標：** Phong照明モデルの3成分（アンビエント・ディフューズ・スペキュラー）を理解し、GLSLで実装して現実的なライティング効果を描画できるようになる。

---

## 📖 Phong照明モデルの概要

### 3つの光の成分

現実の光は極めて複雑ですが、**Phong照明モデル**は3つの成分の組み合わせで十分にリアルな結果を近似します：

```
┌─────────────────────────────────────────────┐
│             Phong 照明モデル                   │
│                                             │
│  最終色 = アンビエント + ディフューズ + スペキュラー  │
│                                             │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐    │
│  │ Ambient  │ │ Diffuse  │ │ Specular │    │
│  │ 環境光   │ │ 拡散光   │ │ 鏡面光    │    │
│  │          │ │          │ │    ★     │    │
│  │ 均一に   │ │ 角度に   │ │ 反射方向  │    │
│  │ 全体を   │ │ 応じて   │ │ にハイ    │    │
│  │ 照らす   │ │ 明暗が   │ │ ライト   │    │
│  │          │ │ つく     │ │          │    │
│  └──────────┘ └──────────┘ └──────────┘    │
└─────────────────────────────────────────────┘
```

| 成分 | 役割 | 直感的な説明 |
|------|------|-------------|
| **アンビエント** | 間接光の近似 | 影の部分も真っ黒にならない最低限の明るさ |
| **ディフューズ** | 直接光の拡散反射 | 光に面した部分が明るくなる |
| **スペキュラー** | 鏡面反射のハイライト | ツルツルした表面のキラッとした光沢 |

---

## 📖 アンビエント（環境光）

### なぜ必要なのか？

現実世界では、光は壁や床に何度も反射して部屋全体を照らします（間接光/グローバルイルミネーション）。しかし、これを正確にシミュレーションするのは非常に高コストです。

アンビエントライティングは**「間接光の大雑把な近似」**です。全体に一定の光を加えることで、光が直接当たらない面も真っ黒にならないようにします。

### 実装

```glsl
// アンビエント成分の計算
float ambientStrength = 0.1; // 環境光の強さ（0.0〜1.0）
vec3 ambient = ambientStrength * lightColor;
```

`ambientStrength` の意味：
- `0.0` → 環境光なし（影は真っ黒）
- `0.1` → 控えめな環境光（自然な暗がり）
- `0.5` → 強い環境光（影が薄い）
- `1.0` → 全体が光の色で均一に照らされる

```
ambientStrength = 0.1            ambientStrength = 0.5
┌──────────┐                    ┌──────────┐
│▓▓▓▓░░░░░░│                    │▒▒▒▒░░░░░░│
│▓▓▓▓░░░░░░│  ← 影が暗い       │▒▒▒▒░░░░░░│  ← 影が明るい
│▓▓▓▓░░░░░░│                    │▒▒▒▒░░░░░░│
└──────────┘                    └──────────┘
```

---

## 📖 ディフューズ（拡散光）

### 理論 — ランバートの余弦則

ディフューズライティングは、物体表面が光源の方向をどれだけ「向いているか」で明るさが決まります。これは**法線ベクトル**と**光の方向ベクトル**の角度関係で計算されます。

```
        光源 ☀️
         │
         │ lightDir
         │
    ─────┼─────
         │θ
    ────►│  法線 (Normal)
    表面  │
```

- θ = 0°（光に正面を向く）→ 最も明るい（`cos(0°) = 1.0`）
- θ = 90°（光と平行）→ 暗い（`cos(90°) = 0.0`）
- θ > 90°（光の反対側）→ 完全に暗い（負の値は 0 にクランプ）

### 必要な情報

ディフューズ計算には以下が必要です：

1. **法線ベクトル（Normal）** — 表面に垂直な方向
2. **光の方向ベクトル（Light Direction）** — フラグメントから光源への方向

### 法線ベクトル

立方体の各面について、法線を頂点データに追加します：

```cpp
float vertices[] = {
    // 位置              // 法線
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 前面
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    // ...
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 背面
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    // ... 他の面も同様
};
```

strideが変わるため、頂点属性の設定を更新します：

```cpp
// 位置属性
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);
// 法線属性
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                      (void*)(3 * sizeof(float)));
glEnableVertexAttribArray(1);
```

### 内積による明るさ計算

```glsl
// フラグメントシェーダー内
vec3 norm = normalize(Normal);
vec3 lightDir = normalize(lightPos - FragPos);

float diff = max(dot(norm, lightDir), 0.0);
vec3 diffuse = diff * lightColor;
```

`max(..., 0.0)` で負の値（光の裏側）をクランプしています。クランプしないと、光の反対側が「負の明るさ」＝不自然に暗くなってしまいます。

---

## 📖 法線行列（Normal Matrix）

### 問題：非一様スケーリング

モデル行列に**非一様スケーリング**（例：X方向だけ2倍に伸ばす）が含まれると、法線ベクトルが歪んで表面に垂直でなくなります：

```
正常な法線:             非一様スケール後:
    ↑ N                      ↗ N（歪んだ！）
────┼────               ─────────────
    │                   伸ばされた面
```

### 解決策：法線行列

法線を正しく変換するには**法線行列**を使います：

```
法線行列 = transpose(inverse(model))
```

```glsl
// バーテックスシェーダー内
Normal = mat3(transpose(inverse(model))) * aNormal;
```

> ⚠️ `inverse()` はGPU上では高コストな演算です。学習目的では問題ありませんが、本番ではCPU側で計算してuniformとして送るのが一般的です。

---

## 📖 スペキュラー（鏡面光）

### 理論 — 反射ベクトルと視線方向

スペキュラーライティングは、光の**反射方向**と**カメラ（視線）方向**がどれだけ一致するかで決まります。一致度が高いほど強い「ハイライト」が見えます。

```
          ☀️ 光源
           \
            \ 入射光
             \
    ──────────\──────────── 表面
              /\
        反射光/  \←→ 視線方向
            /  θ \
           /      👁️ カメラ
          ↙
    θが小さい → ハイライト強い
```

### 実装

```glsl
// スペキュラー成分の計算
float specularStrength = 0.5;
vec3 viewDir = normalize(viewPos - FragPos);
vec3 reflectDir = reflect(-lightDir, norm);

float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
vec3 specular = specularStrength * spec * lightColor;
```

重要なポイント：
- `reflect(-lightDir, norm)` — `lightDir` は「フラグメント→光源」方向なので、反転して「光源→フラグメント」にする
- `pow(..., 32)` — 指数（shininess）で反射の鋭さを調整
- `max(..., 0.0)` — 負の値をクランプ

### shininess（輝き係数）の影響

```
shininess = 2          shininess = 32         shininess = 256
┌──────────┐          ┌──────────┐          ┌──────────┐
│  ░░░░░░  │          │    ░░    │          │     ░    │
│ ░░░░░░░░ │          │   ░░░   │          │    ░░    │
│  ░░░░░░  │          │    ░░    │          │     ░    │
└──────────┘          └──────────┘          └──────────┘
 ぼんやり広い           中程度                 鋭いピンポイント
```

| shininess | 見た目 | 素材の例 |
|-----------|--------|---------|
| 2 | 非常にぼやけたハイライト | ゴム、布 |
| 32 | 適度なハイライト | プラスチック |
| 64 | やや鋭いハイライト | 陶器 |
| 128 | 鋭いハイライト | 金属（磨かれた） |
| 256 | 非常に鋭いピンポイント | 鏡、クロム |

---

## 📖 完全な Phong フラグメントシェーダー

3成分を合成した完全なシェーダーです：

```glsl
#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

void main()
{
    // アンビエント
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // ディフューズ
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // スペキュラー
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    // 合成
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
```

### 対応するバーテックスシェーダー

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
```

### CPU側の uniform 設定

```cpp
objectShader.use();
objectShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
objectShader.setVec3("lightColor",  1.0f, 1.0f, 1.0f);
objectShader.setVec3("lightPos",    lightPos);
objectShader.setVec3("viewPos",     camera.Position);
```

---

## 📖 ワールド座標 vs ビュー座標

ライティング計算は**ワールド座標**と**ビュー座標**のどちらでも行えます：

| 計算空間 | 利点 | 欠点 |
|----------|------|------|
| **ワールド座標** | 直感的、デバッグしやすい | viewPos をuniformで送る必要あり |
| **ビュー座標** | viewPos は常に原点`(0,0,0)` | 全ベクトルをビュー空間に変換する必要あり |

本チュートリアルではワールド座標を使用しています。ビュー座標で計算する場合は、法線と他のベクトルもビュー行列で変換する必要があります。

---

## 💡 ポイントまとめ

| 項目 | 説明 |
|------|------|
| Phong モデル | Ambient + Diffuse + Specular の3成分の合計 |
| アンビエント | `ambientStrength * lightColor`（全体を均一に照らす） |
| ディフューズ | `max(dot(norm, lightDir), 0.0) * lightColor`（角度に依存） |
| スペキュラー | `pow(max(dot(viewDir, reflectDir), 0.0), shininess) * lightColor` |
| 法線行列 | `transpose(inverse(model))` で非一様スケーリング時も法線を正しく変換 |
| reflect関数 | `reflect(-lightDir, norm)` で反射ベクトルを計算 |
| shininess | 値が大きいほどハイライトが鋭くなる |
| max関数 | 内積の結果が負にならないようにクランプ |
| FragPos | ワールド座標でのフラグメント位置（`model * aPos`） |

---

## ✏️ ドリル問題

### 問1: 三成分の穴埋め

Phong照明モデルの最終色の式を完成させよ：

```
最終色 = (______ + ______ + ______) × 物体の色
```

<details><summary>📝 解答</summary>

```
最終色 = (ambient + diffuse + specular) × 物体の色
```

アンビエント（環境光）、ディフューズ（拡散光）、スペキュラー（鏡面光）の3成分を足し合わせた後、物体の色を掛けます。

</details>

### 問2: 内積の計算

法線ベクトルが `(0, 1, 0)`（上向き）、光の方向ベクトルが `(0, 0.707, 0.707)`（斜め上）のとき、ディフューズの強さ `diff` を計算せよ（両ベクトルは正規化済み）。

<details><summary>📝 解答</summary>

```
dot(norm, lightDir) = 0×0 + 1×0.707 + 0×0.707 = 0.707

diff = max(0.707, 0.0) = 0.707
```

光が約45°の角度で当たっているため、最大値(1.0)の約70.7%の明るさになります。`cos(45°) ≈ 0.707` と一致します。

</details>

### 問3: 法線行列

以下のコードの空欄を埋めよ：

```glsl
Normal = mat3(_______(_______(model))) * aNormal;
```

<details><summary>📝 解答</summary>

```glsl
Normal = mat3(transpose(inverse(model))) * aNormal;
```

モデル行列の逆行列の転置行列（= 法線行列）を使い、法線ベクトルを正しくワールド空間に変換します。一様スケーリングのみの場合は `mat3(model)` でも正しい結果が得られますが、非一様スケーリングには対応できません。

</details>

### 問4: reflect 関数

`reflect(-lightDir, norm)` で `lightDir` を反転する理由を説明せよ。

<details><summary>📝 解答</summary>

`lightDir` は「フラグメント位置から光源への方向」として計算します（`lightPos - FragPos`）。一方、`reflect` 関数は「入射方向」（光源からフラグメントへ向かう方向）を期待します。入射方向は `lightDir` の逆向きなので `-lightDir` を渡します。

```
lightDir:   フラグメント → 光源  (使いたい向き)
-lightDir:  光源 → フラグメント  (reflect が期待する入射方向)
```

</details>

### 問5: shininess の効果

shininess を 2 から 256 に変更すると、スペキュラーハイライトの見た目はどう変わるか？

<details><summary>📝 解答</summary>

ハイライトが**非常に小さくて鋭いピンポイント**に変わります。

`pow(x, n)` で n が大きいほど、1.0 に近い値のみが生き残り、少しでも1.0から離れた値は急速に0に近づきます。つまり、反射ベクトルと視線方向がほぼ完全に一致した箇所だけが光って見え、鏡のような質感になります。

</details>

### 問6: クランプの必要性

ディフューズ計算で `max(dot(norm, lightDir), 0.0)` の `max` を省略するとどうなるか？

<details><summary>📝 解答</summary>

光源の反対側にあるフラグメントで内積が負の値になります。負の値がディフューズ成分として使われると、そのフラグメントの色が「引かれて」しまい、不自然に暗い（場合によっては黒より暗い）結果になります。`max` によって負の値を0にクランプすることで、裏側のフラグメントは単にディフューズ寄与がゼロ（= アンビエントだけ）になります。

</details>

---

## 🔨 実践課題

### 課題1: Phong シェーダーの実装 ⭐⭐

上記の完全なPhongフラグメントシェーダーを実装し、サンゴ色の立方体にライティングを適用する。

**チェックポイント：**
- [ ] 立方体の光に面した部分が明るくなる（ディフューズ）
- [ ] 影の部分も完全に黒ではなく、わずかに色がある（アンビエント）
- [ ] 光の反射位置にハイライトが見える（スペキュラー）
- [ ] カメラを動かすとハイライトの位置が変わる

### 課題2: shininess 実験 ⭐⭐

shininess の値を 2, 8, 32, 128, 256 のそれぞれに変更し、ハイライトの変化をスクリーンショットで記録する。

**チェックポイント：**
- [ ] shininess = 2 でぼんやりと広いハイライト
- [ ] shininess = 256 で針のように鋭いハイライト
- [ ] 値の違いによる視覚的な変化を言語化できる

### 課題3: Gouraud シェーディング ⭐⭐⭐

ライティング計算を**フラグメントシェーダー**から**バーテックスシェーダー**に移してみよう（Gouraud Shading）。

**手順：**
1. バーテックスシェーダーでPhong計算を実行し、結果の色を `out` 変数で渡す
2. フラグメントシェーダーは受け取った色をそのまま出力
3. Phong シェーディング（元の実装）と比較する

**チェックポイント：**
- [ ] 立方体の角でハイライトの補間が不自然になる
- [ ] 頂点数が少ないとスペキュラーが「飛んで」しまう現象を確認できる
- [ ] Phong（per-fragment）の方が滑らかな結果になる理由を理解できる

### 課題4: 光源を動かす ⭐⭐

光源の位置を時間とともに立方体の周囲を回るように移動させる。

```cpp
float radius = 2.0f;
float time = glfwGetTime();
lightPos.x = sin(time) * radius;
lightPos.z = cos(time) * radius;
```

**チェックポイント：**
- [ ] 光源（白い立方体）が物体の周りを回転する
- [ ] 物体の明暗が光源の位置に応じてリアルタイムに変化する
- [ ] ハイライト位置がカメラと光源の関係に基づいて移動する

---

## 🔗 ナビゲーション

⬅️ [色](./01-colors.md) | ➡️ [マテリアル →](./03-materials.md)
