// ============================================================================
// ARVAGATE CONTROLLER CONFIGURATION HEADER
// Module: ArvaGate 3-Gate Irrigation Actuation Controller (ARVAGATE-CTRL-01)
// Architecture: Aligned with ArvaCore Gateway & Actuation Architecture
// ============================================================================

#ifndef ARVAGATE_CONFIG_H
#define ARVAGATE_CONFIG_H

// --- Device Identity ---
#ifndef ARVAGATE_DEVICE_ID
#define ARVAGATE_DEVICE_ID "ARVAGATE-CTRL-01"
#endif

#ifndef ARVAGATE_FIRMWARE_VER
#define ARVAGATE_FIRMWARE_VER "1.0.0"
#endif

// --- Network & Candidate Failover Configuration ---
#ifndef GATEWAY_PRIMARY_WIFI_SSID
#define GATEWAY_PRIMARY_WIFI_SSID "ZTE_2.4G_5gKub"
#endif

#ifndef GATEWAY_PRIMARY_WIFI_PASS
#define GATEWAY_PRIMARY_WIFI_PASS "H0meDLR$2025"
#endif

#ifndef GATEWAY_FALLBACK_WIFI_SSID
#define GATEWAY_FALLBACK_WIFI_SSID "Cream"
#endif

#ifndef GATEWAY_FALLBACK_WIFI_PASS
#define GATEWAY_FALLBACK_WIFI_PASS "hatdog123"
#endif

#ifndef GATEWAY_DIRECT_WIFI_SSID
#define GATEWAY_DIRECT_WIFI_SSID "ARVACORE_GATEWAY"
#endif

#ifndef GATEWAY_DIRECT_WIFI_PASS
#define GATEWAY_DIRECT_WIFI_PASS "arvacore2026"
#endif

// --- Cloud Backend Authentication ---
#ifndef BACKEND_BASE
#define BACKEND_BASE "https://api-aquaflow.gladwindr.me/api/v1"
#endif

#ifndef GATEWAY_KEY
#define GATEWAY_KEY "ed630e681ef735f36954469b48f832f210286246ca95fff62e62086cab74e8a9"
#endif

// --- Hardware Pinouts for 3 Independently Controllable Gates ---
// Relays are Active-LOW optocoupled relay channels
// Motor / Actuator Driver: Dual-relay H-Bridge per gate (OPEN relay & CLOSE relay)
// Gate 1: Inflow Canal (Pitak 1-2)
#define GATE_1_RELAY_OPEN   26
#define GATE_1_RELAY_CLOSE  27
#define GATE_1_LIMIT_OPEN   32
#define GATE_1_LIMIT_CLOSE  33

// Gate 2: Mid-Terrace Lateral (Pitak 3-4)
#define GATE_2_RELAY_OPEN   14
#define GATE_2_RELAY_CLOSE  12
#define GATE_2_LIMIT_OPEN   35
#define GATE_2_LIMIT_CLOSE  34

// Gate 3: Distal Distribution (Pitak 5-6)
#define GATE_3_RELAY_OPEN   13
#define GATE_3_RELAY_CLOSE  15
#define GATE_3_LIMIT_OPEN   39
#define GATE_3_LIMIT_CLOSE  36

#define STATUS_LED_PIN      2

// Active LOW relay definitions
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// Actuation & Timing Limits
#define MAX_GATE_TRAVEL_TIME_MS 20000UL // 20-second travel timeout safety lockout
#define POLL_INTERVAL_MS 3000UL         // Command polling interval (3s)
#define TELEMETRY_INTERVAL_MS 5000UL    // Gate state uplink interval (5s)
#define WIFI_CONNECT_TIMEOUT_MS 15000UL

#endif // ARVAGATE_CONFIG_H

