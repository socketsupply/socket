#ifdef _WIN32
#include <stdio.h>
#else
#include <stdlib.h>
#endif
#include <thread>
#include "asset_cache.hh"
#include <math.h>

namespace SSC {  

  void NormzWin32Path(fs::path &path)
  {
#ifdef _WIN32
    path.assign(std::regex_replace(replace(path.string(), "/", "\\"), std::regex(R"(\\\\)"), R"(\)"));
#endif
  }

  /// @brief Clean up paths like directory/sub/../, but currently only removes ../ from start of rel_path
  /// @param path 
  /// @param  
  /// @return 
  fs::path JoinRelPath(fs::path base_path, fs::path rel_path)
  {
#ifdef _WIN32
    const String parent = "..\\";
#else
    const String parent = "../";
#endif

    NormzWin32Path(rel_path);

    auto rel = rel_path.string();

    size_t rel_start = 0;
    while (rel.find(parent, rel_start * parent.length()) == (rel_start * 3))
    {
      rel_start += 1;
    }

    for (int x = 0; x < rel_start; x++)
      base_path = base_path.parent_path();

    return base_path / fs::path(rel_path.string().substr(rel_start * parent.length()));
  }

  AssetCache::AssetCache(String _hashCommandEnv, fs::path _appRoot, String _cacheRoot, String _sscHome,void (*_log)(const String)) {
    this->log = _log;
    this->sscHome = _sscHome;
    this->appRoot = _appRoot;
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

    if (hashCommand.find("{{file}}") == String::npos)
      log("warning! hash command doesn't contain {{file}} argument placeholder\n");

    this->cacheRoot = (_cacheRoot.length() > 0 ? fs::path(_cacheRoot) : (fs::path(_sscHome + "_cache"))); // SSC_CACHE

    log("cache root: " + cacheRoot.string());

    if (!fs::exists(this->cacheRoot))
    {
      log("creating cache root...");
      fs::create_directories(this->cacheRoot.string().c_str());
    }
  }

  AssetCache::~AssetCache()
  {
    for (auto &it : hashFileMutexes) {
      delete it.second;
    }
  }

  bool AssetCache::AddUniqueHeader(std::map<fs::path, bool> &unique_headers, fs::path path) {
    Lock lock(uniqueHeadersMutex);
    if (unique_headers.count(path) > 0) return true;
    unique_headers[path] = true;

    return false;
  }

  
  void AssetCache::GetUniqueHeaders(std::map<fs::path, bool> &unique_headers, std::vector<fs::path> &source_files) {
    Lock lock(uniqueHeadersMutex);
    for (auto& it : unique_headers)
      source_files.push_back(it.first);

    unique_headers.clear();
  }

  Mutex* AssetCache::GetHashFileMapMutex(fs::path file) {
    Lock lock(hashFileMapMutex);
    if (hashFileMutexes.count(file) == 0)
    {
      hashFileMutexes[file] = new Mutex();
    }

    return hashFileMutexes[file];
  }

  String AssetCache::BuildHashCommand(fs::path file) {
    return std::regex_replace(this->hashCommand, std::regex("\\{\\{file\\}\\}"), file.string());
  }

  // pre String / fs::file map changes()

  String AssetCache::HashFile(fs::path file) {
    NormzWin32Path(file);
    Lock lock(hashCacheMutex);
    if (hash_cache.count(file.string()) > 0)
      return hash_cache[file.string()];

    Lock lock2(*GetHashFileMapMutex(file));
  
    auto command = BuildHashCommand(file);
    auto r = exec(command);
    for (auto x = 0; x < 4 && r.exitCode != 0; ++x) {
      if (r.exitCode != 0) {
        // Haven't seen this crop up recently, problem may be resolved
        std::cerr << "Hash " << file.string() << " failed: " << r.output << " with code " << r.exitCode << ". Retrying " << (3-x) << " times, sleep: " << (int)pow(10, x)*5 << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds((int)pow(10, x)*5));
        r = exec(command);
      }
    }

    if (r.exitCode == 0)
      return replace(r.output, "\n", "");

