#ifndef SSC_CORE_ASSET_CACHE_H
#define SSC_CORE_ASSET_CACHE_H

#include <stdio.h>
#include <string.h>
#include <vector>
#include <filesystem>
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
    AssetCache(String _hashCommandEnv, fs::path _appRoot, String _cacheRoot, String _sscHome);
    String BuildHashCommand(fs::path file);
    String HashFile(fs::path file);
    String HashFiles(std::vector<fs::path> files, std::vector<String> tempStrings = std::vector<String>());
    String HashMakeFileInput(fs::path makeFile, String makeCommand);
    String HashTemp(String data);
    bool RestoreCache(String hash, String assetName, fs::path dest);
    void UpdateCache(String hash, String assetName, fs::path source);
  };

}

#endif