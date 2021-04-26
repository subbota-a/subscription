#pragma once
#include "disposable.h"

#include <memory>
#include <type_traits>
#include <vector>

namespace subscriptions {

class LambdaSubscription final {
  // Lambda type erase which can be identified
  class IdentityCallable {
  public:
    class Identity {
    public:
      Identity() : pointer_(nullptr) {}

      explicit Identity(void* pointer) : pointer_(pointer) {}

      bool operator==(const Identity& other) const { return pointer_ == other.pointer_; }

      bool operator!=(const Identity& other) const { return !(*this == other); }

    private:
      void* pointer_;
    };

    template <class Func>
    explicit IdentityCallable(Func func) : invoker_(new Derived<Func>(std::move(func)))
    {
      static_assert(std::is_invocable<Func>::value, "Only callable type allowed");
    }

    void operator()() const
    {
      if (invoker_)
        invoker_->call();
    }

    void release() { invoker_.reset(); }

    [[nodiscard]] Identity identity() const { return Identity(invoker_.get()); }

  private:
    struct Base {
      virtual ~Base() = default;

      virtual void call() const = 0;
    };

    template <class Func>
    struct Derived : Base {
      const Func func_;

      explicit Derived(Func func) : func_(func) {}

      void call() const override { std::invoke(func_); }
    };

  private:
    std::unique_ptr<Base> invoker_;
  };

  class DisposableImpl final : public internal::Disposable {
  public:
    DisposableImpl(
        std::weak_ptr<std::vector<IdentityCallable>> callbacks,
        IdentityCallable::Identity identity);
    ~DisposableImpl() override { dispose(); }

  private:
    void dispose() noexcept;

    friend class LambdaSubscription;

    std::weak_ptr<std::vector<IdentityCallable>> callbacks_;
    IdentityCallable::Identity identity_;
  };

public:
  template <class Callback>
  [[nodiscard]] Disposable subscribe(Callback callback)
  {
    IdentityCallable& c = callbacks_->emplace_back(std::move(callback));
    return Disposable(std::make_unique<DisposableImpl>(callbacks_, c.identity()));
  }

  void notifyAll();

private:
  std::shared_ptr<std::vector<IdentityCallable>> callbacks_ =
      std::make_shared<std::vector<IdentityCallable>>();
};

}  // namespace yandex::maps::navikit::subscriptions