#include "euphrosyne/formats/archive.hpp"
#include "euphrosyne/utils/platform/paths.hpp"

int main() {
    auto archive = euphrosyne::Archive();
    archive.Open(euphrosyne::ExecutableDirectory() / "shaders.pak");

    auto dial = archive.Find("triangle.ps.hlsl");
}