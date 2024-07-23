#include "../platform/string.hh"
#include "color.hh"

namespace SSC {
  template <typename T>
  static inline T clamp (T v, T x, T y) {
    if (v < x) {
      return x;
    }

    if (v > y) {
      return y;
    }

    return x;
  }

  ColorComponents::ColorComponents (const ColorComponents& components) {
    this->red = components.red;
    this->green = components.green;
    this->blue = components.blue;
    this->alpha = components.alpha;
  }

  ColorComponents::ColorComponents (ColorComponents&& components) {
    this->red = components.red;
    this->green = components.green;
    this->blue = components.blue;
    this->alpha = components.alpha;

    components.red = 0;
    components.green = 0;
    components.blue = 0;
    components.alpha = 0.0f;
  }

  ColorComponents::ColorComponents (const String& string) {
    auto buffer = replace(trim(string), "none", "0");
    Vector<String> values;

    if (buffer.starts_with("rgba(")) {
      buffer = buffer.substr(5);
      if (buffer.ends_with(")")) {
        buffer = trim(buffer.substr(0, buffer.size() - 1));
        const auto components = split(buffer, ',');
        if (components.size() == 4) {
          values.push_back(trim(components[0]));
          values.push_back(trim(components[1]));
          values.push_back(trim(components[2]));
          values.push_back(trim(components[3]));
        }
      }
    } else if (buffer.starts_with("rgb(")) {
      buffer = buffer.substr(4);
      if (buffer.ends_with(")")) {
        buffer = trim(buffer.substr(0, buffer.size() - 1));
        const auto components = split(buffer, ',');
        if (components.size() == 3) {
          values.push_back(trim(components[0]));
          values.push_back(trim(components[1]));
          values.push_back(trim(components[2]));
        }
      }
    } else if (buffer.starts_with("#")) {
      buffer = buffer.substr(1);
      if (buffer.size() == 6) {
        try {
          values.push_back(std::to_string(std::stoul(buffer.substr(0, 2), 0, 16)));
          values.push_back(std::to_string(std::stoul(buffer.substr(2, 2), 0, 16)));
          values.push_back(std::to_string(std::stoul(buffer.substr(4, 2), 0, 16)));
        } catch (...) {}
      }
    }

    if (values.size() == 3 || values.size() == 4) {
      try {
        if (values[0].ends_with("%")) {
          this->red = 255 * (
            std::stoi(values[0].substr(0, values[0].size() - 1)) / 100.0f
          );
        } else {
          this->red = std::stoi(values[0]);
        }

        if (values[1].ends_with("%")) {
          this->green = 255 * (
            std::stoi(values[1].substr(0, values[1].size() - 1)) / 100.0f
          );
        } else {
          this->green = std::stoi(values[1]);
        }

        if (values[2].ends_with("%")) {
          this->blue = 255 * (
            std::stoi(values[2].substr(0, values[2].size() - 1)) / 100.0f
          );
        } else {
          this->blue = std::stoi(values[2]);
        }

        if (values.size() == 4) {
          if (values[3].ends_with("%")) {
            this->alpha = std::stoi(values[2].substr(0, values[2].size() - 1)) / 100.0f;
          } else {
            this->alpha = std::stoi(values[2]);
            if (this->alpha > 1.0f) {
              this->alpha = this->alpha / 255.0f;
            }
          }
        }
      } catch (...) {}
    }
  }

  ColorComponents::ColorComponents (
    unsigned int red,
    unsigned int green,
    unsigned int blue,
    float alpha
  ) : red(red & 0xFF),
      green(green & 0xFF),
      blue(blue & 0xFF),
      alpha(clamp(alpha, MIN_FLOAT_CHANNEL, MAX_FLOAT_CHANNEL))
  {}

  ColorComponents::ColorComponents (
    float red,
    float green,
    float blue,
    float alpha
  ) {
    this->red = static_cast<int>(red * MAX_INT_CHANNEL) & 0xFF;
    this->green = static_cast<int>(green * MAX_INT_CHANNEL) & 0xFF;
    this->blue = static_cast<int>(blue * MAX_INT_CHANNEL) & 0xFF;
    this->alpha = clamp(alpha, MIN_FLOAT_CHANNEL, MAX_FLOAT_CHANNEL);
  }

  ColorComponents::ColorComponents (
    unsigned int red,
    unsigned int green,
    unsigned int blue,
    unsigned int alpha
  ) {
    this->red = red & 0xFF;
    this->green = green & 0xFF;
    this->blue = blue & 0xFF;
    this->alpha = clamp(
      alpha / static_cast<float>(MAX_INT_CHANNEL),
      MIN_FLOAT_CHANNEL,
      MAX_FLOAT_CHANNEL
    );
  }

