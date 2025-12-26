#include "ConfigManager.h"

ConfigManager::ConfigManager() {}

bool ConfigManager::begin() { return LittleFS.begin(); }

NetworkConfig ConfigManager::getNetworkConfig() { return _networkConfig; }

void ConfigManager::setNetworkConfig(const NetworkConfig &config) {
  _networkConfig = config;
}

bool ConfigManager::load() {
  if (!LittleFS.exists(_configFile)) {
    Serial.println("Config file not found, using defaults");
    return false;
  }

  File file = LittleFS.open(_configFile, "r");
  if (!file) {
    Serial.println("Failed to open config file");
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.print("Failed to parse config file: ");
    Serial.println(error.c_str());
    return false;
  }

  // Load Network Config
  if (doc.containsKey("network")) {
    JsonObject network = doc["network"];
    _networkConfig.useDHCP = network["useDHCP"] | true;
    _networkConfig.ip = network["ip"] | "10.0.0.21";
    _networkConfig.subnet = network["subnet"] | "255.255.255.0";
    _networkConfig.gateway = network["gateway"] | "10.0.0.1";
    _networkConfig.dns = network["dns"] | "8.8.8.8";
  }

  Serial.println("Config loaded successfully");
  return true;
}

bool ConfigManager::save() {
  JsonDocument doc;

  // Save Network Config
  JsonObject network = doc["network"].to<JsonObject>();
  network["useDHCP"] = _networkConfig.useDHCP;
  network["ip"] = _networkConfig.ip;
  network["subnet"] = _networkConfig.subnet;
  network["gateway"] = _networkConfig.gateway;
  network["dns"] = _networkConfig.dns;

  File file = LittleFS.open(_configFile, "w");
  if (!file) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write to config file");
    file.close();
    return false;
  }

  file.close();
  Serial.println("Config saved successfully");
  return true;
}

IPAddress ConfigManager::stringToIP(const String &ipStr) {
  IPAddress ip;
  ip.fromString(ipStr);
  return ip;
}

String ConfigManager::ipToString(const IPAddress &ip) {
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." +
         String(ip[3]);
}
