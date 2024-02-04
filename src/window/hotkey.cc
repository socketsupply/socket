#include "hotkey.hh"
#include "../core/string.hh"
#include "../core/json.hh"
#include "../ipc/ipc.hh"

namespace SSC {
  // bindings are global to the entire application so we maintain these
  // global variables for all windows
  static HotKeyBinding::ID nextGlobalBindingID = 1024;
  static HotKeyContext::Bindings globalBindings;

#if defined(__APPLE__) && (!TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR)
  static OSStatus carbonEventHandlerCallback (
    EventHandlerCallRef eventHandlerCallRef,
    EventRef eventRef,
    void* userData
   ) {
    // The hot key event identifier that has an unique internal id that maps
    // to a specific hot key binding in a context
    EventHotKeyID eventHotKeyID;

    // bail early if somehow user data is `nullptr`,
    // meaning we are unable to deteremine a `HotKeyContext`
    if (userData == nullptr) {
      return noErr;
    }

    auto context = reinterpret_cast<HotKeyContext*>(userData);

    // if the context was removed somehow, bail early
    if (context == nullptr || context->bridge == nullptr) {
      return noErr;
    }

    GetEventParameter(
      eventRef,
      kEventParamDirectObject,
      typeEventHotKeyID,
      NULL,
      sizeof(eventHotKeyID),
      NULL,
      &eventHotKeyID
    );

    context->onHotKeyBindingCallback(eventHotKeyID.id);

    return noErr;
  }
#endif

