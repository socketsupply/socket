/**
 * @typedef {string} GbnfJsonSchemaImmutableType - A type that represents immutable JSON schema types.
 * @typedef {GbnfJsonBasicSchema|GbnfJsonConstSchema|GbnfJsonEnumSchema|GbnfJsonOneOfSchema|GbnfJsonObjectSchema|GbnfJsonArraySchema} GbnfJsonSchema - A type that represents various JSON schema structures.
 * @typedef {Object} GbnfJsonBasicSchema - A JSON schema with basic type information.
 * @property {GbnfJsonSchemaImmutableType|Array<GbnfJsonSchemaImmutableType>} type - The type or an array of types for the schema.
 * @typedef {Object} GbnfJsonConstSchema - A JSON schema with a constant value.
 * @property {string|number|boolean|null} const - The constant value.
 * @typedef {Object} GbnfJsonEnumSchema - A JSON schema with an enumerated set of values.
 * @property {Array<string|number|boolean|null>} enum - An array of allowed values.
 * @typedef {Object} GbnfJsonOneOfSchema - A JSON schema with a set of alternative schemas.
 * @property {Array<GbnfJsonSchema>} oneOf - An array of alternative schemas.
 * @typedef {Object} GbnfJsonObjectSchema - A JSON schema for an object with properties.
 * @property {string} type - The type of the schema, which is "object".
 * @property {Object} properties - An object representing the properties of the schema.
 * @property {Array<string>} [required] - An optional array of required property names.
 * @typedef {Object} GbnfJsonArraySchema - A JSON schema for an array.
 * @property {string} type - The type of the schema, which is "array".
 * @property {GbnfJsonSchema} items - The schema of items in the array.
 */

/**
 * @template T
 * @typedef {T extends GbnfJsonBasicSchema
 *   ? GbnfJsonBasicSchemaToType<T["type"]>
 *   : T extends GbnfJsonConstSchema
 *     ? T["const"]
 *     : T extends GbnfJsonEnumSchema
 *       ? T["enum"][number]
 *       : T extends GbnfJsonOneOfSchema
 *         ? GbnfJsonSchemaToType<T["oneOf"][number]>
 *         : T extends GbnfJsonObjectSchema
 *           ? GbnfJsonObjectSchemaToType<T["properties"]>
 *           : T extends GbnfJsonArraySchema
 *             ? Array<GbnfJsonSchemaToType<T["items"]>>
 *             : never} GbnfJsonSchemaToType - A type that maps JSON schema types to JavaScript types.
 */

/**
 * @template T
 * @typedef {T extends GbnfJsonSchemaImmutableType
 *   ? ImmutableTypeToType<T>
 *   : T extends Array<GbnfJsonSchemaImmutableType>
 *     ? ImmutableTypeToType<T[number]>
 *     : never} GbnfJsonBasicSchemaToType - A type that maps JSON basic schema types to JavaScript types.
 */

/**
 * @template T
 * @typedef {T extends "string"
 *   ? string
 *   : T extends "number"
 *     ? number
 *     : T extends "integer"
 *       ? number
 *       : T extends "boolean"
 *         ? boolean
 *         : T extends "null"
 *           ? null
 *           : never} ImmutableTypeToType - A type that maps immutable JSON schema types to JavaScript types.
 */

/**
 * Checks if a given JSON schema is a GbnfJsonConstSchema.
 * @param {GbnfJsonSchema} schema - The JSON schema to check.
 * @returns {boolean} - True if the schema is a GbnfJsonConstSchema, otherwise false.
 */
function isGbnfJsonConstSchema (schema) {
  return schema.const !== undefined
}

/**
 * Checks if a given JSON schema is a GbnfJsonEnumSchema.
 * @param {GbnfJsonSchema} schema - The JSON schema to check.
 * @returns {boolean} - True if the schema is a GbnfJsonEnumSchema, otherwise false.
 */