  ColorComponents::ColorComponents (
    uint32_t value,
    const Pack& pack
  ) {
    switch (pack) {
      case Pack::RGB: {
        this->red = (value >> 24) & 0xFF;
        this->green = (value >> 16) & 0xFF;
        this->blue = (value >> 8) & 0xFF;
        this->alpha = 1.0f;
        break;
      }

      case Pack::RGBA: {
        this->red = (value >> 24) & 0xFF;
        this->green = (value >> 16) & 0xFF;
        this->blue = (value >> 8) & 0xFF;
        this->alpha = (value & 0xFF) / static_cast<float>(MAX_INT_CHANNEL);
        break;
      }

      case Pack::ARGB: {
        this->alpha = ((value >> 24) & 0xFF) / static_cast<float>(MAX_INT_CHANNEL);
        this->red = (value >> 16) & 0xFF;
        this->green = (value >> 8) & 0xFF;
        this->blue = value & 0xFF;
        break;
      }
    }
  }

  ColorComponents& ColorComponents::operator = (const ColorComponents& components) {
    this->red = components.red;
    this->green = components.green;
    this->blue = components.blue;
    this->alpha = components.alpha;
    return *this;
  }

  ColorComponents& ColorComponents::operator = (ColorComponents&& components) {
    this->red = components.red;
    this->green = components.green;
    this->blue = components.blue;
    this->alpha = components.alpha;

    components.red = 0;
    components.green = 0;
    components.blue = 0;
    components.alpha = 0.0f;
    return *this;
  }

  JSON::Object ColorComponents::json () const {
    return JSON::Object::Entries {
      {"red", this->red},
      {"green", this->green},
      {"blue", this->blue},
      {"alpha", this->alpha}
    };
  }

  String ColorComponents::str (const Pack& pack) const {
    String output;

    switch (pack) {
      case Pack::RGB:
        output = "rgb({{red}}, {{green}}, {{blue}})";
        break;
      case Pack::RGBA:
        output = "rgba({{red}}, {{green}}, {{blue}}, {{alpha}})";
        break;
      case Pack::ARGB:
        output = "argb({{alpha}}, {{red}}, {{green}}, {{blue}})";
        break;
    }

    return tmpl(output, Map {
      {"red", std::to_string(this->red)},
      {"green", std::to_string(this->green)},
      {"blue", std::to_string(this->blue)},
      {"alpha", std::to_string(this->alpha)},
    });
  }

  uint32_t ColorComponents::pack (const Pack& pack) const {
    switch (pack) {
      case Pack::RGB: {
        return (
          (this->red   & 0xFF) << 24 |
          (this->green & 0xFF) << 16 |
          (this->blue  & 0xFF) << 8  |
          (255) // asume full alpha channel value
        );
      }

      case Pack::RGBA: {
        const auto alpha = static_cast<int>(
          255.0f * clamp(this->alpha, MIN_FLOAT_CHANNEL, MAX_FLOAT_CHANNEL)
        );
        return (
          (this->red   & 0xFF) << 24 |
          (this->green & 0xFF) << 16 |
          (this->blue  & 0xFF) << 8  |
          (alpha       & 0xFF)
        );
      }

      case Pack::ARGB: {
        const auto alpha = static_cast<int>(
          255.0f * clamp(this->alpha, MIN_FLOAT_CHANNEL, MAX_FLOAT_CHANNEL)
        );
        return (
          (alpha       & 0xFF) << 24 |
          (this->red   & 0xFF) << 16 |
          (this->green & 0xFF) << 8 |
          (this->blue  & 0xFF)
        );
      }
    }

    return 0;
  }

  Color::Color (const ColorComponents&) {
  }

  Color::Color (
    unsigned int red,
    unsigned int green,
    unsigned int blue,
    float alpha
  ) : ColorComponents(red, green, blue, alpha)
  {}

  Color::Color (const Color& color)
    : Color(color.red, color.green, color.blue, color.alpha)
  {}

  Color::Color (Color&& color)
    : Color(color.red, color.green, color.blue, color.alpha)
  {
    color.red = 0;
    color.green = 0;
    color.blue = 0;
    color.alpha = 0.0f;
  }

  Color& Color::operator = (const Color& color) {
    this->red = color.red;
    this->green = color.green;
    this->blue = color.blue;
    this->alpha = color.alpha;
    return *this;
  }

  Color& Color::operator = (Color&& color) {
    this->red = color.red;
    this->green = color.green;
    this->blue = color.blue;
    this->alpha = color.alpha;
    color.red = 0;
    color.green = 0;
    color.blue = 0;
    color.alpha = 0.0f;
    return *this;
  }

  bool Color::operator == (const Color& color) const {
    return this->pack() == color.pack();
  }

  bool Color::operator != (const Color& color) const {
    return this->pack() != color.pack();
  }

  bool Color::operator > (const Color& color) const {
    return this->pack() > color.pack();
  }

  bool Color::operator < (const Color& color) const {
    return this->pack() < color.pack();
  }
}
