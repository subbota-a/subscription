#include "ClassicSubscription.h"

#include <cassert>

namespace subscriptions::internal {

ClassicSubscriptionBase::DisposableImpl::DisposableImpl(
    std::weak_ptr<std::vector<void*>> subscribers, void* pointer)
    : subscribers_(std::move(subscribers)), pointer_(pointer)
{
}

void ClassicSubscriptionBase::DisposableImpl::dispose() noexcept
{
  if (auto subscribers = subscribers_.lock()) {
    auto it = std::find(subscribers->begin(), subscribers->end(), pointer_);
    assert(it != subscribers->end());
    *it = nullptr;
  }
  subscribers_.reset();
  pointer_ = nullptr;
}

subscriptions::Disposable ClassicSubscriptionBase::subscribe(void* p)
{
  if (!p)
    throw std::runtime_error("Interface pointer must be not null");

  if (std::find(subscribers_->begin(), subscribers_->end(), p) != subscribers_->end())
    throw std::runtime_error("Subscribe twice is not allowed");

  subscribers_->push_back(p);
  return subscriptions::Disposable(std::make_unique<DisposableImpl>(subscribers_, p));
}

void ClassicSubscriptionBase::clean_released()
{
  subscribers_->erase(
      std::remove(subscribers_->begin(), subscribers_->end(), nullptr), subscribers_->end());
}

}