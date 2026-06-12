# MCStudio Manual Test Cases

## Scope

This checklist targets the current desktop modules:

- Common Interface
- IO Monitor
- Workstation Calibration
- Axis Performance
- App startup, configuration reload, and crash recovery

## Environment

- Build: `build/MCStudioIoTester.exe`
- Main configs:
  - `Config/common_interface.json`
  - `Config/io_config.json`
  - `Config/workstation_calibration.json`
  - `Config/axis_performance_test.json`
- Simulator configs:
  - `Config/common_interface_simulator.json`
  - `Config/io_config_simulator.json`
  - `Config/workstation_calibration_simulator.json`
  - `Config/axis_performance_test_simulator.json`

## Startup And Stability

### TC-001 App launch

- Precondition: app not running
- Steps:
  1. Build the app
  2. Launch `MCStudioIoTester.exe`
  3. Wait 10 seconds
- Expected:
  - Main window title shows `MCStudio`
  - Process remains responsive
  - No `Application Error` or `APPCRASH` event is written

### TC-002 Reopen after abnormal exit

- Steps:
  1. Launch app
  2. Force-close process
  3. Relaunch app
- Expected:
  - App restarts successfully
  - No config corruption

## Common Interface

### TC-101 Load topology

- Steps:
  1. Open Common Interface page
  2. Verify material transfer panel renders
- Expected:
  - Robot node appears in center
  - Station nodes appear around robot
  - Current material location is highlighted

### TC-102 Click station node

- Steps:
  1. Click one station node
- Expected:
  - Configured transfer command executes
  - Material location label updates
  - Highlight moves to the clicked station
  - App does not crash

### TC-103 Click center robot node

- Steps:
  1. Click center robot node
- Expected:
  - Robot node receives click
  - If current location is a station and `pickupCommand` exists, pickup command executes
  - Material location becomes robot
  - App does not crash

### TC-104 Save common interface config in editor

- Steps:
  1. Open Common Interface settings
  2. Edit robot/station fields
  3. Save config
- Expected:
  - Save succeeds
  - Main page refreshes correctly
  - No duplicate hot reload or crash

### TC-105 External config hot reload

- Steps:
  1. Keep Common Interface page open
  2. Edit `common_interface.json` externally
  3. Save file
- Expected:
  - UI reloads updated content
  - No stale path issue
  - No crash during file replace

## IO Monitor

### TC-201 Load IO page

- Steps:
  1. Open IO page
- Expected:
  - IO groups render by module
  - DI/DO/AI/AO controls render correctly

### TC-202 Auto refresh stability

- Steps:
  1. Keep IO page open for 2 minutes
- Expected:
  - App remains responsive
  - Runtime log is not flooded with identical refresh lines

### TC-203 Manual refresh

- Steps:
  1. Trigger manual refresh
- Expected:
  - Summary is shown once
  - Failures are readable

## Workstation Calibration

### TC-301 Main layout

- Steps:
  1. Open Workstation Calibration page
- Expected:
  - Robot-arm module and chuck module are displayed on one row
  - No unexpected wrapping on normal window width

### TC-302 Start position action

- Steps:
  1. Run move-to-start-position action
- Expected:
  - Done variable is reset then waited
  - UI remains responsive

### TC-303 Jog controls

- Steps:
  1. Jog each axis positive and negative
- Expected:
  - Controls map to correct axis
  - Position refresh updates values

## Axis Performance

### TC-401 Open page

- Steps:
  1. Open Axis Performance page
- Expected:
  - Config loads
  - Axis/test selectors populate

### TC-402 Report chart axis labels

- Steps:
  1. Load or import test data
  2. Switch between test types
- Expected:
  - X-axis meaning matches test type
  - Axis labels include variable and unit

### TC-403 Export report

- Steps:
  1. Export report
- Expected:
  - Export succeeds
  - Curves match on-screen rendering

## Regression Focus

- Common Interface center robot click
- Common Interface config save and file watcher interaction
- IO log spam during auto refresh
- Workstation Calibration two-module row layout
- Axis Performance chart axes and export consistency
