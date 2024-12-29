#include "../string.hh"
#include "../color.hh"

using namespace ssc::runtime::string;

namespace ssc::runtime::color {
  Color::Color (const ColorComponents&) {}

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
