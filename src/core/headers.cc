#include "core.hh"

namespace SSC {
  Headers::Header::Header (const Header& header) {
    this->key = header.key;
    this->value = header.value;
  }

  Headers::Header::Header (const String& key, const Value& value) {
    this->key = trim(key);
    this->value = trim(value.str());
  }

  Headers::Headers (const String& source) {
    for (const auto& entry : split(source, '\n')) {
      const auto tuple = split(entry, ':');
      if (tuple.size() == 2) {
        set(trim(tuple.front()), trim(tuple.back()));
      }
    }
  }

  Headers::Headers (const Headers& headers) {
    this->entries = headers.entries;
  }

  Headers::Headers (const Vector<std::map<String, Value>>& entries) {
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

  void Headers::set (const String& key, const String& value) {
    set(Header{ key, value });
  }

  void Headers::set (const Header& header) {
    for (auto& entry : entries) {
      if (header.key == entry.key) {
        entry.value = header.value;
        return;
      }
    }

    entries.push_back(header);
  }

  bool Headers::has (const String& name) const {
    for (const auto& header : entries) {
      if (header.key == name) {
        return true;
      }
    }

    return false;
  }

  const Headers::Header& Headers::get (const String& name) const {
    static const auto empty = Header();

    for (const auto& header : entries) {
      if (header.key == name) {
        return header;
      }
    }

    return empty;
  }

  size_t Headers::size () const {
    return this->entries.size();
  }

  String Headers::str () const {
    StringStream headers;
    auto count = this->size();
    for (const auto& entry : this->entries) {
      headers << entry.key << ": " << entry.value.str();;
      if (--count > 0) {
        headers << "\n";
      }
    }
    return headers.str();
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

#if defined(__APPLE__)
  Headers::Value::Value (ssize_t value) {
    this->string = std::to_string(value);
  }
#endif

  const String& Headers::Value::str () const {
    return this->string;
  }

  const char * Headers::Value::c_str() const {
    return this->str().c_str();
  }
}
