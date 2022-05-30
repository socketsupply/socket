//
// Interface for IO on iOS and MacOS
// 
@interface SocketIO
@property nw_path_monitor_t monitor;
@property (strong, nonatomic) NSObject<OS_dispatch_queue>* monitorQueue;

// Bridge
- (void) route: (std::string)msg;
- (void) emit: (std::string)event message: (std::string)message;
- (void) resolve: (std::string)seq message: (std::string)message;
- (void) reject: (std::string)seq message: (std::string)message;

// Filesystem
- (void) fsOpen: (std::string)seq id: (uint64_t)id path: (std::string)path flags: (int)flags;
- (void) fsClose: (std::string)seq id: (uint64_t)id;
- (void) fsRead: (std::string)seq id: (uint64_t)id len: (int)len offset: (int)offset;
- (void) fsWrite: (std::string)seq id: (uint64_t)id data: (std::string)data offset: (int64_t)offset;
- (void) fsStat: (std::string)seq path: (std::string)path;
- (void) fsUnlink: (std::string)seq path: (std::string)path;
- (void) fsRename: (std::string)seq pathA: (std::string)pathA pathB: (std::string)pathB;
- (void) fsCopyFile: (std::string)seq pathA: (std::string)pathA pathB: (std::string)pathB flags: (int)flags;
- (void) fsRmDir: (std::string)seq path: (std::string)path;
- (void) fsMkDir: (std::string)seq path: (std::string)path mode: (int)mode;
- (void) fsReadDir: (std::string)seq path: (std::string)path;

// TCP
- (void) tcpBind: (std::string)seq serverId: (uint64_t)serverId ip: (std::string)ip port: (int)port;
- (void) tcpConnect: (std::string)seq clientId: (uint64_t)clientId port: (int)port ip: (std::string)ip;
- (void) tcpSetTimeout: (std::string)seq clientId: (uint64_t)clientId timeout: (int)timeout;
- (void) tcpSetKeepAlive: (std::string)seq clientId: (uint64_t)clientId timeout: (int)timeout;
- (void) tcpSend: (uint64_t)clientId message: (std::string)message;
- (void) tcpReadStart: (std::string)seq clientId: (uint64_t)clientId;

// UDP
- (void) udpBind: (std::string)seq serverId: (uint64_t)serverId ip: (std::string)ip port: (int)port;
- (void) udpSend: (std::string)seq clientId: (uint64_t)clientId message: (std::string)message offset: (int)offset len: (int)len port: (int)port ip: (const char*)ip;
- (void) udpReadStart: (std::string)seq serverId: (uint64_t)serverId;

// Common
- (void) sendBufferSize: (std::string)seq clientId: (uint64_t)clientId size: (int)size;
- (void) recvBufferSize: (std::string)seq clientId: (uint64_t)clientId size: (int)size;
- (void) close: (std::string)seq clientId: (uint64_t)clientId;
- (void) shutdown: (std::string)seq clientId: (uint64_t)clientId;
- (void) readStop: (std::string)seq clientId: (uint64_t)clientId;

// Network
- (void) dnsLookup: (std::string)seq hostname: (std::string)hostname;
- (void) initNetworkStatusObserver;
- (std::string) getNetworkInterfaces;
@end
