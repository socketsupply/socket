#ifndef SSC_CORE_ASSET_CACHE_H
#define SSC_CORE_ASSET_CACHE_H

#include <stdio.h>
#include <string.h>
#include <vector>
#include <filesystem>
// Resolve duplicate isDebugEnabled at link time
#ifdef CLI
#undef CLI
#endif
#include "../common.hh"
#include "../process/process.hh"

namespace SSC {
  class ThreadStub {
    fs::path stub;
    public:
      ThreadStub(String section) {
#ifdef DEBUG
        StringStream ss;
        ss << std::this_thread::get_id();
        stub = fs::path("th_" + ss.str());
        writeFile(stub, section);
#endif
      }

      ~ThreadStub() {
#ifdef DEBUG
        try {
          fs::remove(stub);
        } catch (fs::filesystem_error &e) {
          std::cerr << "Error removing ThreadStub " << stub.string() << ": " << e.what();
        }
#endif
      }
  };

  class AssetCache {
  protected:
    String hashCommand;
    fs::path appRoot;
    fs::path sscHome;
    fs::path cacheRoot;
    void (*log)(const String);
    std::map<fs::path, fs::path> ssc_header_prefixes;
    std::map<fs::path, String> ssc_header_contents;
    std::map<String, String> hash_cache;
    Mutex uniqueHeadersMutex;
    Mutex hashFileMapMutex;
    Mutex hashCacheMutex;
    // clean up - delete elements, used new()
    std::map<fs::path, Mutex*> hashFileMutexes;

  public:
    // TODO(mribbons): Accept log func lambda as constructor param, replace cout usage
    AssetCache(String _hashCommandEnv, fs::path _appRoot, String _cacheRoot, String _sscHome, void (*_log)(const String));
    ~AssetCache();
    /// @brief Returns true if the header already existed, otherwise return false.
    /// @param unique_headers 
    /// @param path 
    /// @return 
    bool AddUniqueHeader(std::map<fs::path, bool> &unique_headers, fs::path path);
    void GetUniqueHeaders(std::map<fs::path, bool> &unique_headers, std::vector<fs::path> &source_files);
    Mutex* GetHashFileMapMutex(fs::path file);
    String BuildHashCommand(fs::path file);
    String HashFile(fs::path file);
    String HashFiles(std::vector<fs::path> files, std::vector<String> tempStrings = std::vector<String>());
    void LoadHeaderPrefixes();
    fs::path LocateInHeaderCache(fs::path make_dir, fs::path target, fs::path context, std::map<fs::path, fs::path> prefixes);
    void GetCFileHeaders(fs::path make_dir, std::vector<fs::path> include_paths, fs::path sourcePath, std::map<fs::path, bool> &unique_headers, bool useSscHeaders = false);
    String HashMakeFileInput(fs::path make_file, String makeCommand, bool useSscHeaders = false);
    String HashTemp(String data, String hint);
    bool RestoreCache(String hash, String assetName, fs::path dest);
    void UpdateCache(String hash, String assetName, fs::path source);
  };

}

#endif