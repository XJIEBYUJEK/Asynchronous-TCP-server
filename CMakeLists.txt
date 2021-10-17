cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(server LANGUAGES CXX)

add_executable(${PROJECT_NAME} main.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)
set(Boost_INCLUDE_DIR /home/igor/boost_1_77_0/path/include/)
set(Boost_LIBRARY_DIR /home/igor/boost_1_77_0/path/lib/)
find_package(Boost 1.77.0 REQUIRED COMPONENTS system thread)

target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system Boost::thread)
