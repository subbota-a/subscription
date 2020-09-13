#include "subscriptions/LambdaSubscription.h"
#include <string>
#include <iostream>

// casual listener interface
struct IObserver {
    virtual void onPropertyChanged() = 0;
};

// example of observable with mixed type of subscription
class Observable {
public:
    template<class Func>
    [[nodiscard]] LambdaSubscription::Unsubscriber subscribeOnMyProperty(Func func) {
        return subscription.subscribe(func);
    }

    [[nodiscard]] LambdaSubscription::Unsubscriber subscribe(IObserver *observer) {
        return subscription.subscribe([observer]() { observer->onPropertyChanged(); });
    }

    int myProperty() const { return myProperty_; }

    void setMyProperty(int value) {
        if (value != myProperty_) {
            myProperty_ = value;
            notify();
        }
    }

    void notify() {
        subscription.notifyAll();
    }

private:
    LambdaSubscription subscription;
    int myProperty_ = 0;
};

class Observer : IObserver {
public:
    Observer(Observable &observable, std::string name)
            : observable_(observable), name_(std::move(name)) {
        unsubscribers.push_back(observable_.subscribeOnMyProperty([this]() { nonVirtualCallback(); }));
        unsubscribers.push_back(observable.subscribe(this));
    }

    static std::vector<std::unique_ptr<Observer>> moreObservers;
private:
    void onPropertyChanged() override {
        std::cout << name_ << ".interfaceCallback(" << observable_.myProperty() << ")\n";
    }

    void nonVirtualCallback() {
        std::cout << name_ << ".lambdaCallback(" << observable_.myProperty() << ")\n";
    }

private:
    Observable &observable_;
    std::string name_;
    std::vector<LambdaSubscription::Unsubscriber> unsubscribers;
};

int main() {
    Observable observable;
    {
        Observer observer(observable, "name");
        observable.setMyProperty(10);
    }
    observable.setMyProperty(30); // nothing bad happened

    return 0;
}
