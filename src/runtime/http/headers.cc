#include "../http.hh"
#include "../string.hh"

using namespace ssc::runtime::string;

namespace ssc::runtime::http {
  Headers::Header::Header (const Header& header) {
    this->name = toLowerCase(header.name);
    this->value = header.value;
  }

  Headers::Header::Header (const String& name, const Value& value) {
    this->name = toLowerCase(trim(name));
    this->value = trim(value.str());
  }

  bool Headers::Header::operator == (const Header& header) const {
    return this->value == header.value;
  }

  bool Headers::Header::operator != (const Header& header) const {
    return this->value != header.value;
  }

  bool Headers::Header::operator == (const String& string) const {
    return this->value.string == string;
  }

  bool Headers::Header::operator != (const String& string) const {
    return this->value.string != string;
  }

  const String Headers::Header::operator + (const String& string) const {
    return this->value + string;
  }

  bool Headers::Header::empty () const {
    return this->value.empty();
  }

  size_t Headers::Header::size() const {
    return this->value.size();
  }

  Headers::Headers (const String& source) {
    for (const auto& entry : split(source, '\n')) {
      const auto tuple = split(entry, ':');
      if (tuple.size() == 2) {
        this->set(trim(tuple.front()), trim(tuple.back()));
      }
    }
  }

  Headers::Headers (const Headers& headers) {
    this->entries = headers.entries;
  }

  Headers::Headers (const Vector<Map<String, Value>>& entries) {
    for (const auto& entry : entries) {
      for (const auto& pair : entry) {
        this->entries.push_back(Header { pair.first, pair.second });
      }
    }
  }

  Headers::Headers (const Entries& entries) {
    for (const auto& entry : entries) {
      this->entries.push_back(entry);
    }
  }

  Headers& Headers::set (const String& name, const Value& value) noexcept {
    this->set(Header { name, value });
    return *this;
  }

  Headers& Headers::set (const Header& header) noexcept {
    for (auto& entry : this->entries) {
      if (header.name == entry.name) {
        entry.value = header.value;
        return *this;
      }
    }

    this->entries.push_back(header);
    return *this;
  }

  bool Headers::has (const String& name) const noexcept {
    const auto normalizedName = toLowerCase(name);
    for (const auto& header : this->entries) {
      if (header.name == normalizedName) {
        return true;
      }
    }

    return false;
  }

  const Headers::Header Headers::get (const String& name) const noexcept {
    const auto normalizedName = toLowerCase(name);
    for (const auto& header : this->entries) {
      if (header.name == normalizedName) {
        return header;
      }
    }

    return Header {};
  }

  Headers::Header& Headers::at (const String& name) {
    const auto normalizedName = toLowerCase(name);
    for (auto& header : entries) {
      if (header.name == normalizedName) {
        return header;
      }
    }

    throw std::out_of_range("Header does not exist");
  }

  size_t Headers::size () const {
    return this->entries.size();
  }

  bool Headers::empty () const {
    return this->entries.empty();
  }

  String Headers::str () const {
    StringStream headers;
    auto remaining = this->size();
    for (const auto& entry : this->entries) {
      auto parts = split(entry.name, '-');

      std::transform(
        parts.begin(),
        parts.end(),
        parts.begin(),
        toProperCase
      );

      const auto name = join(parts, '-');

      headers << name << ": " << entry.value.str();
      if (--remaining > 0) {
        headers << "\r\n";
      }
    }

    return headers.str() + "\r\n\r\n";
  }

  const Headers::Iterator Headers::begin () const noexcept {
    return this->entries.begin();
  }

  const Headers::Iterator Headers::end () const noexcept {
    return this->entries.end();
  }

  bool Headers::erase (const String& name) noexcept {
    for (int i = 0; i < this->entries.size(); ++i) {
      const auto& entry = this->entries[i];
      if (entry.name == name) {
        this->entries.erase(this->entries.begin() + i);
        return true;
      }
    }
    return false;
  }

  const bool Headers::clear () noexcept {
    if (this->entries.size() == 0) {
      return false;
    }
    this->entries.clear();
    return true;
  }

  String& Headers::operator [] (const String& name) {
    if (!this->has(name)) {
      this->set(name, "");
    }

    return this->at(name).value.string;
  }

  const String Headers::operator [] (const String& name) const noexcept {
    return this->get(name).value.string;
  }

  JSON::Object Headers::json () const noexcept {
    JSON::Object::Entries entries;
    for (const auto& entry : this->entries) {
      entries[entry.name] = entry.value.string;
    }
    return entries;
  }

  Headers::Value::Value (const String& value) {
    this->string = trim(value);
  }

  Headers::Value::Value (const char* value) {
    this->string = value;
  }

  Headers::Value::Value (const Value& value) {
    this->string = value.string;
  }

  Headers::Value::Value (bool value) {
    this->string = value ? "true" : "false";
  }

  Headers::Value::Value (int value) {
    this->string = std::to_string(value);
  }

  Headers::Value::Value (float value) {
    this->string = std::to_string(value);
  }

  Headers::Value::Value (int64_t value) {
    this->string = std::to_string(value);
  }

  Headers::Value::Value (uint64_t value) {
    this->string = std::to_string(value);
  }

  Headers::Value::Value (double_t value) {
    this->string = std::to_string(value);
  }

#if SOCKET_RUNTIME_PLATFORM_APPLE
  Headers::Value::Value (ssize_t value) {
    this->string = std::to_string(value);
  }
#endif

  bool Headers::Value::operator == (const Value& value) const {
    return this->string == value.string;
  }

  bool Headers::Value::operator != (const Value& value) const {
    return this->string != value.string;
  }

  bool Headers::Value::operator == (const String& string) const {
    return this->string == string;
  }

  bool Headers::Value::operator != (const String& string) const {
    return this->string != string;
  }

  const String Headers::Value::operator + (const String& string) const {
    return this->string + string;
  }

  const String& Headers::Value::str () const {
    return this->string;
  }

  const char * Headers::Value::c_str() const {
    return this->str().c_str();
  }

  size_t Headers::Value::size() const {
    return this->string.size();
  }

  bool Headers::Value::empty () const {
    return this->string.empty();
  }

  const String toHeaderCase (const String& source) {
    Vector<String> parts;
    for (const auto& entry : split(trim(source), '-')) {
      parts.push_back(toProperCase(entry));
    }
    return join(parts, '-');
  }
}
