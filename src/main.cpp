#include "adt-security.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
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

        SecuritySystem system(config);

        system.run();
    }
    return 0;
}