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

            explicit Identity(void* pointer)
                    : pointer_(pointer) {}

            bool operator==(const Identity& other) const { return pointer_ == other.pointer_; }

            bool operator!=(const Identity& other) const { return !(*this == other); }

        private:
            void* pointer_;
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

        [[nodiscard]] Identity identity() const { return Identity(invoker_.get()); }

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
    class Disposable final
    {
    public:
        Disposable()
                : identity_(nullptr) {}

        ~Disposable()
        {
            dispose();
        }

        Disposable(const Disposable&) = delete;

        Disposable& operator=(const Disposable&) = delete;

        Disposable(Disposable&& other) noexcept
                : identity_(nullptr)
        {
            swap(*this, other);
        }

        Disposable& operator=(Disposable&& other) noexcept
        {
            Disposable disposable(std::move(other));
            swap(*this, disposable);
            return *this;
        }

        void dispose() noexcept;

        friend void swap(Disposable& a, Disposable& b)
        {
            std::swap(a.callbacks_, b.callbacks_);
            std::swap(a.identity_, b.identity_);
        }

    private:
        friend class LambdaSubscription;

        Disposable(
                std::weak_ptr<std::vector<IdentityCallable>> callbacks, IdentityCallable::Identity identity);

        std::weak_ptr<std::vector<IdentityCallable>> callbacks_;
        IdentityCallable::Identity identity_;
    };

    template<class Callback>
    [[nodiscard]] Disposable subscribe(Callback callback)
    {
        IdentityCallable& c = callbacks_->emplace_back(std::move(callback));
        return Disposable(callbacks_, c.identity());
    }

    void notifyAll();

private:
    std::shared_ptr<std::vector<IdentityCallable>> callbacks_ = std::make_shared<std::vector<IdentityCallable>>();
};
