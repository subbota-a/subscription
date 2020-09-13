#include "LambdaSubscription.h"

LambdaSubscription::Unsubscriber::Unsubscriber(
        std::weak_ptr<std::vector<IdentityCallable>> callbacks, IdentityCallable::Identity identity)
        : callbacks_(std::move(callbacks))
        , identity_(identity)
{
}


void LambdaSubscription::notifyAll()
{
    for (size_t i = 0, size = callbacks_->size(); i < size; ++i) // NOLINT(modernize-loop-convert)
        callbacks_->at(i)();
    callbacks_->erase(
            std::remove_if(
                    callbacks_->begin(), callbacks_->end(),
                    [](auto &&callable) { return callable.identity() == IdentityCallable::Identity(); }),
            callbacks_->end());
}
