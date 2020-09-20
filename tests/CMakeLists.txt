include_directories(
        ../doctest
        ../fakeit
        ..)

add_executable(subscriptions_test main.cpp lambda_subscription_tests.cpp classic_subscription_tests.cpp)
target_link_libraries(subscriptions_test subscriptions)