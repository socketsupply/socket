#include <thread>
#include "asset_cache.hh"

namespace SSC {
  AssetCache::AssetCache(String _hashCommandEnv, fs::path _appRoot, String _cacheRoot, String _sscHome) {
    this->sscHome.assign(_sscHome);
    this->appRoot.assign(_appRoot);

    if (_hashCommandEnv.length() > 0) // SSC_HASH_COMMAND
      hashCommand.assign(_hashCommandEnv);
    else {
#ifdef _WIN32
    hashCommand = "%SystemRoot%\\system32\\certutil -hashfile {{file}} MD5 | find /i /v \"md5\" | %SystemRoot%\\system32\\find /i /v \"certutil\"";
#endif
#ifdef __linux__
    hashCommand = "md5sum {{file}} | awk '{ print $1 }'"
#endif
#ifdef __APPLE__
    hashCommand = "md5sum {{file}} | awk '{ print $1 }'"
#endif
    }

    printf("app root: %s\n", appRoot.string().c_str());

    printf("hash command: %s\n", hashCommand.c_str());

    if (hashCommand.find("{{file}}") == -1)
      printf("Warning, hash command doesn't contain {{file}} argument placeholder\n");

    this->cacheRoot.assign(_cacheRoot.length() > 0 ? _cacheRoot : (fs::path(_sscHome) / "BuildCache")); // SSC_CACHE_ROOT

    printf("cache root: %s\n", cacheRoot.string().c_str());

    if (!fs::exists(this->cacheRoot))
    {
      printf("Creating cache root...\n");
      fs::create_directories(this->cacheRoot.string().c_str());
    }
  }

  String AssetCache::BuildHashCommand(fs::path file) {
    return std::regex_replace(this->hashCommand, std::regex("\\{\\{file\\}\\}"), 
#ifdef _WIN32
    replace(
#endif
    file.string()
#ifdef _WIN32
    , "/", "\\")
#endif
    )
    ; 
  }

  String AssetCache::HashFile(fs::path file) {
    auto command = BuildHashCommand(file);
    auto r = exec(command);
    return replace(r.output, "\n", "");
  }

  String AssetCache::HashFiles(std::vector<fs::path> files, std::vector<String> tempStrings) {
    StringStream contents;
    std::vector<StringStream*> _contents;

    int thread_count = 8;
    std::vector<std::thread*> threads;
    int fpt = ceil((files.size())/(float)thread_count);

    for (int t = 0; t < thread_count; t++) {
      _contents.push_back(new StringStream());
      threads.push_back(new std::thread([&](int _t) {
        for (int f = fpt * _t; f < fpt * (_t + 1); f++) {
          if (f < files.size() && fs::exists(files[f]))
          {
            try {
              (*_contents[_t]) << HashFile(files[f]);
            }
            catch (std::exception e) {
              std::cout << "Error hashing " << files[f].string().c_str() << "HashFiles: Error joining thread: " << e.what() << std::endl;
              throw e;
            }
          }
        }
      }, (t)));
    }

    for (auto& th : threads)
      if (th->joinable())
      {
        try {
          th->join();
        }
        catch (std::exception e) {
          std::cout << "HashFiles: Error joining thread: " << e.what() << std::endl;
          throw e;
        }
      }

    for (auto c = 0; c < _contents.size(); c++) {
      contents << _contents[c]->str();
    }

    for (auto f = 0; f < tempStrings.size(); f++)
      contents << HashTemp(tempStrings[f]);

    return HashTemp(contents.str());
  }

  String AssetCache::HashTemp(String data) {
    // TODO(mribbons): Use a proper temp file
    writeFile("file.temp", data);
    auto hash = HashFile("file.temp");
    fs::remove("file.temp");
    return hash;
  }

  String AssetCache::HashMakeFileInput(fs::path makeFile, String makeCommand) {
    String contents = readFile(makeFile);
    std::regex re(R"(.*(\.cpp|\.cc|\.c))");
    std::smatch matches;
    std::vector<fs::path> source_files;

    StringStream pathStream;
    String token;

    while (token != contents) {
      token = contents.substr(0,contents.find_first_of("\n"));

      contents = contents.substr(contents.find_first_of("\n") + 1);
      if (std::regex_search(token, matches, re) != 0) {
        String path; path.append(matches[0].str());
        auto f = path.find_first_of("*");

        if (f == -1)
          source_files.push_back(makeFile.parent_path() / fs::path(trim(path)));
      }
    }

    return HashFiles(source_files, { makeCommand });
  }

  bool AssetCache::RestoreCache(String hash, String assetName, fs::path dest) {

    auto cachePath = cacheRoot / hash / assetName;
    if (fs::exists(cachePath)) {
      if (fs::is_directory(cachePath) && !fs::is_empty(cachePath)) {
        std::cout << "Use cache " << cachePath << " => " << dest << std::endl;
        fs::copy(cachePath, dest, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
        return true;
      }
    }

    printf("cache miss for %s, %s\n", hash.c_str(), assetName.c_str());
    return false;
  }

  void AssetCache::UpdateCache(String hash, String assetName, fs::path source) {
    auto cachePath = cacheRoot / hash / assetName;
    std::cout << "Update cache " << source << " => " << cachePath << std::endl;
    fs::create_directories(cachePath);
    fs::copy(source, cachePath, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
  }
}