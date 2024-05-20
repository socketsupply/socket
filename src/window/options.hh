#ifndef SOCKET_RUNTIME_WINDOW_OPTIONS_H
#define SOCKET_RUNTIME_WINDOW_OPTIONS_H

#include "../core/config.hh"
#include "../ipc/preload.hh"

namespace SSC {
  /**
   * `WindowOptions` is an extended `IPC::PreloadOptions` container for
   * configuring a new `Window`.
   */
  struct WindowOptions : public IPC::PreloadOptions {
    /**
     * If `true`, the window can be minimized.
     * This option value is only supported on desktop.
     */
    bool minimizable = true;

    /**
     * If `true`, the window can be maximized.
     * This option value is only supported on desktop.
     */
    bool maximizable = true;

    /**
     * If `true`, the window can be resized.
     * This option value is only supported on desktop.
     */
    bool resizable = true;

    /**
     * If `true`, the window can be closed.
     * This option value is only supported on desktop.
     */
    bool closable = true;

    /**
     * If `true`, the window can be "frameless".
     * This option value is only supported on desktop.
     */
    bool frameless = false;

    /**
     * If `true`, the window is considered a "utility" window.
     * This option value is only supported on desktop.
     */
    bool utility = false;

    /**
     * If `true`, the window, when the window is "closed", it can
     * exit the application.
     * This option value is only supported on desktop.
     */
    bool shouldExitApplicationOnClose = false;

    /**
     * The maximum height in screen pixels the window can be.
     */
    float maxHeight = 0.0;

    /**
     * The minimum height in screen pixels the window can be.
     */
    float minHeight = 0.0;

    /**
     * The absolute height in screen pixels the window can be.
     */
    float height = 0.0;

    /**
     * The maximum width in screen pixels the window can be.
     */
    float maxWidth = 0.0;

    /**
     * The minimum width in screen pixels the window can be.
     */
    float minWidth = 0.0;

    /**
     * The absolute width in screen pixels the window can be.
     */
    float width = 0.0;

    /**
     * The window border/corner radius.
     * This option value is only supported on macOS.
     */
    float radius = 0.0;

    /**
     * Thw window frame margin.
     * This option value is only supported on macOS.
     */
    float margin = 0.0;

    /**
     * A string (split on ':') provides two float values which will
     * set the window's aspect ratio.
     * This option value is only supported on desktop.
     */
    String aspectRatio = "";

    /**
     * A string that describes a style for the window title bar.
     * Valid values are:
     *   - hidden
     *   - hiddenInset
     * This option value is only supported on macOS and Windows.
     */
    String titlebarStyle = "";

    /**
     * A string value (split on 'x') in the form of `XxY` where
     *   - `X` is the value in screen pixels offset from the left of the
     *         window frame
     *   - `Y` is the value in screen pixels offset from the top of the
     *         window frame
     * The values control the offset of the "close", "minimize", and "maximize"
     * button controls for a window.
     * This option value is only supported on macOS.
     */
    String windowControlOffsets = "";

    /**
     * A string value in the form of `rgba(r, g, b, a)` where
     *   - `r` is the "red" channel value, an integer between `0` and `255`
     *   - `g` is the "green" channel value, an integer between `0` and `255`
     *   - `b` is the "blue" channel value, an integer between `0` and `255`
     *   - `a` is the "alpha" channel value, a float between `0` and `1`
     * The values represent the background color of a window when the platform
     * system theme is in "light mode". This also be the "default" theme.
     */
    String backgroundColorLight = "";

    /**
     * A string value in the form of `rgba(r, g, b, a)` where
     *   - `r` is the "red" channel value, an integer between `0` and `255`
     *   - `g` is the "green" channel value, an integer between `0` and `255`
     *   - `b` is the "blue" channel value, an integer between `0` and `255`
     *   - `a` is the "alpha" channel value, a float between `0` and `1`
     * The values represent the background color of a window when the platform
     * system theme is in "dark mode".
     */
    String backgroundColorDark = "";

    /**
     * A callback function that is called when a "script message" is received
     * from the WebVew.
     */
    MessageCallback onMessage = [](const String) {};

    /**
     * A callback function that is called when the window wants to exit the
     * application. This function is called _only_ when the
     * `shouldExitApplicationOnClose` option is `true`.
     */
    ExitCallback onExit = nullptr;
  };
}
#endif