  HotKeyCodeMap::HotKeyCodeMap () {
  #if defined(__APPLE__) && (!TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR)
    keys.insert_or_assign("a", kVK_ANSI_A);
    keys.insert_or_assign("b", kVK_ANSI_B);
    keys.insert_or_assign("c", kVK_ANSI_C);
    keys.insert_or_assign("d", kVK_ANSI_D);
    keys.insert_or_assign("e", kVK_ANSI_E);
    keys.insert_or_assign("f", kVK_ANSI_F);
    keys.insert_or_assign("g", kVK_ANSI_G);
    keys.insert_or_assign("h", kVK_ANSI_H);
    keys.insert_or_assign("i", kVK_ANSI_I);
    keys.insert_or_assign("j", kVK_ANSI_J);
    keys.insert_or_assign("k", kVK_ANSI_K);
    keys.insert_or_assign("l", kVK_ANSI_L);
    keys.insert_or_assign("m", kVK_ANSI_M);
    keys.insert_or_assign("n", kVK_ANSI_N);
    keys.insert_or_assign("o", kVK_ANSI_O);
    keys.insert_or_assign("p", kVK_ANSI_P);
    keys.insert_or_assign("q", kVK_ANSI_Q);
    keys.insert_or_assign("r", kVK_ANSI_R);
    keys.insert_or_assign("s", kVK_ANSI_S);
    keys.insert_or_assign("t", kVK_ANSI_T);
    keys.insert_or_assign("u", kVK_ANSI_U);
    keys.insert_or_assign("v", kVK_ANSI_V);
    keys.insert_or_assign("w", kVK_ANSI_W);
    keys.insert_or_assign("x", kVK_ANSI_X);
    keys.insert_or_assign("y", kVK_ANSI_Y);
    keys.insert_or_assign("z", kVK_ANSI_Z);

    keys.insert_or_assign("0", kVK_ANSI_0);
    keys.insert_or_assign("1", kVK_ANSI_1);
    keys.insert_or_assign("2", kVK_ANSI_2);
    keys.insert_or_assign("3", kVK_ANSI_3);
    keys.insert_or_assign("4", kVK_ANSI_4);
    keys.insert_or_assign("5", kVK_ANSI_5);
    keys.insert_or_assign("6", kVK_ANSI_6);
    keys.insert_or_assign("7", kVK_ANSI_7);
    keys.insert_or_assign("8", kVK_ANSI_8);
    keys.insert_or_assign("9", kVK_ANSI_9);

    keys.insert_or_assign("=", kVK_ANSI_Equal);
    keys.insert_or_assign("-", kVK_ANSI_Minus);
    keys.insert_or_assign("]", kVK_ANSI_RightBracket);
    keys.insert_or_assign("[", kVK_ANSI_LeftBracket);
    keys.insert_or_assign("'", kVK_ANSI_Quote);
    keys.insert_or_assign(";", kVK_ANSI_Semicolon);
    keys.insert_or_assign("\\", kVK_ANSI_Backslash);
    keys.insert_or_assign(",", kVK_ANSI_Comma);
    keys.insert_or_assign("/", kVK_ANSI_Slash);
    keys.insert_or_assign(".", kVK_ANSI_Period);
    keys.insert_or_assign("`", kVK_ANSI_Grave);

    keys.insert_or_assign("return", kVK_Return);
    keys.insert_or_assign("enter", kVK_Return);
    keys.insert_or_assign("tab", kVK_Tab);
    keys.insert_or_assign("space", kVK_Space);
    keys.insert_or_assign("delete", kVK_Delete);
    keys.insert_or_assign("escape", kVK_Escape);
    keys.insert_or_assign("shift", kVK_Shift);
    keys.insert_or_assign("caps lock", kVK_CapsLock);
    keys.insert_or_assign("right shift", kVK_RightShift);
    keys.insert_or_assign("right option", kVK_RightOption);
    keys.insert_or_assign("right opt", kVK_RightOption);
    keys.insert_or_assign("right alt", kVK_RightOption);
    keys.insert_or_assign("right control", kVK_RightControl);
    keys.insert_or_assign("right ctrl", kVK_RightControl);
    keys.insert_or_assign("fn", kVK_Function);
    keys.insert_or_assign("volume up", kVK_VolumeUp);
    keys.insert_or_assign("volume down", kVK_VolumeDown);
    keys.insert_or_assign("volume mute", kVK_Mute);
    keys.insert_or_assign("mute", kVK_Mute);

    keys.insert_or_assign("f1", kVK_F1);
    keys.insert_or_assign("f2", kVK_F2);
    keys.insert_or_assign("f3", kVK_F4);
    keys.insert_or_assign("f4", kVK_F3);
    keys.insert_or_assign("f5", kVK_F5);
    keys.insert_or_assign("f6", kVK_F6);
    keys.insert_or_assign("f7", kVK_F7);
    keys.insert_or_assign("f8", kVK_F8);
    keys.insert_or_assign("f9", kVK_F9);
    keys.insert_or_assign("f10", kVK_F10);
    keys.insert_or_assign("f11", kVK_F11);
    keys.insert_or_assign("f12", kVK_F12);
    keys.insert_or_assign("f13", kVK_F13);
    keys.insert_or_assign("f14", kVK_F14);
    keys.insert_or_assign("f15", kVK_F15);
    keys.insert_or_assign("f16", kVK_F16);
    keys.insert_or_assign("f17", kVK_F17);
    keys.insert_or_assign("f18", kVK_F18);
    keys.insert_or_assign("f18", kVK_F19);
    keys.insert_or_assign("f20", kVK_F20);

    keys.insert_or_assign("help", kVK_Help);
    keys.insert_or_assign("home", kVK_Home);
    keys.insert_or_assign("end", kVK_End);
    keys.insert_or_assign("page up", kVK_PageUp);
    keys.insert_or_assign("page down", kVK_PageDown);
    keys.insert_or_assign("up", kVK_UpArrow);
    keys.insert_or_assign("down", kVK_DownArrow);
    keys.insert_or_assign("left", kVK_LeftArrow);
    keys.insert_or_assign("right", kVK_RightArrow);

    modifiers.insert_or_assign("command", cmdKey);
    modifiers.insert_or_assign("super", cmdKey);
    modifiers.insert_or_assign("cmd", cmdKey);
    modifiers.insert_or_assign("apple", cmdKey);
    modifiers.insert_or_assign("meta", cmdKey);

    modifiers.insert_or_assign("option", optionKey);
    modifiers.insert_or_assign("opt", optionKey);
    modifiers.insert_or_assign("alt", optionKey);

    modifiers.insert_or_assign("control", controlKey);
    modifiers.insert_or_assign("ctrl", controlKey);

    modifiers.insert_or_assign("shift", shiftKey);
  #elif defined(__linux__) && !defined(__ANDROID__)
    keys.insert_or_assign("a", GDK_KEY_a);
    keys.insert_or_assign("b", GDK_KEY_b);
    keys.insert_or_assign("c", GDK_KEY_c);
    keys.insert_or_assign("d", GDK_KEY_d);
    keys.insert_or_assign("e", GDK_KEY_e);
    keys.insert_or_assign("f", GDK_KEY_f);
    keys.insert_or_assign("g", GDK_KEY_g);
    keys.insert_or_assign("h", GDK_KEY_h);
    keys.insert_or_assign("i", GDK_KEY_i);
    keys.insert_or_assign("j", GDK_KEY_j);
    keys.insert_or_assign("k", GDK_KEY_k);
    keys.insert_or_assign("l", GDK_KEY_l);
    keys.insert_or_assign("m", GDK_KEY_m);
    keys.insert_or_assign("n", GDK_KEY_n);
    keys.insert_or_assign("o", GDK_KEY_o);
    keys.insert_or_assign("p", GDK_KEY_p);
    keys.insert_or_assign("q", GDK_KEY_q);
    keys.insert_or_assign("r", GDK_KEY_r);
    keys.insert_or_assign("s", GDK_KEY_s);
    keys.insert_or_assign("t", GDK_KEY_t);
    keys.insert_or_assign("u", GDK_KEY_u);
    keys.insert_or_assign("v", GDK_KEY_v);
    keys.insert_or_assign("w", GDK_KEY_w);
    keys.insert_or_assign("x", GDK_KEY_x);
    keys.insert_or_assign("y", GDK_KEY_y);
    keys.insert_or_assign("z", GDK_KEY_z);

    keys.insert_or_assign("A", GDK_KEY_A);
    keys.insert_or_assign("B", GDK_KEY_B);
    keys.insert_or_assign("C", GDK_KEY_C);
    keys.insert_or_assign("D", GDK_KEY_D);
    keys.insert_or_assign("E", GDK_KEY_E);
    keys.insert_or_assign("F", GDK_KEY_F);
    keys.insert_or_assign("G", GDK_KEY_G);
    keys.insert_or_assign("H", GDK_KEY_H);
    keys.insert_or_assign("I", GDK_KEY_I);
    keys.insert_or_assign("J", GDK_KEY_J);
    keys.insert_or_assign("K", GDK_KEY_K);
    keys.insert_or_assign("L", GDK_KEY_L);
    keys.insert_or_assign("M", GDK_KEY_M);
    keys.insert_or_assign("N", GDK_KEY_N);
    keys.insert_or_assign("O", GDK_KEY_O);
    keys.insert_or_assign("P", GDK_KEY_P);
    keys.insert_or_assign("Q", GDK_KEY_Q);
    keys.insert_or_assign("R", GDK_KEY_R);
    keys.insert_or_assign("S", GDK_KEY_S);
    keys.insert_or_assign("T", GDK_KEY_T);
    keys.insert_or_assign("U", GDK_KEY_U);
    keys.insert_or_assign("V", GDK_KEY_V);
    keys.insert_or_assign("W", GDK_KEY_W);
    keys.insert_or_assign("X", GDK_KEY_X);
    keys.insert_or_assign("Y", GDK_KEY_Y);
    keys.insert_or_assign("Z", GDK_KEY_Z);

    keys.insert_or_assign("0", GDK_KEY_0);
    keys.insert_or_assign("1", GDK_KEY_1);
    keys.insert_or_assign("2", GDK_KEY_2);
    keys.insert_or_assign("3", GDK_KEY_3);
    keys.insert_or_assign("4", GDK_KEY_4);
    keys.insert_or_assign("5", GDK_KEY_5);
    keys.insert_or_assign("6", GDK_KEY_6);
    keys.insert_or_assign("7", GDK_KEY_7);
    keys.insert_or_assign("8", GDK_KEY_8);
    keys.insert_or_assign("9", GDK_KEY_9);

    keys.insert_or_assign("=", GDK_KEY_equal);
    keys.insert_or_assign("-", GDK_KEY_minus);
    keys.insert_or_assign("]", GDK_KEY_bracketright);
    keys.insert_or_assign("[", GDK_KEY_bracketleft);
    keys.insert_or_assign("'", GDK_KEY_quoteright);
    keys.insert_or_assign("\"", GDK_KEY_quotedbl);
    keys.insert_or_assign(";", GDK_KEY_semicolon);
    keys.insert_or_assign("\\", GDK_KEY_backslash);
    keys.insert_or_assign(",", GDK_KEY_comma);
    keys.insert_or_assign("/", GDK_KEY_slash);
    keys.insert_or_assign(".", GDK_KEY_period);
    keys.insert_or_assign("`", GDK_KEY_grave);

    keys.insert_or_assign("return", GDK_KEY_Return);
    keys.insert_or_assign("enter", GDK_KEY_Return);
    keys.insert_or_assign("tab", GDK_KEY_Tab);
    keys.insert_or_assign("space", GDK_KEY_space);
    keys.insert_or_assign("delete", GDK_KEY_Delete);
    keys.insert_or_assign("escape", GDK_KEY_Escape);
    keys.insert_or_assign("shift", GDK_KEY_Shift_L);
    keys.insert_or_assign("left shift", GDK_KEY_Shift_L);
    keys.insert_or_assign("caps lock", GDK_KEY_Caps_Lock);
    keys.insert_or_assign("right shift", GDK_KEY_Shift_R);
    keys.insert_or_assign("right option", GDK_KEY_Option);
    keys.insert_or_assign("right opt", GDK_KEY_Option);
    keys.insert_or_assign("right alt", GDK_KEY_Option);
    keys.insert_or_assign("right control", GDK_KEY_Control_R);
    keys.insert_or_assign("right ctrl", GDK_KEY_Control_R);
    keys.insert_or_assign("left control", GDK_KEY_Control_L);
    keys.insert_or_assign("left ctrl", GDK_KEY_Control_L);
    keys.insert_or_assign("fn", GDK_KEY_function);
    keys.insert_or_assign("volume up", GDK_KEY_AudioRaiseVolume);
    keys.insert_or_assign("volume down", GDK_KEY_AudioLowerVolume);
    keys.insert_or_assign("volume mute", GDK_KEY_AudioMute);
    keys.insert_or_assign("mute", GDK_KEY_AudioMute);
    keys.insert_or_assign("play", GDK_KEY_AudioPlay);

    keys.insert_or_assign("f1", GDK_KEY_F1);
    keys.insert_or_assign("f2", GDK_KEY_F2);
    keys.insert_or_assign("f3", GDK_KEY_F4);
    keys.insert_or_assign("f4", GDK_KEY_F3);
    keys.insert_or_assign("f5", GDK_KEY_F5);
    keys.insert_or_assign("f6", GDK_KEY_F6);
    keys.insert_or_assign("f7", GDK_KEY_F7);
    keys.insert_or_assign("f8", GDK_KEY_F8);
    keys.insert_or_assign("f9", GDK_KEY_F9);
    keys.insert_or_assign("f10", GDK_KEY_F10);
    keys.insert_or_assign("f11", GDK_KEY_F11);
    keys.insert_or_assign("f12", GDK_KEY_F12);
    keys.insert_or_assign("f13", GDK_KEY_F13);
    keys.insert_or_assign("f14", GDK_KEY_F14);
    keys.insert_or_assign("f15", GDK_KEY_F15);
    keys.insert_or_assign("f16", GDK_KEY_F16);
    keys.insert_or_assign("f17", GDK_KEY_F17);
    keys.insert_or_assign("f18", GDK_KEY_F18);
    keys.insert_or_assign("f18", GDK_KEY_F19);
    keys.insert_or_assign("f20", GDK_KEY_F20);

    keys.insert_or_assign("help", GDK_KEY_Help);
    keys.insert_or_assign("home", GDK_KEY_Home);
    keys.insert_or_assign("end", GDK_KEY_End);
    keys.insert_or_assign("page up", GDK_KEY_Page_Up);
    keys.insert_or_assign("page down", GDK_KEY_Page_Down);
    keys.insert_or_assign("up", GDK_KEY_Up);
    keys.insert_or_assign("down", GDK_KEY_Down);
    keys.insert_or_assign("left", GDK_KEY_Left);
    keys.insert_or_assign("right", GDK_KEY_Right);

    modifiers.insert_or_assign("meta", GDK_META_MASK);
    modifiers.insert_or_assign("right alt", GDK_META_MASK);

    modifiers.insert_or_assign("command", GDK_SUPER_MASK);
    modifiers.insert_or_assign("super", GDK_SUPER_MASK);
    modifiers.insert_or_assign("cmd", GDK_SUPER_MASK);

    modifiers.insert_or_assign("option", GDK_ALT_MASK);
    modifiers.insert_or_assign("opt", GDK_ALT_MASK);
    modifiers.insert_or_assign("alt", GDK_ALT_MASK);

    modifiers.insert_or_assign("control", GDK_CONTROL_MASK);
    modifiers.insert_or_assign("ctrl", GDK_CONTROL_MASK);

    modifiers.insert_or_assign("right shift", GDK_SHIFT_MASK);
    modifiers.insert_or_assign("left shift", GDK_SHIFT_MASK);
    modifiers.insert_or_assign("shift", GDK_SHIFT_MASK);
  #elif defined(_WIN32)
    keys.insert_or_assign("a", 0x41);
    keys.insert_or_assign("b", 0x42);
    keys.insert_or_assign("c", 0x43);
    keys.insert_or_assign("d", 0x44);
    keys.insert_or_assign("e", 0x45);
    keys.insert_or_assign("f", 0x46);
    keys.insert_or_assign("g", 0x47);
    keys.insert_or_assign("h", 0x48);
    keys.insert_or_assign("i", 0x49);
    keys.insert_or_assign("j", 0x4A);
    keys.insert_or_assign("k", 0x4B);
    keys.insert_or_assign("l", 0x4C);
    keys.insert_or_assign("m", 0x4D);
    keys.insert_or_assign("n", 0x4E);
    keys.insert_or_assign("o", 0x4F);
    keys.insert_or_assign("p", 0x50);
    keys.insert_or_assign("q", 0x51);
    keys.insert_or_assign("r", 0x52);
    keys.insert_or_assign("s", 0x53);
    keys.insert_or_assign("t", 0x54);
    keys.insert_or_assign("u", 0x55);
    keys.insert_or_assign("v", 0x56);
    keys.insert_or_assign("w", 0x57);
    keys.insert_or_assign("x", 0x58);
    keys.insert_or_assign("y", 0x59);
    keys.insert_or_assign("z", 0x5A);

    keys.insert_or_assign("0", 0x30);
    keys.insert_or_assign("1", 0x31);
    keys.insert_or_assign("2", 0x32);
    keys.insert_or_assign("3", 0x33);
    keys.insert_or_assign("4", 0x34);
    keys.insert_or_assign("5", 0x35);
    keys.insert_or_assign("6", 0x36);
    keys.insert_or_assign("7", 0x37);
    keys.insert_or_assign("8", 0x38);
    keys.insert_or_assign("9", 0x39);

    keys.insert_or_assign("-", VK_OEM_MINUS);
    keys.insert_or_assign("]", VK_OEM_6);
    keys.insert_or_assign("[", VK_OEM_4);
    keys.insert_or_assign("'", VK_OEM_7);
    keys.insert_or_assign(";", VK_OEM_1);
    keys.insert_or_assign("\\", VK_OEM_5);
    keys.insert_or_assign(",", VK_OEM_COMMA);
    keys.insert_or_assign("/", VK_OEM_2);
    keys.insert_or_assign(".", VK_OEM_PERIOD);
    keys.insert_or_assign("`", VK_OEM_3);

    keys.insert_or_assign("return", VK_RETURN);
    keys.insert_or_assign("enter", VK_RETURN);
    keys.insert_or_assign("tab", VK_TAB);
    keys.insert_or_assign("space", VK_SPACE);
    keys.insert_or_assign("backspace", VK_BACK);
    keys.insert_or_assign("delete", VK_DELETE);
    keys.insert_or_assign("escape", VK_ESCAPE);
    keys.insert_or_assign("shift", VK_LSHIFT);
    keys.insert_or_assign("left shift", VK_LSHIFT);
    keys.insert_or_assign("right shift", VK_RSHIFT);
    keys.insert_or_assign("caps lock", VK_CAPITAL);
    keys.insert_or_assign("right option", VK_RMENU);
    keys.insert_or_assign("right opt", VK_RMENU);
    keys.insert_or_assign("right alt", VK_RMENU);
    keys.insert_or_assign("left option", VK_LMENU);
    keys.insert_or_assign("left opt", VK_LMENU);
    keys.insert_or_assign("left alt", VK_LMENU);
    keys.insert_or_assign("right control", VK_RCONTROL);
    keys.insert_or_assign("right ctrl", VK_RCONTROL);
    keys.insert_or_assign("left control", VK_LCONTLOL);
    keys.insert_or_assign("left ctrl", VK_LCONTLOL);
    keys.insert_or_assign("volume up", VK_VOLUME_UP);
    keys.insert_or_assign("volume down", VK_VOLUME_DOWN);
    keys.insert_or_assign("volume mute", VK_VOLUME_MUTE);
    keys.insert_or_assign("mute", VK_VOLUME_MUTE);

    keys.insert_or_assign("f1", VK_F1);
    keys.insert_or_assign("f2", VK_F2);
    keys.insert_or_assign("f3", VK_F4);
    keys.insert_or_assign("f4", VK_F3);
    keys.insert_or_assign("f5", VK_F5);
    keys.insert_or_assign("f6", VK_F6);
    keys.insert_or_assign("f7", VK_F7);
    keys.insert_or_assign("f8", VK_F8);
    keys.insert_or_assign("f9", VK_F9);
    keys.insert_or_assign("f10", VK_F10);
    keys.insert_or_assign("f11", VK_F11);
    keys.insert_or_assign("f12", VK_F12);
    keys.insert_or_assign("f13", VK_F13);
    keys.insert_or_assign("f14", VK_F14);
    keys.insert_or_assign("f15", VK_F15);
    keys.insert_or_assign("f16", VK_F16);
    keys.insert_or_assign("f17", VK_F17);
    keys.insert_or_assign("f18", VK_F18);
    keys.insert_or_assign("f18", VK_F19);
    keys.insert_or_assign("f20", VK_F20);

    keys.insert_or_assign("help", VK_HELP);
    keys.insert_or_assign("home", VK_HOME);
    keys.insert_or_assign("end", VK_END);
    keys.insert_or_assign("page up", VK_PRIOR);
    keys.insert_or_assign("page down", VK_NEXT);
    keys.insert_or_assign("up", VK_UP);
    keys.insert_or_assign("down", VK_DOWN);
    keys.insert_or_assign("left", VK_LEFT);
    keys.insert_or_assign("right", VK_RIGHT);

    modifiers.insert_or_assign("right alt", MOD_ALT);

    modifiers.insert_or_assign("command", MOD_WIN);
    modifiers.insert_or_assign("windows", MOD_WIN);
    modifiers.insert_or_assign("super", MOD_WIN);
    modifiers.insert_or_assign("meta", MOD_WIN);
    modifiers.insert_or_assign("cmd", MOD_WIN);

    modifiers.insert_or_assign("option", MOD_ALT);
    modifiers.insert_or_assign("opt", MOD_ALT);
    modifiers.insert_or_assign("alt", MOD_ALT);

    modifiers.insert_or_assign("control", MOD_CONTROL);
    modifiers.insert_or_assign("ctrl", MOD_CONTROL);

    modifiers.insert_or_assign("shift", MOD_SHIFT);
    modifiers.insert_or_assign("left shift", MOD_SHIFT);
    modifiers.insert_or_assign("right shift", MOD_SHIFT);
  #endif
  }

