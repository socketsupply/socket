#ifndef SOCKET_RUNTIME_CORE_SERVICES_H
#define SOCKET_RUNTIME_CORE_SERVICES_H

#include "../context.hh"
#include "../options.hh"

#include "services/ai.hh"
#include "services/broadcast_channel.hh"
#include "services/conduit.hh"
#include "services/diagnostics.hh"
#include "services/dns.hh"
#include "services/fs.hh"
#include "services/geolocation.hh"
#include "services/media_devices.hh"
#include "services/network_status.hh"
#include "services/notifications.hh"
#include "services/os.hh"
#include "services/permissions.hh"
#include "services/platform.hh"
#include "services/process.hh"
#include "services/timers.hh"
#include "services/udp.hh"

namespace ssc::runtime::core {
  struct Services {
    struct Features {
      bool useAI = true;
      bool useBroadcashChannel = true;
    #if !SOCKET_RUNTIME_PLATFORM_IOS
      bool useChildProcess = true;
    #endif
      bool useConduit = true;
      bool useDiagnostics = true;
      bool useDNS = true;
      bool useFS = true;
      bool useGeolocation = true;
      bool useMediaDevices = true;
      bool useNetworkStatus = true;
      bool useNotifications = true;
      bool useOS = true;
      bool usePermissions = true;
      bool usePlatform = true;
      bool useProcess = true;
      bool useTimers = true;
      bool useUDP = true;
    };

    struct Options {
      context::Dispatcher& dispatcher;
      Features features;
    };

    Mutex mutex;

    core::services::BroadcastChannel broadcastChannel;
    core::services::AI ai;
    core::services::Conduit conduit;
    core::services::Diagnostics diagnostics;
    core::services::DNS dns;
    core::services::FS fs;
    core::services::Geolocation geolocation;
    core::services::MediaDevices mediaDevices;
    core::services::NetworkStatus networkStatus;
    core::services::Notifications notifications;
    core::services::OS os;
    core::services::Permissions permissions;
    core::services::Platform platform;
    core::services::Process process;
    core::services::Timers timers;
    core::services::UDP udp;

    Services (context::RuntimeContext&, const Options&);
    Services () = delete;
    Services (const Services&) = delete;
    Services (Services&&) = delete;
    Services& operator = (const Services&) = delete;
    Services& operator = (Services&&) = delete;
    bool start ();
    bool stop ();
  };
}
#endif
