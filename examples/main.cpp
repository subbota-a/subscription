#include "subscriptions/LambdaSubscription.h"
#include <string>
#include <iostream>

// casual listener interface
struct IObserver
{
    virtual void onPropertyChanged() = 0;
};

// example of observable with mixed type of subscription
class Observable
{
public:
    template<class Func>
    [[nodiscard]] LambdaSubscription::Unsubscriber subscribeOnMyProperty(Func func)
    {
        return subscription.subscribe(func);
    }

    [[nodiscard]] LambdaSubscription::Unsubscriber subscribeOnRawPointer(IObserver *observer)
    {
        return subscription.subscribe([observer]() { observer->onPropertyChanged(); });
    }

    [[nodiscard]] LambdaSubscription::Unsubscriber subscribeOnWeakPtr(std::weak_ptr<IObserver> weak_observer)
    {
        return subscription.subscribe(
                [weak_observer = std::move(weak_observer)]()
                {
                    if (auto lock = weak_observer.lock())
                        lock->onPropertyChanged();
                });
    }

    int myProperty() const { return myProperty_; }

    void setMyProperty(int value)
    {
        if (value != myProperty_) {
            myProperty_ = value;
            notify();
        }
    }

    void notify()
    {
        subscription.notifyAll();
    }

private:
    LambdaSubscription subscription;
    int myProperty_ = 0;
};

class ObserverByWeakPtr : public std::enable_shared_from_this<ObserverByWeakPtr>, public IObserver
{
public:
    ObserverByWeakPtr(Observable &observable)
            : observable_(observable)
    {
    }

    void init()
    {
        disposable = observable_.subscribeOnWeakPtr(weak_from_this());
    }

private:
    void onPropertyChanged() override
    {
        std::cout << "ObserverByWeakPtr.onPropertyChanged(" << observable_.myProperty() << ")\n";
    }

private:
    Observable &observable_;
    LambdaSubscription::Unsubscriber disposable;
};

class ObserverByRawInterfacePointer : public IObserver
{
public:
    ObserverByRawInterfacePointer(Observable &observable)
            : observable_(observable)
    {
        disposable = observable.subscribeOnRawPointer(this);
    }

private:
    void onPropertyChanged() override
    {
        std::cout << "ObserverByRawInterfacePointer.onPropertyChanged(" << observable_.myProperty() << ")\n";
    }

private:
    Observable &observable_;
    LambdaSubscription::Unsubscriber disposable;
};

class ObserverByLambda
{
public:
    ObserverByLambda(Observable &observable)
            : observable_(observable)
    {
        disposable = observable_.subscribeOnMyProperty([this]() { onPropertyChanged(); });
    }

private:
    void onPropertyChanged()
    {
        std::cout << "ObserverByLambda.onPropertyChanged(" << observable_.myProperty() << ")\n";
    }

private:
    Observable &observable_;
    LambdaSubscription::Unsubscriber disposable;
};

int main()
{
    Observable observable;
    {
        auto observerByWeakPtr = std::make_shared<ObserverByWeakPtr>(observable);
        observerByWeakPtr->init();

        ObserverByRawInterfacePointer observerByRawInterfacePointer(observable);

        ObserverByLambda observerByLambda(observable);

        observable.setMyProperty(10);
    }
    observable.setMyProperty(30);

    return 0;
}
