#include <type_traits>

#include "../url.hh"
#include "../string.hh"

using namespace ssc::runtime::string;

namespace ssc::runtime::url {
  PathComponents::PathComponents (const String& pathname) {
    this->set(pathname);
  }

  void PathComponents::set (const String& pathname) {
    const auto parts = split(pathname, "/");
    for (const auto& part : parts) {
      const auto value = trim(part);
      if (value.size() > 0) {
        this->parts.push_back(value);
      }
    }
  }

  const String PathComponents::operator[] (const size_t index) const {
    return this->parts[index];
  }

  const String& PathComponents::operator[] (const size_t index) {
    return this->parts[index];
  }

  const String& PathComponents:: at (const size_t index) const {
    return this->parts.at(index);
  }

  const String PathComponents::str () const noexcept {
    return "/" + join(this->parts, "/");
  }

  const PathComponents::Iterator PathComponents::begin () const noexcept {
    return this->parts.begin();
  }

  const PathComponents::Iterator PathComponents::end () const noexcept {
    return this->parts.end();
  }

  const size_t PathComponents::size () const noexcept {
    return this->parts.size();
  }

  const bool PathComponents::empty () const noexcept {
    return this->parts.empty();
  }

  template <typename T>
  const T PathComponents::get (const size_t index) const {
    if (std::is_same<T, uint64_t>::value) {
      return std::stoull(this->at(index));
    }

    if (std::is_same<T, int64_t>::value) {
      return std::stoll(this->at(index));
    }

    if (std::is_same<T, uint32_t>::value) {
      return std::stoul(this->at(index));
    }

    if (std::is_same<T, int32_t>::value) {
      return std::stol(this->at(index));
    }

    if (std::is_same<T, uint16_t>::value) {
      return std::stoul(this->at(index));
    }

    if (std::is_same<T, int16_t>::value) {
      return std::stol(this->at(index));
    }

    if (std::is_same<T, uint8_t>::value) {
      return std::stoul(this->at(index));
    }

    if (std::is_same<T, int8_t>::value) {
      return std::stod(this->at(index));
    }

    if (std::is_same<T, bool>::value) {
      if (std::stod(this->at(index)) == 0) {
        return false;
      }

      return true;
    }

    throw Error("unhandled type in PathComponents::get");
  }

  template const uint64_t PathComponents::get (const size_t index) const;
  template const int64_t PathComponents::get (const size_t index) const;
  template const uint32_t PathComponents::get (const size_t index) const;
  template const int32_t PathComponents::get (const size_t index) const;
  template const uint16_t PathComponents::get (const size_t index) const;
  template const int16_t PathComponents::get (const size_t index) const;
  template const uint8_t PathComponents::get (const size_t index) const;
  template const int8_t PathComponents::get (const size_t index) const;
  template const bool PathComponents::get (const size_t index) const;
}
