# 入門編 10：まとめ・復習ドリル（Review）

> **目標：** 入門編全体を振り返り、知識を体系化する

---

## 入門編の全体像

```
環境構築
  ├── GLFW（ウィンドウ・入力）
  ├── GLAD（関数ローダー）
  └── GLM（数学ライブラリ）

描画の基本
  ├── VAO（頂点属性の記憶）
  ├── VBO（頂点データの転送）
  ├── EBO（インデックスバッファ）
  └── Shader（GLSL で GPU プログラム）

リソース管理
  ├── テクスチャ（stb_image + OpenGL）
  └── シェーダークラス（ファイルから読み込み）

数学と空間
  ├── 変換（Translation/Rotation/Scale）
  ├── 座標系（Local→World→View→Clip→NDC）
  └── MVP 行列（Model × View × Projection）

インタラクション
  ├── カメラ（lookAt + Euler angles）
  └── デルタタイム（FPS 非依存の移動）
```

---

## 総合ドリル

### 問題 1：OpenGL 基礎

次の説明に対応する用語または関数名を答えなさい。

1. 頂点データを GPU に送るバッファ
2. 頂点属性の設定をまとめて記憶するオブジェクト
3. インデックスを使って重複頂点を減らすバッファ
4. OpenGL 関数を動的にロードするためのライブラリ
5. フラグメントの最終的な色を決める GPU プログラム

<details>
<summary> 解答</summary>

1. VBO（Vertex Buffer Object）
2. VAO（Vertex Array Object）
3. EBO（Element Buffer Object）
4. GLAD
5. フラグメントシェーダー

</details>

---

### 問題 2：GLSL 型

以下の値に対応する最適な GLSL 型を答えなさい。

| 値 | 型 |
|----|----|
| 位置 (x, y, z) | 【①】 |
| 色 (r, g, b, a) | 【②】 |
| UV 座標 (u, v) | 【③】 |
| MVP 変換行列 | 【④】 |
| テクスチャサンプラー | 【⑤】 |

<details>
<summary> 解答</summary>

① `vec3`  
② `vec4`  
③ `vec2`  
④ `mat4`  
⑤ `sampler2D`

</details>

---

### 問題 3：MVP 変換の順序

正しい MVP 変換の順序を選びなさい。

- (A) `model * view * projection * aPos`
- (B) `projection * model * view * aPos`
- (C) `projection * view * model * aPos`
- (D) `view * projection * model * aPos`

<details>
<summary> 解答</summary>

**(C)** `projection * view * model * aPos`

</details>

---

### 問題 4：コード穴埋め（総合）

完全なレンダリングループを完成させなさい。

```cpp
while (!glfwWindowShouldClose(window)) {
    // デルタタイムの計算
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - 【 ① 】;
    lastFrame = currentFrame;

    // 入力処理
    【 ② 】(window);

    // 画面クリア（カラーバッファ + 深度バッファ）
    glClear(【 ③ 】 | 【 ④ 】);

    // シェーダー使用
    shader.【 ⑤ 】();

    // MVP 行列をセット
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, (float)glfwGetTime(),
                        glm::vec3(0.5f, 1.0f, 0.0f));
    shader.setMat4("model", 【 ⑥ 】);
    shader.setMat4("view",  camera.【 ⑦ 】());
    shader.setMat4("projection", projection);

    // 描画
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 【 ⑧ 】);

    // バッファ交換
    【 ⑨ 】(window);
    【 ⑩ 】();
}
```

<details>
<summary> 解答</summary>

① `lastFrame`  
② `processInput`  
③ `GL_COLOR_BUFFER_BIT`  
④ `GL_DEPTH_BUFFER_BIT`  
⑤ `use`  
⑥ `model`  
⑦ `GetViewMatrix`  
⑧ `36`（立方体の頂点数）  
⑨ `glfwSwapBuffers`  
⑩ `glfwPollEvents`

</details>

---

### 問題 5：テクスチャ設定の流れ

テクスチャを作成して使用するまでの手順を正しく並べなさい。

```
(A) glBindTexture(GL_TEXTURE_2D, texture)
(B) glGenTextures(1, &texture)
(C) stbi_load("image.jpg", &w, &h, &ch, 0)
(D) glTexImage2D(...)
(E) glGenerateMipmap(GL_TEXTURE_2D)
(F) glTexParameteri(...) でフィルタリング設定
(G) glActiveTexture(GL_TEXTURE0)
(H) shader.setInt("texture1", 0)
```

<details>
<summary> 解答</summary>

**C → B → A → F → D → E → G → A（再バインド）→ H**

または：B → A → F → C → D → E（生成と読み込みの順序は前後しても可）  
→ 描画時: G → A → H

</details>

---

## 最終総合課題：3D シーンを作る

入門編で学んだすべての技術を使って、以下の仕様のシーンを作成しなさい。

### 仕様

| 項目 | 内容 |
|------|------|
| 背景 | 任意の色でクリア |
| オブジェクト | テクスチャを貼った立方体を 5 個以上配置 |
| アニメーション | 各立方体が異なる速度で回転 |
| カメラ | WASD 移動 + マウス回転 |
| テクスチャ | 2 種類以上のテクスチャを使用 |
| 深度テスト | 正しく奥行きが表示される |

### チェックリスト

- [ ] ウィンドウが開く
- [ ] 複数の立方体が表示される
- [ ] テクスチャが正しく貼られている
- [ ] 立方体が回転している
- [ ] WASD でカメラが移動できる
- [ ] マウスでカメラが回転できる
- [ ] 深度テストが有効で奥行きが正しい
- [ ] ESC でアプリが終了する
- [ ] フレームレートに依存しない移動速度

---

## ナビゲーション

 [カメラ](./09-camera.md) | [ライティング編へ →](../03-lighting/01-colors.md)
