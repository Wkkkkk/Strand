# set minimum cmake version
cmake_minimum_required(VERSION 3.14.0)

# project name and language
project(Strand VERSION 0.1.0 LANGUAGES CXX C)

# require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FetchContent added in CMake 3.11, downloads during the configure step
# FetchContent_MakeAvailable was added in CMake 3.14; simpler usage
include(FetchContent)

# enable testing functionality
include(CTest)
enable_testing()

include_directories(
    include
    external/Remotery/lib)

# main code
add_executable(Strand 
    src/Semaphore.cpp
    external/Remotery/lib/Remotery.c
    main.cpp
)

# The testing code is here
add_subdirectory(tests)
