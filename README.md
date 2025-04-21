# Raspberry Pi GPIO Zone Controller
 This system will turn your Raspberry Pi / Raspberry Pi Zero into an Alarm System Zone controller.
 The idea behind this is to simplify the task of converting an old standard zone-alarm-system logic board into an updated MQTT enabled zone-alarm-system utilizing the GPIO pins. This project will likely require soldering, especially if you are using a Raspberry Pi Zero/W.

<img src="https://github.com/jmscreation/PiGPIO_ZCL/blob/5bf8c15002645dcbc11e9f2e9e705d4faaf1a7c4/parts/panel.jpg" width=50% height=50%></img>


# How To Install

## Prerequisites
For this project, we will assume that you already have an **MQTT Broker/Server** and a running instance of **Home Assistant** for interacting with your MQTT broker. This project uses __Home Assistant__ for the front end UI, and **eclipse-mosquitto** for the MQTT Broker. Your environment may vary, but this service is intended to interact with Home Assistant as it uses [Device Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) to simplify the setup process.

## Configure and Install

1. First, you'll need to create a _config.conf_ file. You can use the [default_config.conf](https://github.com/jmscreation/PiGPIO_ZCL/blob/fbe8872b78c6eda550c1cfcaa16e78300a9133d6/default_config/default_config.conf) file as a guide
2. Next, download the [latest release](https://github.com/jmscreation/PiGPIO_ZCL/tree/latest-stable) of the service, or build it yourself*
3. Upload _install.sh_, _config.conf_, and _program_ to the Raspberry Pi
4. Run `sudo install.sh` with both the _program_ and _config.conf_ file in the same directory
5. Check to ensure your device is connected to your MQTT server, and that Home Assistant can see your new Zone Controller

#### Currently, there is no documentation to fully express the capabilities of this application/service. Future releases may include a table of all the applicable Zones/Features. For now, see the [default_config.conf](https://github.com/jmscreation/PiGPIO_ZCL/blob/fbe8872b78c6eda550c1cfcaa16e78300a9133d6/default_config/default_config.conf) file as a reference guide.

_*There are some libraries that are required when building. If you are on a windows environment, you'll need the cross compiler for the raspberry pi [here](https://gnutoolchains.com/raspberry/)_
