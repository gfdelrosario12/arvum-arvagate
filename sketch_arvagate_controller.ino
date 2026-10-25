// ============================================================================
// ARVAGATE - 3-GATE IRRIGATION ACTUATION CONTROLLER
// Platform: ESP32 Dev Module / Industrial Relay Enclosure
// Responsibility: Physically actuating Gates 1, 2, and 3 based on backend commands
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#if __has_include("arvagate_config.h")
#include "arvagate_config.h"
#endif

// Candidate Wi-Fi networks (aligned with ArvaNode Gateway Architecture)
struct GatewayCandidate {
  const char* ssid;
  const char* password;
  const char* label;
};

const GatewayCandidate NETWORKS[] = {
  { GATEWAY_PRIMARY_WIFI_SSID, GATEWAY_PRIMARY_WIFI_PASS, "Primary Field AP" },
  { GATEWAY_FALLBACK_WIFI_SSID, GATEWAY_FALLBACK_WIFI_PASS, "Fallback Hotspot" },
  { GATEWAY_DIRECT_WIFI_SSID, GATEWAY_DIRECT_WIFI_PASS, "LilyGO ArvaGate AP" }
};
const size_t NUM_NETWORKS = sizeof(NETWORKS) / sizeof(NETWORKS[0]);

// Gate State Definitions
enum GateOperationalState {
  GATE_STATE_CLOSED,
  GATE_STATE_OPEN,
  GATE_STATE_OPENING,
  GATE_STATE_CLOSING,
  GATE_STATE_STOPPED,
  GATE_STATE_FAULT
};

struct GateChannel {
  uint8_t index;
  uint8_t pinOpen;
  uint8_t pinClose;
  uint8_t pinLimitOpen;
  uint8_t pinLimitClose;
  GateOperationalState state;
  unsigned long actuationStartTime;
  bool isActuating;
  bool physicalConfirmed;
};

GateChannel gates[3] = {
  { 1, GATE_1_RELAY_OPEN, GATE_1_RELAY_CLOSE, GATE_1_LIMIT_OPEN, GATE_1_LIMIT_CLOSE, GATE_STATE_CLOSED, 0, false, false },
  { 2, GATE_2_RELAY_OPEN, GATE_2_RELAY_CLOSE, GATE_2_LIMIT_OPEN, GATE_2_LIMIT_CLOSE, GATE_STATE_CLOSED, 0, false, false },
  { 3, GATE_3_RELAY_OPEN, GATE_3_RELAY_CLOSE, GATE_3_LIMIT_OPEN, GATE_3_LIMIT_CLOSE, GATE_STATE_CLOSED, 0, false, false }
};

unsigned long lastPollTime = 0;
unsigned long lastTelemetryTime = 0;
String lastExecutedCommandId = "";
int currentNetworkIndex = 0;

// Function Declarations
bool connectWiFi();
void pollGateCommands();
void uplinkGateTelemetry();
void sendGateAck(const String& commandId, const String& status, const String& details);
void stopGate(uint8_t gateIndex);
void openGate(uint8_t gateIndex);
void closeGate(uint8_t gateIndex);
String gateStateToString(GateOperationalState state);

// ------------------------------------------------------------
// Setup & Safe State Initialization
// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println(F("\n=============================================="));
  Serial.println(F(" ARVAGATE 3-GATE ACTUATION CONTROLLER v1.0.0"));
  Serial.println(F("=============================================="));

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  // Initialize all gate relay pins to SAFE OFF (Active LOW relays -> HIGH = OFF)
  for (int i = 0; i < 3; i++) {
    pinMode(gates[i].pinOpen, OUTPUT);
    pinMode(gates[i].pinClose, OUTPUT);
    digitalWrite(gates[i].pinOpen, RELAY_OFF);
    digitalWrite(gates[i].pinClose, RELAY_OFF);

    // Limit switch pull-ups
    pinMode(gates[i].pinLimitOpen, INPUT_PULLUP);
    pinMode(gates[i].pinLimitClose, INPUT_PULLUP);
  }

  Serial.println(F("[SAFETY] All Gate Relays Initialized to SAFE DE-ENERGIZED (OFF)"));
  connectWiFi();
}

// ------------------------------------------------------------
// Wi-Fi Connection Failover
// ------------------------------------------------------------
bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  for (size_t i = 0; i < NUM_NETWORKS; i++) {
    int idx = (currentNetworkIndex + i) % NUM_NETWORKS;
    Serial.printf("[NET] Connecting to %s ('%s')...\n", NETWORKS[idx].label, NETWORKS[idx].ssid);
    WiFi.begin(NETWORKS[idx].ssid, NETWORKS[idx].password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
      delay(500);
      Serial.print(F("."));
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      currentNetworkIndex = idx;
      Serial.printf("[NET] Connected! IP: %s | Gateway: %s | RSSI: %d dBm\n",
                    WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str(), WiFi.RSSI());
      return true;
    }
  }
  return false;
}