  const HotKeyCodeMap::Code HotKeyCodeMap::get (Key key) const {
    // normalize key to lower case
    std::transform(
      key.begin(),
      key.end(),
      key.begin(),
      [](auto ch) { return std::tolower(ch); }
    );


    if (keys.contains(key)) {
      return keys.at(key);
    }

    if (modifiers.contains(key)) {
      return modifiers.at(key);
    }

    return -1;
  }

  const bool HotKeyCodeMap::isModifier (Key key) const {
    // normalize key to lower case
    std::transform(
      key.begin(),
      key.end(),
      key.begin(),
      [](auto ch) { return std::tolower(ch); }
    );

    return modifiers.contains(key);
  }

  const bool HotKeyCodeMap::isKey (Key key) const {
    // normalize key to lower case
    std::transform(
      key.begin(),
      key.end(),
      key.begin(),
      [](auto ch) { return std::tolower(ch); }
    );

    return keys.contains(key);
  }


  HotKeyBinding::Sequence HotKeyBinding::parseExpression (Expression input) {
    static const auto delimiter = "+";
    const auto tokens = split(trim(input), delimiter);
    Sequence sequence;

    for (auto token : tokens) {
      std::transform(
        token.begin(),
        token.end(),
        token.begin(),
        [](auto ch) { return std::tolower(ch); }
      );

      if (std::find(sequence.begin(), sequence.end(), token) == sequence.end()) {
        sequence.push_back(token);
      }
    }

    return sequence;
  }

