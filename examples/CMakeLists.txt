include_directories(../doctest ..)

add_executable(example main.cpp)

target_link_libraries(example lambda_subscription)