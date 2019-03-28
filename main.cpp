#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
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

template <typename T>
struct Vec2 {
   T x, y;
};

static auto snappiness = 16.0ms;
static bool shown      = true;
void        signalHandler(int sigNum) { shown = !shown; }

int searchFilter(const struct nk_text_edit*, nk_rune unicode) { return nk_true; }

inline bool tryHandleKey(std::unordered_map<SDL_Keycode, std::function<bool()>>& keyMaps, SDL_Keycode code) {
   return keyMaps.find(code) != keyMaps.end() && !keyMaps[code]();
}

struct App {
   std::string name;
   std::string exec;

   App(std::string newName, std::string newExec) : name{newName}, exec{newExec} {}
};

using AppList = std::vector<std::unique_ptr<App>>;


struct LevenAppComparator {
   const std::string& searchText;

   static int editDistance(const std::string& lhs, const std::string& rhs) {
      // https://www.algorithmist.com/index.php/Edit_Distance
      using std::min;
      using std::vector;

      long int m = lhs.length();
      long int n = rhs.length();

      vector<vector<long int>> v(m + 1, vector<long int>(n + 1));

      for (int i = 0; i < m; i++)
         v[i][0] = i;

      for (int j = 0; j < n; j++)
         v[0][j] = j;

      for (int i = 1; i < m; i++)
         for (int j = 1; j < n; j++)
            if (lhs[i - 1] == rhs[j - 1])
               v[i][j] = v[i - 1][j - 1];
            else
               v[i][j] = 1 + min(min(v[i][j - 1], v[i - 1][j]), v[i - 1][j - 1]);

      return v[m - 1][n - 1];
   }

   float rank(const std::string& str) const {
      // Thanks to
      // https://stackoverflow.com/questions/10405440/percentage-rank-of-matches-using-levenshtein-distance-matching
      auto distance = editDistance(searchText, str);
      int  bigger   = std::max(searchText.size(), str.size());
      return (bigger - distance) / static_cast<float>(bigger);
   }

   bool operator()(const std::unique_ptr<App>& lhs, const std::unique_ptr<App>& rhs) const {
      static bool t = false;

      auto lhsRank = rank(lhs->name);
      auto rhsRank = rank(rhs->name);

      std::cerr << "search text was '" << searchText << "'\n";
      std::cerr << "compared " << lhs->name << " (" << lhsRank << ") against " << rhs->name << "(" << rhsRank << ")\n";
      return rhsRank < lhsRank;
   }
};

void addApps(AppList& apps, std::string path) {
   for (auto& f : std::filesystem::directory_iterator(path)) {
      if (!f.is_regular_file())
         continue;

      INIFile ini;
      auto    file = std::ifstream(f.path());
      ini.parse(file);

      auto& entry = ini["Desktop Entry"];
      auto& name  = entry["Name"];
      auto& exec  = entry["Exec"];

      if (name.empty() || exec.empty())
         continue;

      apps.push_back(std::make_unique<App>(name, exec));
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

   while (running) {
      if (!shown) {
         continue;
      } else {
         Vec2<int> windowSize;
         auto      window = SDL_CreateWindow("volund", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 900, 300,
                                        SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN |
                                            SDL_WINDOW_INPUT_GRABBED | SDL_WINDOW_POPUP_MENU);

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

         std::unordered_map<SDL_Keycode, std::function<bool()>> keyMaps;  // ret true if handled/consumed
         keyMaps.insert({SDLK_ESCAPE, [&]() {
                            shown = false;
                            return true;
                         }});
         keyMaps.insert({SDLK_BACKSPACE, [&]() {
                            searchText.clear();
                            return true;
                         }});


         while (running && shown) {
            if (searchText[0] != '\0' && searchText != prevText) {
               std::sort(apps.begin(), apps.end(), LevenAppComparator{searchText});
               prevText = searchText;
            }


            nk_input_begin(ctx);

            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
               if (ev.type == SDL_QUIT)
                  // shown = false; // For production
                  running = false;  // For debug
               else if (ev.type == SDL_KEYDOWN && tryHandleKey(keyMaps, ev.key.keysym.sym)) {
               } else
                  nk_sdl_handle_event(&ev);
            }
            nk_input_end(ctx);

            if (nk_begin(ctx, "volund", nk_rect(0, 0, windowSize.x, windowSize.y), 0)) {
               nk_layout_row_dynamic(ctx, 25, 1);
               nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, &searchText[0], 127, searchFilter);
            }

            for (size_t i = 0; i < apps.size() && i < 9; i++) {
               nk_layout_row_dynamic(ctx, 25.0, 1);
               if (nk_button_label(ctx, apps[i]->name.c_str())) {
                  system(("setsid " + apps[i]->exec + '&').c_str());
                  shown = false;
                  goto cleanupLoopIter;  // Our Bjorne who art in heaven above forgive me
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
