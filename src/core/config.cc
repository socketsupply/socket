#include "config.hh"
#include "ini.hh"
#include "string.hh"

namespace SSC {
  static const String NAMESPACE_SEPARATOR = "_";

  Config::Config (const String& source) {
    this->map = INI::parse(source);
  }

  Config::Config (const Config& source) {
    this->map = source.data();
  }

  Config::Config (const Map& source) {
    this->map = source;
  }

  const String Config::get (const String& key) const {
    if (this->contains(key)) {
      return this->at(key);
    }

    return "";
  }

  const String& Config::at (const String& key) const {
    return this->map.at(key);
  }

  void Config::set (const String& key, const String& value) {
    this->map.insert_or_assign(key, value);
  }

  bool Config::contains (const String& key) const {
    if (this->map.contains(key) && this->map.at(key).size() > 0) {
      return true;
    }

    for (const auto& tuple : this->map) {
      if (
        tuple.first.starts_with(key + NAMESPACE_SEPARATOR) &&
        tuple.second.size() > 0
      ) {
        return true;
      }
    }

    return false;
  }

  bool Config::erase (const String& key) {
    if (this->map.contains(key)) {
      this->map.erase(key);
      return true;
    }

    return false;
  }

  const Map& Config::data () const {
    return this->map;
  }

  const Config Config::slice (const String& key) const {
    Map slice;

    for (const auto& tuple : this->map) {
      if (
        tuple.first.starts_with(key + NAMESPACE_SEPARATOR) &&
        tuple.second.size() > 0
      ) {
        const auto k = tuple.first.substr(key.size() + 1, tuple.first.size());
        const auto v = tuple.second;
        slice.insert_or_assign(k, v);
      }
    }

    return Config { slice };
  }

  const String Config::operator [] (const String& key) const {
    return this->map.at(key);
  }

  const String& Config::operator [] (const String& key) {
    return this->map[key];
  }
}
