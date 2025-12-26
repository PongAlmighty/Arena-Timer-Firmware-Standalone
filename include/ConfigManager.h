#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <IPAddress.h>
#include <LittleFS.h>

struct NetworkConfig {
  bool useDHCP;
  String ip;
  String subnet;
  String gateway;
  String dns;

  // Defaults
  NetworkConfig() {
    useDHCP = true;
    ip = "10.0.0.21";
    subnet = "255.255.255.0";
    gateway = "10.0.0.1";
    dns = "8.8.8.8";
  }
};

class ConfigManager {
public:
  ConfigManager();
  bool begin();

  // Config access
  NetworkConfig getNetworkConfig();
  void setNetworkConfig(const NetworkConfig &config);

  // Persistence
  bool load();
  bool save();

  // Helper to IPAddress
  static IPAddress stringToIP(const String &ipStr);
  static String ipToString(const IPAddress &ip);

private:
  NetworkConfig _networkConfig;
  const char *_configFile = "/config.json";
};

#endif // CONFIG_MANAGER_H