  HotKeyBinding::HotKeyBinding (ID id, Expression input) : id(id) {
    this->expression = input;
    this->sequence = parseExpression(input);

    for (const auto& token : this->sequence) {
      const auto code = codeMap.get(token);
      if (codeMap.isModifier(token)) {
        this->modifiers += code;
      } else if (codeMap.isKey(token)) {
        this->keys = code;
      }
    }

    if (this->isValid()) {
      this->hash = std::hash<String>{}(join(this->sequence, "+"));
    }
  }

  bool HotKeyBinding::isValid () const {
    return (
      this->id > 0 &&
      this->keys > 0 &&
      this->modifiers > 0 &&
      this->expression.size() > 0
    );
  }

  HotKeyContext::HotKeyContext (Window* window)
    : window(window),
      bindings(globalBindings)
  {}

  HotKeyContext::~HotKeyContext () {
    this->reset();
  }

  void HotKeyContext::init (IPC::Bridge* bridge) {
    static auto userConfig = SSC::getUserConfig()
    this->bridge = bridge;

  #if defined(__APPLE__) && (!TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR)
    // Carbon API event type spec
    static const EventTypeSpec eventTypeSpec = {
      .eventClass = kEventClassKeyboard,
      .eventKind = kEventHotKeyPressed
    };

    eventTarget = GetApplicationEventTarget();

    InstallApplicationEventHandler(
      &carbonEventHandlerCallback,
      1,
      &eventTypeSpec,
      this,
      nullptr
    );
  #elif defined(__linux__) && !defined(__ANDROID__)
    // TODO(@jwerle)
  #elif defined(_WIN32)
  #endif

    this->bridge->router.map("window.hotkey.bind", [=, this](auto message, auto router, auto reply) mutable {
      auto expression = message.get("expression");

      if (expression.size() == 0) {
        auto err = JSON::Object::Entries {
          {"message", "Expression 'expression' in parameters"}
        };
        return reply(IPC::Result::Err { message, err });
      }

      if (userConfig["permissions_allow_hotkeys"] == "false") {
        auto err = JSON::Object::Entries {
          {"type", "SecurityError"},
          {"message", "The HotKey API is not allowed."}
        };
        return reply(IPC::Result::Err { message, err });
      }

      auto binding = this->bind(expression);

      if (!binding.isValid()) {
        auto err = JSON::Object::Entries {
          {"message", "Invalid 'expression' in parameters"}
        };
        return reply(IPC::Result::Err { message, err });
      }

      auto sequence = JSON::Array::Entries {};

      for (const auto& token : binding.sequence) {
        sequence.push_back(token);
      }

      auto data = JSON::Object::Entries {
        {"expression", expression},
        {"sequence", sequence},
        {"hash", binding.hash},
        {"id", binding.id}
      };

      reply(IPC::Result::Data { message, data });
    });

    this->bridge->router.map("window.hotkey.unbind", [=, this](auto message, auto router, auto reply) mutable {
      HotKeyBinding::ID id;
      const auto expression = message.get("expression");

      if (userConfig["permissions_allow_hotkeys"] == "false") {
        auto err = JSON::Object::Entries {
          {"type", "SecurityError"},
          {"message", "The HotKey API is not allowed."}
        };
        return reply(IPC::Result::Err { message, err });
      }

      if (expression.size() > 0) {
        if (this->hasBindingForExpression(expression)) {
          const auto binding = this->getBindingForExpression(expression);
          if (!binding.isValid()) {
            auto err = JSON::Object::Entries {
              {"type", "NotFoundError"},
              {"message", "Binding for expression '" + expression + "' is not valid"}
            };
            return reply(IPC::Result::Err { message, err });
          }

          if (!this->unbind(binding.id)) {
            auto err = JSON::Object::Entries {
              {"message", "Failed to unbind hotkey expression"}
            };
            return reply(IPC::Result::Err { message, err });
          }

          auto data = JSON::Object::Entries {
            {"id", binding.id}
          };

          return reply(IPC::Result::Data { message, data });
        } else {
          auto err = JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Binding for expression '" + expression + "' in does not exist"}
          };
          return reply(IPC::Result::Err { message, err });
        }
      }

      if (!message.has("id") || message.get("id").size() == 0) {
        auto err = JSON::Object::Entries {
          {"message", "Expression 'id' in parameters"}
        };
        return reply(IPC::Result::Err { message, err });
      }

      try {
        id = std::stoul(message.get("id"));
      } catch (...) {
        auto err = JSON::Object::Entries {
          {"message", "Invalid 'id' given in parameters"}
        };

        return reply(IPC::Result::Err { message, err });
      }

      if (!this->bindings.contains(id)) {
        auto err = JSON::Object::Entries {
          {"type", "NotFoundError"},
          {"message", "Invalid 'id' given in parameters"}
        };

        return reply(IPC::Result::Err { message, err });
      }

      if (!this->unbind(id)) {
        auto err = JSON::Object::Entries {
          {"message", "Failed to unbind hotkey expression"}
        };

        return reply(IPC::Result::Err { message, err });
      }

      auto data = JSON::Object::Entries {
        {"id", id}
      };

      return reply(IPC::Result::Data { message, data });
    });

