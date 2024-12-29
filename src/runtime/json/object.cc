#include "../string.hh"

#include "../json.hh"

using ssc::runtime::string::replace;

namespace ssc::runtime::JSON {
  Type Object::valueType = Type::Object;

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

  Object::Object (const Object::Entries& entries) {
    for (const auto& tuple : entries) {
      const auto& key = tuple.first;
      const auto& value = tuple.second;
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const runtime::Map<runtime::String, std::nullptr_t>& map) {
    for (const auto& tuple : map) {
      const auto& key = tuple.first;
      const auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const runtime::Map<runtime::String, const Null>& map) {
    for (const auto& tuple : map) {
      const auto& key = tuple.first;
      const auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const runtime::Map<runtime::String, bool>& map) {
    for (const auto& tuple : map) {
      const auto& key = tuple.first;
      const auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const runtime::Map<runtime::String, int64_t>& map) {
    for (const auto& tuple : map) {
      const auto& key = tuple.first;
      const auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const runtime::Map<runtime::String, uint64_t>& map) {
    for (const auto& tuple : map) {
      const auto& key = tuple.first;
      const auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const runtime::Map<runtime::String, uint32_t>& map) {
    for (const auto& tuple : map) {
      const auto& key = tuple.first;
      const auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const runtime::Map<runtime::String, int32_t>& map) {
    for (const auto& tuple : map) {
      const auto& key = tuple.first;
      const auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const runtime::Map<runtime::String, double>& map) {
    for (const auto& tuple : map) {
      const auto& key = tuple.first;
      const auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }

#if SOCKET_RUNTIME_PLATFORM_APPLE
  Object::Object (const runtime::Map<runtime::String, size_t>& map) {
    for (const auto& tuple : map) {
      const auto& key = tuple.first;
      const auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const runtime::Map<runtime::String, ssize_t>& map) {
    for (const auto& tuple : map) {
      const auto& key = tuple.first;
      const auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }

#elif !SOCKET_RUNTIME_PLATFORM_WINDOWS
  Object::Object (const runtime::Map<runtime::String, long long>& map) {
    for (const auto& tuple : map) {
      const auto& key = tuple.first;
      const auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }
#endif

  Object::Object (const Object& object) {
    this->data = object.value();
  }

  Object::Object (const Map<runtime::String, runtime::String>& map) {
    for (const auto& tuple : map) {
      const auto& key = tuple.first;
      const auto value = Any(tuple.second);
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

  const runtime::String Object::str () const {
    runtime::StringStream stream;
    auto count = this->data.size();
    stream << runtime::String("{");
    for (const auto& tuple : this->data) {
      const auto key = replace(tuple.first, "\"","\\\"");
      const auto value = tuple.second.str();

      stream << runtime::String("\"");
      stream << key;
      stream << runtime::String("\":");
      stream << value;

      if (--count > 0) {
        stream << runtime::String(",");
      }
    }

    stream << runtime::String("}");
    return stream.str();
  }

  const Object::Entries Object::value () const {
    return this->data;
  }

  const Any& Object::get (const runtime::String& key) const {
    if (this->data.find(key) != this->data.end()) {
      return this->data.at(key);
    }

    return nullAny;
  }

  const Any& Object::get (const runtime::String& key) {
    if (this->data.find(key) != this->data.end()) {
      return this->data.at(key);
    }

    return nullAny;
  }

  Any& Object::at (const runtime::String& key) {
    return this->data.at(key);
  }

  void Object::set (const runtime::String& key, const Any& value) {
    this->data[key] = value;
  }

  bool Object::has (const runtime::String& key) const {
    return this->contains(key);
  }

  bool Object::contains (const runtime::String& key) const {
    return this->data.contains(key);
  }

  Any& Object::operator [] (const char* key) {
    return this->operator[](runtime::String(key));
  }

  Any& Object::operator [] (const runtime::String& key) {
    if (!this->contains(key)) {
      this->data[key] = Any {};
    }
    return this->data.at(key);
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

  Object::iterator Object::begin () noexcept {
    return this->data.begin();
  }

  Object::iterator Object::end () noexcept {
    return this->data.end();
  }
}
