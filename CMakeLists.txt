cmake_minimum_required(VERSION 3.16)

project(EMU65 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

add_executable(EMU65
    src/main.cpp
    src/aim65.cpp
    include/debug/aiminspector.h
    src/debug/aiminspector.cpp
    src/filehandling/fileprinter.cpp
    src/filehandling/imageloader.cpp
    src/iocomponents/audiotape.cpp
    src/iocomponents/channel.cpp
    src/iocomponents/cpuchannel.cpp
    src/iocomponents/iobus.cpp
    src/iocomponents/iochannel.cpp
    src/iocomponents/iocomponent.cpp
    src/iocomponents/iocomponentirq.cpp
    src/iocomponents/keyboard.cpp
    src/iocomponents/leddisplay.cpp
    src/iocomponents/printer.cpp
    src/iocomponents/unmappedmemoryexception.cpp
    src/M6502/cpu.cpp
    src/M6502/M6502.cpp
    include/ui/aim65controller.h
    src/ui/aim65controller.cpp
    include/ui/aim65proxy.h
    src/ui/aim65proxy.cpp
    include/ui/keyboardqtproxy.h
    src/ui/keyboardqtproxy.cpp
    include/ui/leddisplayqtproxy.h
    src/ui/leddisplayqtproxy.cpp
    src/ui/uiproxy.cpp
    src/ui/uiproxycollection.cpp
)

target_include_directories(EMU65 PRIVATE include)

target_link_libraries(EMU65 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

# The app locates its QML and the emulated ROM image via paths relative to
# its working directory (qml/EMU65/*.qml, res/rom/AIMMON11.BIN), matching the
# original qmake deployment layout. Keep those directories next to the built
# binary so `./EMU65` keeps working when run from the build directory.
add_custom_command(TARGET EMU65 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/qml $<TARGET_FILE_DIR:EMU65>/qml
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:EMU65>/res
    COMMENT "Copying qml/ and res/ next to the EMU65 binary"
)
