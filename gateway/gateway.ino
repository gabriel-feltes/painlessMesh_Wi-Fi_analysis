#define BLYNK_TEMPLATE_ID "TMPL2CmlqFtSq"
#define BLYNK_TEMPLATE_NAME "MESH Gateway"
#define BLYNK_AUTH_TOKEN "YOUR TOKEN"

#define INTERNET "SSID"     // DYNAMIC INTERNET CONECTIVITY WI-FI SSID
#define PASSWORD "PASSWORD" // DYNAMIC INTERNET CONECTIVITY WI-FI PASSWORD

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>

const int RSSI_MAX = -50;    // define maximum strength of signal in dBm
const int RSSI_MIN = -100;   // define minimum strength of signal in dBm

const int displayEnc = 1;    // set to 1 to display Encryption or 0 not to display

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = INTERNET;
char pass[] = PASSWORD;

painlessMesh mesh;
#define MESH_PREFIX "meshNetwork"
#define MESH_PASSWORD "meshPassword"
#define MESH_PORT 5555

unsigned long lastBlynkConnectTime = 0;
const unsigned long blynkConnectInterval = 60000;
bool connectedToMesh = true;
unsigned long lastSyncTime = 0;
const unsigned long syncInterval = 20000;

unsigned long lastSendTime = 0; // Variable for timer
unsigned long storedTimeSinceLastSend = 0; // Global variable to store the captured time since last send

String lastRelPressure;
String lastAbsPressure;
String lastTemperature;
String lastHumidity;
String lastLuminosity;
String lastNodeID; // Store the node ID
int nodeMeshNetworkRSSI;
int nodeMeshNetworkChannel;
int nodeInternetRSSI;
int nodeInternetChannel;
bool sendToBlynkFlag = false;

struct NetworkInfo {
  String ssid;
  int channel;
  int rssi;
  int percentage;
  String mac;
  bool hidden;
  String encryption;
};

NetworkInfo networks[50];    // Assuming maximum of 50 networks, adjust as necessary

int meshNetworkRSSI = 0;
int meshNetworkChannel = 0;
int internetRSSI = 0;
int internetChannel = 0;

void getNetworkInfo() {
  String output = "";
  output += "Wifi scan started\n";
  int n = WiFi.scanNetworks();
  output += "Wifi scan ended\n";

  if (n == 0) {
    output += "No networks found\n";
  } else {
    output += String(n) + " networks found\n";

    for (int i = 0; i < n; ++i) {
      networks[i].ssid = WiFi.SSID(i);
      networks[i].channel = WiFi.channel(i);
      networks[i].rssi = WiFi.RSSI(i);
      networks[i].percentage = dBmtoPercentage(WiFi.RSSI(i));
      networks[i].mac = WiFi.BSSIDstr(i);
      networks[i].hidden = WiFi.isHidden(i);
      if (displayEnc) {
        networks[i].encryption = encType(i);
      }

      // Check for meshNetwork and TP-Link
      if (networks[i].ssid == "meshNetwork") {
        meshNetworkRSSI = networks[i].percentage;
        meshNetworkChannel = networks[i].channel;
      } else if (networks[i].ssid == INTERNET) {
        internetRSSI = networks[i].percentage;
        internetChannel = networks[i].channel;
      }

      delay(10);
    }

    // Sort networks based on percentage signal strength
    for (int i = 0; i < n - 1; i++) {
      for (int j = i + 1; j < n; j++) {
        if (networks[i].percentage < networks[j].percentage) {
          NetworkInfo temp = networks[i];
          networks[i] = networks[j];
          networks[j] = temp;
        }
      }
    }

    // Construct the output string with sorted network information
    for (int i = 0; i < n; ++i) {
      output += String(i + 1) + ") ";
      output += networks[i].ssid + " ch:";
      output += String(networks[i].channel) + " ";
      output += String(networks[i].rssi) + "dBm (";
      output += String(networks[i].percentage) + "%)";

      output += " MAC:";
      output += networks[i].mac;

      if (networks[i].hidden) {
        output += " <<Hidden>>";
      }
      if (displayEnc) {
        output += " Encryption:";
        output += networks[i].encryption;
      }

      output += "\n";
    }
  }

  output += "\n";
  delay(5000);
  WiFi.scanDelete();
  Serial.println(output);
}

