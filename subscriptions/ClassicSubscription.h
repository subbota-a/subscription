#pragma once

#include <vector>

class ClassicSubscriptionBase
{
public:
    ClassicSubscriptionBase()
            : subscribers_(std::make_shared<std::vector<void*>>()) {}

    class Disposable
    {
    public:
        void dispose() noexcept;

        ~Disposable()
        {
            dispose();
        }

        Disposable(const Disposable&) = delete;

        Disposable& operator=(const Disposable&) = delete;

        Disposable(Disposable&& other) noexcept
        {
            swap(*this, other);
        }

        Disposable& operator=(Disposable&& other) noexcept
        {
            Disposable disposable(std::move(other));
            swap(*this, disposable);
            return *this;
        }

        friend void swap(Disposable& a, Disposable& b) noexcept
        {
            std::swap(a.subscribers_, b.subscribers_);
            std::swap(a.pointer_, b.pointer_);
        }

    private:
        friend class ClassicSubscriptionBase;

        explicit Disposable(std::weak_ptr<std::vector<void*>> subscribers, void* pointer);

    private:
        std::weak_ptr<std::vector<void*>> subscribers_;
        void* pointer_ = nullptr;
    };

protected:
    [[nodiscard]] Disposable subscribe(void* p);

    void clean_released();

protected:
    std::shared_ptr<std::vector<void*>> subscribers_;
};

template<class Interface>
class ClassicSubscription final : public ClassicSubscriptionBase
{
public:
    [[nodiscard]] Disposable subscribe(Interface* anInterface)
    {
        return ClassicSubscriptionBase::subscribe(anInterface);
    }

    template<typename ...Args>
    void notifyAll(void (Interface::* member)(Args...), Args...args)
    {
        const auto size = subscribers_->size();
        for (size_t i = 0; i < size; ++i) {
            Interface* anInterface = static_cast<Interface*>(subscribers_->at(i));
            if (anInterface)
                (anInterface->*member)(args...);
        }
        clean_released();
    }
};

