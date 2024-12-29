#include "../runtime.hh"
#include "../bridge.hh"

namespace ssc::runtime::bridge {
  Manager::Manager (context::RuntimeContext& context)
    : context(context)
  {}

  SharedPointer<Bridge> Manager::get (int index) {
    if (!this->has(index)) {
      this->entries[index].reset(new Bridge({
        this->context,
        static_cast<runtime::Runtime&>(this->context).dispatcher
      }));
    }

    return this->entries[index];
  }

  bool Manager::has (int index) const {
    return index < this->entries.size() && this->entries.at(index) != nullptr;
  }

  bool Manager::remove (int index) {
    if (this->has(index)) {
      this->entries.erase(this->entries.begin() + index);
      return true;
    }

    return false;
  }
}
