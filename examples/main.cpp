#include "subscriptions/LambdaSubscription.h"
#include "subscriptions/ClassicSubscription.h"
#include <string>
#include <iostream>

// casual listener interface
struct IOnePropertyListener
{
    virtual ~IOnePropertyListener() = default;

    virtual void onPropertyChanged() = 0;
};

struct IManyPropertiesListener{
    virtual ~IManyPropertiesListener() = default;

    virtual void onAppleChanged() = 0;
    virtual void onPearChanged() = 0;
};

// example of observable with mixed type of subscription
class Provider
{
public:
    template<class Func>
    [[nodiscard]] LambdaSubscription::Disposable subscribeOnMyPropertyByLambda(Func func)
    {
        return subscription.subscribe(func);
    }

    [[nodiscard]] LambdaSubscription::Disposable subscribeOnMyPropertyByRawPointer(IOnePropertyListener *observer)
    {
        return subscription.subscribe([observer]() { observer->onPropertyChanged(); });
    }

    [[nodiscard]] LambdaSubscription::Disposable subscribeOnMyPropertyByWeakPtr(std::weak_ptr<IOnePropertyListener> weak_observer)
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
            notifyMyPropertyChanged();
        }
    }

    void notifyMyPropertyChanged()
    {
        subscription.notifyAll();
    }

    int apple() const { return apple_; }
    void setApple(int value) {
        apple_ = value;
        notifyAppleChanged();
    }

    int pear() const { return pear_; }
    void setPear(int value) {
        pear_ = value;
        notifyPearChanged();
    }

    ClassicSubscription<IManyPropertiesListener>::Disposable subscribeOnManyProperties(IManyPropertiesListener* listener){
        return classicSubscription.subscribe(listener);
    }
private:
    void notifyAppleChanged(){
        classicSubscription.notifyAll(&IManyPropertiesListener::onAppleChanged);
    }
    void notifyPearChanged(){
        classicSubscription.notifyAll(&IManyPropertiesListener::onPearChanged);
    }
private:
    LambdaSubscription subscription;
    ClassicSubscription<IManyPropertiesListener> classicSubscription;
    int myProperty_ = 0;
    int apple_ = 0;
    int pear_ = 0;
};

class ObserverByWeakPtr : public std::enable_shared_from_this<ObserverByWeakPtr>, public IOnePropertyListener
{
public:
    ObserverByWeakPtr(Provider &observable)
            : observable_(observable)
    {
    }

    void init()
    {
        disposable = observable_.subscribeOnMyPropertyByWeakPtr(weak_from_this());
    }

private:
    void onPropertyChanged() override
    {
        std::cout << "ObserverByWeakPtr.onPropertyChanged(" << observable_.myProperty() << ")\n";
    }

private:
    Provider &observable_;
    LambdaSubscription::Disposable disposable;
};

class ObserverByRawInterfacePointer : public IOnePropertyListener
{
public:
    ObserverByRawInterfacePointer(Provider &observable)
        : observable_(observable)
    {
        disposable = observable.subscribeOnMyPropertyByRawPointer(this);
    }

private:
    void onPropertyChanged() override
    {
        std::cout << "ObserverByRawInterfacePointer.onPropertyChanged(" << observable_.myProperty() << ")\n";
    }

private:
    Provider &observable_;
    LambdaSubscription::Disposable disposable;
};

class ObserverByLambda
{
public:
    ObserverByLambda(Provider &observable)
            : observable_(observable)
    {
        disposable = observable_.subscribeOnMyPropertyByLambda([this]() { onPropertyChanged(); });
    }

private:
    void onPropertyChanged()
    {
        std::cout << "ObserverByLambda.onPropertyChanged(" << observable_.myProperty() << ")\n";
    }

private:
    Provider &observable_;
    LambdaSubscription::Disposable disposable;
};

class ManyPropertiesListener : public IManyPropertiesListener{
public:
    ManyPropertiesListener(Provider& provider)
        : provider_(provider)
        , disposable_(provider.subscribeOnManyProperties(this))
    {}
    void onAppleChanged() override{
        std::cout << "onAppleChanged(" << provider_.apple() << ")\n";
    }
    void onPearChanged() override{
        std::cout << "onPearChanged(" << provider_.pear() << ")\n";
    }

private:
    Provider& provider_;
    ClassicSubscription<IManyPropertiesListener>::Disposable disposable_;
};

int main()
{
    Provider provider;
    {
        auto observerByWeakPtr = std::make_shared<ObserverByWeakPtr>(provider);
        observerByWeakPtr->init();

        ObserverByRawInterfacePointer observerByRawInterfacePointer(provider);

        ObserverByLambda observerByLambda(provider);

        ManyPropertiesListener manyPropertiesListener(provider);

        provider.setMyProperty(10);
        provider.setApple(15);
        provider.setPear(20);
    }
    provider.setMyProperty(25);
    provider.setApple(30);
    provider.setPear(35);

    return 0;
}
