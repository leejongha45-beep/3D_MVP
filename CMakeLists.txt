# CMakeList.txt: 3D_MVP에 대한 CMake 프로젝트
cmake_minimum_required (VERSION 3.28)

# 지원되는 경우 MSVC 컴파일러에 대해 핫 다시 로드 사용하도록 설정합니다.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("3D_MVP")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")
option(ENABLE_CPP20_MODULE "Enable Vulkan C++20 Module" OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Stb REQUIRED)

# src/ 하위 소스 자동 수집
file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/*.h"
)

# 이 프로젝트의 실행 파일에 소스를 추가합니다.
add_executable (3D_MVP "3D_MVP.cpp" "3D_MVP.h" ${SRC_FILES})

# src/ 하위 include 경로 자동 수집
file(GLOB_RECURSE SRC_HEADERS "${CMAKE_CURRENT_LIST_DIR}/src/*.h")
set(SRC_INCLUDE_DIRS "")
foreach(HEADER ${SRC_HEADERS})
    get_filename_component(DIR ${HEADER} DIRECTORY)
    list(APPEND SRC_INCLUDE_DIRS ${DIR})
endforeach()
list(REMOVE_DUPLICATES SRC_INCLUDE_DIRS)

target_include_directories(3D_MVP PRIVATE
    ${Stb_INCLUDE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/utils
    ${SRC_INCLUDE_DIRS}
)

target_compile_definitions(3D_MVP PRIVATE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    SHADER_DIR="${CMAKE_CURRENT_LIST_DIR}/shader/"
)
target_link_libraries(3D_MVP PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    tinyobjloader::tinyobjloader
)

set_property(TARGET 3D_MVP PROPERTY CXX_STANDARD 20)

# Slang 셰이더 컴파일
find_program(SLANGC_EXECUTABLE slangc HINTS "$ENV{VULKAN_SDK}/bin")

function (add_slang_shader_target TARGET)
  cmake_parse_arguments ("SHADER" "" "" "SOURCES" ${ARGN})
  set (SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR}/shader)
  set (ENTRY_POINTS -entry main)
  add_custom_command (
          OUTPUT ${SHADERS_DIR}
          COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
  )
  add_custom_command (
          OUTPUT  ${SHADERS_DIR}/slang.spv
          COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o slang.spv
          WORKING_DIRECTORY ${SHADERS_DIR}
          DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
          COMMENT "Compiling Slang Shaders"
          VERBATIM
  )
  add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()

set(SHADER_SLANG_SOURCES ${CMAKE_CURRENT_LIST_DIR}/shader/shader.slang)
add_slang_shader_target(3D_MVPShaders SOURCES ${SHADER_SLANG_SOURCES})
add_dependencies(3D_MVP 3D_MVPShaders)
