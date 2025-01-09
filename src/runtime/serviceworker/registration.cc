#include "../url.hh"

#include "../serviceworker.hh"

using ssc::runtime::url::encodeURIComponent;

namespace ssc::runtime::serviceworker {
  static String getPriorityString (const Registration::Priority& priority) {
    if (priority == Registration::Priority::High) {
      return "high";
    } else if (priority == Registration::Priority::Low) {
      return "low";
    }

    return "default";
  }

  String Registration::key (
    const String& scope,
    const URL& origin,
    const String& scheme
  ) {
    auto url = URL(scope, origin);
    url.scheme = scheme;
    return url.str();
  }

  Registration::Registration (
    const ID id,
    const State state,
    const Origin origin,
    const Options& options
  ) : options(options),
      origin(origin),
      state(state),
      id(id)
  {}

  Registration::Registration (const Registration& registration) {
    this->id = registration.id;
    this->state = registration.state.load();
    this->origin = registration.origin;
    this->options = registration.options;
  }

  Registration::Registration (Registration&& registration) {
    this->id = registration.id;
    this->state = registration.state.load();
    this->origin = registration.origin;
    this->options = registration.options;

    registration.id = 0;
    registration.state = State::None;
    registration.origin = Origin {};
    registration.options = Options {};
  }

  Registration& Registration::operator= (const Registration& registration) {
    this->id = registration.id;
    this->state = registration.state.load();
    this->origin = registration.origin;
    this->options = registration.options;
    return *this;
  }

  Registration& Registration::operator= (Registration&& registration) {
    this->id = registration.id;
    this->state = registration.state.load();
    this->origin = registration.origin;
    this->options = registration.options;

    registration.id = 0;
    registration.state = State::None;
    registration.origin = Origin {};
    registration.options = Options {};
    return *this;
  }

  const JSON::Object Registration::json (
    bool includeSerializedWorkerArgs
  ) const {
    return JSON::Object::Entries {
      {"id", std::to_string(this->id)},
      {"scriptURL", this->options.scriptURL},
      {"scope", this->options.scope},
      {"state", this->getStateString()},
      {"scheme", this->options.scheme},
      {"origin", this->origin.name()},
      {"serializedWorkerArgs", includeSerializedWorkerArgs ? encodeURIComponent(this->options.serializedWorkerArgs) : ""},
      {"priority", getPriorityString(this->options.priority)}
    };
  }

  bool Registration::isActive () const {
    return (
      this->state == Registration::State::Activating ||
      this->state == Registration::State::Activated
    );
  }

  bool Registration::isWaiting () const {
    return this->state == Registration::State::Installed;
  }

  bool Registration::isInstalling () const {
    return this->state == Registration::State::Installing;
  }

  const String Registration::getStateString () const {
    String stateString = "none";

    if (this->state == Registration::State::Error) {
      stateString = "error";
    } else if (this->state == Registration::State::Registering) {
      stateString = "registering";
    } else if (this->state == Registration::State::Registered) {
      stateString = "registered";
    } else if (this->state == Registration::State::Installing) {
      stateString = "installing";
    } else if (this->state == Registration::State::Installed) {
      stateString = "installed";
    } else if (this->state == Registration::State::Activating) {
      stateString = "activating";
    } else if (this->state == Registration::State::Activated) {
      stateString = "activated";
    }

    return stateString;
  };

  const JSON::Object Registration::Storage::json () const {
    return JSON::Object { this->data };
  }

  void Registration::Storage::set (const String& key, const String& value) {
    this->data.insert_or_assign(key, value);
  }

  const String Registration::Storage::get (const String& key) const {
    if (this->data.contains(key)) {
      return this->data.at(key);
    }

    return key;
  }

  void Registration::Storage::remove (const String& key) {
    if (this->data.contains(key)) {
      this->data.erase(key);
    }
  }

  void Registration::Storage::clear () {
    this->data.clear();
  }
}
