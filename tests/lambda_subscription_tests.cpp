#include "doctest.h"

#include "subscriptions/LambdaSubscription.h"

TEST_SUITE("LambdaSubscription") {

    TEST_CASE ("Constructor")
    {
        REQUIRE_NOTHROW(LambdaSubscription());
    }

    TEST_CASE ("NotifyAll")
    {
        LambdaSubscription subscription;
        int invoke_count = 0;
        auto callback = [&]() { ++invoke_count; };

        SUBCASE("notifyAll does nothing for empty class") {
            REQUIRE_NOTHROW(subscription.notifyAll());
        }

        SUBCASE("notifyAll for one callback") {
            auto disposable = subscription.subscribe(callback);
            subscription.notifyAll();
            REQUIRE_EQ(1, invoke_count);
        }

        SUBCASE("notifyAll for two callbacks") {
            auto disposable1 = subscription.subscribe(callback);
            auto disposable2 = subscription.subscribe(callback);
            subscription.notifyAll();
            REQUIRE_EQ(2, invoke_count);
        };

        SUBCASE("notifyAll for unsubscribed callback") {
            SUBCASE("if only one subscriber") {
                {
                    auto disposable = subscription.subscribe(callback);
                }
                subscription.notifyAll();
                REQUIRE_EQ(0, invoke_count);
            };
            SUBCASE("if only one subscriber") {
                auto disposable = subscription.subscribe(callback);
                disposable.unsubscribe();
                subscription.notifyAll();
                REQUIRE_EQ(0, invoke_count);
            };

            SUBCASE("if the first") {
                auto disposable1 = subscription.subscribe(callback);
                auto disposable2 = subscription.subscribe(callback);
                auto disposable3 = subscription.subscribe(callback);
                disposable1.unsubscribe();
                subscription.notifyAll();
                REQUIRE_EQ(2, invoke_count);
            };

            SUBCASE("if in the middle") {
                auto disposable1 = subscription.subscribe(callback);
                auto disposable2 = subscription.subscribe(callback);
                auto disposable3 = subscription.subscribe(callback);
                disposable2.unsubscribe();
                subscription.notifyAll();
                REQUIRE_EQ(2, invoke_count);
            };

            SUBCASE("if the last") {
                auto disposable1 = subscription.subscribe(callback);
                auto disposable2 = subscription.subscribe(callback);
                auto disposable3 = subscription.subscribe(callback);
                disposable3.unsubscribe();
                subscription.notifyAll();
                REQUIRE_EQ(2, invoke_count);
            };

        };

        SUBCASE("unsubscribtion  during a call") {
            LambdaSubscription::Unsubscriber disposable;
            auto auto_dispose_callback = [&]()
            {
                ++invoke_count;
                disposable.unsubscribe();
            };
            SUBCASE("if oneself") {
                disposable = subscription.subscribe(auto_dispose_callback);
                subscription.notifyAll();
                REQUIRE_EQ(1, invoke_count);
                subscription.notifyAll();
                REQUIRE_EQ(1, invoke_count);
            }

            SUBCASE("if previous") {
                disposable = subscription.subscribe(callback);
                auto disposable2 = subscription.subscribe(auto_dispose_callback);
                subscription.notifyAll();
                REQUIRE_EQ(2, invoke_count);
                subscription.notifyAll();
                REQUIRE_EQ(3, invoke_count);
            }

            SUBCASE("if after") {
                auto disposable1 = subscription.subscribe(auto_dispose_callback);
                disposable = subscription.subscribe(callback);
                subscription.notifyAll();
                REQUIRE_EQ(1, invoke_count);
                subscription.notifyAll();
                REQUIRE_EQ(2, invoke_count);
            }
        };
    }

    TEST_CASE ("Unsubscribe")
    {
        SUBCASE("Unsubscribe twice") {
            LambdaSubscription subscription;
            auto disposable = subscription.subscribe([]() {});
            disposable.unsubscribe();
            disposable.unsubscribe();
        }

        SUBCASE("Unsubscribe when subscription has passed away") {
            LambdaSubscription::Unsubscriber disposable;
            {
                LambdaSubscription subscription;
                disposable = subscription.subscribe([]() {});
            }
            disposable.unsubscribe();
        }
    }

    TEST_CASE("Subscribe when notify"){
        LambdaSubscription subscription;
        SUBCASE("Force to realloc of subscriptors in the middle of process"){
            int counter = 0;
            const int addedCount = 20;
            std::vector<LambdaSubscription::Unsubscriber> disposable;
            auto callback = [&](){ ++counter; };
            auto callbackWhichMakesSubscription = [&](){
                ++counter;
                for(int i=0; i<addedCount; ++i)
                    disposable.push_back(subscription.subscribe(callback));
            };
            disposable.push_back(subscription.subscribe(callbackWhichMakesSubscription));
            disposable.push_back(subscription.subscribe(callback));
            int expectedCount = disposable.size();
            subscription.notifyAll();
            CHECK_EQ(expectedCount, counter);
            REQUIRE_EQ(expectedCount + addedCount, disposable.size());
        }
    }
}