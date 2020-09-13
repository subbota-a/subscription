include_directories(../doctest ..)

add_executable(subscriptions_test main.cpp lambda_subscription_tests.cpp)
target_link_libraries(subscriptions_test lambda_subscription)