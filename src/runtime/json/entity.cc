#include "../debug.hh"
#include "../json.hh"

namespace ssc::runtime::JSON {
  Entity::~Entity () {}

  bool Entity::getEntityBooleanValue () const {
    if (this->isBoolean()) {
      return dynamic_cast<const Boolean*>(this)->value();
    } else if (this->isNull() || this->isEmpty()) {
      return false;
    } else if (this->isNumber()) {
      return dynamic_cast<const Number*>(this)->value() != 0;
    } else if (this->isString()) {
      return dynamic_cast<const String*>(this)->size() > 0;
    } else if (this->isObject()) {
      return dynamic_cast<const Object*>(this)->size() > 0;
    } else if (this->isArray()) {
      return dynamic_cast<const Array*>(this)->size() > 0;
    } else if (this->isRaw()) {
      return dynamic_cast<const Raw*>(this)->data.size() > 0;
    } else if (this->getEntityType() == Type::Any) {
      return dynamic_cast<const Any*>(this)
        ->value()
        ->getEntityBooleanValue();
    } else if (this->isError()) {
      return true;
    }

    return false;
  }

  const runtime::String Entity::typeof () const {
    static const char* table[] = {
      "empty",
      "raw",
      "any",
      "array",
      "boolean",
      "number",
      "null",
      "object",
      "string",
      "error"
    };
    const auto type = this->getEntityType();
    return table[static_cast<size_t>(type)];
  }

  bool Entity::isError () const {
    return this->getEntityType() == Type::Error;
  }

  bool Entity::isRaw () const {
    return this->getEntityType() == Type::Raw;
  }

  bool Entity::isArray () const {
    return this->getEntityType() == Type::Array;
  }

  bool Entity::isBoolean () const {
    return this->getEntityType() == Type::Boolean;
  }

  bool Entity::isNumber () const {
    return this->getEntityType() == Type::Number;
  }

  bool Entity::isNull () const {
    return this->getEntityType() == Type::Null;
  }

  bool Entity::isObject () const {
    return this->getEntityType() == Type::Object;
  }

  bool Entity::isString () const {
    return this->getEntityType() == Type::String;
  }

  bool Entity::isEmpty () const {
    return this->getEntityType() == Type::Empty;
  }

  const runtime::String Entity::str () const {
    return "";
  }

  const Entity::ID Entity::getEntityID () {
    return this->id;
  }

  Entity::operator bool () const {
    return this->getEntityBooleanValue();
  }

  SharedEntityPointer::ControlBlock::ControlBlock (
    Entity* entity
  ) : entity(entity),
      size(1)
  {}

  SharedEntityPointer::SharedEntityPointer (Entity* entity) {
    this->reset(entity);
  }

  SharedEntityPointer::SharedEntityPointer (const SharedEntityPointer& other)
    : control(other.control)
  {
    if (this->control) {
      this->control->size.fetch_add(1, std::memory_order_relaxed);
    }
  }

  SharedEntityPointer::SharedEntityPointer (SharedEntityPointer&& other)
    : control(std::move(other.control))
  {
    other.control = nullptr;
  }

  SharedEntityPointer::~SharedEntityPointer () {
    this->reset();
  }

  SharedEntityPointer& SharedEntityPointer::operator = (const SharedEntityPointer& other) {
    this->control = other.control;
    if (this->control) {
      this->control->size.fetch_add(1, std::memory_order_relaxed);
    }
    return *this;
  }

  SharedEntityPointer& SharedEntityPointer::operator = (SharedEntityPointer&& other) {
    this->control = std::move(other.control);
    other.control = nullptr;
    return *this;
  }

  SharedEntityPointer::operator bool () const {
    return this->get() != nullptr;
  }

  Entity* SharedEntityPointer::operator -> () const {
    return this->get();
  }

  void SharedEntityPointer::reset (Entity* entity) {
    if (this->control && this->control->size.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      if (entity == this->control->entity) {
        return;
      }

      delete this->control->entity;
    }

    if (entity) {
      this->control = std::make_shared<ControlBlock>(entity);
    } else {
      this->control = nullptr;
    }
  }

  size_t SharedEntityPointer::use_count () const {
    if (this->control) {
      return this->control->size.load();
    }
    return 0;
  }

  void SharedEntityPointer::swap (SharedEntityPointer& other) {
    std::swap(this->control, other.control);
  }

  Entity* SharedEntityPointer::get () const {
    return this->control ? this->control->entity : nullptr;
  }

  template <typename T> T* SharedEntityPointer::as () const {
    return dynamic_cast<T*>(this->get());
  }
}
