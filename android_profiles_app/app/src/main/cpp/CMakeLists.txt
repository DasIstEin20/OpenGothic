cmake_minimum_required(VERSION 3.22)
project(inputmapper)

add_library(inputmapper SHARED
    ../../../../input/InputMapperJNI.cpp
    ../../../../input/InputMapper.cpp
    ../../../../input/InputProfile.cpp)

target_include_directories(inputmapper PRIVATE
    ../../../../input
    ../../../../external/regoth_legacy/lib/json)

find_library(log-lib log)

target_link_libraries(inputmapper ${log-lib})
