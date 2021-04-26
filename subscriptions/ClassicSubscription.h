#pragma once
#include "disposable.h"
#include <vector>

namespace subscriptions {

namespace internal {
class ClassicSubscriptionBase {
public:
  ClassicSubscriptionBase()
      : subscribers_(std::make_shared<std::vector<void *>>()) {}

  class DisposableImpl final : public Disposable {
  public:
    DisposableImpl(std::weak_ptr<std::vector<void *>> subscribers,
                   void *pointer);

    ~DisposableImpl() override { dispose(); }

  private:
    void dispose() noexcept;
    std::weak_ptr<std::vector<void *>> subscribers_;
    void *pointer_ = nullptr;
  };

protected:
  [[nodiscard]] subscriptions::Disposable subscribe(void *p);

  void clean_released();

protected:
  std::shared_ptr<std::vector<void *>> subscribers_;
};
}

template <class Interface>
class ClassicSubscription final : public internal::ClassicSubscriptionBase {
public:
  [[nodiscard]] Disposable subscribe(Interface* anInterface)
  {
    return ClassicSubscriptionBase::subscribe(anInterface);
  }

  template <typename... Args>
  void notifyAll(void (Interface::*member)(Args...), Args... args)
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

}