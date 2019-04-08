#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <regex>
#include <set>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include <csignal>

using namespace std::chrono_literals;

#include <SDL2/SDL.h>

#include "AppDB.hpp"
#include "Picker.hpp"

static auto snappiness = 16.0ms;
bool shown             = true;
void signalHandler(int sigNum) { shown = !shown; }

int main() {
    signal(SIGUSR1, signalHandler);

    SDL_Init(SDL_INIT_EVENTS);

    bool running = true;

    AppDB db;
    db.addPath("/usr/share/applications");
    db.addPath("/home/oakenbow/.local/share/applications");
    std::cerr << "Found " << db.numApps() << " apps!\n";

    std::vector<const std::string*> topTen;

    while (running) {
        if (!shown)
            std::this_thread::sleep_for(snappiness);
        else {
            Picker picker{db};

            while (running && shown) picker.update();
        }
    }
    SDL_Quit();
    return 0;
}
