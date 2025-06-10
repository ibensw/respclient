cmake_minimum_required(VERSION 3.15)

# Set project name and C++ standard
project(respclient LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Enable tests" ON)
option(CLANG_TIDY "Enable clang-tidy" ON)

if (${CLANG_TIDY})
    set(CMAKE_CXX_CLANG_TIDY clang-tidy)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address,undefined -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

add_library(respc SHARED
    src/respc/parser.cpp
    src/respc/syncexecutor.cpp
    src/respc/connection.cpp
    src/respc/asyncexecutor.cpp
)
target_compile_options(respc PRIVATE
    -Wall -Wextra -Werror -pedantic -Wshadow -Wnon-virtual-dtor
    -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual
    -Wmisleading-indentation -Wnull-dereference)

target_include_directories(respc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)
target_include_directories(respc
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/respc>
)
find_package(fmt REQUIRED)
target_link_libraries(respc fmt::fmt)

add_executable(${PROJECT_NAME}
    src/main.cpp
)
target_link_libraries(${PROJECT_NAME} respc)


# ## Install steps
# Install library
install(TARGETS respc
        EXPORT respcTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include)

# Install public headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")

install(EXPORT respcTargets
        FILE respcTargets.cmake
        NAMESPACE respc::
        DESTINATION lib/cmake/respc)
