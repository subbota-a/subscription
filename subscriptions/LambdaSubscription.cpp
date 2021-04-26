#include "LambdaSubscription.h"

#include <cassert>

namespace subscriptions {

LambdaSubscription::DisposableImpl::DisposableImpl(
    std::weak_ptr<std::vector<IdentityCallable>> callbacks, IdentityCallable::Identity identity)
    : callbacks_(std::move(callbacks)), identity_(identity)
{
}

void LambdaSubscription::DisposableImpl::dispose() noexcept
{
    if (auto lock = callbacks_.lock()) {
        auto it = std::find_if(lock->begin(), lock->end(), [id = identity_](IdentityCallable& c) {
            return c.identity() == id;
        });
        assert(it != lock->end());
        it->release();
    }
    callbacks_.reset();
    identity_ = {};
}

void LambdaSubscription::notifyAll()
{
    for (size_t i = 0, size = callbacks_->size(); i < size; ++i)  // NOLINT(modernize-loop-convert)
        callbacks_->at(i)();

    callbacks_->erase(
        std::remove_if(
            callbacks_->begin(),
            callbacks_->end(),
            [](auto&& callable) { return callable.identity() == IdentityCallable::Identity(); }),
        callbacks_->end());
}

}  // namespace subscriptions