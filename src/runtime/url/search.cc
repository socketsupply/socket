#include "../url.hh"
#include "../string.hh"

using namespace ssc::runtime::string;

namespace ssc::runtime::url {
  SearchParams::Value::Value (const Value& value)
    : JSON::Raw(std::move(value))
  {
    this->decodeURIComponents = value.decodeURIComponents;
  }

  SearchParams::Value::Value (Value&& value)
    : JSON::Raw(std::move(value))
  {
    this->decodeURIComponents = value.decodeURIComponents;
    value.data = nullptr;
  }

  SearchParams::Value::Value (const String& source)
    : JSON::Raw(source)
  {}

  SearchParams::Value::Value (const JSON::Any& source)
    : JSON::Raw(std::move(source))
  {}

  SearchParams::Value::Value (const std::nullptr_t source)
    : JSON::Raw(nullptr)
  {}

  SearchParams::Value& SearchParams::Value::operator = (const Value& value) {
    this->data = value.data;
    this->decodeURIComponents = value.decodeURIComponents;
    return *this;
  }

  SearchParams::Value& SearchParams::Value::operator = (Value&& value) {
    this->data = std::move(value.data);
    this->decodeURIComponents = value.decodeURIComponents;
    value.data = nullptr;
    return *this;
  }

  SearchParams::Value& SearchParams::Value::operator = (const String& value) {
    this->data = value;
    return *this;
  }

  SearchParams::Value& SearchParams::Value::operator = (const JSON::Any& value) {
    this->data = value;
    return *this;
  }

  SearchParams::Value& SearchParams::Value::operator = (const std::nullptr_t& value) {
    this->data = nullptr;
    return *this;
  }

  const String SearchParams::Value::str () const {
    if (this->data.size() > 0 && this->decodeURIComponents) {
      return decodeURIComponent(this->data);
    }

    return this->data;
  }

  SearchParams::SearchParams (const Options& options)
    : options(options)
  {}

  SearchParams::SearchParams (
    const String& input,
    const Options& options
  ) : options(options)
  {
    this->set(input);
  }

  SearchParams::SearchParams (const SearchParams& input)
    : options(input.options),
      data(input.data)
  {}

  SearchParams::SearchParams (SearchParams&& input)
    : options(std::move(input.options)),
      data(std::move(input.data))
  {
    input.options = Options {};
    input.data = Entries {};
  }

  SearchParams::SearchParams (
    const Map<String, String>& input,
    const Options& options
  ) : options(options)
  {
    for (const auto& entry : input) {
      this->set(entry.first, entry.second);
    }
  }

  SearchParams::SearchParams (
    const JSON::Object& input,
    const Options& options
  ) : options(options)
  {
    for (const auto& entry : input) {
      if (entry.second.isString()) {
        this->set(entry.first, entry.second.as<JSON::String>().data);
      } else {
        this->set(entry.first, entry.second.str());
      }
    }
  }

  SearchParams& SearchParams::operator = (const SearchParams& params) {
    this->data = params.data;
    this->options = params.options;
    return *this;
  }

  SearchParams& SearchParams::operator = (SearchParams&& params) {
    this->data = std::move(params.data);
    this->options = std::move(params.options);
    params.options = Options {};
    params.data = Entries {};
    return *this;
  }

  SearchParams::Value SearchParams::operator [] (const String& key) const {
    return this->get(key);
  }

  SearchParams::Value& SearchParams::operator [] (const String& key) {
    return this->data.at(key);
  }

  SearchParams& SearchParams::set (const String& key, const Value& value) {
    this->data[key] = value;
    return *this;
  }

  SearchParams& SearchParams::set (const String& input) {
    const auto query = input.starts_with("?")
      ? input.substr(1)
      : input;

    for (const auto& entry : split(query, '&')) {
      const auto parts = split(entry, '=');
      if (parts.size() == 2) {
        const auto key = trim(parts[0]);
        const auto value = trim(parts[1]);
        this->set(key, value);
      }
    }
    return *this;
  }

  const SearchParams::Value& SearchParams::at (const String& key) const {
    return this->data.at(key);
  }

  const SearchParams::Value SearchParams::get (const String& key) const {
    return this->data.at(key);
  }

  const SearchParams::Value SearchParams::get (const String& key) {
    return this->data[key];
  }

  bool SearchParams::contains (const String& key) const {
    return this->data.contains(key);
  }

  SearchParams::Entries::size_type SearchParams::size () const {
    return this->data.size();
  }

  const SearchParams::const_iterator SearchParams::begin () const noexcept {
    return this->data.begin();
  }

  const SearchParams::const_iterator SearchParams::end () const noexcept {
    return this->data.end();
  }

  SearchParams::iterator SearchParams::begin () noexcept {
    return this->data.begin();
  }

  SearchParams::iterator SearchParams::end () noexcept {
    return this->data.end();
  }

  const String SearchParams::str () const {
    Vector<String> components;
    for (const auto& entry : *this) {
      const auto parts = Vector<String> {
        entry.first,
        entry.second.str()
      };

      components.push_back(join(parts, "="));
    }

    return join(components, "&");
  }

  const Map<String, String> SearchParams::map () const {
    auto map = Map<String, String>{};
    for (const auto& entry : *this) {
      map[entry.first] = entry.second.str();
    }
    return map;
  }

  const JSON::Object SearchParams::json () const {
    return this->map();
  }
}
