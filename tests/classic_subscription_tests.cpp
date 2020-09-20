#include "subscriptions/ClassicSubscription.h"

#include "doctest.h"
#include "fakeit.hpp"

#include <vector>

struct IManyPropertiesListener
{
    virtual ~IManyPropertiesListener() = default;

    virtual void onXChanged() = 0;

    virtual void onYChanged() = 0;
};

using namespace fakeit;

TEST_SUITE ("ClassicSubscription") {
    TEST_CASE ("Empty constructor")
    {
        ClassicSubscription<IManyPropertiesListener> subscription;
    }

    TEST_CASE("Notification without listeners"){
        ClassicSubscription<IManyPropertiesListener> subscription;

        subscription.notifyAll(&IManyPropertiesListener::onXChanged);
        subscription.notifyAll(&IManyPropertiesListener::onYChanged);
    }

    TEST_CASE("Subscription")
    {
        ClassicSubscription<IManyPropertiesListener> subscription;
        Mock<IManyPropertiesListener> listener;
        auto disposable = subscription.subscribe(&listener.get());

        SUBCASE("Subscription twice is not allowed") {
            REQUIRE_THROWS(subscription.subscribe(&listener.get()));
        }

        SUBCASE("Subscription on nullptr is not allowed") {
            REQUIRE_THROWS(subscription.subscribe(nullptr));
        }

        SUBCASE("Implicit unsubscription") {
            {
                auto d = std::move(disposable);
            }
            subscription.notifyAll(&IManyPropertiesListener::onXChanged);
        }

        SUBCASE("Explicit unsubscription") {
            disposable.dispose();
            subscription.notifyAll(&IManyPropertiesListener::onXChanged);
        }
    }

    TEST_CASE("Notification")
    {
        ClassicSubscription<IManyPropertiesListener> subscription;
        std::vector<Mock<IManyPropertiesListener>> listeners(3);
        std::vector<ClassicSubscriptionBase::Disposable> disposables;
        for(auto& mock : listeners)
            disposables.push_back(subscription.subscribe(&mock.get()));

        SUBCASE ("Notification on a first member")
        {
            for(auto& mock : listeners){
                Fake(Method(mock, onXChanged));
            }

            subscription.notifyAll(&IManyPropertiesListener::onXChanged);

            for(auto& mock : listeners){
                Verify(Method(mock, onXChanged)).Once();
            }
        }

        SUBCASE ("Notification on second member")
        {
            for(auto& mock : listeners){
                Fake(Method(mock, onYChanged));
            }

            subscription.notifyAll(&IManyPropertiesListener::onYChanged);

            for(auto& mock : listeners){
                Verify(Method(mock, onYChanged)).Once();
            }
        }

        SUBCASE("Add listener in the middle of notification"){
            std::vector<Mock<IManyPropertiesListener>> new_listener(listeners.size());
            int new_listener_counter = 0;
            std::vector<ClassicSubscriptionBase::Disposable> new_disposables;

            for(auto& mock : listeners){
                When(Method(mock, onYChanged)).AlwaysDo([&](){
                    new_disposables.push_back(subscription.subscribe(&new_listener[new_listener_counter++].get()));
                });
            }

            subscription.notifyAll(&IManyPropertiesListener::onYChanged);

            for(auto& listener : listeners)
                CHECK_NOTHROW(Verify(Method(listener, onYChanged)).Once());

            REQUIRE_EQ(listeners.size(), new_listener.size());

            for(auto& listener : new_listener){
                CHECK_NOTHROW(VerifyNoOtherInvocations(listener));
            }
        }
        SUBCASE("Remove listener in the middle of notification"){
            std::vector<Mock<IManyPropertiesListener>> new_listener(listeners.size());
            int new_listener_counter = 0;
            std::vector<ClassicSubscriptionBase::Disposable> new_disposables;

            for(size_t i=0; i< listeners.size(); ++i){
                When(Method(listeners[i], onYChanged))
                .AlwaysDo([&](){
                    disposables.clear();
                });
            }

            subscription.notifyAll(&IManyPropertiesListener::onYChanged);

            for(size_t i=0; i<listeners.size(); ++i)
                if (i == 0)
                    CHECK_NOTHROW(Verify(Method(listeners[i], onYChanged)).Once());
                else
                    CHECK_NOTHROW(VerifyNoOtherInvocations(listeners[i]));
        }
    }
}