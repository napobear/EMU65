EMU65
=====

EMU65 is a Qt/QML emulator of the Rockwell AIM 65, a single-board 6502-based
computer. It emulates the 6502 CPU, the on-board RAM, the Monitor ROM, the
keyboard, the LED display, the printer and the audio cassette interface, and
presents them through a QtQuick UI.


Project layout
--------------

CMakeLists.txt     Build definition (CMake + Qt6). See "Building" below.
include/            Project headers, mirroring the layout of src/.
src/                 Application sources.
  M6502/              The 6502 CPU core (originally by Marat Fayzullin) and
                       its C++ facade (Cpu, cpu.h/.cpp).
  iocomponents/       Emulated AIM 65 peripherals (RAM, keyboard, LED
                       display, printer, audio tape) and the IOBus/IOChannel
                       infrastructure that routes CPU memory accesses to
                       them.
  filehandling/       Loading the ROM image and writing printer output.
  ui/                  Qt-facing proxy classes exposed to QML.
  debug/               AimInspector, a debug/inspector panel for the
                       emulator's internal state (built only when
                       EMU65_DEBUG is defined).
qml/EMU65/           QML UI: the main window (main.qml) and the debug
                       inspector window (aiminspector.qml).
res/                 Resources: UI images (res/img) and the AIM 65 Monitor
                       ROM image (res/rom/AIMMON11.BIN).
doc/                 Doxygen configuration and generated API documentation.
LICENCE.txt          License terms (see "License" below).


Requirements
------------

- CMake 3.16 or newer
- A C++17 compiler (tested with GCC)
- Qt 6, with the following modules: Core, Gui, Qml, Quick, QuickControls2

On openSUSE (Tumbleweed and similar), this is typically:

    sudo zypper install cmake ninja gcc-c++ \
        qt6-base-devel qt6-declarative-devel

On Debian/Ubuntu-based distributions:

    sudo apt install cmake ninja-build g++ \
        qt6-base-dev qt6-declarative-dev qml6-module-qtquick-controls


Building
--------

From the repository root:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build

If CMake cannot find your Qt 6 installation automatically, point it at the
Qt 6 CMake package directory, e.g.:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH=/usr/lib64/qt6

Any CMake generator works (drop `-G Ninja` to use the default one, e.g.
Unix Makefiles).

The build copies qml/ and res/ next to the produced binary, since the
application locates its QML files and the ROM image via paths relative to
its working directory.


Running
-------

From the build directory:

    ./EMU65

On a system with no display available (e.g. a headless environment), Qt's
offscreen platform plugin can be used to start the application without a
window:

    QT_QPA_PLATFORM=offscreen ./EMU65


License
-------

The EMU65 project is licensed under the MIT License. The 6502 CPU core
(include/M6502/Codes.h, include/M6502/M6502.h, include/M6502/Tables.h,
src/M6502/M6502.cpp), originally written by Marat Fayzullin, is distributed
under separate terms. See LICENCE.txt for the full text of both.
