#ifndef SOCKET_RUNTIME_WINDOW_HOTKEY_H
#define SOCKET_RUNTIME_WINDOW_HOTKEY_H

#include "../platform.hh"

namespace ssc::runtime::window {
  // forward
  class Window;
  class HotKeyContext;

  class HotKeyCodeMap {
    public:
      using Code = unsigned int;
      using Key = String;
      using Map = Map<Key, Code>;

      Map keys;
      Map modifiers;

      HotKeyCodeMap ();
      const Code get (Key key) const;
      const bool isModifier (Key key) const;
      const bool isKey (Key key) const;
  };

  class HotKeyBinding {
    public:
      using ID = uint32_t;
      using Hash = std::size_t;
      using Token = String;
      using Sequence = Vector<Token>;
      using Expression = String;

      struct Options {
        bool passive = true;
      };

    #if SOCKET_RUNTIME_PLATFORM_LINUX
      struct GTKKeyPressEventContext {
        HotKeyContext* context = nullptr;
        ID id = 0;
        guint signal = 0;
      };
    #endif

      static Sequence parseExpression (Expression input);

      HotKeyCodeMap codeMap;
      HotKeyCodeMap::Code modifiers = 0;
      HotKeyCodeMap::Code keys = 0;
      Hash hash = 0;
      ID id = 0;

      Expression expression;
      Sequence sequence;

      Options options;

    #if SOCKET_RUNTIME_PLATFORM_MACOS
      // Apple Carbon API
      EventHotKeyRef eventHotKeyRef;
    #endif

      HotKeyBinding (ID id, Expression input);
      bool isValid () const;
  };

  class HotKeyContext {
    public:
      using Bindings = Map<HotKeyBinding::ID, HotKeyBinding>;

    #if SOCKET_RUNTIME_PLATFORM_MACOS
      // Apple Carbon API
      EventTargetRef eventTarget;
    #elif SOCKET_RUNTIME_PLATFORM_LINUX
      Map<
        HotKeyBinding::ID,
        HotKeyBinding::GTKKeyPressEventContext
      > gtkKeyPressEventContexts;
    #endif

      Window* window = nullptr;

      // state
      Mutex mutex;
      // owned by the context
      Vector<HotKeyBinding::ID> bindingIds;
      Bindings& bindings;

      HotKeyContext (const HotKeyContext&) = delete;
      HotKeyContext (Window* window);
      ~HotKeyContext ();

      void init ();
      void reset ();
      const HotKeyBinding bind (
        HotKeyBinding::Expression expression,
        HotKeyBinding::Options options = {}
      );
      bool unbind (HotKeyBinding::ID id);
      bool hasBindingForExpression (HotKeyBinding::Expression expression) const;
      const HotKeyBinding getBindingForExpression (HotKeyBinding::Expression expression) const;
      bool onHotKeyBindingCallback (HotKeyBinding::ID);
  };
}
#endif
