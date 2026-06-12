# MCStudio

MCStudio is a Qt/C++ desktop utility for ACS motion-control workstations.

Current scope:

- Connect to ACS simulator.
- Connect to ACS controller over Ethernet TCP or UDP.
- Common Interface command execution.
- Material-transfer topology and local refresh.
- IO test and polling.
- Workstation calibration.
- Axis performance tests and reports.
- System logs.
- Runtime logs written to `Logs/`.

## Project Layout

```text
MCStudio/
├─ CMakeLists.txt
├─ README.md
├─ Config/
├─ docs/
├─ Exports/
├─ Image/
├─ resources/
├─ src/
├─ tools/
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

Existing local build folder can also be rebuilt with:

```bat
cd /d C:\Users\22841\Desktop\MCStudio\build
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
ninja
```

## Run

After build, the executable is:

```text
build\MCStudioIoTester.exe
```

Recommended first test flow:

1. Start with `Simulator`.
2. Verify default page is `常用接口`.
3. Click material-transfer buttons and confirm only local topology/status updates.
4. Switch to IO, workstation calibration, and axis performance pages as needed.
5. Update `Config/*.json` for real controller deployments.

## Documentation

Latest handoff:

- [docs/handoff_20260613.md](docs/handoff_20260613.md)

Architecture and structure:

- [docs/software_architecture.md](docs/software_architecture.md)
- [docs/software_structure_diagram.md](docs/software_structure_diagram.md)

