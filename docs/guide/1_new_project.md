# Making a new project
## Required dependencies
You will need the **Vulkan SDK**, **CMake** and a **C/C++ compiler**.
We also recommend **CLion** as your IDE.

<details>
<summary><b>Windows</b></summary>

```shell
# Euphrosyne works with MinGW and it's less painful to setup than MSVC,
# so we recommend using MinGW to develop with it, therefore this guide 
# downloads a standalone GCC
winget install Kitware.CMake KhronosGroup.VulkanSDK BrechtSanders.WinLibs.POSIX.UCRT
```
</details>

<details>
<summary><b>Debian/Ubuntu</b></summary>

```shell
sudo apt install build-essential cmake libvulkan-dev vulkan-tools vulkan-validationlayers glslc
```
</details>

<details>
<summary><b>Arch</b></summary>

```shell
sudo pacman -S base-devel cmake vulkan-devel
```
</details>

<details>
<summary><b>Fedora</b></summary>

```shell
sudo dnf install gcc-c++ make cmake vulkan-headers vulkan-loader-devel vulkan-tools vulkan-validation-layers glslc
```
</details>

## Creating a project
TODO: Guide to setting up a new CMake Project fetching the engine with CPM

## Building


## Running