#pragma once
#include "forward_declarations.h"

#include "gpio.h"
#include "clock.h"
#include "ReconnectingMqttClient.h"
#include "jsonloader.h"

#include <vector>
#include <iostream>
#include <functional>
#include <string>
#include <atomic>
#include <memory>


// Base Zone / All Zones Inherited From

struct ZoneMetaFields {
    std::string device_class;
    std::string icon;
    uint32_t trigger_timeout_threshold;
};

class Zone {

    std::atomic_bool state_changed;
    Clock last_state_changed;
    std::atomic_int16_t state;
    std::string unique_id;
    int trigger_timeout;

protected:
    void set_state_level(int16_t level) { state = level; state_changed = true; }

    JsonLoader metadata;

public:
    static const std::vector<std::string> ZoneTypes;

    enum IO : int {
        IO_OUTPUT, IO_INPUT
    };

    std::string get_meta() const { return metadata.toString(); }
    const std::string& get_unique_id() const { return unique_id; }

    virtual void set(int state) = 0; // abstract - all zones must implement / sets the real state
    virtual int get() = 0; // abstract - all zones must implement / gets the real state and returns

    Zone(const std::string& name, IO type, bool invert, const ZoneMetaFields& meta);
    virtual ~Zone() = default;

    Zone(const Zone&) = delete;

    inline friend std::ostream& operator<<(std::ostream& stream, const Zone& obj) {
        stream << obj.get_meta();
        return stream;
    }
    
    friend class ZoneManager;
};

class DigitalGPIO_Zone : public Zone {
    std::unique_ptr<GPIO> gpio;
    IO type;
    static void onGPIOStateChange(DigitalGPIO_Zone* _this, int level);

public:
    DigitalGPIO_Zone(const std::string& name, IO type, int pin, bool invert=false, const ZoneMetaFields& meta={"",""}, GPIO::PinType mode=GPIO::PIN_INPUT_PULLDOWN);
    virtual ~DigitalGPIO_Zone();

    void set(int level) override;
    int get() override;
};

// A virtual zone is one that must be controlled in software
using VirtualCallback = std::function<bool(int)>; // virtual callback to simulate a GPIO pin
class Virtual_Zone : public Zone {
    VirtualCallback virtual_callback;
public:
    Virtual_Zone(const std::string& name, IO type, bool invert, VirtualCallback cb={}, const ZoneMetaFields& meta={"",""});
    virtual ~Virtual_Zone() = default;

    void set(int level) override;
    int get() override;
};


class ZoneManager {
    SecuritySystem* system;

    using ZoneList = std::vector<std::unique_ptr<Zone>>;

    ZoneList zones;

public:

    const ZoneList& get_zones() const { return zones; }
    void refresh_states();
    void update();

    ZoneManager(SecuritySystem* system);
    virtual ~ZoneManager();

};