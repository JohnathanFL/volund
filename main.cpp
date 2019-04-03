#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <regex>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

#include <csignal>

using namespace std::chrono_literals;

#include <SDL2/SDL.h>

#include "glad.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_gl3.h"

#include "yaip.hpp"

template <typename T> struct Vec2 { T x, y; };

static auto snappiness = 16.0ms;
static bool shown      = true;
void signalHandler(int sigNum) { shown = !shown; }

int searchFilter(const struct nk_text_edit*, nk_rune unicode) { return nk_true; }

inline bool tryHandleKey(std::unordered_map<SDL_Keycode, std::function<bool()>>& keyMaps, SDL_Keycode code) {
    return keyMaps.find(code) != keyMaps.end() && !keyMaps[code]();
}

using AppList = std::unordered_map<std::string, std::string>;

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void addApps(AppList& apps, std::string path) {
    for (auto& f : std::filesystem::directory_iterator(path)) {
        if (!f.is_regular_file()) continue;

        INIFile ini;
        auto file = std::ifstream(f.path());
        ini.parse(file);

        auto& entry = ini["Desktop Entry"];
        auto& name  = entry["Name"];
        auto exec  = entry["Exec"];
        replace(exec, "%f", "");
        replace(exec, "%F", "");
        replace(exec, "%d", "");
        replace(exec, "%D", "");
        replace(exec, "%u", "");
        replace(exec, "%U", "");
        replace(exec, "%N", "");
        replace(exec, "%k", "");
        replace(exec, "%v", "");

        if (name.empty() || exec.empty()) continue;

        apps.insert({name, exec});
    }
}

int main() {
    signal(SIGUSR1, signalHandler);

    SDL_Init(SDL_INIT_EVENTS);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    bool running = true;

    AppList apps;
    addApps(apps, "/usr/share/applications");
    std::cerr << "Found " << apps.size() << " apps!\n";

    std::vector<const std::string*> topTen;

    std::cerr << std::regex_match("Firefox Nightly", std::regex("(.*)Fi(.*)")) << std::endl;

    while (running) {
        if (!shown) {
            continue;
        } else {
            Vec2<int> windowSize;
            auto window = SDL_CreateWindow("volund", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080,
                                           SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN |
                                               SDL_WINDOW_INPUT_GRABBED | SDL_WINDOW_FULLSCREEN);

            SDL_GetWindowSize(window, &windowSize.x, &windowSize.y);
            SDL_GLContext glCtx = SDL_GL_CreateContext(window);
            SDL_GL_MakeCurrent(window, glCtx);

            gladLoadGL();
            glViewport(0, 0, windowSize.x, windowSize.y);

            nk_context* ctx = nk_sdl_init(window);
            {
                nk_font_atlas* atlas;
                nk_sdl_font_stash_begin(&atlas);
                nk_sdl_font_stash_end();
            }
            std::string searchText, prevText;
            searchText.resize(128);

            std::unordered_map<SDL_Keycode, std::function<bool()>> keyMaps; // ret true if handled/consumed
            keyMaps.insert({SDLK_ESCAPE, [&]() {
                                shown = false;
                                return true;
                            }});
            keyMaps.insert({SDLK_BACKSPACE, [&]() {
                                searchText[0] = '\0';
                                return true;
                            }});

            while (running && shown) {
                if (searchText[0] == '\0' && searchText != prevText) {
                    topTen.clear();
                    for (const auto& app : apps) topTen.push_back(&app.first);
                    prevText = searchText;
                }

                else if (searchText[0] != '\0' && searchText != prevText) {


                    std::cerr << "Checking against '";
                    auto txt = searchText;
                    txt.erase(std::find(txt.begin(), txt.end(), '\0'), txt.end());

                    auto exp = "(.*)" + txt + "(.*)";
                    std::cerr << "Updating top apps with expression '" << exp << "'\n";
                    auto regex = std::regex(exp, std::regex_constants::icase);

                    std::cerr << "'\n";

                    topTen.clear();
                    for (const auto& app : apps) {



                        if (std::regex_match(app.first, regex)) {
                          topTen.push_back(&app.first); }
                    }

                    prevText = searchText;
                }

                nk_input_begin(ctx);

                SDL_Event ev;
                while (SDL_PollEvent(&ev)) {
                    if (ev.type == SDL_QUIT)
                        // shown = false; // For production
                        running = false; // For debug
                    else if (ev.type == SDL_KEYDOWN && tryHandleKey(keyMaps, ev.key.keysym.sym)) {
                    } else
                        nk_sdl_handle_event(&ev);
                }
                nk_input_end(ctx);

                if (nk_begin(ctx, "volund", nk_rect(0, 0, windowSize.x, windowSize.y), 0)) {
                    nk_layout_row_dynamic(ctx, 25, 1);
                    nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, &searchText[0], 127, searchFilter);

                    for (size_t i = 0; i < topTen.size() && i < 9; i++) {
                        nk_layout_row_dynamic(ctx, 25.0, 1);
                        if (nk_button_label(ctx, topTen[i]->c_str())) {
                            system(("setsid " + apps[*topTen[i]] + '&').c_str());
                            shown = false;
                            goto cleanupLoopIter; // Our Bjorne who art in heaven above forgive me
                        }
                    }
                }

            cleanupLoopIter:
                nk_end(ctx);

                glViewport(0, 0, windowSize.x, windowSize.y);
                glClear(GL_COLOR_BUFFER_BIT);
                glClearColor(0.5, 0.5, 0.5, 0.5);
                nk_sdl_render(NK_ANTI_ALIASING_ON, 9999999, 99999999);

                SDL_GL_SwapWindow(window);
                SDL_GetWindowSize(window, &windowSize.x, &windowSize.y);
            }

            nk_sdl_shutdown();
            SDL_GL_DeleteContext(glCtx);
            SDL_DestroyWindow(window);
        }

        std::this_thread::sleep_for(snappiness);
    }

    SDL_Quit();
    return 0;
}