String encType(int id) {
  String type;
  switch (WiFi.encryptionType(id)) {
    case ENC_TYPE_WEP:
      type = " WEP";
      break;
    case ENC_TYPE_TKIP:
      type = " WPA / PSK";
      break;
    case ENC_TYPE_CCMP:
      type = " WPA2 / PSK";
      break;
    case ENC_TYPE_AUTO:
      type = " WPA / WPA2 / PSK";
      break;
    case ENC_TYPE_NONE:
      type = " <<OPEN>>";
      break;
    default:
      type = " Unknown";
      break;
  }
  return type;
}

int dBmtoPercentage(int dBm) {
  int quality;
  if (dBm <= RSSI_MIN) {
    quality = 0;
  } else if (dBm >= RSSI_MAX) {
    quality = 100;
  } else {
    quality = 2 * (dBm + 100);
  }
  return quality;
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Message received from %u: %s\n", from, msg.c_str());
  
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, msg);

  if (error) {
    Serial.println("Failed to parse JSON, trying CSV format.");
    handleCommaSeparatedValues(from, msg);
    return;
  }

  if (!doc.containsKey("type") || !doc.containsKey("msg")) {
    Serial.println("Invalid JSON format.");
    return;
  }

  int type = doc["type"];
  if (type == 8) {
    String commaSeparatedValues = doc["msg"];
    handleCommaSeparatedValues(from, commaSeparatedValues);
  }
}

void handleCommaSeparatedValues(uint32_t from, String csvValues) {
  String values[9]; // Adjusted to 9 to include the new fields
  int numValues = splitValues(csvValues, values, ',', 9);

  if (numValues == 9) {
    lastNodeID = String(from);
    lastRelPressure = values[0];
    lastAbsPressure = values[1];
    lastTemperature = values[2];
    lastHumidity = values[3];
    lastLuminosity = values[4];
    nodeMeshNetworkRSSI = values[5].toInt();
    nodeMeshNetworkChannel = values[6].toInt();
    nodeInternetRSSI = values[7].toInt();
    nodeInternetChannel = values[8].toInt();
    sendToBlynkFlag = true;

    Serial.println("Data parsed successfully:");
    Serial.println("RelPressure: " + lastRelPressure);
    Serial.println("AbsPressure: " + lastAbsPressure);
    Serial.println("Temperature: " + lastTemperature);
    Serial.println("Humidity: " + lastHumidity);
    Serial.println("Luminosity: " + lastLuminosity);
    Serial.println("Node ID: " + lastNodeID);
    Serial.println("MeshNetwork RSSI: " + String(nodeMeshNetworkRSSI));
    Serial.println("MeshNetwork Channel: " + String(nodeMeshNetworkChannel));
    Serial.println("TP-Link RSSI: " + String(nodeInternetRSSI));
    Serial.println("TP-Link Channel: " + String(nodeInternetChannel));
  } else {
    Serial.println("Incorrect number of CSV values.");
  }
}

int splitValues(String data, String* values, char separator, int maxValues) {
  int index = 0;
  int fromIndex = 0;
  int length = data.length();

  while (index < maxValues && fromIndex < length) {
    int separatorIndex = data.indexOf(separator, fromIndex);
    if (separatorIndex == -1) {
      separatorIndex = length;
    }

    values[index] = data.substring(fromIndex, separatorIndex);
    fromIndex = separatorIndex + 1;
    index++;
  }

  return index;
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New connection, nodeId = %u\n", nodeId);
  // Start the timer here
  lastSendTime = millis();
  Serial.printf("Timer started at: %lu ms\n", lastSendTime);
}

void changedConnectionCallback() {
  Serial.printf("Connections changed\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Time adjusted. Offset = %d\n", offset);
}

void setup() {
  Serial.begin(74880);
  Blynk.begin(auth, ssid, pass);

  mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  WiFi.disconnect();
  connectedToMesh = true;
}

unsigned long lastWifiScanTime = 0;
const unsigned long wifiScanInterval = 30000; // Scan every 30 seconds (adjust as needed)