    throw std::runtime_error("Failed to hash" + r.output + ", code: " + std::to_string(r.exitCode));
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
        ThreadStub ts("Hash files " + std::to_string(_t) + "/" + std::to_string(thread_count) + " " + (tempStrings.size() > 0 ? tempStrings[0] : ""));
        for (int f = fpt * _t; f < fpt * (_t + 1) && f < files.size(); f++) {
          if (fs::exists(files[f]) || true) {
            try {
                (*_contents[_t]) << HashFile(files[f]);
            }
            catch (std::exception e) {
              std::cerr << "Error hashing " << files[f].string().c_str() << e.what() << std::endl;
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
          std::cerr << "HashFiles: Error joining thread: " << e.what() << std::endl;
          throw e;
        }
      }

    for (auto c = 0; c < _contents.size(); c++) {
      contents << _contents[c]->str();
    }

    for (auto f = 0; f < tempStrings.size(); f++)
      contents << HashTemp(tempStrings[f], "tempstrings[" + std::to_string(f) + "]");

    return HashTemp(contents.str(), "contents");
  }

  String AssetCache::HashTemp(String data, String hint) {
    if (data.length() == 0) {
      String error = "HashTemp: (" + hint + ") data should not be empty.";
      std::cerr << error << std::endl;
      throw std::runtime_error(error);
    }
#ifdef _WIN32
    char tempname[256];
    auto r = tmpnam_s(tempname, 256);
    
    // r is something different on OS X, only check non zero on windows
    if (r)
      std::cerr << "Unable to make temp name: " << r << std::endl;
#else
    char tempname[] = "/tmp/ssc_XXXXXX";
    auto r = mkstemp(tempname);
#endif

    writeFile(tempname, data);
    auto hash = HashFile(tempname);
    fs::remove(tempname);
    return hash;
  }

