#include "../json.hh"

namespace SSC::JSON {
#if SOCKET_RUNTIME_PLATFORM_LINUX
  Object::Object (JSCValue* value) {
    if (value != nullptr && jsc_value_is_object(value) && !jsc_value_is_array(value)) {
      const auto keys = jsc_value_object_enumerate_properties(value);
      if (keys != nullptr) {
        for (int i = 0; keys[i] != nullptr; ++i) {
          const auto key = keys[i];
          const auto property = jsc_value_object_get_property(value, key);
          if (jsc_value_is_string(property)) {
            this->set(key, JSON::String(jsc_value_to_string(property)));
          } else if (jsc_value_is_boolean(property)) {
            this->set(key, JSON::Boolean(jsc_value_to_boolean(property)));
          } else if (jsc_value_is_null(property)) {
            this->set(key, JSON::Null());
          } else if (jsc_value_is_number(property)) {
            this->set(key, JSON::Number(jsc_value_to_double(property)));
          } else if (jsc_value_is_array(property)) {
            this->set(key, JSON::Array(property));
          } else if (jsc_value_is_object(property)) {
            this->set(key, JSON::Object(property));
          }
        }

        g_strfreev(keys);
      }
    }
  }
#endif
  Object::Object (const std::map<SSC::String, int>& entries) {
    for (auto const &tuple : entries) {
      auto key = tuple.first;
      auto value = tuple.second;
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const std::map<SSC::String, bool>& entries) {
    for (auto const &tuple : entries) {
      auto key = tuple.first;
      auto value = tuple.second;
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const std::map<SSC::String, double>& entries) {
    for (auto const &tuple : entries) {
      auto key = tuple.first;
      auto value = tuple.second;
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const std::map<SSC::String, int64_t>& entries) {
    for (auto const &tuple : entries) {
      auto key = tuple.first;
      auto value = tuple.second;
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const Object::Entries& entries) {
    for (const auto& tuple : entries) {
      auto key = tuple.first;
      auto value = tuple.second;
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const Object& object) {
    this->data = object.value();
  }

  Object::Object (const std::map<SSC::String, SSC::String>& map) {
    for (const auto& tuple : map) {
      auto key = tuple.first;
      auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const Error& error) {
    if (error.name.size() > 0) {
      this->set("name", error.name);
    }

    if (error.message.size() > 0) {
      this->set("message", error.message);
    }

    if (error.location.size() > 0) {
      this->set("location", error.location);
    }

    if (error.code != 0) {
      this->set("code", error.code);
    }
  }

  const SSC::String Object::str () const {
    SSC::StringStream stream;
    auto count = this->data.size();
    stream << SSC::String("{");
    for (const auto& tuple : this->data) {
      auto key = replace(tuple.first, "\"","\\\"");
      auto value = tuple.second.str();

      stream << SSC::String("\"");
      stream << key;
      stream << SSC::String("\":");
      stream << value;

      if (--count > 0) {
        stream << SSC::String(",");
      }
    }

    stream << SSC::String("}");
    return stream.str();
  }

  const Object::Entries Object::value () const {
    return this->data;
  }

  const Any& Object::get (const SSC::String& key) const {
    if (this->data.find(key) != this->data.end()) {
      return this->data.at(key);
    }

    return anyNull;
  }

  Any& Object::get (const SSC::String& key) {
    if (this->data.find(key) != this->data.end()) {
      return this->data.at(key);
    }

    return anyNull;
  }

  void Object::set (const SSC::String& key, const Any& value) {
    this->data[key] = value;
  }

  bool Object::has (const SSC::String& key) const {
    return this->contains(key);
  }

  bool Object::contains (const SSC::String& key) const {
    return this->data.contains(key);
  }

  Any Object::operator [] (const SSC::String& key) const {
    if (this->data.find(key) != this->data.end()) {
      return this->data.at(key);
    }

    return nullptr;
  }

  Any& Object::operator [] (const SSC::String& key) {
    return this->data[key];
  }

  ObjectEntries::size_type Object::size () const {
    return this->data.size();
  }

  const Object::const_iterator Object::begin () const noexcept {
    return this->data.begin();
  }

  const Object::const_iterator Object::end () const noexcept {
    return this->data.end();
  }
}
