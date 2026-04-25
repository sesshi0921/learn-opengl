/*
Shader.h

シェーダーのコンパイルとリンクを管理するための共通クラス
*/

#include "pch.h"

class Shader {
   public:
    /// @brief シェーダープログラムのID
    unsigned int id{0};

    /// @brief シェーダーを作成し、コンパイルおよびリンクを行う
    /// @param vertexPath 頂点シェーダーのファイルパス
    /// @param fragmentPath フラグメントシェーダーのファイルパス
    /// @details
    /// シェーダーのソースコードをファイルから読み込み、コンパイルしてプログラムオブジェクトを作成します。
    ///          コンパイルやリンクに失敗した場合はエラーメッセージを標準エラー出力に表示します。
    Shader(const char* vertexPath, const char* fragmentPath) {
        // ファイルを読み込む
        std::string vertexCode, fragmentCode;
        std::ifstream vShaderFile, fShaderFile;

        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // コンパイル
        unsigned int vertex, fragment;
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // リンク
        id = glCreateProgram();
        glAttachShader(id, vertex);
        glAttachShader(id, fragment);
        glLinkProgram(id);
        checkCompileErrors(id, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    /// @brief シェーダープログラムを使用する
    void use() {
        glUseProgram(id);
    }

    /// @brief ユニフォーム変数に値を設定する(bool)
    void setBool(const std::string& name, bool v) const {
        glUniform1i(glGetUniformLocation(id, name.c_str()), (int)v);
    }
    /// @brief ユニフォーム変数に値を設定する(int)
    void setInt(const std::string& name, int v) const {
        glUniform1i(glGetUniformLocation(id, name.c_str()), v);
    }
    /// @brief ユニフォーム変数に値を設定する(float)
    void setFloat(const std::string& name, float v) const {
        glUniform1f(glGetUniformLocation(id, name.c_str()), v);
    }
    /// @brief ユニフォーム変数に値を設定する(vec3)
    void setVec3(const std::string& name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(id, name.c_str()), x, y, z);
    }
    /// @brief ユニフォーム変数に値を設定する(vec3)
    void setVec3(const std::string& name, const glm::vec3& v) const {
        glUniform3fv(glGetUniformLocation(id, name.c_str()), 1,
                     glm::value_ptr(v));
    }
    /// @brief ユニフォーム変数に値を設定する(vec4)
    void setVec4(const std::string& name, float x, float y, float z,
                 float w) const {
        glUniform4f(glGetUniformLocation(id, name.c_str()), x, y, z, w);
    }
    /// @brief ユニフォーム変数に値を設定する(mat4)
    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE,
                           glm::value_ptr(mat));
    }

   private:
    void checkCompileErrors(unsigned int shader, std::string type) {
        int success{0};
        static constexpr int LOG_BUF_SIZE{1024};
        char infoLog[LOG_BUF_SIZE];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, LOG_BUF_SIZE, NULL, infoLog);
                std::cerr << "シェーダーコンパイルエラー [" << type << "]:\n"
                          << infoLog << std::endl;
            }
            return;
        }
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, LOG_BUF_SIZE, NULL, infoLog);
            std::cerr << "シェーダーリンクエラー:\n" << infoLog << std::endl;
        }
    }
};