# Sandbox
Tiny and lightweight renderer for learning purposes, final goal of this project is to get a wraping layer on top of modern Vulkan 1.3 and use it to create small demos.

<p align="center">
<b>**Work In Progress**</b>
</p>

---

## Features
- Vulkan context class which handles window management and swapchain.
- Pipeline and Layout builder helpers.
- Image layout state presistence.
- **Dynamic rendering**.
- **Buffer Device Address** (BDA) based push constant.
- **Bindless textures** support.

## Technology
**Languages**: C++ 20, GLSL  
**Build System**: CMake  
**Asset Management**: [TinyGLTF](https://github.com/syoyo/tinygltf), [Shader-C](https://github.com/google/shaderc)  
**Third-Party**: [Volk](https://github.com/zeux/volk), [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator), [SDL3](https://github.com/libsdl-org/SDL), [GLM](https://github.com/g-truc/glm)
