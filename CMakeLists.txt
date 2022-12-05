cmake_minimum_required (VERSION 3.19 FATAL_ERROR)

project(aqualink-automate VERSION 0.0.1 LANGUAGE CXX)

add_executable (aqualink-automate src/aqualink-automate.cpp)
target_compile_features(aqualink-automate PRIVATE cxx_std_20)
