#ifndef SOCKET_RUNTIME_CORE_SERVICES_BROADCAST_CHANNEL_H
#define SOCKET_RUNTIME_CORE_SERVICES_BROADCAST_CHANNEL_H

#include "../../core.hh"
#include "../../unique_client.hh"

namespace ssc::runtime::core::services {
  class BroadcastChannel : public core::Service {
    public:
      using SubscriptionID = uint64_t;
      using MessageEventID = uint64_t;
      using MessageID = uint64_t;
      using Client = UniqueClient;

      struct Subscription;

      struct Message {
        const Client client;
        const String origin = "";
        const String token = "";
        const String name = "";
        JSON::Any data;
        const MessageID id = rand64();

        const JSON::Object json () const;
      };

      struct MessageEvent : public Message {
        const Subscription& subscription;
        const MessageEventID id = rand64();
        const JSON::Object json () const;
      };

      using Observer = Observer<MessageEvent>;
      using Observers = Observers<Observer>;
      using ObserversMap = Map<String, Observers>;
      using SubscriptionObservers = Map<String, Observer>;

      struct Subscription {
        const String name = "";
        const String origin = "";
        const Client client;
        const Observer::Callback callback;
        const SubscriptionID id = rand64();
        const JSON::Object json () const;
        const String key () const;
      };

      using Subscriptions = Map<SubscriptionID, Subscription>;

      Mutex mutex;
      ObserversMap observers;
      Subscriptions subscriptions;
      SubscriptionObservers subscriptionObservers;

      BroadcastChannel (const Options& options)
        : core::Service(options)
      {}

      const Subscription& subscribe (const Subscription& subscription);
      bool unsubscribe (const SubscriptionID id);
      bool postMessage (const Message& message);
  };
}
#endif
