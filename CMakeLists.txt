cmake_minimum_required(VERSION 3.22.1)
project(Api)
set(CROW_FEATURES ssl)


# Link against the pqxx library
add_executable(${PROJECT_NAME} main.cpp)


