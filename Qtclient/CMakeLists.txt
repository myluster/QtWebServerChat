cmake_minimum_required(VERSION 3.16)

project(Qtclient VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    WebSockets
)

qt_standard_project_setup(REQUIRES 6.8)

# 自动处理.qrc 文件
set(CMAKE_AUTORCC ON)

qt_add_executable(appQtclient
    main.cpp
    resource.qrc
    backend/networkmanager.h backend/networkmanager.cpp
    utils/logger.h utils/logger.cpp
    utils/qmllogger.h utils/qmllogger.cpp
)

set_target_properties(appQtclient PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appQtclient PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::WebSockets
)

include(GNUInstallDirs)
install(TARGETS appQtclient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)