// ------------------------------------------------------------
// Gate Actuation Functions
// ------------------------------------------------------------
void openGate(uint8_t gateIndex) {
  if (gateIndex < 1 || gateIndex > 3) return;
  GateChannel &g = gates[gateIndex - 1];

  // Interlock: Ensure CLOSE relay is OFF before engaging OPEN relay
  digitalWrite(g.pinClose, RELAY_OFF);
  delay(100);
  digitalWrite(g.pinOpen, RELAY_ON);

  g.state = GATE_STATE_OPENING;
  g.actuationStartTime = millis();
  g.isActuating = true;
  g.physicalConfirmed = false;
  digitalWrite(STATUS_LED_PIN, HIGH);
  Serial.printf("[ARVAGATE] Gate %d: Actuation OPEN started.\n", gateIndex);
}

void closeGate(uint8_t gateIndex) {
  if (gateIndex < 1 || gateIndex > 3) return;
  GateChannel &g = gates[gateIndex - 1];

  // Interlock: Ensure OPEN relay is OFF before engaging CLOSE relay
  digitalWrite(g.pinOpen, RELAY_OFF);
  delay(100);
  digitalWrite(g.pinClose, RELAY_ON);

  g.state = GATE_STATE_CLOSING;
  g.actuationStartTime = millis();
  g.isActuating = true;
  g.physicalConfirmed = false;
  digitalWrite(STATUS_LED_PIN, HIGH);
  Serial.printf("[ARVAGATE] Gate %d: Actuation CLOSE started.\n", gateIndex);
}

void stopGate(uint8_t gateIndex) {
  if (gateIndex < 1 || gateIndex > 3) return;
  GateChannel &g = gates[gateIndex - 1];

  digitalWrite(g.pinOpen, RELAY_OFF);
  digitalWrite(g.pinClose, RELAY_OFF);

  g.state = GATE_STATE_STOPPED;
  g.isActuating = false;
  g.actuationStartTime = 0;
  Serial.printf("[ARVAGATE] Gate %d: Actuator STOPPED.\n", gateIndex);
}

// ------------------------------------------------------------
// Monitor Physical Limits and Travel Timeouts
// ------------------------------------------------------------
void checkActuationSafety() {
  unsigned long now = millis();
  bool anyActive = false;

  for (int i = 0; i < 3; i++) {
    GateChannel &g = gates[i];
    if (g.isActuating) {
      anyActive = true;

      // 1. Check physical limit switch feedback (Active LOW when pressed)
      if (g.state == GATE_STATE_OPENING && digitalRead(g.pinLimitOpen) == LOW) {
        stopGate(g.index);
        g.state = GATE_STATE_OPEN;
        g.physicalConfirmed = true;
        Serial.printf("[FEEDBACK] Gate %d: Physical OPEN limit reached!\n", g.index);
      } else if (g.state == GATE_STATE_CLOSING && digitalRead(g.pinLimitClose) == LOW) {
        stopGate(g.index);
        g.state = GATE_STATE_CLOSED;
        g.physicalConfirmed = true;
        Serial.printf("[FEEDBACK] Gate %d: Physical CLOSED limit reached!\n", g.index);
      }

      // 2. Timeout Safety Lockout: Prevent motor burn if limit switch is missing/untriggered
      if (g.isActuating && (now - g.actuationStartTime >= MAX_GATE_TRAVEL_TIME_MS)) {
        stopGate(g.index);
        g.state = (g.state == GATE_STATE_OPENING) ? GATE_STATE_OPEN : GATE_STATE_CLOSED;
        g.physicalConfirmed = false; // Unconfirmed since stopped by timeout
        Serial.printf("[SAFETY TIMEOUT] Gate %d travel timeout reached. Motor shut off.\n", g.index);
      }
    }
  }

  if (!anyActive) {
    digitalWrite(STATUS_LED_PIN, LOW);
  }
}

// ------------------------------------------------------------
// Backend Communication (Polling & Uplink)
// ------------------------------------------------------------
String extractField(const String &json, const String &fieldName) {
  int keyIndex = json.indexOf("\"" + fieldName + "\":");
  if (keyIndex < 0) return "";
  int valStart = keyIndex + fieldName.length() + 3;
  while (valStart < json.length() && (json[valStart] == ' ' || json[valStart] == '\"')) valStart++;
  int valEnd = valStart;
  while (valEnd < json.length() && json[valEnd] != '\"' && json[valEnd] != ',' && json[valEnd] != '}') valEnd++;
  return json.substring(valStart, valEnd);
}

