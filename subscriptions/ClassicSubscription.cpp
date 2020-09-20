#include "ClassicSubscription.h"

void ClassicSubscriptionBase::Disposable::dispose() noexcept
{
    if (auto subscribers = subscribers_.lock()) {
        auto it = std::find(subscribers->begin(), subscribers->end(), pointer_);
        assert(it != subscribers->end());
        *it = nullptr;
    }
    subscribers_.reset();
    pointer_ = nullptr;
}

ClassicSubscriptionBase::Disposable::Disposable(std::weak_ptr<std::vector<void*>> subscribers, void* pointer)
        : subscribers_(std::move(subscribers))
        , pointer_(pointer) {}

ClassicSubscriptionBase::Disposable ClassicSubscriptionBase::subscribe(void* p)
{
    if (!p)
        throw std::runtime_error("Interface pointer must be not null");

    if (std::find(subscribers_->begin(), subscribers_->end(), p) != subscribers_->end())
        throw std::runtime_error("Subscribe twice is not allowed");

    subscribers_->push_back(p);
    return Disposable(subscribers_, p);
}

void ClassicSubscriptionBase::clean_released()
{
    subscribers_->erase(
            std::remove(subscribers_->begin(), subscribers_->end(), nullptr), subscribers_->end());
}
