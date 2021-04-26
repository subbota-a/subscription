#pragma once
#pragma once

#include <memory>

namespace subscriptions {
namespace internal {

class Disposable {
public:
  virtual ~Disposable() = default;
};

} // namespace internal

class Disposable final {
public:
  Disposable() = default;

  explicit Disposable(std::unique_ptr<internal::Disposable> &&disposable)
      : disposable_(std::move(disposable)) {}

  Disposable(const Disposable &) = delete;

  Disposable &operator=(const Disposable &) = delete;

  Disposable(Disposable &&other) noexcept { swap(*this, other); }

  Disposable &operator=(Disposable &&other) noexcept {
    Disposable disposable(std::move(other));
    swap(*this, disposable);
    return *this;
  }

  void dispose() noexcept { disposable_.reset(); }

  friend void swap(Disposable &a, Disposable &b) {
    std::swap(a.disposable_, b.disposable_);
  }

private:
  std::unique_ptr<internal::Disposable> disposable_;
};
}