void pollGateCommands() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url = String(BACKEND_BASE) + "/gates/pending-command/" + String(ARVAGATE_DEVICE_ID);

  if (!http.begin(client, url)) return;
  http.addHeader("Accept", "application/json");
  http.addHeader("X-ArvaCore-Gateway-Key", GATEWAY_KEY);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    String commandId = extractField(payload, "commandId");
    String action = extractField(payload, "action");
    String targetGatesStr = extractField(payload, "targetGates");

    if (commandId.length() > 0 && action.length() > 0 && commandId != lastExecutedCommandId) {
      Serial.printf("[CMD] Incoming Command: %s | Action: %s | Targets: %s\n",
                    commandId.c_str(), action.c_str(), targetGatesStr.c_str());

      lastExecutedCommandId = commandId;
      sendGateAck(commandId, "ACCEPTED", "Command accepted for execution");

      // Execute on target gates
      for (int i = 1; i <= 3; i++) {
        String gateNum = String(i);
        if (targetGatesStr.indexOf(gateNum) >= 0 || targetGatesStr.equalsIgnoreCase("ALL")) {
          if (action.equalsIgnoreCase("OPEN")) {
            openGate(i);
          } else if (action.equalsIgnoreCase("CLOSE")) {
            closeGate(i);
          } else if (action.equalsIgnoreCase("STOP")) {
            stopGate(i);
          }
        }
      }

      sendGateAck(commandId, "EXECUTED", "Gates actuation triggered successfully");
    }
  }
  http.end();
}

void sendGateAck(const String& commandId, const String& status, const String& details) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url = String(BACKEND_BASE) + "/gates/ack/" + commandId;

  if (!http.begin(client, url)) return;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-ArvaCore-Gateway-Key", GATEWAY_KEY);

  String json = "{";
  json += "\"deviceId\":\"" + String(ARVAGATE_DEVICE_ID) + "\",";
  json += "\"status\":\"" + status + "\",";
  json += "\"details\":\"" + details + "\",";
  json += "\"gate1State\":\"" + gateStateToString(gates[0].state) + "\",";
  json += "\"gate2State\":\"" + gateStateToString(gates[1].state) + "\",";
  json += "\"gate3State\":\"" + gateStateToString(gates[2].state) + "\"";
  json += "}";

  http.POST(json);
  http.end();
}

void uplinkGateTelemetry() {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url = String(BACKEND_BASE) + "/gates/telemetry/" + String(ARVAGATE_DEVICE_ID);

  if (!http.begin(client, url)) return;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-ArvaCore-Gateway-Key", GATEWAY_KEY);

  String json = "{";
  json += "\"deviceId\":\"" + String(ARVAGATE_DEVICE_ID) + "\",";
  json += "\"gates\":[";
  for (int i = 0; i < 3; i++) {
    json += "{";
    json += "\"gateIndex\":" + String(gates[i].index) + ",";
    json += "\"state\":\"" + gateStateToString(gates[i].state) + "\",";
    json += "\"isActuating\":" + String(gates[i].isActuating ? "true" : "false") + ",";
    json += "\"physicalConfirmed\":" + String(gates[i].physicalConfirmed ? "true" : "false");
    json += "}";
    if (i < 2) json += ",";
  }
  json += "]}";

  http.POST(json);
  http.end();
}

String gateStateToString(GateOperationalState state) {
  switch (state) {
    case GATE_STATE_OPEN: return "OPEN";
    case GATE_STATE_CLOSED: return "CLOSED";
    case GATE_STATE_OPENING: return "OPENING";
    case GATE_STATE_CLOSING: return "CLOSING";
    case GATE_STATE_STOPPED: return "STOPPED";
    default: return "FAULT";
  }
}

// ------------------------------------------------------------
// Main Loop
// ------------------------------------------------------------
void loop() {
  unsigned long now = millis();

  // 1. Check physical limits & timeout protection
  checkActuationSafety();

  // 2. Poll Backend for pending Gate commands
  if (now - lastPollTime >= POLL_INTERVAL_MS) {
    lastPollTime = now;
    pollGateCommands();
  }

  // 3. Uplink Gate telemetry status
  if (now - lastTelemetryTime >= TELEMETRY_INTERVAL_MS) {
    lastTelemetryTime = now;
    uplinkGateTelemetry();
  }
}

