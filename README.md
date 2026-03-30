# learn-opengl

↓↓↓ [learnopengl](https://learnopengl.com/) を自分用にカスタマイズしました！ ↓↓↓

### [https://sesshi0921.github.io/learn-opengl](https://sesshi0921.github.io/learn-opengl/#/)

## 概要
[learnopengl](https://learnopengl.com/) の自己学習用リポジトリ

## セットアップと実行

1. リポジトリのクローン
    ```bash
    git clone https://github.com/sesshi0921/learn-opengl.git
    ```
2. クローンしたリポジトリの階層に移動
    ```bash
    cd learn-opengl
    ```
3. CMakeでビルド環境を構築
    ```bash
    cmake -S . -B build
    ```
4. CMakeビルドを実行
    ```bash
    cmake --build build -j
    ```
5. ビルド生成ファイルを実行

    ビルド後、`build/` 配下に実行ファイルができます。
