# stm32_mebtoroadster

Firmware for a Roadster-style battery message emulator driven by MEB BMS data, with CHaDeMo support.

## High-level architecture

- `MebBms` (CAN1 RX): receives and aggregates MEB cell/module data.
- `RoadsterBmb` (CAN2 TX): emits Roadster sheet-style BMB messages derived from MEB data.
- `ChaDeMo` (CAN1 RX/TX via `CanMap`): handles charger telemetry and publishes CHaDeMo status/request frames.
- Scheduler:
  - 10 ms task: fast data updates and CAN map transmission path.
  - 100 ms task: watchdog, background housekeeping, and CHaDeMo state update.

## CHaDeMo data model

The CHaDeMo-related parameters (`cdm_*`) are filled at runtime:

- `cdm_bat_vtg`: total battery voltage from `MebBms`.
- `cdm_cur_req`: permitted current = minimum of:
  - charger max current (from CHaDeMo charger capabilities frame),
  - battery max charge current (from `MebBms` current limit logic),
  - user limit (`cdmcurlim`).
- `cdm_soc`: computed SoC source described below.
- `cdm_charge_added`: integrated charger output current in ampere-seconds while charging is active.
- `cdm_enabled`: set while SoC is below 100%.
- `cdm_chg_*`: live charger telemetry (max current, output current, output voltage, status).

## SoC strategy

While `cdm_chg_status > 0`, CHaDeMo integrates `cdm_chg_cur` in the 100 ms task and uses the accumulated `cdm_charge_added` value for coulomb-count based SoC tracking.  
If charging remains active but current stays at zero for 3 minutes, SoC falls back to `MebBms::EstimateSocFromVoltage()` and charge integration restarts from zero.  
When charging is not active, all `cdm_*` values are reset to zero.

## Build

Requires `arm-none-eabi` toolchain for firmware build:

```bash
make
```

For quick host-side parameter list validation (no ARM toolchain required):

```bash
g++ -std=c++11 -Iinclude -Ilibopeninv/include -Ilibopencm3/include -c libopeninv/src/params.cpp
```
