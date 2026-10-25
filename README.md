# ArvaGate: 3-Gate Automated Irrigation Actuator Firmware

ArvaGate is the dedicated physical gate actuation subsystem within the ArvaCore agricultural irrigation ecosystem.

## Overview
- **Device ID:** `ARVAGATE-CTRL-01`
- **Firmware Version:** `1.0.0`
- **Platform:** ESP32 NodeMCU / Custom Controller
- **Gates:** 3 Independently Controllable Irrigation Gates (Gate 1: Inflow Canal, Gate 2: Mid Lateral, Gate 3: Distal Distribution)

## Pinout and Hardware Configuration

| Component | Pin (GPIO) | Mode | Description |
|---|---|---|---|
| Gate 1 Open Relay | GPIO 26 | Output (Active LOW) | Inflow Canal Opening Motor |
| Gate 1 Close Relay | GPIO 27 | Output (Active LOW) | Inflow Canal Closing Motor |
| Gate 1 Open Limit | GPIO 32 | Input (Pullup) | Physical Fully OPEN limit switch |
| Gate 1 Close Limit | GPIO 33 | Input (Pullup) | Physical Fully CLOSED limit switch |
| Gate 2 Open Relay | GPIO 14 | Output (Active LOW) | Mid Lateral Opening Motor |
| Gate 2 Close Relay | GPIO 12 | Output (Active LOW) | Mid Lateral Closing Motor |
| Gate 2 Open Limit | GPIO 35 | Input (Pullup) | Physical Fully OPEN limit switch |
| Gate 2 Close Limit | GPIO 34 | Input (Pullup) | Physical Fully CLOSED limit switch |
| Gate 3 Open Relay | GPIO 13 | Output (Active LOW) | Distal Distribution Opening Motor |
| Gate 3 Close Relay | GPIO 15 | Output (Active LOW) | Distal Distribution Closing Motor |
| Gate 3 Open Limit | GPIO 39 | Input (Pullup) | Physical Fully OPEN limit switch |
| Gate 3 Close Limit | GPIO 36 | Input (Pullup) | Physical Fully CLOSED limit switch |
| Status LED | GPIO 2 | Output | Heartbeat & Connectivity indicator |

## Security & Architectural Alignment
- Authenticates with ArvaNet Backend using the shared `X-ArvaCore-Gateway-Key` header (`ROLE_GATEWAY`).
- Multi-candidate failover Wi-Fi connectivity identical to ArvaNode and ArvaCore controller.
- Software and electrical interlock: 100ms settling dead-time between opposing relays to prevent motor short circuits.
- Failsafe 20-second travel timeout shutoff if a limit switch fails or wire is severed.
