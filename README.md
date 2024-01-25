# Quick Assembler

## Simple (experimental) x86/x86 assembler and emulator.

Quick Assembler is built with [wxWidgets](https://www.wxwidgets.org/downloads/). This is my first attempt at using wxWidgets for something real. I have created a private build of wxWidgets that supports static linking of wxWidgets with CRT static linking. If you build it, you may need to change project properties to use dynamic linking with the CRT, as I have not published my tweaks of wxWidgets build.

Here is a list of currently supported features:

* Editing, opening, and saving ASM files, including undo/redo.
* Assemble files with NASM or Keystone engine. For NASM, place `nasm.exe` in the output directory. (NASM can be downloaded from its [official site](https://nasm.us/).
* Execute the assembled code (uses [Unicorn Engine](https://www.unicorn-engine.org/).
* Set breakpoints in the disassembled code.
* Change register values and memory.
* View registers and memory.

## Building

My build setup uses [Vcpkg](https://vcpkg.io/en/) where possible. [Capstone Engine](https://www.capstone-engine.org/) and Uniocrn Engine are included using Vcpkg:

`vcpkg install unicorn:x64-windows`

`vcpkg install unicorn:x64-windows-static`

`vcpkg install capstone[x86]-windows`

The wxWidgets installation folder must be set with an environment variable named `WXWIN`. You can get the files by cloning the [wxWidgets repo](https://github.com/wxWidgets/wxWidgets).

## Screenshots

![image](https://github.com/zodiacon/QuickAsm/assets/4227784/b417f6fa-24cd-42ff-8f10-3469c71a4b09)