function isGbnfJsonEnumSchema (schema) {
  return schema.enum != null
}

/**
 * Checks if a given JSON schema is a GbnfJsonOneOfSchema.
 * @param {GbnfJsonSchema} schema - The JSON schema to check.
 * @returns {boolean} - True if the schema is a GbnfJsonOneOfSchema, otherwise false.
 */
function isGbnfJsonOneOfSchema (schema) {
  return schema.oneOf != null
}

/**
 * Checks if a given JSON schema is a GbnfJsonObjectSchema.
 * @param {GbnfJsonSchema} schema - The JSON schema to check.
 * @returns {boolean} - True if the schema is a GbnfJsonObjectSchema, otherwise false.
 */
function isGbnfJsonObjectSchema (schema) {
  return schema.type === 'object'
}

/**
 * Checks if a given JSON schema is a GbnfJsonArraySchema.
 * @param {GbnfJsonSchema} schema - The JSON schema to check.
 * @returns {boolean} - True if the schema is a GbnfJsonArraySchema, otherwise false.
 */
function isGbnfJsonArraySchema (schema) {
  return schema.type === 'array'
}

/**
 * Checks if a GBNF JSON basic schema includes a specific type.
 *
 * @param {object} schema - The GBNF JSON basic schema to check.
 * @param {string} type - The type to check for inclusion in the schema.
 * @returns {boolean} True if the schema includes the type, false otherwise.
 */
function isGbnfJsonBasicSchemaIncludesType (schema, type) {
  if (Array.isArray(schema.type)) return schema.type.includes(type)

  return schema.type === type
}

/**
 * Validates an object against a Gbnf JSON schema and returns whether the object conforms to the schema.
 * @template {GbnfJsonSchema} T
 * @param {*} object - The object to validate.
 * @param {T} schema - The Gbnf JSON schema to validate against.
 * @returns {boolean} - True if the object conforms to the schema, otherwise false.
 */
export function validateObjectAgainstGbnfSchema (object, schema) {
  try {
    return validateObjectWithGbnfSchema(object, schema)
  } catch (err) {
    if (err instanceof TechnicalValidationError) {
      throw new JsonSchemaValidationError(err.message, object, schema)
    }
    throw err
  }
}

/**
 * Error class for JSON schema validation errors.
 */
export class JsonSchemaValidationError extends Error {
  /**
   * Creates a new instance of JsonSchemaValidationError.
   * @param {string} message - The error message.
   * @param {*} object - The object being validated.
   * @param {GbnfJsonSchema} schema - The JSON schema.
   */
  constructor (message, object, schema) {
    super(message)
    this.object = object
    this.schema = schema
  }
}

/**
 * Error class for technical validation errors.
 */
class TechnicalValidationError extends Error {}

/**
 * Validates an object with a Gbnf JSON schema.
 * @template {GbnfJsonSchema} T
 * @param {*} object - The object to validate.
 * @param {T} schema - The Gbnf JSON schema to validate against.
 * @returns {boolean} - True if the object conforms to the schema, otherwise false.
 */
function validateObjectWithGbnfSchema (object, schema) {
  if (isGbnfJsonArraySchema(schema)) {
    return validateArray(object, schema)
  } else if (isGbnfJsonObjectSchema(schema)) {
    return validateObject(object, schema)
  } else if (isGbnfJsonOneOfSchema(schema)) {
    return validateOneOf(object, schema)
  } else if (isGbnfJsonEnumSchema(schema)) {
    return validateEnum(object, schema)
  } else if (isGbnfJsonConstSchema(schema)) {
    return validateConst(object, schema)
  }

  if (schema.type instanceof Array) {
    for (const type of schema.type) {
      if (validateImmutableType(object, type)) {
        return true
      }
    }

    throw new Error(`Expected one type of [${schema.type.map((type) => JSON.stringify(type)).join(', ')}] but got type "${object === null ? null : typeof object}"`)
  }

  if (validateImmutableType(object, schema.type)) {
    return true
  }

  throw new Error(`Expected "${schema.type}" but got "${object === null ? 'null' : typeof object}"`)
}

