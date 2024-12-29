#ifndef SOCKET_RUNTIME_RUNTIME_COLOR_H
#define SOCKET_RUNTIME_RUNTIME_COLOR_H

#include "json.hh"

namespace ssc::runtime::color {
  /**
   * A container for RGB color components with an alpha channel
   */
  class ColorComponents {
    public:
      /**
       * Various pack formats for color components represented as a
       * 32 bit unsigned integer.
       */
      enum class Pack { RGB, RGBA, ARGB };

      /**
       * The minimum "int" color component value.
       */
      static constexpr unsigned int MIN_INT_CHANNEL = 0;

      /**
       * The maximum "int" color component value.
       */
      static constexpr unsigned int MAX_INT_CHANNEL = 255;

      /**
       * The minimum "float" color component value.
       */
      static constexpr float MIN_FLOAT_CHANNEL = 0.0f;

      /**
       * The maximum "float" color component value.
       */
      static constexpr float MAX_FLOAT_CHANNEL = 1.0f;

      /**
       * The "red" color component.
       */
      unsigned int red = MIN_INT_CHANNEL;

      /**
       * The "green" color component.
       */
      unsigned int green = MIN_INT_CHANNEL;

      /**
       * The "blue" color component.
       */
      unsigned int blue = MIN_INT_CHANNEL;

      /**
       * The alpha channel component.
       */
      float alpha = MAX_FLOAT_CHANNEL;

      /**
       * Default `ColorComponents` constructor.
       */
      ColorComponents () = default;

      /**
       * `ColorComponents` "copy" constructor.
       */
      ColorComponents (const ColorComponents&);

      /**
       * `ColorComponents` "move" constructor.
       */
      ColorComponents (ColorComponents&&);

      /**
       * `ColorComponents` constructor from a "rgba()", "rgb()",
       * or "#RRGGBB" syntax DSL.
       */
      ColorComponents (const String&);

      /**
       * `ColorComponents` constructor for "int" color components
       * with a "float" alpha channel.
       */
      ColorComponents (
        unsigned int red,
        unsigned int green,
        unsigned int blue,
        float alpha = MAX_FLOAT_CHANNEL
      );

      /**
       * `ColorComponents` constructor for "float" color components
       * with a "float" alpha channel.
       */
      ColorComponents (
        float red,
        float green,
        float blue,
        float alpha = MAX_FLOAT_CHANNEL
      );

      /**
       * `ColorComponents` constructor for "float" color components
       * with a "float" alpha channel.
       */
      ColorComponents (
        unsigned int red,
        unsigned int green,
        unsigned int blue,
        unsigned int alpha = MAX_INT_CHANNEL
      );

      /**
       * `ColorComponents` constructor "packed" color components
       * with a pack type.
       */
      ColorComponents (
        uint32_t value,
        const Pack& pack = Pack::RGBA
      );

      /**
       * `ColorComponents` "copy" assignment.
       */
      ColorComponents& operator = (const ColorComponents&);

      /**
       * `ColorComponents` "move" assignment.
       */
      ColorComponents& operator = (ColorComponents&&);

      /**
       * Returns a JSON object of color components.
       */
      JSON::Object json () const;

      /**
       * Returns a string representation of the color components.
       */
      String str (const Pack& pack = Pack::RGBA) const;

      /**
       * Returns a packed representation of the color components.
       */
      uint32_t pack (const Pack& pack = Pack::RGBA) const;
  };

  /**
   * A container for RGBA based color space.
   */
  class Color : public ColorComponents {
    public:
      /**
       * An alias to `ColorComponents::Pack`
       */
      using Pack = ColorComponents::Pack;

      /**
       * An alias for `ColorComponents::MIN_INT_CHANNEL`
       */
      static constexpr unsigned int MIN_INT_CHANNEL = ColorComponents::MIN_INT_CHANNEL;

