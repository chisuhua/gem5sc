# CMakeLists.txt - 主项目

cmake_minimum_required(VERSION 3.16)  # SystemC 3.0.1 需要较新版本
project(Gem5StyleSim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用调试和 DPRINTF
add_compile_definitions(DEBUG_PRINT)
# 启用调试信息
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG_PRINT")


# -----------------------------
# 引入 SystemC 作为子项目
# -----------------------------
if (NOT TARGET systemc)
    add_subdirectory(systemc-3.0.1 EXCLUDE_FROM_ALL)
endif()

# 如果你想控制 build 类型
set_target_properties(systemc PROPERTIES CXX_STANDARD 17)

# -----------------------------
# 包含目录
# -----------------------------
include_directories(
    include
    systemc-3.0.1/src
)
include_directories(external/json)

# -----------------------------
# 源文件列表
# -----------------------------
#set(SOURCES
#    src/main.cpp
#    include/event_queue.hh
#    include/sim_core.hh
#    include/sim_object.hh
#    include/simple_port.hh
#    include/packet.hh
#    include/modules/cpu_sim.hh
#    include/modules/cache_sim.hh
#    include/modules/stream_modules.hh
#    include/modules/credit_flow.hh
#    include/modules/router.hh
#    include/modules/memory_sim.hh
#)

file(GLOB_RECURSE SOURCES "include/modules/*.hh" "src/main.cpp", "src/module_factory.cpp")

# -----------------------------
# 创建可执行文件并链接 systemc
# -----------------------------
add_executable(sim ${SOURCES})

# 链接 systemc 库（由 add_subdirectory 提供）
target_link_libraries(sim systemc)

# 可选：设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build)
# 可选：启用 sanitizer（调试用）
# target_compile_options(sim PRIVATE -fsanitize=address -fno-omit-frame-pointer)
# target_link_options(sim PRIVATE -fsanitize=address)


