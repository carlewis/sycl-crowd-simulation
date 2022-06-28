#include <sycl/sycl.hpp>
#include <SDL2/SDL.h>
#include <iostream>
#include <chrono>
#include <random>
#include <memory>

#include "Actor.hpp"
#include "Room.hpp"
#include "MathHelper.hpp"
#include "DifferentialEq.hpp"
#include "VectorMaths.hpp"
#include "ParseInputFile.hpp"
#include "Path.hpp"

int WIDTH; // metres
int HEIGHT; // metres
int SCALE;
int DELAY;

void init(SDL_Window* &win, SDL_Renderer* &render, std::vector<Actor> &actors, Room &room, std::vector<Path> &paths, int argc, char **argv) {
    // Read from input file path JSON
    if (argc > 1) {
        std::string inputPath = argv[1];
        parseInputFile(inputPath, actors, room, paths, WIDTH, HEIGHT, SCALE, DELAY);
    }
    
    // Initialise SDL
    SDL_Init(SDL_INIT_VIDEO);
    win = SDL_CreateWindow("SYCL Crowd Simulation", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH * SCALE, HEIGHT * SCALE, SDL_WINDOW_SHOWN);
    render = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
}

void drawCircle(SDL_Renderer* &render, SDL_Point center, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(render, color.r, color.g, color.b, color.a);
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx*dx + dy*dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(render, center.x + dx, center.y + dy);
            }
        }
    }
}

void update(sycl::queue myQueue, std::vector<Actor> &actors, Room room, std::vector<Path> paths) {
    for (auto &a : actors) {
        a.refreshVariation();
    }

    auto actorBuf = sycl::buffer<Actor>(actors.data(), actors.size());

    auto walls = room.getWalls();
    auto wallsBuf = sycl::buffer<std::array<vecType, 2>>(walls.data(), walls.size());

    auto pathsBuf = sycl::buffer<Path>(paths.data(), paths.size());

    myQueue.submit([&](sycl::handler& cgh) {
        auto actorAcc = actorBuf.get_access<sycl::access::mode::read_write>(cgh);

        auto wallsAcc = wallsBuf.get_access<sycl::access::mode::read>(cgh);

        auto pathsAcc = pathsBuf.get_access<sycl::access::mode::read>(cgh);

        auto out = sycl::stream{1024, 768, cgh};

        cgh.parallel_for(sycl::range<1>{actors.size()}, [=](sycl::id<1> index) {
            if (!actorAcc[index].getAtDestination()) {
                differentialEq(index, actorAcc, wallsAcc, pathsAcc, out);
            }
        });
    }).wait();
}

void draw(SDL_Renderer* &render, std::vector<Actor> actors, Room room) {
    SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
    SDL_RenderClear(render);

    for (Actor actor : actors) {
        SDL_Point pos = {int(actor.getPos()[0] * SCALE), int(actor.getPos()[1] * SCALE)};
        SDL_Color actorColor = {Uint8(actor.getColor()[0]), Uint8(actor.getColor()[1]), Uint8(actor.getColor()[2]), 255};
        drawCircle(render, pos, actor.getRadius() * SCALE, actorColor);
    }

    SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
    auto walls = room.getWalls();
    for (auto wall : walls) {
        SDL_RenderDrawLine(render, wall[0][0] * SCALE, wall[0][1] * SCALE, wall[1][0] * SCALE, wall[1][1] * SCALE);
    }

    SDL_RenderPresent(render);
}

void close(SDL_Window* win, SDL_Renderer* render) {
    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    SDL_Window* win;
    SDL_Renderer* render;

    std::vector<Actor> actors;
    Room room = Room({});
    std::vector<Path> paths;

    sycl::queue myQueue{sycl::gpu_selector()};

    init(win, render, actors, room, paths, argc, argv);

    draw(render, actors, room);

    int delayCounter = 0;
    bool isQuit = false;
    SDL_Event event;

    while(!isQuit) {
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isQuit = true;
            }
        }

        if (delayCounter >= DELAY) {
            delayCounter = 0;
            update(myQueue, actors, room, paths);
            draw(render, actors, room);
        }
        else {
            delayCounter++;
        }
    }

    close(win, render);
    return 0;
}
