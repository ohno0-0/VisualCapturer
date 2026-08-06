# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.15)

# 设置项目名称和版本
project(Visual-Catcher VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准 (C++11 或更高)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置自动处理 MOC、UIC 和 RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# 根据操作系统设置不同的编译选项
if(WIN32)
    message(STATUS "Building for Windows")
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    
    # Windows 特定设置
    if(MSVC)
        # MSVC 编译器设置
        add_compile_options(/W4 /EHsc)
    else()
        # MinGW 编译器设置
        add_compile_options(-Wall -Wextra)
    endif()
elseif(UNIX AND NOT APPLE)
    message(STATUS "Building for Linux")
    add_compile_options(-Wall -Wextra -pthread)
    
    # Linux 下可能需要设置动态库的运行时路径
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,\$ORIGIN")
endif()

# 设置输出目录结构
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找 Qt
# 可以选择使用 Qt5 或 Qt6，这里以 Qt5 为例
# 如果使用 Qt6，将下面的 5 改为 6
set(QT_VERSION 5 CACHE STRING "Qt version to use (5 or 6)")

if(${QT_VERSION} EQUAL 5)
    find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
    set(QT_LIBRARIES Qt5::Core Qt5::Gui Qt5::Widgets)
elseif(${QT_VERSION} EQUAL 6)
    find_package(Qt6 COMPONENTS Core Gui Widgets  REQUIRED)
    set(QT_LIBRARIES Qt6::Core Qt6::Gui Qt6::Widgets)
else()
    message(FATAL_ERROR "Unsupported Qt version. Please use 5 or 6.")
endif()

# 查找 OpenCV
find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
    message(STATUS "OpenCV library status:")
    message(STATUS "    version: ${OpenCV_VERSION}")
    message(STATUS "    libraries: ${OpenCV_LIBS}")
    message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "OpenCV not found. Please install OpenCV.")
endif()

# 查找 FFmpeg
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
    #libavdevice
    libswresample
)

# 包含项目的头文件目录
# set(HEAD_FILE
#     inc/HPLFFmpegVideoWriter.h
#     inc/HPLFramelessWidget.h
#     inc/HPLVideo.h
#     inc/HPLVideoCompressor.h)
file(GLOB MAIN_HEAD ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
#include_directories(${PROJECT_SOURCE_DIR}/inc)


# 添加可执行文件
# set(SOURCES
#     src/HPLFFmpegVideoWriter.cpp
#     src/HPLFramelessWidget.cpp
#     src/HPLVideo.cpp
#     src/HPLVideoCompressor.cpp
#     src/main.cpp
# )
file(GLOB MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# 创建可执行目标
add_executable(${PROJECT_NAME} ${MAIN_SRC} ${MAIN_HEAD})

# 包含资源文件目录
set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# 定义资源宏（用于代码访问资源路径）
target_compile_definitions(${PROJECT_NAME} PRIVATE
    RESOURCE_DIR="${PROJECT_ROOT_DIR}/Resource/"
)

# 正确设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${OpenCV_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${OpenCV_LIBS} 
    PkgConfig::FFMPEG
    ${QT_LIBRARIES}
)

# 设置 Qt 特定的编译定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
)

# Windows 特定链接
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ws2_32
        winmm
        ole32
        oleaut32
        uuid
        gdi32
        comdlg32
        comctl32
    )
    
    # Windows 下 Qt 插件路径处理
    if(${QT_VERSION} EQUAL 5)
        get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
        execute_process(COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_PLUGINS
            OUTPUT_VARIABLE QT_PLUGINS_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        
        # 复制 Qt 插件
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${QT_PLUGINS_DIR}/platforms"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${QT_PLUGINS_DIR}/imageformats"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/imageformats"
        )
    endif()
endif()

# 在构建后创建必要的目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMENT "Creating necessary directories..."
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/WorkOrder"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/VideoPath"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/PhotoPath"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/logs"
)

# 安装目标
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    CONFIGURATIONS Release
)

# Windows 下安装运行时依赖
if(WIN32)
    # 复制 OpenCV 和 FFmpeg DLL
    install(FILES ${OpenCV_LIBS} DESTINATION bin)
    
    # 复制 Qt DLL
    if(${QT_VERSION} EQUAL 5)
        install(DIRECTORY ${QT_PLUGINS_DIR}/platforms DESTINATION bin/plugins)
        install(DIRECTORY ${QT_PLUGINS_DIR}/imageformats DESTINATION bin/plugins)
    endif()
endif()

# 设置可执行文件输出名称
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "VisualCatcherApp"
    DEBUG_POSTFIX "_d"
    WIN32_EXECUTABLE ON  # Windows 下作为 GUI 应用运行
)

# 打印构建信息
message(STATUS "========================================")
message(STATUS "Project configuration:")
message(STATUS "  Project name: ${PROJECT_NAME}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt version: ${QT_VERSION}")
message(STATUS "  OpenCV version: ${OpenCV_VERSION}")
message(STATUS "  Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "========================================")
