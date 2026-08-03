cmake_minimum_required(VERSION 3.19)
project(AA-Tool LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)

qt_standard_project_setup()

qt_add_executable(AA-Tool
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    settingsdialog.h settingsdialog.cpp settingsdialog.ui
    nbtreader.h nbtreader.cpp
)

qt_add_translations(
    TARGETS AA-Tool
    TS_FILES AA-Tool_en_US.ts
)

target_link_libraries(AA-Tool
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Widgets z
)

install(TARGETS AA-Tool
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET AA-Tool
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
