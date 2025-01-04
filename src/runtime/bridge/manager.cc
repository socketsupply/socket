#include "../runtime.hh"
#include "../bridge.hh"

namespace ssc::runtime::bridge {
  Manager::Manager (context::RuntimeContext& context)
    : context(context)
  {}

  Manager::~Manager () {}

  SharedPointer<Bridge> Manager::get (int index, const BridgeOptions& options) {
    Lock lock(this->mutex);
    if (index >= this->entries.size() || this->entries[index] == nullptr) {
      if (index >= this->entries.size()) {
        this->entries.resize(index + 1);
      }

      this->entries[index] = std::make_shared<Bridge>(Bridge::Options {
        .context = this->context,
        .dispatcher = static_cast<runtime::Runtime&>(this->context).dispatcher,
        .userConfig = options.userConfig
      });
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
