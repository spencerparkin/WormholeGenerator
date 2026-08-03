# CMakeLists.txt for WormholeGenerator C++ library

set(WORMHOLE_GENERATOR_LIBRARY_SOURCES
	Source/WormholeGenerator/WormholeTree.cpp
	Source/WormholeGenerator/WormholeTree.h
)

add_library(WormholeGenerator STATIC ${WORMHOLE_GENERATOR_LIBRARY_SOURCES})

target_compile_definitions(WormholeGenerator PRIVATE
    _USE_MATH_DEFINES
)

target_link_libraries(WormholeGenerator PUBLIC HappyMath)

target_include_directories(WormholeGenerator PUBLIC Source)