    this->bridge->router.map("window.hotkey.reset", [=, this](auto message, auto router, auto reply) mutable {
      if (userConfig["permissions_allow_hotkeys"] == "false") {
        auto err = JSON::Object::Entries {
          {"type", "SecurityError"},
          {"message", "The HotKey API is not allowed."}
        };
        return reply(IPC::Result::Err { message, err });
      }

      return reply(IPC::Result { message.seq, message });
    });

    this->bridge->router.map("window.hotkey.bindings", [=, this](auto message, auto router, auto reply) mutable {
      auto data = JSON::Array::Entries {};

      if (userConfig["permissions_allow_hotkeys"] == "false") {
        auto err = JSON::Object::Entries {
          {"type", "SecurityError"},
          {"message", "The HotKey API is not allowed."}
        };
        return reply(IPC::Result::Err { message, err });
      }

      for (const auto& entry : this->bindings) {
        const auto& binding = entry.second;
        auto sequence = JSON::Array::Entries {};

        for (const auto& token : binding.sequence) {
          sequence.push_back(token);
        }

        auto json = JSON::Object::Entries {
          {"expression", binding.expression},
          {"sequence", sequence},
          {"hash", binding.hash},
          {"id", binding.id}
        };

        data.push_back(json);
      }

      return reply(IPC::Result::Data { message, data });
    });

