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
void openSignalHandler(int sigNum) { 
    if(sigNum == SIGUSR1) // Never hurts to be sure
        shown = !shown; 
}


void loadDefaultPaths(AppDB& db) {
    db.addPath("/usr/share/applications");
    db.addPath("/home/oakenbow/.local/share/applications");
}

bool shouldReload = true;
void reloadSignalHandler(int sigNum) {
    if(sigNum == SIGUSR2)
        shouldReload = true;
}



int main() {
    auto newSnappiness = std::getenv("VOLUND_SNAPPINESS");
    if(newSnappiness) {
      snappiness = std::chrono::duration<long double, std::milli>(std::stof(newSnappiness));
     std::cerr << "Read snappiness from VOLUND_SNAPPINESS as " << snappiness.count() << std::endl;
    } else {
      std::cerr << "$VOLUND_SNAPPINESS was not set. Defaulting to " << snappiness.count() << std::endl;
    }

    signal(SIGUSR1, openSignalHandler);
    signal(SIGUSR2, reloadSignalHandler);

    SDL_Init(SDL_INIT_EVENTS);

    bool running = true;

    AppDB db;
    
    

    std::vector<const std::string*> topTen;

    while (running) {
        if(shouldReload) {
            std::cerr << "Reloading paths...\n";
            shouldReload = false;
            db.clear();
            loadDefaultPaths(db);
            std::cerr << "Found " << db.numApps() << " apps!\n";
        }

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
