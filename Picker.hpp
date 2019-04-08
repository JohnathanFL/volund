#pragma once
#include <functional>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>

#include "AppDB.hpp"

struct nk_context;

template <typename T> struct Vec2 { T x, y; };

class Picker {
  public:
    Picker(const AppList&);
    ~Picker();

    void updateSearch();
    void draw();
    inline void update() {
        updateSearch();
        draw();
    }
    inline bool tryHandleKey(std::unordered_map<SDL_Keycode, std::function<bool()>>& keyMaps, SDL_Keycode code) {
        return keyMaps.find(code) != keyMaps.end() && !keyMaps[code]();
    }

    void escapeString(std::string& str);

    const AppList& list;
    std::vector<AppList::const_pointer> toDisplay;

    std::string searchText, prevText;
    nk_context* ctx;

    std::unordered_map<SDL_Keycode, std::function<bool()>> keyMaps; // ret true if handled/consumed
    SDL_Window* window;
    SDL_GLContext glCtx;
    Vec2<int> windowSize;
};
