#include "../../debug.hh"

#include "broadcast_channel.hh"

namespace ssc::runtime::core::services {
  const JSON::Object BroadcastChannel::Message::json () const {
    return JSON::Object::Entries {
      {"origin", this->origin},
      {"token", this->token},
      {"name", this->name},
      {"data", this->data},
      {"id", this->id}
    };
  }

  const JSON::Object BroadcastChannel::MessageEvent::json () const {
    return JSON::Object::Entries {
      {"subscription", this->subscription.json()},
      {"message", Message::json()},
      {"origin", this->origin},
      {"name", this->name},
      {"id", this->id}
    };
  }

  const JSON::Object BroadcastChannel::Subscription::json () const {
    return JSON::Object::Entries {
      {"origin", this->origin},
      {"name", this->name},
      {"id", std::to_string(this->id)}
    };
  }

  const String BroadcastChannel::Subscription::key () const {
    if (this->origin.ends_with("/")) {
      return this->origin + this->name;
    } else {
      return this->origin + "/" + this->name;
    }
  }

  const BroadcastChannel::Subscription& BroadcastChannel::subscribe (
    const Subscription& subscription
  ) {
    Lock lock(this->mutex);
    this->subscriptions.emplace(subscription.id, subscription);
    const auto& result = this->subscriptions.at(subscription.id);
    const auto& callback = result.callback;
    const auto& observer = this->subscriptionObservers[subscription.key()];
    auto& observers = this->observers[subscription.key()];

    observers.add(observer, [this](const auto event) {
      for (const auto& entry : this->subscriptions) {
        if (entry.first == event.subscription.id) {
          entry.second.callback(event);
        }
      }
    });

    return result;
  }

  bool BroadcastChannel::unsubscribe (const SubscriptionID id) {
    Lock lock(this->mutex);
    if (this->subscriptions.contains(id)) {
      this->subscriptions.erase(id);
    }
    return false;
  }

  bool BroadcastChannel::postMessage (const Message& message) {
    Lock lock(this->mutex);
    bool dispatched = false;
    int count = 0;

    for (const auto& entry : this->subscriptions) {
      const auto& subscription = entry.second;
      if (message.origin == "*") {
        if (subscription.name == message.name) {
          const auto event = MessageEvent {message, subscription};
          if (this->observers.at(subscription.key()).dispatch(event)) {
            dispatched = true;
            count++;
          }
        }
      } else if (
        subscription.origin == message.origin &&
        subscription.name == message.name
      ) {
        const auto event = MessageEvent {message, subscription};
        if (this->observers[subscription.key()].dispatch(event)) {
          dispatched = true;
          count++;
        }
      }
    }

    return dispatched;
  }
}