      /**
       * An alias for `ColorComponents::MIN_INT_CHANNEL`
       */
      static constexpr unsigned int MAX_INT_CHANNEL = ColorComponents::MAX_INT_CHANNEL;

      /**
       * An alias for `ColorComponents::MIN_INT_CHANNEL`
       */
      static constexpr float MIN_FLOAT_CHANNEL = ColorComponents::MIN_FLOAT_CHANNEL;

      /**
       * An alias for `ColorComponents::MIN_INT_CHANNEL`
       */
      static constexpr float MAX_FLOAT_CHANNEL = ColorComponents::MAX_FLOAT_CHANNEL;

      /**
       * Creates a packed rgba value from `red`, `green`, `blue` "int"
       * color components and an `alpha` "float" channel.
       */
      static uint32_t rgba (
        unsigned int red = MIN_INT_CHANNEL,
        unsigned int green = MIN_INT_CHANNEL,
        unsigned int blue = MIN_INT_CHANNEL,
        float alpha = MAX_FLOAT_CHANNEL
      ) {
        return Color(red, green, blue, alpha).pack(Pack::RGBA);
      }

      /**
       * Creates a packed rgba value from `red`, `green`, `blue` "float"
       * color components and an `alpha` "float" channel.
       */
      static uint32_t rgba (
        float red = MIN_FLOAT_CHANNEL,
        float green = MIN_FLOAT_CHANNEL,
        float blue = MIN_FLOAT_CHANNEL,
        float alpha = MAX_FLOAT_CHANNEL
      ) {
        return Color(red, green, blue, alpha).pack(Pack::RGBA);
      }

      /**
       * Creates a packed rgba value from `red`, `green`, `blue` "int"
       * color components and an `alpha` "int" channel.
       */
      static uint32_t rgba (
        unsigned int red = MIN_INT_CHANNEL,
        unsigned int green = MIN_INT_CHANNEL,
        unsigned int blue = MIN_INT_CHANNEL,
        unsigned int alpha = MAX_INT_CHANNEL
      ) {
        return Color(red, green, blue, alpha).pack(Pack::RGBA);
      }

      /**
       * Creates a packed rgb value from `red`, `green`, and `blue` "int"
       * color components.
       */
      static uint32_t rgb (
        unsigned int red = MIN_INT_CHANNEL,
        unsigned int green = MIN_INT_CHANNEL,
        unsigned int blue = MIN_INT_CHANNEL
      ) {
        return Color(red, green, blue).pack(Pack::RGB);
      }

      /**
       * Creates a packed rgb value from `red`, `green`, and `blue` "float"
       * color components.
       */
      static uint32_t rgb (
        float red = MIN_FLOAT_CHANNEL,
        float green = MIN_FLOAT_CHANNEL,
        float blue = MIN_FLOAT_CHANNEL
      ) {
        return Color(red, green, blue).pack(Pack::RGB);
      }

      /**
       * Default `Color` constructor.
       */
      Color () = default;

      /**
       * `Color` constructor from `ColorComponents`
       */
      Color (const ColorComponents&);

      /**
       * `Color` constructor with "int" color components and
       * a "float" alpha channel.
       */
      Color (
        unsigned int red,
        unsigned int green,
        unsigned int blue,
        float alpha = MIN_FLOAT_CHANNEL
      );

      /**
       * `Color` "copy" constructor.
       */
      Color (const Color&);

      /**
       * `Color` "move" constructor.
       */
      Color (Color&&);

      /**
       * `Color` "copy" assignment.
       */
      Color& operator = (const Color&);

      /**
       * `Color` "move" assignment.
       */
      Color& operator = (Color&&);

      /**
       * `Color` equality operator.
       */
      bool operator == (const Color&) const;

      /**
       * `Color` inequality operator.
       */
      bool operator != (const Color&) const;

      /**
       * `Color` greater than inequality operator.
       */
      bool operator > (const Color&) const;

      /**
       * `Color` less than inequality operator.
       */
      bool operator < (const Color&) const;
  };
}
#endif
