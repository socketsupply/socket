#include "cli.hh"

namespace SSC::CLI {
  static Command::Option NULL_OPTION;

  static inline bool compareOption (
    const Command::Option& option,
    const String& name
  ) {
    if (name.size() == 0) {
      return false;
    }

    return (
      option.name == name ||
      option.alias == name ||
      option.name == concat("-", name) ||
      option.name == concat("--", name) ||
      option.alias == concat("-", name) ||
      option.alias == concat("--", name)
    );
  }

  static inline bool validateOption (
    const Command::Option& spec,
    const String& value
  ) {
    if (spec.isBoolean()) {
      return true;
    }

    if (spec.isSingle()) {
      if (
        spec.expected.string.size() > 0 &&
        spec.expected.string != value
      ) {
        return false;
      }
    }

    if (spec.expected.vector.size() > 0) {
      auto cursor = std::find(
        spec.expected.vector.begin(),
        spec.expected.vector.end(),
        value
      );

      if (cursor == spec.expected.vector.end()) {
        return false;
      }
    }

    return true;
  }

  static inline Command::Option& getOption (
    Command::Options& options,
    const String& name
  ) {
    for (auto& option : options) {
      if (compareOption(option, name)) {
        return option;
      }
    }

    return NULL_OPTION;
  }

  static inline const Command::Option& getOption (
    const Command::Options& options,
    const String& name
  ) {
    for (auto& option : options) {
      if (compareOption(option, name)) {
        return option;
      }
    }

    return NULL_OPTION;
  }

  static inline const String getArgumentName (const String& argument) {
    if (argument.starts_with("--")) {
      const auto start = argument.substr(2);
      const auto parts = split(start, "=");
      return parts[0];
    }

    if (argument.starts_with("-")) {
      return argument.substr(1);
    }

    return "";
  }

  static inline const String getArgumentValue (const String& argument) {
    if (argument.starts_with("--")) {
      const auto start = argument.substr(2);
      const auto parts = split(start, "=");

      if (parts.size() > 1) {
        return parts[1];
      }

      return "";
    }

    if (argument.starts_with("-")) {
      return argument.substr(2);
    }

    return "";
  }

  const Vector<Command::Option> Command::Option::getPositionals (const Options& options) {
    Options positionals;

    for (const auto& option : options) {
      if (option.isPositional()) {
        positionals.push_back(option);
      }
    }

    return positionals;
  }

  Command::Option::Option (
    const String& name,
    const String& alias,
    const Type type,
    const Requirement requirement,
    const Value expected
  ) : name(name),
      alias(alias.size() > 0 ? alias : name),
      type(type),
      requirement(requirement),
      expected(expected)
  {}

  Command::Option::Option (
    const Type type,
    const Requirement requirement,
    const Value expected
  ) : type(type),
      requirement(requirement),
      expected(expected)
  {}

  Command::Option::Option (const Option& option) :
    name(option.name),
    type(option.type),
    alias(option.alias),
    expected(option.expected),
    requirement(option.requirement)
  {
    this->value = option.value;
    this->index = option.index;
    this->isFromRestArguments = option.isFromRestArguments;
  }

  bool Command::Option::isMultiple () const {
    return this->type == Type::Multiple;
  }

  bool Command::Option::isBoolean () const {
    return this->type == Type::Boolean;
  }

  bool Command::Option::isSingle () const {
    return this->type == Type::Single;
  }

  bool Command::Option::isRequired () const {
    return this->requirement == Requirement::Required;
  }

  bool Command::Option::isOptional () const {
    return this->requirement == Requirement::Optional;
  }

  bool Command::Option::isRest () const {
    return this->isFromRestArguments;
  }

  bool Command::Option::isPositional () const {
    return (
      this->value.string.size() > 0 &&
      this->alias.size() == 0 &&
      this->name.size() == 0
    );
  }

  const String Command::Option::str () const {
    return this->value.string;
  }

  const Vector<String> Command::Option::values () const {
    if (this->value.vector.size() == 0) {
      Vector<String> values;
      values.push_back(this->value.string);
      return values;
    }

    return this->value.vector;
  }

