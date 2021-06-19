#include "common.hh"
#include "../process.h"

#if defined(_WIN32)
  #include "win.hh"
#elif defined(__APPLE__)
  #include "mac.hh"
#elif defined(__linux__)
  #include "linux.hh"
#endif

using namespace Opkit;

MAIN {
  App app;

  Window w0(app, WindowOptions {
    .resizable = true,
    .height = 250,
    .width = 250,
    .title = Str("Main"),
    .frameless = false,
    .preload = Str("")
  });

  Window w1(app, WindowOptions {
    .resizable = false,
    .frameless = true,
    .height = 120,
    .width = 350,
    .preload = Str("")
  });

  Process process(
    "node index.js",
    "test/example/src/main/",
    [&](auto out) {
      //
      // Route messages from the main process to the render process,
      // or if they are "commands" try to do something with them.
      //
      app.dispatch([&] {

        Parse cmd(out);

        if (cmd.name == "show") {
          if (cmd.index == 0) w0.show();
          if (cmd.index == 1) w1.show();
          return;
        }

        if (cmd.name == "hide") {
          if (cmd.index == 0) w0.hide();
          if (cmd.index == 1) w1.hide();
          return;
        }

        if (cmd.name == "navigate") {
          auto s = decodeURIComponent(cmd.args["url"]);

          if (cmd.index == 0) w0.navigate(s);
          if (cmd.index == 1) w1.navigate(s);
          return;
        }

        if (cmd.name == "title") {
          auto s = decodeURIComponent(cmd.args["value"]);

          if (cmd.index == 0) w0.setTitle(s);
          if (cmd.index == 1) w1.setTitle(s);
          return;
        }

        if (cmd.name == "respond") {
          if (cmd.index == 0) w0.eval(cmd.args["value"]);
          if (cmd.index == 1) w1.eval(cmd.args["value"]);
          return;
        }

        std::cout << out << std::endl;
      });
    },
    [&](auto err) {
      std::cerr << err << std::endl;
    }
  );

  //
  // Route messages from the render processes to the main process.
  //
  w0.onMessage([&](auto s) {
    process.write(s);
  });

  w1.onMessage([&](auto s) {
    process.write(s);
  });

  while(app.run() == 0);
}
