#include <include/Shader.h>
#include <include/pch.h>
#include <include/utils.h>

// テクスチャ画像の読み込み
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace {
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// clang-format off
constexpr float vertices[] = {
    // 位置(xyz)      // UV
     0.5f,  0.5f,  0.0f,  1.0f, 1.0f,  // 右上
     0.5f, -0.5f,  0.0f,  1.0f, 0.0f,  // 右下
    -0.5f, -0.5f,  0.0f,  0.0f, 0.0f,  // 左下
    -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,  // 左上
};
// clang-format on
}  // namespace

int main() {
    // GLFW 初期化
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // ウィンドウ作成
    GLFWwindow* window =
        glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Textures", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // GLAD 初期化
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD 初期化失敗" << std::endl;
        return -1;
    }

    // アルファブレンディング有効化
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int texture{0};
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // テクスチャの折り返し設定
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // テクスチャフィルタリング
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // テクスチャ画像の読み込み
    int width, height, nrChannels, alphaChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load("training/textures/dack-removebg.png",
                                    &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                     GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    // シェーダーオブジェクトの作成
    Shader shader("training/src/02-getting-started/06-textures.vert",
                  "training/src/02-getting-started/06-textures.frag");

    // インデックスデータ（四角形を三角形2枚で描く）
    // clang-format off
    constexpr unsigned int indices[] = {
        0, 1, 3,  // 1枚目
        1, 2, 3,  // 2枚目
    };
    // clang-format on

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    // 属性0：位置 (xyz)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
    // 属性1：UV (st)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // サンプラーとテクスチャユニット0を紐付け
    shader.use();
    shader.setInt("ourTexture", 0);

    float xOffset{0};
    float yOffset{0};

    // レンダリングループ
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();

        // 上下キーで動くかな
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            yOffset = std::min(yOffset + 0.01f, 1.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            yOffset = std::max(yOffset - 0.01f, -1.0f);
        }
        // 左右キーで動くかな
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            xOffset = std::min(xOffset + 0.01f, 1.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            xOffset = std::max(xOffset - 0.01f, -1.0f);
        }
        // セット
        shader.setFloat("xOffset", xOffset);
        shader.setFloat("yOffset", yOffset);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &texture);
    glfwTerminate();
    return 0;
}