  void AssetCache::LoadHeaderPrefixes() {
    const String hh = "hh|h|hpp";
    const std::regex backslash(R"(\\)");
    String home = sscHome.string();
    std::vector<fs::path> prefixes;
#ifdef _WIN32
    home = std::regex_replace(home, backslash, R"(\\)");
#endif
    for (auto const& dir_entry : fs::recursive_directory_iterator(home)) {
        if (!dir_entry.is_directory()) {
          if (hh.find_first_of(dir_entry.path().extension().string()) != -1) {
            fs::path header = dir_entry.path();
            NormzWin32Path(header);
            header = replace(header.string(), home, "");
            String header_str = header.string();
#ifdef _WIN32
            if (header_str.find("\\src\\") == 0) {
              header_str = header_str.substr(5);
            } else if (header_str.find("src\\") == 0) {
              header_str = header_str.substr(4);
            } else if (header_str.find("\\") == 0) {
              header_str = header_str.substr(1);
            }

              header = fs::path(header_str);
#else
            if (header.string().find("src/") == 0)
              header = fs::path(header.string().substr(4));
#endif
            
            ssc_header_prefixes[header] = fs::path(dir_entry.path());
            prefixes.push_back(header);

            ssc_header_contents[header] = "";
            hash_cache[header.string()] = "";
          }
        }
    }

    int thread_count = 8;
    std::vector<std::thread*> threads;
    int fpt = ceil((prefixes.size())/(float)thread_count);

    Mutex mapMutex;

    for (int t = 0; t < thread_count; t++) {
      threads.push_back(new std::thread([&](int _t) {
        ThreadStub ts("Load header prefixes");
        for (int f = fpt * _t; f < fpt * (_t + 1) && f < prefixes.size(); f++) {
          Lock lock(mapMutex);
          auto header = prefixes[f];
          ssc_header_contents[header] = readFile(ssc_header_prefixes[header]);
          hash_cache[header.string()] = HashFile(ssc_header_prefixes[header]);
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
          std::cerr << "LoadHeaderPrefixes: Error joining thread: " << e.what() << std::endl;
          throw e;
        }
      }
  }

  fs::path AssetCache::LocateInHeaderCache(fs::path make_dir, fs::path target, fs::path context, std::map<fs::path, fs::path> prefixes)
  {
    const String p1 = "../";    
    const String p2 = "..\\";
    fs::path base = target;
    size_t b_start = 0;
    while (base.string().find(p1, b_start) == (b_start*3) || base.string().find(p2, b_start) == (b_start*3))
      b_start += 3;

    base = fs::path(base.string().substr(b_start));

    std::vector<fs::path> test_paths { base, context.parent_path().stem() / base, "include" / base };
    //for (auto& th : source_threads)
    for (auto test_path : test_paths) {
      if (prefixes.count(test_path) != 0) {
        return test_path;
      }
    }

    base.assign("");
    return base;
  }

  void AssetCache::GetCFileHeaders(fs::path make_dir, std::vector<fs::path> include_paths, fs::path sourcePath, std::map<fs::path, bool> &unique_headers, bool useSscHeaders) {
    try {
      // doesn't account for #ifdef includes, might not be a problem
      // remove parent from path when checking uniqueness, or make it absolute
      const std::regex re(R"(.*#include \"(.*)\")");
      std::smatch matches;

      auto realAppRoot = appRoot.parent_path().parent_path();
      const std::vector<fs::path> include_dirs_base = {
        realAppRoot,
        realAppRoot / "include",
        sourcePath.parent_path() / "include",
      };

      std::vector<fs::path> include_dirs =  { sourcePath.parent_path() };
      for (auto include_path : include_paths)
        include_dirs.push_back(include_path);

      for (auto include_path : include_dirs_base)
        include_dirs.push_back(include_path);

      String contents = readFile(sourcePath);
      String token;

      while (token != contents) {
        token = contents.substr(0,contents.find_first_of("\n"));

        contents = contents.substr(contents.find_first_of("\n") + 1);
        if (std::regex_search(token, matches, re) != 0) {
          String path; path.append(matches[1].str());
          fs::path includePath = fs::path(trim(path));

          int d = 0;
          // Ignore weird imports like #include "//'SYS1.SAMPLIB(CSRSIC)'", these make fs::exists crash on Windows
          if (includePath.string().find("//") == 0)
          {
            d = include_dirs.size();
          }

          auto header_located = false;
          auto header_previously_located = false;

          for (; d < include_dirs.size(); d++) {
            if (fs::exists(include_dirs[d] / includePath)) {
              includePath = JoinRelPath(include_dirs[d], includePath);
              NormzWin32Path(includePath);

              if (!fs::exists(includePath))
                std::cerr << "File doesn't exist after JoinRelPath: " << includePath.string() << std::endl;
              
              header_located = true;              
              header_previously_located = AddUniqueHeader(unique_headers, includePath);
              break;
            }
          }

          // if path not found using include_dirs list above, search in SSC Headers (LocateInHeaderCache)
          if (!header_located)
          {
            fs::path prefix;
            if (includePath.string().find("//") == String::npos)
              prefix = LocateInHeaderCache(make_dir, includePath, sourcePath, ssc_header_prefixes);

            if (prefix.string().length() > 0)
            {
              includePath = ssc_header_prefixes[prefix];
              fs::path dest = JoinRelPath(make_dir, prefix);
              if (dest.string().find("//") == String::npos && !fs::exists(dest)) {
                if (ssc_header_contents.count(prefix) != 0 && ssc_header_contents[prefix].length() == 0) {
                    ssc_header_contents[prefix] = readFile(includePath);
                  }

                  // we can hash ssc headers because they are used multiple times and the source path stays the same
                  Lock lock(hashCacheMutex);
                  if (hash_cache.count(prefix.string()) > 0) 
                    hash_cache[dest.string()] = hash_cache[prefix.string()];
                  else
                    hash_cache[prefix.string()] = hash_cache[dest.string()] = HashFile(includePath);
                  
                  fs::create_directories(dest.parent_path());
                try {
                  Lock lock(*GetHashFileMapMutex(dest));
                  writeFile(dest.string(), ssc_header_contents[prefix]);
                }
                catch (std::exception e) {
                  std::cerr << "Tried to copy to " << dest.string() << " but " << e.what() << std::endl;
                }
              }

              includePath = dest;
              if (dest.string().find("//") == String::npos && fs::exists(includePath)){
                  header_located = true;
                  // don't update header_previously_located here, can't have been located based on include_dirs if file was found in SSC cache
                  AddUniqueHeader(unique_headers, dest);
                }
            }
          }

          if (header_located) {
            if (!header_previously_located) 
              GetCFileHeaders(make_dir, include_paths, includePath, unique_headers);
          }
          else {
            std::cerr << "Unable to locate header: " << includePath.string() << std::endl;
            throw std::runtime_error("Unable to locate header: " + includePath.string());
          }
        }
      }
    } catch (std::runtime_error &e) {
      std::cerr << "GetCFileHeaders error: " << sourcePath.string() << ": " << e.what() << std::endl;
    }
  }
  
  void GetWildcardFiles(fs::path fs_path, std::vector<String> &curr_list) {
    std::smatch m2;
    const std::regex dot(R"(\.)");
    const std::regex asterix(R"(\*)");
    const std::regex backslash(R"(\\)");
    auto path = fs_path.string();
    #ifdef _WIN32
    // path will be used as a regex, so we need to double escape backslash
    path = std::regex_replace(path, backslash, R"(\\)");
    #endif
    path = std::regex_replace(path, dot, "\\.");
    path = std::regex_replace(path, asterix, ".*");    
    std::regex path_regex(path);

    try {
    
      for (auto const& dir_entry : fs::directory_iterator(fs_path.parent_path())) {
        auto file_path_str = dir_entry.path().string();
        if (!dir_entry.is_directory() && (std::regex_search(file_path_str, m2, path_regex) != 0)) {
          curr_list.push_back(file_path_str);
        }
      }
    } catch (fs::filesystem_error &e) {
      StringStream error;
      error << "FS Error getting files for wildcard pattern: " << fs_path.string();
      std::cerr << error.str() << std::endl;
      throw;
    } catch (std::exception &e) {
      StringStream error;
      error << "STD Error getting files for wildcard pattern: " << fs_path.string();
      std::cerr << error.str() << std::endl;
      throw;
    }
  }

  void GetForeachFiles(String input, std::vector<String> &curr_list, String raw) {
    try {
      //foreach uses , separated list
      auto arguments = split(input, ',');
      for (auto arg : arguments)
      if (arguments.size() < 3)
        std::cerr << "foreach should have 3 arguments, found " << arguments.size() << ": " << raw << std::endl;

      if (arguments[0].length() < 9)
        std::cerr << "foreach doesn't contain variable name after 'foreach ': " << raw << std::endl;

      fs::path path(trim(arguments[2]));
      NormzWin32Path(path);
      auto path_str = path.string();
      std::regex pattern("\\{\\{" + arguments[0].substr(8) + "\\}\\}");
      for (auto file : split(arguments[1], ' ' )) {
        curr_list.push_back(std::regex_replace(path_str, pattern, trim(file)));
      }
    } catch (std::exception e) {
      std::cerr << "error parsing foreach: " << e.what() << std::endl;
    }
  }

  void ApplyMakeExpansions(std::map<String, std::vector<String>> &lists, std::vector<String> &curr_list, fs::path make_dir, std::smatch matches) {
    const std::regex expansion(R"(\$\((.*)( (.*))?\))");
    const std::regex switch_expansion(R"((-\w+)\$\((.*)\)(.*))");
    const std::regex named_list(R"(\$\((\w+)\))");
    std::regex target_regex = named_list;
    std::smatch m;

    String input = matches[1].str();

    bool is_switch_expansion = false;

    if (input.find("include") == 0)
      return;

    if (input.length() > 0 && input[0] == '-') {
      is_switch_expansion = true;
      target_regex = switch_expansion;
      // safe to remove backslash at this point, before paths are expanded
      input = trim(replace(matches[0].str(), "\\\\", "")); // yep we need all 4 to replace one bs
    }

    while (!is_switch_expansion && std::regex_search(input, m, named_list) != 0) {

      StringStream list;
      String result;

      if (lists.count(m[1].str()) > 0)
      {
        for (auto item : lists[m[1].str()])
          list << item << " ";

        result = trim(list.str());
      } else {
        // change format so loop won't get stuck, need to leave arg in place for foreach
        result = "{{" + m[1].str() + "}}";
      }

      // if (is_switch_expansion) 
      //   input = 
      // else
      input = input.substr(0, m.position(0)) + result + input.substr(m.position(0) + m[0].str().length());
    }

    while (is_switch_expansion && std::regex_search(input, m, switch_expansion) != 0) {
      StringStream list;
      String result;

      if (lists.count(m[2].str()) > 0)
      {
        for (auto item : lists[m[2].str()])
          list << item << " ";

        result = trim(list.str());
      } else {
        // change format so loop won't get stuck, need to leave arg in place for foreach
        result = m[2].str();
      }

      input = m[1].str() + result + m[3].str();
    }

    if (is_switch_expansion) {
      curr_list.push_back(input);
      return;
    }

    auto parts = split(input, ' ');
    if (parts[0] == "call" && parts[1] == "my-dir")
    {
      curr_list.clear();
      curr_list.push_back(trim(make_dir.string()));
      return;
    }

    if (parts[0] == "wildcard") {
      fs::path path(parts[1]);
      NormzWin32Path(path);
      GetWildcardFiles(path, curr_list);
      return;
    }

    if (parts[0] == "foreach")
    {
      GetForeachFiles(input, curr_list, matches[1].str());
      return;
    }

    curr_list.push_back(input);
  }

  String AssetCache::HashMakeFileInput(fs::path make_file, String makeCommand, bool useSscHeaders) {
    String contents = readFile(make_file);
    const std::regex listInit(R"((.*)\s(\:|\+?)=)");
    const std::regex listAdd(R"((.*)\s*\+=)");
    const std::regex re(R"(.*(\.cpp|\.cc|\.c))");
    // const std::regex expansion(R"(\$\((\w+) (.*)\))");
    const std::regex switch_expansion(R"((-\w+)\$\((.*)\)(.*))");
    const std::regex expansion(R"(\$\((.*)\))");
    const std::regex wildcard(R"(\$\(wildcard (.*)\))");
    const std::regex reLocalPath(R"(\$\(LOCAL_PATH\))");
    const std::regex dot(R"(\.)");
    const std::regex asterix(R"(\*)");
    const std::regex backslash(R"(\\)");
    const std::regex SSC_(R"(-D((SSC_)(\w+)=(\s*)([\w|.]*)))");
    std::smatch matches;
    std::smatch m2;
    std::smatch m3;
    std::vector<fs::path> source_files;

    String token;
    auto realAppRoot = appRoot.parent_path().parent_path();
    std::map<fs::path, bool> unique_headers;
    std::vector<std::thread*> source_threads;
    int active_threads = 0;

    String curr_list;
    // lists only partially thread locked, don't use in threads
    Mutex listsMutex;
    std::map<String, std::vector<String>> lists;

    int joined = 0;
    int errored = 0;

    StringStream sansVersionMake;

    while (token != contents) {
      token = contents.substr(0,contents.find_first_of("\n"));
      contents = contents.substr(contents.find_first_of("\n") + 1);
      
      try {        
        String tempInput = token;
        while (std::regex_search(tempInput, m3, SSC_) != 0)
        {
          if (m3[3].str().find("VERSION") == std::string::npos)
          {
            // not a version or hash, remove SSC_ so it won't match again, but keep rest of settings as changing it should change hash
            tempInput = tempInput.substr(0, m3.position(2)) + tempInput.substr(m3.position(2) + m3.length(2));
          } else {
            // remove entire -D option, we don't want values that change
            tempInput = tempInput.substr(0, m3.position(0)) + tempInput.substr(m3.position(0) + m3.length(0));
          }
        }

        sansVersionMake << tempInput << std::endl;
      } catch (std::exception &e) {
        std::cerr << "SSC_Match error: " << e.what() << std::endl;
        throw;
      }

      if (token.length() == 0 || token[0] == '#')
        ; // ignore
      else if (token == "include $(BUILD_SHARED_LIBRARY)")
      {
        // stop processing, don't overwrite LOCAL_SRC_FILES with libs
        break;
      } else if (token.find("include") == 0) {
        // include not yet handled, need to set up a separate regex so it doesn't nuke other values, otherwise expansion handled as an assignment to curr_list
      }
      else if (std::regex_search(token, matches, listInit) != 0) {
        curr_list = matches[1].str();

        String rhs = (String)token.substr(matches.position(2)+3);
        if (std::regex_search(rhs, m2, expansion)) {
          Lock lock(listsMutex);
          ApplyMakeExpansions(lists, lists[curr_list], make_file.parent_path(), m2);
        } else if (std::regex_search(rhs, m2, re)) {
          lists[curr_list].push_back(trim(m2[0].str()));
        }
      } else if (std::regex_search(token, matches, switch_expansion) != 0) {
          Lock lock(listsMutex);
          ApplyMakeExpansions(lists, lists[curr_list], make_file.parent_path(), matches);
      } else if (std::regex_search(token, matches, expansion) != 0) {
          Lock lock(listsMutex);
          ApplyMakeExpansions(lists, lists[curr_list], make_file.parent_path(), matches);
      } else if (std::regex_search(token, matches, re) != 0) {
        lists[curr_list].push_back(trim(matches[0].str()));
      } else if (curr_list.length() > 0)
      {
        lists[curr_list].push_back(token);
      }
    }

    std::vector<fs::path> include_paths;
    for (auto option : lists["LOCAL_CFLAGS"]) {
      if (option.find("-I") == 0) {
        fs::path include(option.substr(2));
        NormzWin32Path(include);
        include_paths.push_back(include);
      }
    }

    for (auto file : lists["LOCAL_SRC_FILES"]) {
      fs::path source(file);

      if (!fs::exists(file))
      {
        if (lists.count("LOCAL_PATH") > 0)
          source = lists["LOCAL_PATH"][0] / source;
        else
          source = make_file.parent_path() / source;
      }
      
      NormzWin32Path(source);

      try {        
        while (active_threads >= 16) {
          for (auto& th : source_threads)
            if (th->joinable())
            {
              try {
                th->join();
              }
              catch (std::exception e) {
                errored++;
                std::cerr << "HashFiles: Error joining thread: " << e.what() << std::endl;
                throw e;
              }
              joined++;
              active_threads--;
              break;
            }
        }

        source_threads.push_back(new std::thread([&](fs::path _source) {
          try {
            this->GetCFileHeaders(make_file.parent_path(), include_paths, _source, unique_headers, useSscHeaders);
          } catch (std::exception &e) {
            std::cerr << "make header error on " << _source.string().c_str() << e.what() << std::endl;
          }
        }, (source)));

        active_threads++;

        source_files.push_back(source);
      } catch (std::exception &e) {
        std::cerr << "make header error on " << source.string().c_str() << e.what() << std::endl;
      }
    }

    while (joined < source_threads.size()) {
      if (joined > 0)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }
      for (auto& th : source_threads)
        if (th->joinable())
        {
          try {
            th->join();
            joined++;
          }
          catch (std::exception e) {
            std::cerr << "HashFiles: Error joining thread: " << e.what() << std::endl;
            throw e;
          }
          active_threads--;
        }
    }

    // auto report = (make_file.parent_path().parent_path().stem().string() + ": Hashed " + 
    //   std::to_string(source_files.size()) + " .cc / " + std::to_string(unique_headers.size()) + " .hh, joined: " + 
    //   std::to_string(joined) + "/" + std::to_string(source_threads.size()) + ", "
    //   );

    GetUniqueHeaders(unique_headers, source_files);

    auto hash = HashFiles(source_files, { sansVersionMake.str(), makeCommand });
    // log(report + hash);
    return hash;
  }

  bool AssetCache::RestoreCache(String hash, String assetName, fs::path dest) {

    auto cachePath = cacheRoot / hash / assetName;
    if (fs::exists(cachePath)) {
      if (fs::is_directory(cachePath) && !fs::is_empty(cachePath)) {
        log("Use cache " + cachePath.string() + " => " + dest.string());

        // create directories and ignore error, it is likely that other RestoreCache calls have already created folder.
        std::error_code ec;
        fs::create_directories(dest, ec);

        try {
          fs::copy(cachePath, dest, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
        } catch (fs::filesystem_error &e) {
          // TODO(mribbons): compare source/dest files before crashing
          std::cerr << "Failed to restore cache to " << dest.string() << " , exists? " << fs::exists(dest) << ": " << e.what() << std::endl;
          throw;
        }
        return true;
      }
    }

    log("cache miss for " + hash + ", " + assetName);
    return false;
  }

  void AssetCache::UpdateCache(String hash, String assetName, fs::path source) {
    auto cachePath = cacheRoot / hash / assetName;
    log("Update cache " + source.string() + " => " + cachePath.string());
    fs::create_directories(cachePath);
    fs::copy(source, cachePath, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
  }
}