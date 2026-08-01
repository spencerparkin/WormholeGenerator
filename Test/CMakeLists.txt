# CMakeLists.txt for program to test WormholeGenerator

set(TEST_SOURCES
	Source/App.cpp
	Source/App.h
	Source/Frame.cpp
	Source/Frame.h
	Source/Canvas.cpp
	Source/Canvas.h
)

add_executable(TestApp WIN32
	${TEST_SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/wx_dpi_aware_pmv2.manifest
)

set(wxBUILD_SHARED OFF)

include(FetchContent)

FetchContent_Declare(
	wxWidgets
	GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
	GIT_TAG 3.3-release
)

FetchContent_MakeAvailable(wxWidgets)

target_compile_definitions(TestApp PUBLIC
	__WXMSW__
	_UNICODE
)

target_compile_definitions(TestApp PRIVATE
	WIN32_LEAN_AND_MEAN
	NOMINMAX
	_USE_MATH_DEFINES
	_CRT_SECURE_NO_WARNINGS
)

target_link_libraries(TestApp
	WormholeGenerator
	wx::core
	wx::base
	wx::gl
)