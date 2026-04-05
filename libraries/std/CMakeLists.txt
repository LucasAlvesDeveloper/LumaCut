cmake_minimum_required(VERSION 3.30)

project(StandardLibrary C)

add_library(
	StandardLibrary STATIC
	src/null.c
)

target_include_directories(
	StandardLibrary
   	PRIVATE src
    PUBLIC include
)
