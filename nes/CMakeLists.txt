# NES/CMakeLists.txt

# 设置 C 和 C++ 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# NES 核心 C 源文件（所有 .c 文件）
file(GLOB NES_CORE_SOURCES CONFIGURE_DEPENDS "*.c")

# 显式指定要编译的 Mapper 列表（避免编译未完成或出错的 Mapper）
set(MAPPER_SOURCES
        mapper/000.cpp
        mapper/001.cpp
        mapper/002.cpp
        mapper/003.cpp
        mapper/004.cpp
        mapper/006.cpp
        mapper/007.cpp
        mapper/008.cpp
        mapper/011.cpp
        mapper/013.cpp
        mapper/015.cpp
        mapper/016.cpp
        mapper/017.cpp
        mapper/018.cpp
        mapper/019.cpp
        mapper/021.cpp
        mapper/022.cpp
        mapper/023.cpp
        mapper/024.cpp
        mapper/025.cpp
        mapper/032.cpp
        mapper/033.cpp
        mapper/034.cpp
        mapper/064.cpp
        mapper/065.cpp
        mapper/066.cpp
        mapper/067.cpp
        mapper/069.cpp
        mapper/070.cpp
        mapper/071.cpp
        mapper/072.cpp
        mapper/073.cpp
        mapper/075.cpp
        mapper/076.cpp
        mapper/078.cpp
        mapper/079.cpp
        mapper/085.cpp
        mapper/087.cpp
        mapper/088.cpp
        mapper/099.cpp
        mapper/113.cpp
        mapper/189.cpp
        mapper/225.cpp
        mapper/227.cpp
        mapper/240.cpp
        mapper/245.cpp
)

# 创建静态库 NES
add_library(NES ${NES_CORE_SOURCES} ${MAPPER_SOURCES})

# 链接 devices 静态库
add_subdirectory(../devices ../build/devices)
target_link_libraries(NES PRIVATE nes_devices)

# 设置包含目录
target_include_directories(NES
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../devices>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        mapper
)

# Add Windows specific libraries if on Windows
if (WIN32)
    target_link_libraries(NES PRIVATE user32 gdi32 winmm)
endif ()