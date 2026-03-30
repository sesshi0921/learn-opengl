cmake_minimum_required(VERSION 3.21)

project(learn_opengl LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
find_package(OpenGL REQUIRED)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# glm
FetchContent_Declare(
	glm
	GIT_REPOSITORY https://github.com/g-truc/glm.git
	GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# glad (OpenGL 3.3 core)
FetchContent_Declare(
	glad
	GIT_REPOSITORY https://github.com/JoeyDeVries/LearnOpenGL.git
	GIT_TAG master
	GIT_SHALLOW TRUE
)
FetchContent_GetProperties(glad)
if(NOT glad_POPULATED)
	FetchContent_Populate(glad)
endif()

add_library(glad STATIC "${glad_SOURCE_DIR}/src/glad.c")
target_include_directories(glad PUBLIC "${glad_SOURCE_DIR}/includes")

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

if(NOT APP_SOURCES)
	message(FATAL_ERROR "No .cpp files found under src/")
endif()

foreach(src_file IN LISTS APP_SOURCES)
	file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}/src" "${src_file}")
	string(REPLACE "/" "_" target_name "${rel_path}")
	string(REPLACE ".cpp" "" target_name "${target_name}")

	add_executable("${target_name}" "${src_file}")
	target_include_directories("${target_name}" PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
	)
	target_link_libraries("${target_name}" PRIVATE glfw glad glm::glm OpenGL::GL)
endforeach()
