cmake_minimum_required(VERSION 3.6)
project(Template)

message("Source " "${CMAKE_SOURCE_DIR}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(LIB "${CMAKE_SOURCE_DIR}/lib")
set(INCLUDE "${CMAKE_SOURCE_DIR}/include")

add_definitions(-DGLEW_STATIC -D_USE_MATH_DEFINES -DPERF)

find_package(OpenGL REQUIRED)

link_directories(${LIB})
include_directories(${OPENGL_INCLUDE_DIRS})
include_directories(${INCLUDE})

set(SOURCE_FILES main.cpp gl_includes.h legendre.h Perf.h Perf.cpp stb_image_impl.cpp)
add_executable(Template ${SOURCE_FILES})

target_link_libraries(Template ${LIB}/libglew32.a)
target_link_libraries(Template ${LIB}/libglfw3.a)
target_link_libraries(Template ${OPENGL_LIBRARIES})
target_link_libraries(Template -static-libgcc -static-libstdc++)

file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}/)