  const String Command::Option::dump (bool alias) const {
    const auto& prefix = (
      alias && this->alias.size() > 0
        ? this->alias
        : this->name
    );

    if (prefix.size() > 0) {
      if (this->isBoolean()) {
        return prefix;
      } else if (this->isSingle()) {
        return prefix + " " + this->str();
      } else {
        Vector<String> dumped;

        for (const auto& value : this->values()) {
          dumped.push_back(prefix + " " + value);
        }

        return join(dumped, " ");
      }
    }

    if (this->value.string.size() > 0) {
      return this->value.string;
    }

    if (this->value.vector.size() > 0) {
      return join(this->value.vector, " ");
    }

    return this->value.boolean ? "true" : "false";
  }

  Command::Option::operator bool () const {
    if (this->isBoolean()) {
      return this->value.boolean;
    }

    if (this->isSingle()) {
      return this->value.string.size() > 0;
    }

    if (this->isMultiple()) {
      return this->value.vector.size() > 0;
    }

    return false;
  }

  bool Command::Option::operator == (const Option& right) const {
    if (this == std::addressof(right)) return true;
    if (compareOption(right, this->name)) return true;
    if (compareOption(right, this->alias)) return true;
    return false;
  }

  bool Command::Option::operator != (const Option& right) const {
    return !this->operator==(right);
  }

  bool Command::Option::operator == (const String& right) const {
    if (compareOption(*this, right)) return true;
    return false;
  }

  bool Command::Option::operator != (const String& right) const {
    return !this->operator==(right);
  }

  Command::Command (const String& name, const String& help, const Options& options)
    : name(name),
      help(help),
      options(options)
  {
    this->options.push_back(Option {
      "--prod",
      "",
      Option::Type::Boolean,
      Option::Requirement::Optional
    });

    this->options.push_back(Option {
      "--help",
      "-h",
      Option::Type::Boolean,
      Option::Requirement::Optional
    });

    this->options.push_back(Option {
      "--verbose",
      "-V",
      Option::Type::Boolean,
      Option::Requirement::Optional
    });
  }

  Command& Command::operator = (const Command& other) {
    this->commands = other.commands;
    return *this;
  }

  const Command& Command::command (
    const Command& command
  ) {
    this->commands.insert_or_assign(command.name, command);
    return command;
  }

  const Command& Command::command (const String& name, const String& help) {
    return this->command(Command { name, help, Options {} });
  }

  const Command::Option& Command::option (const Option& option) {
    this->options.push_back(option);
    return this->options.at(this->options.size() - 1);
  }

  const Command::Option& Command::option (
    const String& name,
    const String& alias,
    const Option::Type type,
    const Option::Requirement requirement,
    const Option::Value expected
  ) {
    return this->option(Option { name, alias, type, requirement, expected });
  }

