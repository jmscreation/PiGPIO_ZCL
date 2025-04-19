#include "gpio.h"

const std::array<const char*,4> GPIO::StringType {
    "OUTPUT", "INPUT", "INPUT_PULLUP", "INPUT_PULLDOWN"
};

void GPIO::gpioStaticAlert(int gpio, int level, uint32_t tick, void* _this) {
    static_cast<GPIO*>(_this)->gpioAlert(gpio, level, tick);
}

void GPIO::gpioAlert(int gpio, int level, uint32_t tick) {
    if(gpio == pin){
        gpio_callback(level);
    }
}

GPIO::GPIO(int pin, GPIO::PinType type, GPIO::Callback cb): pin(pin), type(type), gpio_callback(cb) {
    gpioSetAlertFuncEx(pin, &gpioStaticAlert, this);
    updateMode();
}

GPIO::~GPIO() {
    gpioSetMode(pin, PI_INPUT);
    gpioSetPullUpDown(pin, PI_OFF);
    gpioSetAlertFunc(pin, nullptr);
}

void GPIO::updateMode() {
    if(type != PIN_OUTPUT){
        gpioSetMode(pin, PI_INPUT);
    }
    switch (type) {
        case PIN_INPUT_PULLDOWN:
            gpioSetPullUpDown(pin, PI_PUD_DOWN);
            break;
        case PIN_INPUT_PULLUP:
            gpioSetPullUpDown(pin, PI_PUD_UP);
            break;
        default:
            gpioSetPullUpDown(pin, PI_PUD_OFF);
            break;
    }
    if(type == PIN_OUTPUT){
        gpioSetMode(pin, PI_OUTPUT);
    }
}

int GPIO::read() {
    return gpioRead(pin);
}

bool GPIO::write(int output) {
    if(type == PIN_OUTPUT){
        return gpioWrite(pin, output) == 0;
    }
    return false;
}