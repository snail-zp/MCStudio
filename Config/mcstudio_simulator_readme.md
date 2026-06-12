# MCStudio Simulator Pack

## Buffers

- `Exports/mcstudio_simulator_buffer_23.prg`
  - Common Interface
  - Material Transfer
  - Workstation Calibration actions
- `Exports/mcstudio_simulator_axis_suite.prg`
  - Axis Performance live tests

## Config Files

- `Config/common_interface_simulator.json`
- `Config/workstation_calibration_simulator.json`
- `Config/io_config_simulator.json`
- `Config/axis_performance_test_simulator.json`
- `Config/acs_io_config_simulator.json`

## Suggested Usage

1. Connect the software with `Simulator` mode.
2. Download buffer `23` and buffer `32` into the ACS simulator.
3. In each page, switch to the corresponding `*_simulator.json` config file.
4. Run `Simulator_Reset` once from Common Interface before starting the demo.
