# CMakeLists.txt for WormholeGenerator C++ library

set(WORMHOLE_GENERATOR_LIBRARY_SOURCES
	Source/WormholeGenerator/WormholeTree.cpp
	Source/WormholeGenerator/WormholeTree.h
	Source/WormholeGenerator/WormholeRenderer.cpp
	Source/WormholeGenerator/WormholeRenderer.h
)

include(FetchContent)

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.8
	SOURCE_SUBDIR cmake
)

FetchContent_MakeAvailable(glad)

glad_add_library(glad_gl_core_45
	REPRODUCIBLE
	API gl:core=4.5
)

add_library(WormholeGenerator STATIC ${WORMHOLE_GENERATOR_LIBRARY_SOURCES})

target_compile_definitions(WormholeGenerator PRIVATE
    _USE_MATH_DEFINES
)

target_link_libraries(WormholeGenerator
	PUBLIC
	HappyMath
	glad_gl_core_45
)

target_include_directories(WormholeGenerator PUBLIC Source)