# MCStudio Test Report

Date: 2026-06-06

## Test Method

- Static code review of high-risk UI paths
- Local build and launch verification
- Crash event inspection from Windows Application log
- Runtime log inspection

## Round 1 Results

### Verified

- App can be rebuilt successfully from `compile.bat`
- App can be launched and reach main window state
- No new `APPCRASH` event was found during the latest launch check window
- Common Interface topology widget now uses a real center button for the robot node
- Workstation Calibration robot-arm and chuck modules are arranged on one row

### Fixed During Testing

1. Common Interface topology crash risk

- Problem:
  - Clicking topology nodes triggered in-place widget mutation and config save/reload interactions that were likely causing access violations.
- Fix:
  - Kept topology buttons persistent and updated content in place instead of deleting/recreating them during click flow.
  - Added self-save watcher suppression for Common Interface config writes.

2. IO page log flooding

- Problem:
  - IO auto polling wrote the same refresh summary into the runtime log on every timer tick.
- Fix:
  - Auto refresh now logs only when the summary changes or when refresh is manual.

## Current Known Risks

1. Center robot click still needs direct GUI interaction confirmation

- Code path has been hardened, but a real click in the desktop UI still needs final operator confirmation.

2. IO page may still show repeated unreadable-variable state when controller is disconnected

- This is no longer log-spamming continuously, but the functional state still depends on connection setup.

## Latest Smoke Status

- Build: PASS
- Launch: PASS
- Recent crash events after latest restart: NONE observed in checked window

