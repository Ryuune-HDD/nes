cmake_minimum_required(VERSION 4.0)
project(nes)

set(CMAKE_CXX_STANDARD 20)

# 添加NES子目录
add_subdirectory(NES)

# 生成compile_commands.json文件
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 创建主可执行文件
add_executable(nes main.c)

# 链接NES库
target_link_libraries(nes PRIVATE NES)
