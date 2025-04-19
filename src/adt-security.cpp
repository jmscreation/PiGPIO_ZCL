#include "adt-security.h"
#include <regex>

SecuritySystem::SecuritySystem(const std::string& config_string):
mqtt(nullptr), zone_manager(nullptr),
system_online(false),
serial_number(calculate_serial())

{
    config.displayErrors = true;

    // import system config
    if(!config.parseString(config_string)){
        std::cerr << "failed to load config\n";
        return;
    }

    int gpio_state = gpioInitialise();
    
    if(gpio_state != PIGPIO_VERSION){
        gpioTerminate();
        std::cerr << "failed to initialize GPIO!\n";
        return;
    }

    config.loadProperty("name", system_name);
    config.loadProperty("system_runtime_name", system_uptime_name);
    config.loadProperty("auto_refresh_states", auto_refresh_timer);
    
    // load mqtt settings
    uint8_t ip[4];
    int port;
    std::string ip_address, user, password;
    config.loadProperty("mqtt_ip", ip_address);
    std::cout << ip_address << "\n";
    size_t p=0;
    for(int i=0;i < 4;++i){
        int xp = ip_address.find_first_of(".",p);
        std::string oct = ip_address.substr(p,xp - p);
        p = xp + 1;
        ip[i] = uint8_t(std::stoi(oct));
    }
    config.loadProperty("mqtt_port", port);
    config.loadProperty("mqtt_user", user);
    config.loadProperty("mqtt_password", password);
    config.loadProperty("birth", mqtt_birth_payload);
    config.loadProperty("will", mqtt_will_payload);
    
    // load mqtt topics
    mqtt_topic_version_info = system_name + "/version";
    mqtt_topic_device_discovery = "homeassistant/device/" + system_name + "/config";
    mqtt_topic_entity_state = system_name + "/state";
    mqtt_topic_entity_update = system_name + "/set";
    mqtt_topic_homeassistant_status = "homeassistant/status";
    mqtt_topic_system_status = system_name + "/system/status";
    mqtt_topic_system_command = system_name + "/system/command";

    std::cout << user << ":" << password << "@" << int(ip[0]) << "." << int(ip[1]) << "." << int(ip[2]) << "." << int(ip[3]) << ":" << port << "\n";
    // initialize mqtt
    mqtt = new ReconnectingMqttClient(ip, port, system_name.c_str());
    mqtt->user = user;
    mqtt->password = password;
    mqtt->will_topic = mqtt_topic_system_status;
    mqtt->will_payload = mqtt_will_payload;
    
    zone_manager = new ZoneManager(this);
    
    {
        JsonLoader info;
        info.saveProperty("gpio_version", PIGPIO_VERSION);
        info.saveProperty("gpio_state", gpio_state);
        info.saveProperty("mqtt_version", mqtt->MQTT_VERSION);
        info.saveProperty("adt_version", int(SYS_VERSION));
        info.saveProperty("adt_serial", serial_number);
        info.saveProperty("adt_zones", zone_manager->get_zones().size());
        
        system_version_json = info.toString();
    }
    
    connect();
    
    if(online()){
        autodiscover();
    }
}

SecuritySystem::~SecuritySystem() {    
    if(mqtt != nullptr && mqtt->is_connected()){
        mqtt_pub(mqtt_topic_system_status, "offline");

        for(const auto& [topic,callback] : sub_hooks){
            mqtt->send_subscribe(topic.c_str(), 1, true); // unsubscribe all topics
        }
    }

    delete mqtt;
    mqtt = nullptr;

    delete zone_manager;
    zone_manager = nullptr;

    gpioTerminate();
}

// this function is called once in the class initializer list
std::string SecuritySystem::calculate_serial() {
    std::cout << "calculate serial...\n";
    std::string serial = "NULL";

    std::stringstream proc;
    {
        std::ifstream fproc("/proc/cpuinfo");
        if(!fproc.is_open()) return serial;
        proc << fproc.rdbuf();
    }

    std::string line;
    while(std::getline(proc, line)){
        auto split = line.find(":");
        if(split == std::string::npos) continue;
        
        std::string k = line.substr(0, split);
        while(k.find_last_of("\t") == k.size() - 1){ k.pop_back(); }
        std::string v = line.substr(split+1);
        while(v.find_first_of(" ") == 0){ v.erase(v.begin()); }

        cpuinfo.insert_or_assign(k, v);
    }

    if(cpuinfo.count("Serial")){
        serial = sha1(cpuinfo.at("Serial"));
    }

    return serial;
}

void SecuritySystem::static_mqtt_callback(const char *topic, const uint8_t *payload, uint16_t len, void *_this) {
    static_cast<SecuritySystem*>(_this)->mqtt_callback(topic, std::string((const char*)payload,len));
}

void SecuritySystem::mqtt_callback(const std::string& topic, const std::string& message) {
    // handle wild card subscribed topics
    for(auto& [ktop,cbs] : sub_hooks){
        if(topic.size() < ktop.size() - 1 || (!ktop.ends_with("#") && !ktop.ends_with("+"))) continue;
        if( topic.find_first_of(ktop.substr(0, ktop.size() - 1)) == 0) {
            for(auto& cb : cbs){
                cb(topic, message);
            }
        }
    }

    if(!sub_hooks.count(topic)) return;

    for(auto& cb : sub_hooks.at(topic)){
        cb(topic, message);
    }
}

bool SecuritySystem::mqtt_pub(const std::string& topic, const std::string& payload, bool retain, uint8_t qos) {
    return mqtt->is_connected() ? mqtt->publish(topic.c_str(), payload.c_str(), retain, qos) : false;
}

