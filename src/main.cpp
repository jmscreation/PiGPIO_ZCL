#include "adt-security.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <cstdlib>
#include <csignal>

namespace fs = std::filesystem;

static SecuritySystem* system_service = nullptr;

void signalHandler(int signum) {
    if (signum == SIGTERM) {
        std::cout << "... SIGTERM" << std::endl;

        system_service->shutdown_system();
    }
}

int main(int argc, const char* argv[]) {
    // Register the signal handler for SIGTERM
    if (signal(SIGTERM, signalHandler) == SIG_ERR) {
        std::cerr << "Error registering signal handler." << std::endl;
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    {
        std::string config;
        if(fs::exists("config.conf")){
            std::ifstream cfg("config.conf");
            std::stringstream str;
            str << cfg.rdbuf();
            config = str.str();
        } else {
            std::cout << "config file is missing!\n";
            return 1;
        }
        system_service = new SecuritySystem(config);
    }

    if(system_service != nullptr){
        system_service->run();
        delete system_service; // finally free the system process
    }

    return 0;
}