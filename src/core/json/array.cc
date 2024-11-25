#include "../json.hh"

namespace SSC::JSON {
  Array::Array (const Array& array) {
    this->data = array.value();
  }

  Array::Array (const Array::Entries& entries) {
    for (const auto& value : entries) {
      this->data.push_back(value);
    }
  }
#if SOCKET_RUNTIME_PLATFORM_LINUX
  Array::Array (JSCValue* value) {
    if (value != nullptr && jsc_value_is_array(value)) {
      const auto length = jsc_value_object_get_property(value, "length");
      if (jsc_value_is_number(length)) {
        const auto count = jsc_value_to_int32(length);
        for (int i = 0; i < count; ++i) {
          const auto item = jsc_value_object_get_property_at_index(value, i);
          if (jsc_value_is_string(item)) {
            this->push(JSON::String(jsc_value_to_string(item)));
          } else if (jsc_value_is_boolean(item)) {
            this->push(JSON::Boolean(jsc_value_to_boolean(item)));
          } else if (jsc_value_is_null(item)) {
            this->push(JSON::Null());
          } else if (jsc_value_is_number(item)) {
            this->push(JSON::Number(jsc_value_to_double(item)));
          } else if (jsc_value_is_array(item)) {
            this->push(JSON::Array(item));
          } else if (jsc_value_is_object(item)) {
            this->push(JSON::Object(item));
          }
        }
      }
    }
  }
#endif

  const SSC::String Array::str () const {
    SSC::StringStream stream;
    auto count = this->data.size();
    stream << SSC::String("[");

    for (const auto& value : this->data) {
      stream << value.str();

      if (--count > 0) {
        stream << SSC::String(",");
      }
    }

    stream << SSC::String("]");
    return stream.str();
  }

  Array::Entries Array::value () const {
    return this->data;
  }

  bool Array::has (const unsigned int index) const {
    return this->data.size() < index;
  }

  Array::Entries::size_type Array::size () const {
    return this->data.size();
  }

  Any Array::get (const unsigned int index) const {
    if (index < this->data.size()) {
      return this->data.at(index);
    }

    return nullptr;
  }

  void Array::set (const unsigned int index, const Any& value) {
    if (index >= this->data.size()) {
      this->data.resize(index + 1);
    }

    this->data[index] = value;
  }

  void Array::push (Any value) {
    this->set(this->size(), value);
  }

  Any& Array::pop () {
    if (this->size() == 0) {
      return anyNull;
    }

    auto& value = this->data.back();
    this->data.pop_back();
    return value;
  }

  Any Array::operator [] (const unsigned int index) const {
    if (index >= this->data.size()) {
      return nullptr;
    }

    return this->data.at(index);
  }

  Any& Array::operator [] (const unsigned int index) {
    if (index >= this->data.size()) {
      this->data.resize(index + 1);
    }

    return this->data.at(index);
  }

  const Array::const_iterator Array::begin () const noexcept {
    return this->data.begin();
  }

  const Array::const_iterator Array::end () const noexcept {
    return this->data.end();
  }
}
