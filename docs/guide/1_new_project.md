# Making a new project
## Required dependencies
You will need the **Vulkan SDK** (with **dxc** and **DirectX** headers), **CMake**, and a **C/C++ compiler**.
We also recommend **CLion** as your IDE.

## Creating a CMake project
Create a new CMake project and fetch the engine with your method of choice, either with `FetchContent`, `CPM`, or as a submodule. We recommend using `CPM`.

You could theoretically use any build system that integrates with CMake, such as XMake, Meson or Bazel, but there's no good reason to and you will lose IDE compatibility and community support.

```cmake
cmake_minimum_required(VERSION 3.22)
project(mygame VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23) # Use C++23
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Have everything use C++23
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON) # LTO on release for faster code

# If fetching with CPM
include(cmake/CPM.cmake)
CPMAddPackage(
        NAME euphrosyne
        VERSION 0.1.0 # or the latest version
        URL https://github.com/GhostInCerulean/euphrosyne/releases/archive/refs/tags/v0.1.0.tar.gz
)

# Handles everything for you, cooking, linking libraries
euphrosyne_add_game(
        NAME mygame
        IDENTIFIER org.example.mygame
        SOURCE_FILES
            src/main.cpp
            # ...
        ASSET_FOLDER
            assets
)
```

## Writing an hello world
in `src/main.cpp` write:
```c++
#include <euphrosyne/game/game.hpp>
#include <euphrosyne/game/start.hpp>
#include <euphrosyne/logging/log.hpp>
#include <flecs.hpp>

using namespace euphrosyne;

DECLARE_LOGGER_IMPL(GAME, "My game");


struct HelloWorldModule 
{
    HelloWorldModule(flecs::world& ecs) 
    {
        ecs.system("Prints a message at startup")
            .kind(FlecsOnLoad)
            .run([](){
                GAME_LOG_INFO("hi :DDDDDD")
            });
    }
};

class MyGame : public Game 
{
public:
    virtual void RegisterUserModules() override 
    {
        ecs.import<HelloWorldModule>();
    }
};


int main(int argc, char** argv) 
{
    euphrosyne::Start<MyGame>(argc, argv);
}
```


## Building and running
You should set the environment variable `CPM_SOURCE_CACHE` to a cache folder beforehand, so it caches all the CMake dependencies.

The configure step is expected to take between 2 and 20 minutes depending on your internet quality, but it will be much faster the following runs if you use cache. The build step should take inbetween 2 and 5 minutes, depending on your CPU count, but next builds will be way quicker.

We recommend you use an IDE. But if you really want to build and run from the commandline, run:
```shell
# Configure (includes fetching deps)
cmake -S . -B build # configure step

# Build
cmake --build build --parallel 

# Run
./build/mygame
```
