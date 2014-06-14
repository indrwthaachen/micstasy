cmake_minimum_required(VERSION 2.8)

project(micstasyc)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(PortMidi REQUIRED)
include_directories(${PortMidi_INCLUDE_DIRS})


message(${PortMidi_INCLUDE_DIRS})
add_library(micstasyc STATIC micstasyc.c)
SET_TARGET_PROPERTIES(micstasyc PROPERTIES COMPILE_FLAGS -fPIC)

install(TARGETS micstasyc DESTINATION lib)
install(FILES micstasyc.h DESTINATION include)


target_link_libraries(micstasyc portmidi)


