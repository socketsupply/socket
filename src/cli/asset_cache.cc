#ifdef _WIN32
#include <stdio.h>
#else
#include <stdlib.h>
#endif
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
    hashCommand = "md5sum {{file}} | awk '{ print $1 }'";
#endif
#ifdef __APPLE__
    hashCommand = "md5 -q {{file}}";
#endif
    }

    printf("app root: %s\n", appRoot.string().c_str());

    printf("hash command: %s\n", hashCommand.c_str());

    if (hashCommand.find("{{file}}") == -1)
      printf("Warning, hash command doesn't contain {{file}} argument placeholder\n");

    this->cacheRoot = (_cacheRoot.length() > 0 ? fs::path(_cacheRoot) : (fs::path(_sscHome) / "BuildCache")); // SSC_CACHE_ROOT

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
        for (int f = fpt * _t; f < fpt * (_t + 1) && f < files.size(); f++) {
          if (fs::exists(files[f])) {
            try {
              (*_contents[_t]) << HashFile(files[f]);
            }
            catch (std::exception e) {
              std::cout << "Error hashing " << files[f].string().c_str() << e.what() << std::endl;
              throw e;
            }
          } else {
            std::cout << "Hash: file doesn't exist: " << files[f] << std::endl;
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
#ifdef _WIN32
    char tempname[256];
    tmpnam_s(tempname, 256);
#else
    char tempname[] = "/tmp/ssc_XXXXXX";
    auto r = mkstemp(tempname);
#endif  
    writeFile(tempname, data);
    auto hash = HashFile(tempname);
    fs::remove(tempname);
    return hash;
  }

  void AssetCache::GetCFileHeaders(fs::path sourcePath, std::map<fs::path, bool> &unique_headers) {
    // doesn't account for #ifdef includes, might not be a problem
    // remove parent from path when checking uniqueness, or make it absolute
    const std::regex re(R"(.*#include \"(.*)\")");
    std::smatch matches;


    auto realAppRoot = appRoot.parent_path().parent_path();
    // TODO(mribbons): Read include dirs from makefile
    std::vector<fs::path> include_dirs = {
      "",
      realAppRoot,
      realAppRoot / "include",
      sourcePath.parent_path(),
      sourcePath.parent_path() / "include",
    };

    String contents = readFile(sourcePath);
    String token;

    while (token != contents) {
      token = contents.substr(0,contents.find_first_of("\n"));

      contents = contents.substr(contents.find_first_of("\n") + 1);
      if (std::regex_search(token, matches, re) != 0) {
        String path; path.append(matches[1].str());
        fs::path includePath = fs::path(trim(path));

        for (int d = 0; d < include_dirs.size(); d++) {
          if (fs::exists(include_dirs[d] / includePath))
            {
              includePath = include_dirs[d] / includePath;
              GetCFileHeaders(includePath, unique_headers);
              break;
            }
        }
          
        // std::cout << "Include " << includePath.string() << std::endl;
        unique_headers[includePath] = true;
      }
    }
  }

  String AssetCache::HashMakeFileInput(fs::path makeFile, String makeCommand) {
    String contents = readFile(makeFile);
    const std::regex re(R"(.*(\.cpp|\.cc|\.c))");
    const std::regex reLocalPath(R"(\$\(LOCAL_PATH\))");
    std::smatch matches;
    std::vector<fs::path> source_files;
    source_files.push_back(makeFile);

    String token;

    auto realAppRoot = appRoot.parent_path().parent_path();

    std::map<fs::path, bool> unique_headers;

    while (token != contents) {
      token = contents.substr(0,contents.find_first_of("\n"));

      contents = contents.substr(contents.find_first_of("\n") + 1);
      if (std::regex_search(token, matches, re) != 0) {
        String path; path.append(replace(matches[0].str(), "#hint", ""));
        auto f = path.find_first_of("*");

        if (f == -1) {
          fs::path source = fs::path(trim(
#ifdef _WIN32
          replace(
#endif
            path
#ifdef _WIN32
            , "/", "\\")
#endif
            
            ));

          // fs::
          if (fs::exists(realAppRoot / source)) {
            source = realAppRoot / source;
          } else {
            source = makeFile.parent_path() / source;
          }
          this->GetCFileHeaders(source, unique_headers);
          source_files.push_back(source);
        }
      }
    }

    for(std::map<fs::path, bool>::iterator header = unique_headers.begin(); header != unique_headers.end(); ++header)
      source_files.push_back(header->first);

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