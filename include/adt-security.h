#pragma once
#include "forward_declarations.h"

#include "clock.h"
#include "jsonloader.h"
#include "ReconnectingMqttClient.h"
#include "zone.h"
#include "gpio.h"
#include "sha1.h"

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <functional>
#include <sstream>
#include <atomic>

// publish device discovery to: "homeassistant/device/adtcs/config"
// subscribe to birth/will message: "homeassistant/status"

using MQTTCallback = std::function<void(const std::string&, const std::string&)>;

class SecuritySystem {
    std::map<std::string, std::string> cpuinfo;
    JsonLoader config;

    ReconnectingMqttClient* mqtt;
    ZoneManager* zone_manager;

    std::atomic_bool system_online;
    std::string system_name;
    std::string system_uptime_name;
    std::string system_version_json;
    size_t auto_refresh_timer;

    const std::string serial_number;

    //      ------- MQTT topics -------
    std::string mqtt_topic_version_info;
    std::string mqtt_topic_device_discovery;
    std::string mqtt_topic_homeassistant_status;
    std::string mqtt_topic_entity_state;
    std::string mqtt_topic_entity_update;
    std::string mqtt_topic_system_status;
    std::string mqtt_topic_system_command;
    std::string mqtt_will_payload;
    std::string mqtt_birth_payload;
    // -------------------------------------
    
    std::map<std::string, std::vector<MQTTCallback>> sub_hooks;

    static void static_mqtt_callback(const char *topic, const uint8_t *payload, uint16_t len, void *_this);
    void mqtt_callback(const std::string& topic, const std::string& message);

    std::string calculate_serial();
    void connect(); // connect to MQTT broker
    void autodiscover(); // send mqtt auto-discover message for home-assistant
    void handle_device_commands(const std::string& topic, const std::string& json_payload); // handle all commands for device control
    void handle_device_updates(const std::string& zone_name, int level); // handle all zone updates and publish MQTT updates

    bool mqtt_pub(const std::string& topic, const std::string& payload, bool retain = false, uint8_t qos=0);
    bool mqtt_sub(const std::string& topic, MQTTCallback cb, uint8_t qos=1);

public:
    
    // Security system connects to MQTT
    SecuritySystem(const std::string& config_string);
    //SecuritySystem(const char* name, uint8_t ip[4], uint16_t port, const char* user, const char* password);
    
    virtual ~SecuritySystem();

    bool online() const { return system_online;}
    void shutdown_system();
    void run();

    friend class ZoneManager;
};