void loop() {
  unsigned long currentMillis = millis();

  // WiFi scanning at defined intervals
  if (currentMillis - lastWifiScanTime >= wifiScanInterval) {
    lastWifiScanTime = currentMillis;
    getNetworkInfo();
  }

  // Calculate time since last send
  unsigned long timeSinceLastSend = currentMillis - lastSendTime;

  // Print time since last send for debugging and store the value
  Serial.printf("Time since last send: %lu ms\n", timeSinceLastSend);
  storedTimeSinceLastSend = timeSinceLastSend;

  // Sending data to Blynk if flag is set
  if (sendToBlynkFlag) {
    if (connectedToMesh) {
      mesh.stop();
      delay(1000);
    }
    connectToBlynk();
  } else {
    // Regularly update the mesh or Blynk connection
    if (connectedToMesh) {
      mesh.update();
      if (currentMillis - lastBlynkConnectTime >= blynkConnectInterval) {
        connectToBlynk();
      }
    } else {
      Blynk.run();
      if (currentMillis - lastBlynkConnectTime >= 10000) {
        reconnectToMesh();
      }
    }
  }

  delay(10);
}

bool connectToWiFi() {
  WiFi.begin(ssid, pass);
  int wifiTimeout = 30000;
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
    delay(500);
  }

  return WiFi.status() == WL_CONNECTED;
}

void connectToBlynk() {
  // Print the stored time since last send before attempting to connect to Wi-Fi
  Serial.printf("Time since last send before Wi-Fi connect: %lu ms\n", storedTimeSinceLastSend);

  if (connectToWiFi()) {
    Blynk.connect();
    if (Blynk.connected()) {
      connectedToMesh = false;
      lastBlynkConnectTime = millis();

      unsigned long currentMillis = millis(); // Update the current time

      if (lastRelPressure.length() > 0 && lastAbsPressure.length() > 0 && lastTemperature.length() > 0 && lastHumidity.length() > 0 && lastLuminosity.length() > 0) {
        Blynk.virtualWrite(V3, lastRelPressure.toDouble());
        Blynk.virtualWrite(V2, lastAbsPressure.toDouble());
        Blynk.virtualWrite(V0, lastTemperature.toDouble());
        Blynk.virtualWrite(V1, lastHumidity.toDouble());
        Blynk.virtualWrite(V4, lastLuminosity.toDouble());
        Blynk.virtualWrite(V5, storedTimeSinceLastSend); // Use the stored timeSinceLastSend variable

        if (nodeMeshNetworkRSSI != 0 && nodeMeshNetworkChannel != 0 &&
            nodeInternetRSSI != 0 && nodeInternetChannel != 0 &&
            meshNetworkRSSI != 0 && meshNetworkChannel != 0 &&
            internetRSSI != 0 && internetChannel != 0) {
          Blynk.virtualWrite(V6, nodeInternetRSSI); // V6 for last TP-Link RSSI
          Blynk.virtualWrite(V9, internetRSSI); // V9 for current TP-Link RSSI
          
          String nodeNetworkInfo = String(nodeMeshNetworkRSSI) + "%, ch" + nodeMeshNetworkChannel + " / " +
                                  String(nodeInternetRSSI) + "%, ch" + nodeInternetChannel;
          String gatewayNetworkInfo = String(meshNetworkRSSI) + "%, ch" + meshNetworkChannel + " / " +
                                      String(internetRSSI) + "%, ch" + internetChannel;
                                      
          Blynk.virtualWrite(V7, nodeNetworkInfo);
          Blynk.virtualWrite(V8, gatewayNetworkInfo);
        }

        Serial.println("\nData sent to Blynk");
        Serial.println("Node ID: " + lastNodeID + "\n");
        Serial.printf("Timer ended at: %lu ms\n", currentMillis);
        Serial.printf("Time since last send: %lu ms\n", storedTimeSinceLastSend);
      }

      // Update last send time
      lastSendTime = currentMillis;

      sendToBlynkFlag = false;
      reconnectToMesh();
    } else {
      reconnectToMesh();
    }
  } else {
    reconnectToMesh();
  }
}

void reconnectToMesh() {
  Blynk.disconnect();
  WiFi.disconnect();
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  connectedToMesh = true;
}
