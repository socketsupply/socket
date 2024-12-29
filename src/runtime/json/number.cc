#include "../json.hh"
#include "../debug.hh"

namespace ssc::runtime::JSON {
  Type Number::valueType = Type::Number;

  Number::Number (const String& string) {
    this->data = std::stod(string.data);
  }

  Number::Number (const Number& number) {
    this->data = number.value();
  }

  Number::Number (double number) {
    this->data = number;
  }

  Number::Number (char number) {
    this->data = (double) number;
  }

  Number::Number (int number) {
    this->data = (double) number;
  }

  Number::Number (int64_t number) {
    this->data = (double) number;
  }

  Number::Number (bool number) {
    this->data = (double) number;
  }

  const double Number::value () const {
    return this->data;
  }

  const runtime::String Number::str () const {
    if (this->data == 0) {
      return "0";
    }

    auto value = this->data;
    auto output = std::to_string(value);
    auto decimal = output.find(".");

    // trim trailing zeros
    if (decimal >= 0) {
      auto i = output.size() - 1;
      while (output[i] == '0' && i >= decimal) {
        i--;
      }

      return output.substr(0, i);
    }

    return output;
  }
}
