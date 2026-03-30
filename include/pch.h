#pragma once

// C++ standard library
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// OpenGL stack
#if __has_include(<glad/glad.h>)
#include <glad/glad.h>
#endif

#if __has_include(<GLFW/glfw3.h>)
#include <GLFW/glfw3.h>
#endif

// GLM
#if __has_include(<glm/glm.hpp>)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#endif