    this->bridge->router.map("window.hotkey.mappings", [=, this](auto message, auto router, auto reply) mutable {
      static const HotKeyCodeMap map;
      auto modifiers = JSON::Object::Entries {};
      auto keys = JSON::Object::Entries {};

      if (userConfig["permissions_allow_hotkeys"] == "false") {
        auto err = JSON::Object::Entries {
          {"type", "SecurityError"},
          {"message", "The HotKey API is not allowed."}
        };
        return reply(IPC::Result::Err { message, err });
      }

      for (const auto& entry : map.modifiers) {
        modifiers.insert(entry);
      }

      for (const auto& entry : map.keys) {
        keys.insert(entry);
      }

      auto data = JSON::Object::Entries {
        {"modifiers", modifiers},
        {"keys", keys}
      };

      return reply(IPC::Result::Data { message, data });
    });
  }

  void HotKeyContext::reset () {
    // unregister bindings owned by this context _only_,
    // likely when the window/bridge is unloaded/destroyed
    for (const auto& id : this->bindingIds) {
      this->unbind(id);
    }

    this->bindingIds.clear();
  }

  const HotKeyBinding HotKeyContext::bind (HotKeyBinding::Expression expression) {
    Lock lock(this->mutex);
    static auto userConfig = SSC::getUserConfig();

    if (userConfig["permissions_allow_hotkeys"] == "false") {
      return HotKeyBinding(0, "");
    }

    if (this->bridge == nullptr) {
      return HotKeyBinding(0, "");
    }

    if (this->hasBindingForExpression(expression)) {
      return this->getBindingForExpression(expression);
    }

    auto binding = HotKeyBinding(nextGlobalBindingID, expression);

  #if defined(__APPLE__) && (!TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR)
    EventHotKeyID eventHotKeyID;
    eventHotKeyID.id = binding.id;
    eventHotKeyID.signature = binding.id;

    auto status = RegisterEventHotKey(
      binding.keys,
      binding.modifiers,
      eventHotKeyID,
      eventTarget,
      0,
      &binding.eventHotKeyRef
    );

    if (status != 0) {
      return HotKeyBinding(0, "");
    }
  #elif defined(__linux__) && !defined(__ANDROID__)
    // TODO(@jwerle)
  #elif defined(_WIN32)
    auto status = RegisterHotKey(
      this->window->window,
      (int) binding.id,
      bindings.modifiers,
      bindings.keys
    );

    if (!status) {
      return HotKeyBinding(0, "");
    }
  #endif

    this->bindingIds.push_back(binding.id);
    nextGlobalBindingID = nextGlobalBindingID + 1;
    this->bindings.insert_or_assign(binding.id, binding);

    return binding;
  }

  bool HotKeyContext::unbind (HotKeyBinding::ID id) {
    Lock lock(this->mutex);

    if (this->bridge == nullptr) {
      return false;
    }

    if (!this->bindings.contains(id)) {
      return false;
    }

    const auto& binding = this->bindings.at(id);
  #if defined(__APPLE__) && (!TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR)
    if (UnregisterEventHotKey(binding.eventHotKeyRef) == 0) {
      this->bindings.erase(id);
      return true;
    }
  #elif defined(__linux__) && !defined(__ANDROID__)
    // TODO(@jwerle)
  #elif defined(_WIN32)
    if (UnregisterHotKey(this->window->window, id)) {
      this->bindings.erase(id);
      return true;
    }
  #endif

    return false;
  }

  bool HotKeyContext::hasBindingForExpression (HotKeyBinding::Expression expression) const {
    const auto sequence = HotKeyBinding::parseExpression(expression);
    const auto hash = std::hash<String>{}(join(sequence, "+"));

    for (const auto& binding : bindings) {
      if (binding.second.hash == hash) {
        return true;
      }
    }

    return false;
  }

  const HotKeyBinding HotKeyContext::getBindingForExpression (
    HotKeyBinding::Expression expression
  ) const {
    const auto sequence = HotKeyBinding::parseExpression(expression);
    const auto hash = std::hash<String>{}(join(sequence, "+"));

    for (const auto& entry : this->bindings) {
      if (entry.second.hash == hash) {
        return entry.second;
      }
    }

    return HotKeyBinding(0, "");
  }

  bool HotKeyContext::onHotKeyBindingCallback (HotKeyBinding::ID id) {
    if (this->bridge != nullptr && this->bindings.contains(id)) {
      const auto& binding = this->bindings.at(id);

      if (!binding.isValid()) {
        return false;
      }

      auto sequence = JSON::Array::Entries {};

      for (const auto& token : binding.sequence) {
        sequence.push_back(token);
      }

      auto data = JSON::Object::Entries {
        {"expression", binding.expression},
        {"sequence", sequence},
        {"hash", binding.hash},
        {"id", binding.id}
      };

      auto json = JSON::Object{ data };

      // dispatch 'hotkey' event to webview
      return this->bridge->router.emit("hotkey", json.str());
    }

    return false;
  }
}