/**
 * Validates an array against a Gbnf JSON array schema.
 * @template {GbnfJsonArraySchema} T
 * @param {*} object - The object to validate.
 * @param {T} schema - The Gbnf JSON array schema to validate against.
 * @returns {boolean} - True if the object conforms to the schema, otherwise false.
 */
function validateArray (object, schema) {
  if (!(object instanceof Array)) {
    throw new TechnicalValidationError(`Expected an array but got "${typeof object}"`)
  }

  let res = true
  for (const item of object) {
    res &&= validateObjectWithGbnfSchema(item, schema.items)
  }

  return res
}

/**
 * Validates an object against a Gbnf JSON object schema.
 * @template {GbnfJsonObjectSchema} T
 * @param {*} object - The object to validate.
 * @param {T} schema - The Gbnf JSON object schema to validate against.
 * @returns {boolean} - True if the object conforms to the schema, otherwise false.
 */
function validateObject (object, schema) {
  if (typeof object !== 'object' || object === null) {
    throw new TechnicalValidationError(`Expected an object but got "${typeof object}"`)
  }

  const objectKeys = Object.keys(object)
  const objectKeysSet = new Set(objectKeys)
  const schemaKeys = Object.keys(schema.properties)
  const schemaKeysSet = new Set(schemaKeys)

  const extraKeys = objectKeys.filter((key) => !schemaKeysSet.has(key))
  if (extraKeys.length > 0) {
    throw new TechnicalValidationError(`Unexpected keys: ${extraKeys.map((key) => JSON.stringify(key)).join(', ')}`)
  }

  const missingKeys = schemaKeys.filter((key) => !objectKeysSet.has(key))
  if (missingKeys.length > 0) {
    throw new TechnicalValidationError(`Missing keys: ${missingKeys.map((key) => JSON.stringify(key)).join(', ')}`)
  }

  let res = true
  for (const key of schemaKeys) {
    res &&= validateObjectWithGbnfSchema(object[key], schema.properties[key])
  }

  return res
}

/**
 * Validates an object against a Gbnf JSON oneOf schema.
 * @template {GbnfJsonOneOfSchema} T
 * @param {*} object - The object to validate.
 * @param {T} schema - The Gbnf JSON oneOf schema to validate against.
 * @returns {boolean} - True if the object conforms to the schema, otherwise false.
 */
function validateOneOf (object, schema) {
  for (const item of schema.oneOf) {
    try {
      return validateObjectWithGbnfSchema(object, item)
    } catch (err) {
      if (err instanceof TechnicalValidationError) {
        continue
      }

      throw err
    }
  }

  throw new TechnicalValidationError(`Expected one of ${schema.oneOf.length} schemas but got ${JSON.stringify(object)}`)
}

/**
 * Validates an object against a Gbnf JSON enum schema.
 * @template {GbnfJsonEnumSchema} T
 * @param {*} object - The object to validate.
 * @param {T} schema - The Gbnf JSON enum schema to validate against.
 * @returns {boolean} - True if the object conforms to the schema, otherwise false.
 */
function validateEnum (object, schema) {
  for (const value of schema.enum) {
    if (object === value) {
      return true
    }
  }

  throw new TechnicalValidationError(`Expected one of [${schema.enum.map((item) => JSON.stringify(item)).join(', ')}] but got ${JSON.stringify(object)}`)
}

/**
 * Validates an object against a Gbnf JSON const schema.
 * @template {GbnfJsonConstSchema} T
 * @param {*} object - The object to validate.
 * @param {T} schema - The Gbnf JSON const schema to validate against.
 * @returns {boolean} - True if the object conforms to the schema, otherwise false.
 */
