# MCStudio ACS IO Tester

`MCStudio ACS IO Tester` is a Qt/C++ desktop utility for ACS motion controllers.

Current scope:

- Connect to ACS simulator
- Connect to ACS controller over Ethernet TCP or UDP
- Poll digital input port state
- Poll digital output port state
- Toggle digital outputs
- Load IO points from JSON config
- Write runtime logs to `Logs/`

## Project Layout

```text
MCStudio/
├─ CMakeLists.txt
├─ README.md
├─ Config/
│  ├─ acs_io_config.json
│  └─ io_config.json
├─ docs/
│  ├─ qt_io_tester_usage.md
│  └─ software_architecture.md
├─ src/
│  ├─ acscontroller.cpp/.h
│  ├─ filelogger.cpp/.h
│  ├─ ioconfig.cpp/.h
│  ├─ main.cpp
│  └─ mainwindow.cpp/.h
└─ build/
```

## Dependencies

- Qt 6 Widgets
- MSVC 2022 toolchain
- ACS SPiiPlus C library

The current local machine uses:

- Qt: `D:\Qt\6.11.1\msvc2022_64`
- ACS SDK headers/libs:
  `C:\Program Files (x86)\ACS Motion Control\SPiiPlus ADK Suite v3.13.01\ACSC\C_CPP`
- ACS runtime DLLs:
  `C:\Program Files (x86)\ACS Motion Control\SPiiPlus Runtime Kit\Redist`

## Build

Example local build command:

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="D:\Qt\6.11.1\msvc2022_64"
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build
```

## Run

After build, the executable is:

```text
build\MCStudioIoTester.exe
```

Recommended first test flow:

1. Start with `Simulator`
2. Verify DI/DO table refreshes
3. Switch to `Ethernet TCP`
4. Set controller IP and port
5. Update `Config/acs_io_config.json` to your real point list

## Config

The IO point table is driven by:

`Config/acs_io_config.json`

Each point includes:

- `name`
- `direction`
- `port`
- `bit`
- `description`

## Current Limitations

- Digital IO only
- No analog IO yet
- No grouped test workflow yet
- No axis, calibration, or interface test modules yet

## Next Recommended Steps

- Add AI/AO support
- Add batch IO test sequences
- Add signal edge-trigger logging
- Add controller status diagnostics
- Add operator-oriented Chinese UI text once the workflow is stable
