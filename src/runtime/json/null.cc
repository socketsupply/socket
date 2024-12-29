#include "../json.hh"

namespace ssc::runtime::JSON {
  const Null null = nullptr;

  Type Null::valueType = Type::Null;

  Null::Null (std::nullptr_t)
    : Null()
  {}

  const std::nullptr_t Null::value () const {
    return nullptr;
  }

  const runtime::String Null::str () const {
    return "null";
  }
}
