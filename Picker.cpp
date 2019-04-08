#include "Picker.hpp"

#include <iostream>
#include <regex>
#include <sstream>

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

int searchFilter(const struct nk_text_edit*, nk_rune unicode) { return nk_true; }



void Picker::escapeString(std::string& str) {
    std::stringstream ss;

    static const std::string escaped = R"~(\|()[]^${}*+?)~";
    for (char c : str) {
        if (escaped.find(c) != escaped.npos) ss << '\\';
        ss << c;
    }

    str = ss.str();
}

extern bool shown;

Picker::Picker(const AppList& newList) : list{newList} {
    searchText.resize(128);

    keyMaps.insert({SDLK_ESCAPE, [&]() {
                        shown = false;
                        return true;
                    }});
    keyMaps.insert({SDLK_BACKSPACE, [&]() {
                        searchText[0] = '\0';
                        return true;
                    }});

    keyMaps.insert({SDLK_RETURN, [&]() {
                        searchText[0] = '\0';
                        if (toDisplay[0]) system(("setsid " + toDisplay[0]->second + '&').c_str());
                        shown = false;
                        return true;
                    }});

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    Vec2<int> windowSize;
    window = SDL_CreateWindow("volund", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 200,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_GRABBED);

    SDL_GetWindowSize(window, &windowSize.x, &windowSize.y);
    glCtx = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, glCtx);

    gladLoadGL();
    glViewport(0, 0, windowSize.x, windowSize.y);

    ctx = nk_sdl_init(window);
    {
        nk_font_atlas* atlas;
        nk_sdl_font_stash_begin(&atlas);
        nk_sdl_font_stash_end();
    }
}

Picker::~Picker() {
  nk_sdl_shutdown();
  SDL_GL_DeleteContext(glCtx);
  SDL_DestroyWindow(window);
}

void Picker::updateSearch() {

  if (searchText[0] == '\0' && searchText != prevText) {
    toDisplay.clear();
    for (const auto& app : list) toDisplay.push_back(&app);
        prevText = searchText;
    }

    else if (searchText[0] != '\0' && searchText != prevText) {
        // std::cerr << "Checking against '";
        auto txt = searchText;
        escapeString(txt);
        txt.erase(std::find(txt.begin(), txt.end(), '\0'), txt.end());

        auto exp = "(.*)" + txt + "(.*)";
        // std::cerr << "Updating top apps with expression '" << exp << "'\n";
        auto regex = std::regex(exp, std::regex_constants::icase);

        std::cerr << "'\n";

        toDisplay.clear();

        for (const auto& app : list) {
            if (std::regex_match(app.first, regex)) {
                toDisplay.push_back(&app);
            }
        }

        prevText = searchText;
    }

    nk_input_begin(ctx);

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT)
            shown = false;
        else if (ev.type == SDL_KEYDOWN && tryHandleKey(keyMaps, ev.key.keysym.sym)) {
        } else
            nk_sdl_handle_event(&ev);
    }
    nk_input_end(ctx);
}

void Picker::draw() {
    if (nk_begin(ctx, "volund", nk_rect(0, 0, windowSize.x, windowSize.y), 0)) {
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_edit_focus(ctx, NK_EDIT_ALWAYS_INSERT_MODE);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, &searchText[0], 127, searchFilter);

        bool first = true;
        for (auto& app : toDisplay) {
            nk_layout_row_dynamic(ctx, 25.0, 1);
            if (nk_button_label(ctx, (first ? (std::string{">>  "} + app->first + "  <<").c_str() : app->first.c_str()))) {
                system(("setsid " + app->second + '&').c_str());
                shown = false;
                goto cleanupLoopIter; // Our Bjorne who art in heaven above forgive me
            }

            first = false;
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
