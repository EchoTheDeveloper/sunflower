#define SOL_ALL_SAFETIES_ON 1

#include <SDL3/SDL.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <sol/sol.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

std::string GetExecutablePath() {
    char buffer[1024];
#ifdef _WIN32

    GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
#else
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) buffer[len] = '\0';
#endif
    return std::filesystem::path(buffer).parent_path().string();
}

std::string luaLocation;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
int* rendererColor = nullptr;

int main(int argc, char* argv[]) {
    std::string exePath = GetExecutablePath();
    bool isPauseOnCrash = false;

    if (argc >= 2) {
        luaLocation = argv[1];
    }
    else {
        luaLocation = exePath + std::string(1, std::filesystem::path::preferred_separator) + "Lua" + std::string(1, std::filesystem::path::preferred_separator) + "Application.lua";
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    rendererColor = new int[4] {255, 255, 255, 255};

    try {
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::table);

        lua["sunflower"] = lua.create_table();
        lua["sunflower"]["init"] = [](std::string title, int width, int height) -> bool {
            window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_RESIZABLE);
            if (!window) {
                std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
                return false;
            }
            renderer = SDL_CreateRenderer(window, NULL);
            if (!renderer) {
                std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
                SDL_DestroyWindow(window);
                return false;
            }
            return true;
            };

        lua["sunflower"]["graphics"] = lua.create_table();
        lua["sunflower"]["graphics"]["color"] = [](int r, int g, int b, int a) {
            if (rendererColor) {
                rendererColor[0] = r;
                rendererColor[1] = g;
                rendererColor[2] = b;
                rendererColor[3] = a;
            }
            };
        lua["sunflower"]["debugger"] = lua.create_table();
		lua["sunflower"]["debugger"]["pauseOnCrash"] = false;
        isPauseOnCrash = lua["sunflower"]["debugger"]["pauseOnCrash"];

        sol::protected_function_result result = lua.script_file(luaLocation);
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "Lua Error: " << err.what() << std::endl;
            throw err;
        }
    }
    catch (const sol::error& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        delete[] rendererColor;
        SDL_Quit();
        return -1;
    }

    if (!window || !renderer) {
        std::cerr << "Initialization failed, exiting..." << std::endl;
        delete[] rendererColor;
        SDL_Quit();
        return -1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, rendererColor[0], rendererColor[1], rendererColor[2], rendererColor[3]);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    delete[] rendererColor;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (isPauseOnCrash) {
		system("pause");
    }

    return 0;
}
