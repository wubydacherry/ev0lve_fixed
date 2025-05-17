project(renderer)
cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)

include_directories(include)

set(REN_ADAPTER_SRC "")
if (REN_USE_DX9)
    include_directories($ENV{DXSDK_DIR}/Include)
    set(REN_ADAPTER_SRC ${REN_ADAPTER_SRC}
            src/adapters/adapter_dx9.cpp
            src/buffers/vb_dx9.cpp
            src/buffers/ib_dx9.cpp
    )
endif()

if (REN_USE_DX11)
    include_directories($ENV{DXSDK_DIR}/Include)

    set(REN_ADAPTER_SRC ${REN_ADAPTER_SRC}
            src/adapters/adapter_dx11.cpp
            src/buffers/vb_dx11.cpp
            src/buffers/ib_dx11.cpp
    )
endif()

if (REN_DEBUG)
    add_compile_definitions(REN_DEBUG)
endif()

add_library(renderer STATIC
        src/renderer.cpp
        src/layer.cpp
        src/buffers/cmd_buffer.cpp
        src/types/texture.cpp
        src/types/animated_texture.cpp
        src/types/font.cpp
        src/types/shader.cpp
        ${REN_ADAPTER_SRC}
)