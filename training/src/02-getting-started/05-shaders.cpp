#include <include/Shader.h>
#include <include/pch.h>
#include <include/utils.h>

namespace {
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
}  // namespace

int main() {
    // GLFW 初期化
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // ウィンドウ作成
    GLFWwindow* window =
        glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Shader Class", NULL, NULL);
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

    // シェーダーオブジェクトの作成
    Shader shader("training/src/02-getting-started/05-shaders.vert",
                  "training/src/02-getting-started/05-shaders.frag");

    // 課題1：虹色三角形
    // 3頂点に異なる色（赤・緑・青）を設定して、グラデーション三角形を表示する
    // 頂点データ：位置(xyz) + 色(rgb)
    float vertices[] = {
        0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,  // 右下：赤
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // 左下：緑
        0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f,  // 上　：青
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 属性0：位置（offset=0）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
    // 属性1：色（offset=12）
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    float xoffset{0.0f};
    float yoffset{0.0f};

    // レンダリングループ
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        // 課題4：時間に応じてoffsetを変化させ、三角形を上下に揺らす
        xoffset = static_cast<float>(cos(glfwGetTime())) * 0.5f;
        yoffset = static_cast<float>(sin(glfwGetTime())) * 0.5f;
        shader.setFloat("xOffset", xoffset);
        shader.setFloat("yOffset", yoffset);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}