  const Command::Options Command::parseOptions (
    const Command::Arguments& arguments
  ) const {
    auto program = Program::instance();
    auto didSeeFirstPositional = false;
    auto didSeeRestArgumentsSeparator = false;
    Options options;
    Option* lastOption = nullptr;

    for (int i = 0; i < arguments.size(); ++i) {
      const auto& argument = arguments[i];

      if (argument == "--") {
        didSeeRestArgumentsSeparator = true;
      } else if (argument.starts_with("-")) {
        const auto name = getArgumentName(argument);
        const auto& spec = getOption(this->options, name);

        if (spec == NULL_OPTION) {
          // continue without error as positional may indicate
          // a subcommand or command argument
          if (didSeeFirstPositional) {
            continue;
          }

          if (program != nullptr) {
            program->logger.error("Unknown option: " + argument);
            program->exit(1);
          }
          break;
        }

        auto& existing = getOption(options, name);
        const auto value = getArgumentValue(argument);
        const auto& nextArgument = (
          !spec.isBoolean() && i + 1 < arguments.size() && !arguments[i + 1].starts_with("-")
            ? arguments[i + 1]
            : ""
        );

        Option option = Option { spec };
        option.index = i;
        option.isFromRestArguments = didSeeRestArgumentsSeparator;

        if (value.size() == 0 && nextArgument.size() == 0) {
          if (spec.isSingle() || spec.isMultiple()) {
            if (program != nullptr) {
              program->logger.error("No value given for '" + argument + "' option");
              program->exit(1);
            }
            break;
          }

          option.value.boolean = true;
        } else if (spec.isBoolean()) {
          if (value.size() > 0) {
            if (program != nullptr) {
              program->logger.error("Unexpected value given for '" + argument + "' option");
              program->exit(1);
            }
            break;
          }

          option.value.boolean = true;
        } else if (spec.isSingle()) {
          if (value.size() > 0) {
            if (spec.expected.string.size() > 0 && !validateOption(spec, value)) {
              program->logger.error(
                "Unexpected value given for '" + argument + "' option. "
                "Expected '" + spec.expected.string + "'"
              );
              program->exit(1);
            }

            if (spec.expected.vector.size() > 0 && !validateOption(spec, value)) {
              program->logger.error(
                "Unexpected value given for '" + argument + "' option. "
                "Expected one of '" + join(spec.expected.vector, ", ") + "'"
              );
              program->exit(1);
            }

            if (!didSeeRestArgumentsSeparator && existing != NULL_OPTION) {
              existing.value.string = value;
            } else {
              option.value.string = value;
            }
          }
        } else if (spec.isMultiple()) {
          if (value.size() > 0) {
            if (spec.expected.vector.size() > 0 && !validateOption(spec, value)) {
              program->logger.error(
                "Unexpected value given for '" + argument + "' option. "
                "Expected one of '" + join(spec.expected.vector, ", ") + "'"
              );
              program->exit(1);
            }

            if (!didSeeRestArgumentsSeparator && existing != NULL_OPTION) {
              existing.value.vector.push_back(value);
            } else {
              option.value.vector.push_back(value);
            }
          }
        }

        if (didSeeRestArgumentsSeparator || existing == NULL_OPTION) {
          options.push_back(option);
          lastOption = &options.back();
        } else {
          lastOption = &existing;
        }

        if (value.size() > 0) {
          lastOption = nullptr;
        }
      } else if (argument.size() > 0) { // positional or value
        if (lastOption != nullptr) {
          const auto isBoolean = lastOption->isBoolean();
          const auto& spec = getOption(this->options, lastOption->name);
          const auto value = argument;

          if (lastOption->isSingle()) {
            if (lastOption->value.string.size() == 0) {
              if (spec.expected.string.size() > 0 && !validateOption(spec, value)) {
                program->logger.error(
                  "Unexpected value given for '" + argument + "' option. "
                  "Expected '" + spec.expected.string + "'"
                );
                program->exit(1);
              }

              if (spec.expected.vector.size() > 0 && !validateOption(spec, value)) {
                program->logger.error(
                  "Unexpected value given for '" + argument + "' option. "
                  "Expected one of '" + join(spec.expected.vector, ", ") + "'"
                );
                program->exit(1);
              }

              lastOption->value.string = value;
            }
          } else if (lastOption->isMultiple()) {
            if (spec.expected.vector.size() > 0 && !validateOption(spec, value)) {
              program->logger.error(
                "Unexpected value given for '" + argument + "' option. "
                "Expected one of '" + join(spec.expected.vector, ", ") + "'"
              );
              program->exit(1);
            }

            lastOption->value.vector.push_back(value);
          }

          lastOption = nullptr;
          if (!isBoolean) {
            continue;
          }
        }

        auto option = Option { };
        option.isFromRestArguments = didSeeRestArgumentsSeparator;
        option.value.string = argument;
        option.index = i;
        didSeeFirstPositional = true;
        options.push_back(option);
      }
    }

    for (const auto& spec : this->options) {
      if (spec.isRequired()) {
        const auto& option = getOption(options, spec.name);
        if (option == NULL_OPTION) {
          if (program != nullptr) {
            program->logger.error("'" + spec.name + "' requires a value");
            program->exit(1);
          }

          break;
        }
      }
    }

    return options;
  }

  bool Command::invoke (
    const Arguments& arguments,
    const Options& options
  ) {
    for (const auto& option : options) {
      if (option.name == "--help") {
        this->printHelp(std::cout);
        return true;
      }
    }

    if (this->callback != nullptr) {
      return this->callback(*this, options);
    }

    return false;
  }

  void Command::exit (int status) const {
    auto program = Program::instance();
    if (program != nullptr) {
      program->exit(status);
    }
  }

  void Command::printHelp (std::ostream& stream) const {
    const auto program = Program::instance();
    if (program != nullptr) {
      if (this->help.size() > 0) {
        stream << trim(tmpl(this->help, program->config.data())) << std::endl;
      } else {
        stream << tmpl("ssc v{{ssc.version}}", program->config.data()) << std::endl;
        stream << "usage:" << std::endl;
        stream << "  ssc " << this->name << std::endl;
      }
    }
  }
}