function validateConst (object, schema) {
  if (object === schema.const) {
    return true
  }

  throw new TechnicalValidationError(`Expected ${JSON.stringify(schema.const)} but got ${JSON.stringify(object)}`)
}

/**
 * Validates an object against an immutable type.
 * @param {*} value - The value to validate.
 * @param {GbnfJsonSchemaImmutableType} type - The immutable type to validate against.
 * @returns {boolean} - True if the value conforms to the immutable type, otherwise false.
 */
function validateImmutableType (value, type) {
  if (type === 'string') {
    return typeof value === 'string'
  } else if (type === 'number') {
    return typeof value === 'number'
  } else if (type === 'integer') {
    if (typeof value !== 'number') {
      return false
    }
    return value % 1 === 0
  } else if (type === 'boolean') {
    return typeof value === 'boolean'
  } else if (type === 'null') {
    return value === null
  } else {
    throw new TechnicalValidationError(`Unknown immutable type ${JSON.stringify(type)}`)
  }
}

class GbnfGrammarGenerator {
  constructor () {
    this.rules = new Map()
    this.ruleId = 0
  }

  generateRuleName () {
    const ruleId = this.ruleId
    this.ruleId++

    return `rule${ruleId}`
  }
}

class GbnfTerminal {
  constructor () {
    this._ruleName = null
  }

  getRuleName (grammarGenerator) {
    if (this._ruleName != null) { return this._ruleName }

    const ruleName = grammarGenerator.generateRuleName()
    this._ruleName = ruleName

    return ruleName
  }

  // Abstract method placeholder - should be implemented in subclasses
  getGrammar (grammarGenerator) {
    throw new Error('You have to implement the method getGrammar!')
  }

  resolve (grammarGenerator) {
    const ruleName = this.getRuleName(grammarGenerator)

    if (!grammarGenerator.rules.has(ruleName)) { grammarGenerator.rules.set(ruleName, this.getGrammar(grammarGenerator)) }

    return ruleName
  }
}

class GbnfArray extends GbnfTerminal {
  constructor (items) {
    super()
    this.items = items
  }

  getGrammar (grammarGenerator) {
    const whitespaceRuleName = new GbnfWhitespace().resolve(grammarGenerator)
    const itemsGrammarRuleName = this.items.resolve(grammarGenerator)

    return new GbnfGrammar([
      '"["', whitespaceRuleName,
      new GbnfOr([
        new GbnfGrammar([
          '(', itemsGrammarRuleName, ')',
          '(', '","', whitespaceRuleName, itemsGrammarRuleName, ')*'
        ]),
        new GbnfGrammar([
          '(', itemsGrammarRuleName, ')?'
        ])
      ]).getGrammar(grammarGenerator),
      whitespaceRuleName, '"]"'
    ]).getGrammar()
  }
}

class GbnfGrammar extends GbnfTerminal {
  constructor (grammar) {
    super()
    this.grammar = grammar
  }

  getGrammar () {
    if (Array.isArray(this.grammar)) {
      return this.grammar
        .filter(item => item !== '')
        .join(' ')
    }

    return this.grammar
  }
}

class GbnfWhitespace extends GbnfTerminal {
  constructor ({ newLinesAllowed = true } = {}) {
    super()
    this.newLinesAllowed = newLinesAllowed
  }

  getGrammar () {
    if (this.newLinesAllowed) { return '[\\n]? [ \\t]* [\\n]?' }

    return '[ \\t]*'
  }

  getRuleName () {
    if (this.newLinesAllowed) { return reservedRuleNames.whitespace.withNewLines }

    return reservedRuleNames.whitespace.withoutNewLines
  }
}

class GbnfBoolean extends GbnfTerminal {
  getGrammar (grammarGenerator) {
    return new GbnfOr([
      new GbnfGrammar('"true"'),
      new GbnfGrammar('"false"')
    ]).getGrammar(grammarGenerator)
  }

  getRuleName () {
    return reservedRuleNames.boolean
  }
}