bool SecuritySystem::mqtt_sub(const std::string& topic, MQTTCallback cb, uint8_t qos) {
    bool status = mqtt->send_subscribe(topic.c_str(), qos);

    if(status){
        if(!sub_hooks.count(topic)){
            sub_hooks.insert_or_assign(topic, std::vector<MQTTCallback>{} );
        }

        sub_hooks.at(topic).push_back(cb);
        sub_hooks.at(topic).push_back(
            [](const std::string& topic, const std::string& message){
                std::cout << topic << " : " << message << std::endl;
            }
        );
    }
    return status;
}


void SecuritySystem::connect() {
    if(mqtt->is_connected() || system_online){
        system_online = true;
        return;
    }

    mqtt->connect();

    Clock timeout;
    while(!mqtt->is_connected() && timeout.getSeconds() < 10){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if(!mqtt->is_connected()){
        std::cout << "failed to connect to MQTT broker\n";
        system_online = false;
        return;
    }
    mqtt->set_receive_callback(&SecuritySystem::static_mqtt_callback, this);

    // subscribe to essential topics
    mqtt_sub(mqtt_topic_homeassistant_status, [=,this](const std::string& topic, const std::string& status){
        if(status == "online") autodiscover();
    });
    mqtt_sub(mqtt_topic_system_command, [=,this](const std::string& topic, const std::string& command){
        std::map<std::string, std::function<void()>> commands {
            {"shutdown", [=,this](){
                shutdown_system();
            }}
        };

        if(commands.count(command)){
            commands.at(command)();
        }
    });

    mqtt_sub(mqtt_topic_entity_update + "/+",[=,this](const std::string& topic, const std::string& payload){
        handle_device_commands(topic.substr(topic.find_last_of("/") + 1), payload);
    });

    // publish essential information
    mqtt_pub(mqtt_topic_version_info, system_version_json);    
    system_online = true;
}


void SecuritySystem::autodiscover() {
    JsonLoader json;

    JsonLoader::Object dev;
    json.saveProperty(dev, "name", system_uptime_name);
    json.saveProperty(dev, "mf", std::string(""));
    json.saveProperty(dev, "mdl", std::string(""));
    json.saveProperty(dev, "sw", std::to_string(SYS_VERSION));
    json.saveProperty(dev, "hw", cpuinfo.count("CPU revision") ? cpuinfo.at("CPU revision") : "");
    JsonLoader::Array ids;
    json.appendArrayValue(ids, serial_number);
    json.savePropertyArray(dev, "ids", ids);
    json.saveProperty("dev", dev);
    
    JsonLoader::Object o;
    json.saveProperty(o, "name", std::string("adt2mqtt"));
    json.saveProperty(o, "sw", std::string(mqtt->MQTT_VERSION));
    json.saveProperty(o, "url", std::string("https://github.com/jmscreation"));
    json.saveProperty("o", o);

    json.saveProperty("availability_topic", mqtt_topic_system_status);
    json.saveProperty("qos", std::string("1"));

    JsonLoader::Object cmps;
    for(const auto& ptr : zone_manager->get_zones()){
        const Zone& zone = *ptr;
        
        JsonLoader::Object cmp;
        json.parseStringObject(zone.get_meta(), cmp);
        
        std::string name,id; // used for topic population
        json.loadProperty(cmp, "name", name);
        json.loadProperty(cmp, "unique_id", id);

        json.saveProperty(cmp, "state_topic", mqtt_topic_entity_state + "/" + id);
        json.saveProperty(cmp, "command_topic", mqtt_topic_entity_update + "/" + id);
        
        json.saveProperty(cmps, (new std::string(id))->c_str() , cmp);
    }
    json.saveProperty("cmps", cmps);

    std::string device_discovery_payload = json.toString();
        
    mqtt_pub(mqtt_topic_device_discovery, device_discovery_payload, true, 1);
    
    mqtt_pub(mqtt_topic_system_status, mqtt_birth_payload, false, 1);
    
    // std::cout << mqtt_topic_device_discovery << "\n" << device_discovery_payload << "\n";

    zone_manager->refresh_states(); // refresh all the entity states so the MQTT can get the latest image
}

void SecuritySystem::handle_device_commands(const std::string& unique_id, const std::string& payload) {
    std::cout << "device command: set " << unique_id << " to level " << payload << "\n";
    for(auto& ptr : zone_manager->get_zones()){
        Zone& zone = *ptr;
        if(zone.get_unique_id() != unique_id) continue;
        if(std::regex_match(payload, std::regex("^\\d{1,3}$"))){
            zone.set(std::stoi(payload));
        }
    }
}

void SecuritySystem::handle_device_updates(const std::string& device_id, int level) {
    std::cout << "device state changed: " << device_id << " is now " << level << "\n";
    mqtt_pub(mqtt_topic_entity_state + "/" + device_id, std::to_string(level), false, 1);
}

void SecuritySystem::run() {
    if(!system_online){
        std::cout << "failed to start system runtime!\n";
    } else {
        Clock refreshTimeout;
        while(system_online) {
            mqtt->update();
            zone_manager->update();

            if(refreshTimeout.getSeconds() > auto_refresh_timer){
                zone_manager->refresh_states();
                refreshTimeout.restart();
            }
            
            std::cout << std::flush;

            std::this_thread::sleep_for(std::chrono::milliseconds(80)); // cool-down
        }
        
        std::cout << "System shutting down...\n";
    }
}

void SecuritySystem::shutdown_system() {
    system_online = false;
}