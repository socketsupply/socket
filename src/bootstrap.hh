#ifndef BOOTSTRAP_HH
  #define BOOTSTRAP_HH

  #if _IOS == 0 && _ANDROID == 0
    #include <curl/curl.h>
    #include <filesystem>
    #include <functional>

    namespace Operator {

      #ifndef CURLPIPE_MULTIPLEX
        #define CURLPIPE_MULTIPLEX 0
      #endif

      struct download {
        CURL *easy;
        unsigned int num;
        FILE *out;
      };

      using BootstrapCb =
        std::function<int(void*, double, double, double, double)>;

      inline int bootstrap (const char* src, const char* dest, BootstrapCb* cb) {
        if (std::filesystem::exists(dest)) return 0;

        struct download d;
        CURLM *multi_handle;
        int running = 0; /* keep number of running handles */
        multi_handle = curl_multi_init();
        CURL *hnd = d.easy = curl_easy_init();

        d.out = fopen(dest, "wb");
        if (!d.out) return 1;

        curl_easy_setopt(hnd, CURLOPT_WRITEDATA, d.out);
        curl_easy_setopt(hnd, CURLOPT_URL, src);

        // curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L);
        // curl_easy_setopt(hnd, CURLOPT_DEBUGDATA, t);
        curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, false);
        curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

        // curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);
        // curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 0L);
        if (cb != nullptr) curl_easy_setopt(hnd, CURLOPT_PROGRESSFUNCTION, cb);

        #if (CURLPIPE_MULTIPLEX > 0)
          curl_easy_setopt(hnd, CURLOPT_PIPEWAIT, 1L);
        #endif

        curl_multi_add_handle(multi_handle, d.easy);
        curl_multi_setopt(multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

        do {
          CURLMcode mc = curl_multi_perform(multi_handle, &running);
          if (running) mc = curl_multi_poll(multi_handle, NULL, 0, 1000, NULL);
          if (mc) break;
        } while (running);

        curl_multi_remove_handle(multi_handle, d.easy);
        curl_easy_cleanup(d.easy);
        curl_multi_cleanup(multi_handle);
        return 0;
      }
    } // namespace Operator
  #endif
#endif // BOOTSTRAP_HH
