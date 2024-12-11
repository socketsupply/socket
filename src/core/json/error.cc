#include "../json.hh"

namespace SSC::JSON {
  Error::Error ()
    : std::invalid_argument("")
  {}

  Error::Error (const Error& error)
    : std::invalid_argument(error.str())
  {
    this->code = error.code;
    this->name = error.name;
    this->message = error.message;
    this->location = error.location;
  }

  Error::Error (Error* error)
    : std::invalid_argument(error->str())
  {
    this->code = error->code;
    this->name = error->name;
    this->message = error->message;
    this->location = error->location;
  }

  Error::Error (
    const SSC::String& name,
    const SSC::String& message,
    int code
  ) : std::invalid_argument(name + ": " + message)
  {
    this->name = name;
    this->code = code;
    this->message = message;
  }

  Error::Error (const SSC::String& message)
    : std::invalid_argument(message)
  {
    this->message = message;
  }

  Error::Error (
    const SSC::String& name,
    const SSC::String& message,
    const SSC::String& location
    ) : std::invalid_argument(
      name + ": " + message + " (from " + location + ")"
    )
  {
    this->name = name;
    this->message = message;
    this->location = location;
  }

  const SSC::String Error::value () const {
    return this->str();
  }

  const char* Error::what () const noexcept {
    return this->message.c_str();
  }

  const SSC::String Error::str () const {
    if (this->name.size() > 0 && this->message.size() > 0 && this->location.size() > 0) {
      return this->name + ": " + this->message + " (from " + this->location + ")";
    } else if (this->name.size() > 0 && this->message.size() > 0) {
      return this->name + ": " + this->message;
    } else if (this->name.size() > 0 && this->location.size() > 0) {
      return this->name + " (from " + this->location + ")";
    } else if (this->message.size() > 0 && this->location.size() > 0) {
      return this->message + " (from " + this->location + ")";
    } else if (this->name.size() > 0) {
      return this->name;
    } else if (this->message.size() > 0) {
      return this->message;
    }

    return "";
  }
}