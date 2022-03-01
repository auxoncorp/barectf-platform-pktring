cmake_minimum_required(VERSION 3.5)

project(example LANGUAGES C)

if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 99)
endif()

set(ASAN ON CACHE BOOL "Use address sanitzer")

function(append value)
  foreach(variable ${ARGN})
    set(${variable}
        "${${variable}} ${value}"
        PARENT_SCOPE)
  endforeach(variable)
endfunction()

if(ASAN)
    message(STATUS "Building with ASAN")
    append("-fno-omit-frame-pointer" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    if(uppercase_CMAKE_BUILD_TYPE STREQUAL "DEBUG")
      append("-O1" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    endif()
    append("-fsanitize=address" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
endif()

enable_testing()
add_subdirectory(tests)