class GbnfBooleanValue extends GbnfTerminal {
  constructor (value) {
    super()
    this.value = value
  }

  getGrammar () {
    if (this.value) { return '"true"' }

    return '"false"'
  }

  resolve () {
    return this.getGrammar()
  }
}

class GbnfNull extends GbnfTerminal {
  getGrammar () {
    return '"null"'
  }

  getRuleName () {
    return reservedRuleNames.null
  }
}

class GbnfNumber extends GbnfTerminal {
  constructor ({ allowFractional = true } = {}) {
    super()
    this.allowFractional = allowFractional
  }

  getGrammar () {
    const numberGrammar = '("-"? ([0-9] | [1-9] [0-9]*))'

    if (this.allowFractional) { return numberGrammar + ' ("." [0-9]+)? ([eE] [-+]? [0-9]+)?' }

    return numberGrammar
  }

  getRuleName () {
    if (this.allowFractional) { return reservedRuleNames.number.fractional }

    return reservedRuleNames.number.integer
  }
}

class GbnfNumberValue extends GbnfTerminal {
  constructor (value) {
    super()
    this.value = value
  }

  getGrammar () {
    return '"' + JSON.stringify(this.value) + '"'
  }

  resolve () {
    return this.getGrammar()
  }
}

class GbnfObjectMap extends GbnfTerminal {
  constructor (fields) {
    super()
    this.fields = fields
  }

  getGrammar (grammarGenerator) {
    const whitespaceRuleName = new GbnfWhitespace().resolve(grammarGenerator)

    return new GbnfGrammar([
      '"{"', whitespaceRuleName,
      ...this.fields.map(({ key, value }, index) => {
        return new GbnfGrammar([
          key.getGrammar(), '":"', '[ ]?', value.resolve(grammarGenerator),
          index < this.fields.length - 1 ? '","' : '',
          whitespaceRuleName
        ]).getGrammar()
      }),
      '"}"'
    ]).getGrammar()
  }
}

class GbnfString extends GbnfTerminal {
  getGrammar () {
    return (
      '"\\"" ( ' +
      '[^"\\\\]' +
      ' | ' +
      '"\\\\" (["\\\\/bfnrt] | "u" [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F])' + // escape sequences
      ')* "\\""'
    )
  }

  getRuleName () {
    return reservedRuleNames.string
  }
}

class GbnfStringValue extends GbnfTerminal {
  constructor (value) {
    super()
    this.value = value
  }

  getGrammar () {
    return [
      '"',
      '\\"',
      this.value
        .replaceAll('\\', '\\\\')
        .replaceAll('\t', '\\t')
        .replaceAll('\r', '\\r')
        .replaceAll('\n', '\\n')
        .replaceAll('"', '\\\\"' + '\\"'),
      '\\"',
      '"'
    ].join('')
  }
}

const grammarNoValue = '""'

const reservedRuleNames = {
  null: 'null-rule',
  boolean: 'boolean-rule',
  number: {
    fractional: 'fractional-number-rule',
    integer: 'integer-number-rule'
  },
  string: 'string-rule',
  whitespace: {
    withNewLines: 'whitespace-new-lines-rule',
    withoutNewLines: 'whitespace-no-new-lines-rule'
  }
}

// Optionally, to make reservedRuleNames as immutable as possible in JavaScript:
Object.freeze(reservedRuleNames)
Object.freeze(reservedRuleNames.number)
Object.freeze(reservedRuleNames.whitespace)

class GbnfOr extends GbnfTerminal {
  constructor (values) {
    super()
    this.values = values
  }

  getGrammar (grammarGenerator) {
    const mappedValues = this.values
      .map(v => v.resolve(grammarGenerator))
      .filter(value => value !== '' && value !== grammarNoValue)

    if (mappedValues.length === 0) { return grammarNoValue } else if (mappedValues.length === 1) { return mappedValues[0] }

    return '( ' + mappedValues.join(' | ') + ' )'
  }

