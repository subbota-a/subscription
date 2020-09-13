#pragma once

#include <memory>
#include <vector>
#include <type_traits>

class LambdaSubscription final
{
    // Lambda type erase which can be identified
    class IdentityCallable
    {
    public:
        class Identity
        {
        public:
            Identity()
                    : pointer_(nullptr) {}

            Identity(void *pointer)
                    : pointer_(pointer) {}

            bool operator==(const Identity &other) const { return pointer_ == other.pointer_; }
            bool operator!=(const Identity &other) const { return !(*this == other); }

        private:
            void *pointer_;
        };

        template<class Func>
        explicit IdentityCallable(Func func)
                : invoker_(new Derived<Func>(std::move(func)))
        {
            static_assert(std::is_invocable<Func>::value, "Only callable type allowed");
        }

        void operator()() const
        {
            if (invoker_)
                invoker_->call();
        }

        void release()
        {
            invoker_.reset();
        }

        [[nodiscard]] Identity identity() const { return invoker_.get(); }

    private:
        struct Base
        {
            virtual ~Base() = default;

            virtual void call() const = 0;
        };

        template<class Func>
        struct Derived : Base
        {
            const Func func_;

            explicit Derived(Func func)
                    : func_(func) {}

            void call() const override
            {
                std::invoke(func_);
            }
        };

    private:
        std::unique_ptr<Base> invoker_;
    };

public:
    class Unsubscriber final
    {
    public:
        Unsubscriber()
                : identity_(nullptr) {}

        ~Unsubscriber()
        {
            unsubscribe();
        }

        Unsubscriber(const Unsubscriber &) = delete;

        Unsubscriber &operator=(const Unsubscriber &) = delete;

        Unsubscriber(Unsubscriber &&other) noexcept
                : identity_(nullptr)
        {
            swap(*this, other);
        }

        Unsubscriber &operator=(Unsubscriber &&other) noexcept
        {
            unsubscribe();
            swap(*this, other);
            return *this;
        }

        void unsubscribe() noexcept
        {
            if (auto lock = callbacks_.lock()) {
                auto it = std::find_if(
                        lock->begin(), lock->end(),
                        [id = identity_](IdentityCallable &c) { return c.identity() == id; });
                assert(it != lock->end());
                it->release();
            }
            callbacks_.reset();
            identity_ = nullptr;
        }

        friend void swap(Unsubscriber &a, Unsubscriber &b)
        {
            std::swap(a.callbacks_, b.callbacks_);
            std::swap(a.identity_, b.identity_);
        }

    private:
        friend class LambdaSubscription;

        Unsubscriber(
                std::weak_ptr<std::vector<IdentityCallable>> callbacks, IdentityCallable::Identity identity);

        std::weak_ptr<std::vector<IdentityCallable>> callbacks_;
        IdentityCallable::Identity identity_;
    };

    template<class Callback>
    [[nodiscard]] Unsubscriber subscribe(Callback callback)
    {
        IdentityCallable &c = callbacks_->emplace_back(std::move(callback));
        return Unsubscriber(callbacks_, c.identity());
    }

    void notifyAll();

private:
    std::shared_ptr<std::vector<IdentityCallable>> callbacks_ = std::make_shared<std::vector<IdentityCallable>>();
};
