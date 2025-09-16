# sample/CMakeLists.txt
cmake_minimum_required(VERSION 3.14)
project(SampleSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

# 假设主框架在 ../build/libsim_core.a 和 ../include
set(FRAMEWORK_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../include)
set(FRAMEWORK_LIB_DIR ${CMAKE_SOURCE_DIR}/../build)

include_directories(${FRAMEWORK_INCLUDE_DIR})
link_directories(${FRAMEWORK_LIB_DIR})

# 查找 nlohmann_json
find_package(nlohmann_json REQUIRED)
include_directories(${nlohmann_json_INCLUDE_DIRS})

# 主可执行文件
add_executable(sample_sim main.cpp)

# 编译插件为 .so
add_library(cpu_cluster_plugin SHARED src/cpu_cluster.cc)
add_library(noc_tile_plugin SHARED src/noc_tile.cc)

# 插件依赖主框架
target_link_libraries(cpu_cluster_plugin sim_core dl)
target_link_libraries(noc_tile_plugin sim_core dl)

# 主程序链接
target_link_libraries(sample_sim sim_core dl)

# 复制配置文件到输出目录（可选）
add_custom_command(TARGET sample_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/configs $<TARGET_FILE_DIR:sample_sim>/configs
)
