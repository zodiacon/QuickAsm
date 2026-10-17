# Quick Assembler

## Simple (experimental) x86/x64 assembler and emulator.

Quick Assembler is a Windows application built with [WTL](https://sourceforge.net/projects/wtl/) and my [WTLHelper](https://github.com/zodiacon/WTLHelper) library, with light and dark themes.

Here is a list of currently supported features:

* Editing, opening, and saving ASM files, including undo/redo, with syntax highlighting (Scintilla).
* Assemble files with NASM or Keystone engine in 16, 32 or 64 bit mode, at a chosen address. For NASM, place `nasm.exe` in the output directory. NASM can be downloaded from its [official site](https://nasm.us/).
* Execute the assembled code (uses [Unicorn Engine](https://www.unicorn-engine.org/) emulator): run, stop, restart, step into and step over.
* Set breakpoints in the disassembled code.
* View and change register values and memory.
* Light, dark, or same-as-system theme.

## Building

Clone with the WTLHelper submodule:

```cmd
git clone --recursive https://github.com/zodiacon/QuickAsm.git
```

(or run `git submodule update --init` in an existing clone).

Open `QuickAsm.sln` with Visual Studio 2026 and build the x64 configuration. The other libraries come from [Vcpkg](https://vcpkg.io/en/) through its MSBuild integration (`vcpkg integrate install`), linked statically:

```cmd
vcpkg install unicorn capstone[x86] keystone scintilla lexilla wil detours --triplet x64-windows-static
```

## Screenshots

![image](https://github.com/zodiacon/QuickAsm/assets/4227784/b417f6fa-24cd-42ff-8f10-3469c71a4b09)
