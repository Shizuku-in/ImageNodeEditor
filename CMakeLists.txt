cmake_minimum_required(VERSION 3.20)

project(ImageNodeEditor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(CTest)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core Test LinguistTools)

set(TS_FILES
    translations/ImageNodeEditor_zh_CN.ts
)

set(COMMON_SOURCES
    src/core/Node.cpp
    src/core/NodeFactory.cpp
    src/core/WorkflowGraph.cpp
    src/core/WorkflowExecutor.cpp
    src/imageops/ImageOps.cpp
    src/i18n/LanguageManager.cpp
    src/nodes/ImageNodes.cpp
    src/io/WorkflowSerializer.cpp
)

set(GUI_SOURCES
    src/gui/ParameterEditorWidget.cpp
    src/gui/GraphScene.cpp
    src/gui/MainWindow.cpp
    src/gui/ParameterPanel.cpp
)

add_executable(ImageNodeEditor
    src/main.cpp
    ${COMMON_SOURCES}
    ${GUI_SOURCES}
)

target_include_directories(ImageNodeEditor PRIVATE src)
target_link_libraries(ImageNodeEditor PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core)

qt_add_translation(QM_FILES ${TS_FILES})
qt_add_lupdate(ImageNodeEditor TS_FILES ${TS_FILES})
add_custom_target(ImageNodeEditorTranslations ALL DEPENDS ${QM_FILES})
add_dependencies(ImageNodeEditor ImageNodeEditorTranslations)
add_custom_command(TARGET ImageNodeEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ImageNodeEditor>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QM_FILES} "$<TARGET_FILE_DIR:ImageNodeEditor>/translations"
)

if (BUILD_TESTING)
    add_executable(ImageNodeEditorTests
        tests/core_tests.cpp
        ${COMMON_SOURCES}
    )
    target_include_directories(ImageNodeEditorTests PRIVATE src)
    target_link_libraries(ImageNodeEditorTests PRIVATE Qt6::Test Qt6::Gui Qt6::Core)
    add_test(NAME ImageNodeEditorTests COMMAND ImageNodeEditorTests)
endif()

if (MSVC)
    target_compile_options(ImageNodeEditor PRIVATE /W4)
    if (BUILD_TESTING)
        target_compile_options(ImageNodeEditorTests PRIVATE /W4)
    endif()
else()
    target_compile_options(ImageNodeEditor PRIVATE -Wall -Wextra -Wpedantic)
    if (BUILD_TESTING)
        target_compile_options(ImageNodeEditorTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