  resolve (grammarGenerator) {
    const mappedValues = this.values
      .map(v => v.resolve(grammarGenerator))
      .filter(value => value !== '' && value !== grammarNoValue)

    if (mappedValues.length === 0) { return grammarNoValue } else if (mappedValues.length === 1) { return mappedValues[0] }

    return super.resolve(grammarGenerator)
  }
}

function getGbnfJsonTerminalForLiteral (literal) {
  if (literal === null) { return new GbnfNull() }
  if (typeof literal === 'boolean') { return new GbnfBooleanValue(literal) }
  if (typeof literal === 'number') { return new GbnfNumberValue(literal) }
  if (typeof literal === 'string') { return new GbnfStringValue(literal) }
  throw new Error(`Unrecognized literal type: ${typeof literal}`)
}

function getGbnfJsonTerminalForGbnfJsonSchema (schema, grammarGenerator) {
  if (isGbnfJsonOneOfSchema(schema)) {
    const values = schema.oneOf
      .map((altSchema) => getGbnfJsonTerminalForGbnfJsonSchema(altSchema, grammarGenerator))

    return new GbnfOr(values)
  } else if (isGbnfJsonConstSchema(schema)) {
    return getGbnfJsonTerminalForLiteral(schema.const)
  } else if (isGbnfJsonEnumSchema(schema)) {
    return new GbnfOr(schema.enum.map((item) => getGbnfJsonTerminalForLiteral(item)))
  } else if (isGbnfJsonObjectSchema(schema)) {
    return new GbnfObjectMap(
      Object.entries(schema.properties).map(([propName, propSchema]) => {
        return {
          required: true,
          key: new GbnfStringValue(propName),
          value: getGbnfJsonTerminalForGbnfJsonSchema(propSchema, grammarGenerator)
        }
      })
    )
  } else if (isGbnfJsonArraySchema(schema)) {
    return new GbnfArray(getGbnfJsonTerminalForGbnfJsonSchema(schema.items, grammarGenerator))
  }

  const terminals = []

  if (isGbnfJsonBasicSchemaIncludesType(schema, 'string')) { terminals.push(new GbnfString()) }
  if (isGbnfJsonBasicSchemaIncludesType(schema, 'number')) { terminals.push(new GbnfNumber({ allowFractional: true })) }
  if (isGbnfJsonBasicSchemaIncludesType(schema, 'integer')) { terminals.push(new GbnfNumber({ allowFractional: false })) }
  if (isGbnfJsonBasicSchemaIncludesType(schema, 'boolean')) { terminals.push(new GbnfBoolean()) }
  if (isGbnfJsonBasicSchemaIncludesType(schema, 'null')) { terminals.push(new GbnfNull()) }

  return new GbnfOr(terminals)
}

/**
 * Generates a GBNF grammar string for a given GbnfJsonSchema.
 * @param {GbnfJsonSchema} schema - The GbnfJsonSchema for which to generate the grammar.
 * @returns {string} - The GBNF grammar string.
 */
export function getGbnfGrammarForGbnfJsonSchema (schema) {
  const grammarGenerator = new GbnfGrammarGenerator()
  const rootTerminal = getGbnfJsonTerminalForGbnfJsonSchema(schema, grammarGenerator)
  const rootGrammar = rootTerminal.getGrammar(grammarGenerator)

  const rules = [{
    name: 'root',
    grammar: rootGrammar + ' [\\n]'.repeat(4) + ' [\\n]*'
  }]

  for (const [ruleName, grammar] of grammarGenerator.rules.entries()) {
    if (grammar == null) {
      continue
    }

    rules.push({
      name: ruleName,
      grammar
    })
  }

  const ruleStrings = rules.map((rule) => rule.name + ' ::= ' + rule.grammar)
  const gbnf = ruleStrings.join('\n')

  return gbnf
}
