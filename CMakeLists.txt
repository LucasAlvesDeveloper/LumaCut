cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(VideoEditor)

find_package(PkgConfig REQUIRED)
pkg_check_modules(
	FFMPEG REQUIRED IMPORTED_TARGET
	libavdevice
	libavfilter
	libavformat
	libavcodec
	libswresample
	libswscale
	libavutil
)

add_subdirectory(vendor)

# add_subdirectory(libraries/std)

add_executable(
	VideoEditor
	src/main.c
)

target_compile_features(VideoEditor PRIVATE c_std_99)
# target_include_directories(VideoEditor PRIVATE )
set(IS_MSVC $<CXX_COMPILER_ID:MSVC>)
set(MSVC_WARNING_FLAGS $<${IS_MSVC}:/W4;/WX>)
set(GNU_WARNING_FLAGS $<$<NOT:${IS_MSVC}>:-Wall;-Wextra;-Wpedantic>)

target_compile_options(VideoEditor PRIVATE
	$<$<CONFIG:Debug>:${MSVC_WARNING_FLAGS};${GNU_WARNING_FLAGS}>
)
target_link_libraries(
	VideoEditor PRIVATE
	# StandardLibrary

	PkgConfig::FFMPEG
	SDL3::SDL3
	# SDL3_image::SDL3_image
	# SDL3_ttf::SDL3_ttf
	# clay
)
