#include "zone.h"
#include "clock.h"
#include "adt-security.h"

#include <algorithm>
#include <regex>
#include <map>

const std::vector<std::string> Zone::ZoneTypes {
    "gpio_digital", "virtual"
};

Zone::Zone(const std::string& name, IO type, bool invert, const ZoneMetaFields& meta) {
    unique_id = std::regex_replace(name, std::regex("[^a-zA-Z0-9]"), "");
    state_changed = true;
    last_state_changed.setSeconds(100);
    
    metadata.saveProperty("name", name); // device name
    metadata.saveProperty("unique_id", unique_id); // device id
    metadata.saveProperty("p", std::string( type == IO::IO_OUTPUT ? "switch" : "binary_sensor" )); // platform
    metadata.saveProperty("payload_on", std::string(invert ? "0":"1") ); // when on
    metadata.saveProperty("payload_off", std::string(invert ? "1":"0")); // when off

    if(!meta.device_class.empty()) metadata.saveProperty("device_class", meta.device_class); // device class type
    if(!meta.icon.empty()) metadata.saveProperty("icon", meta.icon); // device class type
}


DigitalGPIO_Zone::DigitalGPIO_Zone(const std::string& name, IO type, int pin, bool invert, const ZoneMetaFields& meta, GPIO::PinType mode):
    Zone(name, type, invert, meta),
    gpio( std::make_unique<GPIO>(pin, type == IO::IO_INPUT ? mode : GPIO::PIN_OUTPUT, std::bind(&onGPIOStateChange, this, std::placeholders::_1)) ),
    type(type)
{
    set_state_level(get());
}

DigitalGPIO_Zone::~DigitalGPIO_Zone() {}

void DigitalGPIO_Zone::onGPIOStateChange(DigitalGPIO_Zone* _this, int level) {
    _this->set_state_level(level);
}

void DigitalGPIO_Zone::set(int level) {
    if(type == IO::IO_OUTPUT){
        // update the internal state if the GPIO updates successfully
        if( gpio->write(level) ){
            set_state_level(level);
        } else {
            std::cout << "gpio state change failure\n";
        }
    }
}

int DigitalGPIO_Zone::get() {
    return gpio->read();
}

Virtual_Zone::Virtual_Zone(const std::string& name, IO type, bool invert, VirtualCallback cb, const ZoneMetaFields& meta):
    Zone(name, type, invert, meta),
    virtual_callback(cb)
{

}

void Virtual_Zone::set(int level) {
    if(!virtual_callback || virtual_callback(level)) {
        set_state_level(level);
    } else {
        std::cout << "state change failure\n";
    }
}

int Virtual_Zone::get() {
    return 0;
}

/* Zone Manager */

void ZoneManager::refresh_states() {
    for(auto& zone : zones){
        if(zone->last_state_changed.getSeconds() > 1){
            zone->state_changed = true;
        }
    }
}

void ZoneManager::update() {
    for(auto& ptr : zones){
        Zone& zone = *ptr;
        if(zone.state_changed && zone.last_state_changed.getMilliseconds() > 100){
            system->handle_device_updates(zone.get_unique_id(), zone.state);
            zone.state_changed = false;
            zone.last_state_changed.restart();
        }
    }
}

ZoneManager::ZoneManager(SecuritySystem* system): system(system) {
    JsonLoader& json = system->config;
    
    JsonLoader::Array zns;
    if(json.loadPropertyArray("zones", zns)){
        for(auto it = zns.Begin(); it < zns.End(); it++){
            JsonLoader::Object zone = it->GetObject();
            std::unique_ptr<Zone> new_zone;
            
            static const std::map<std::string,GPIO::PinType> pull_modes {
                { "pullup", GPIO::PIN_INPUT_PULLUP } ,
                { "pulldown", GPIO::PIN_INPUT_PULLDOWN },
                { "off", GPIO::PIN_INPUT }
            };
            static const std::map<std::string,Zone::IO> io_modes {
                { "output",  Zone::IO_OUTPUT} ,
                { "input", Zone::IO_INPUT }
            };

            ZoneMetaFields meta;
            std::string name, zone_type;
            int pin = -1;
            bool invert = false;
            Zone::IO io = Zone::IO_INPUT;
            GPIO::PinType pmode = GPIO::PIN_INPUT;

            if(!json.loadProperty(zone, "name", name) || name.empty()){
                std::cout << "a zone has no name! skipping...\n";
                continue;
            }
            
            if( !json.loadProperty(zone, "zone_type", zone_type) ||
                    std::find(Zone::ZoneTypes.begin(),Zone::ZoneTypes.end(), zone_type) == Zone::ZoneTypes.end() ){
                std::cout << name << " has an invalid zone_type! skipping...\n";
                continue;
            }
            
            json.loadProperty(zone, "device_class", meta.device_class);
            json.loadProperty(zone, "icon", meta.icon);
            json.loadProperty(zone, "invert", invert);

            std::string s_io, s_pmode;
            if(json.loadProperty(zone, "io", s_io) && io_modes.count(s_io)){
                io = io_modes.at(s_io);
            }

            if(zone_type == "gpio_digital"){
                if(!json.loadProperty(zone, "pin", pin) || pin == -1){
                    std::cout << name << " has an invalid pin! skipping...\n";
                    continue;
                }
                if(json.loadProperty(zone, "pullmode", s_pmode) && pull_modes.count(s_pmode)){
                    pmode = pull_modes.at(s_pmode);
                }

                new_zone = std::make_unique<DigitalGPIO_Zone>( name, io, pin, invert, meta, pmode);
                std::cout << "Loaded Zone: " << *new_zone << "\n";
            }
            
            if(new_zone){
                zones.push_back(std::move(new_zone));
            }
        }
    } else {
        std::cout << "No zones are configured!\n";
    }

    std::cout << "ZoneManager is ready\n";
}

ZoneManager::~ZoneManager() {}