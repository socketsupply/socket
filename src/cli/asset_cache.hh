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
  class AssetCache {
  protected:
    String hashCommand;
    fs::path appRoot;
    fs::path sscHome;
    fs::path cacheRoot;

  public:
    // TODO(mribbons): Accept log func lambda as constructor param, replace cout usage
    AssetCache(String _hashCommandEnv, fs::path _appRoot, String _cacheRoot, String _sscHome);
    String BuildHashCommand(fs::path file);
    String HashFile(fs::path file);
    String HashFiles(std::vector<fs::path> files, std::vector<String> tempStrings = std::vector<String>());
    void GetCFileHeaders(fs::path sourcePath, std::map<fs::path, bool> &unique_headers);
    String HashMakeFileInput(fs::path makeFile, String makeCommand);
    String HashTemp(String data);
    bool RestoreCache(String hash, String assetName, fs::path dest);
    void UpdateCache(String hash, String assetName, fs::path source);
  };

}

#endif