import { test } from 'socket:test'
import { JsonSchemaGrammar } from 'socket:llm'

test('object', t => {
  const grammar = new JsonSchemaGrammar({
    type: 'object',
    properties: {
      message: {
        type: ['string', 'null']
      },
      numberOfWordsInMessage: {
        type: 'integer'
      },
      feelingGoodPercentage: {
        type: ['number']
      },
      feelingGood: {
        type: 'boolean'
      },
      feelingOverall: {
        enum: ['good', 'bad']
      },
      verbsInMessage: {
        type: 'array',
        items: {
          type: 'string'
        }
      }
    }
  })
  const schemaType = {
    message: 'string | null',
    numberOfWordsInMessage: 'number',
    feelingGoodPercentage: 'number',
    feelingGood: 'boolean',
    feelingOverall: '"good" | "bad"',
    verbsInMessage: 'string[]'
  }
  const exampleValidValue = {
    message: 'Hello, world!',
    numberOfWordsInMessage: 3,
    feelingGoodPercentage: 0.5,
    feelingGood: true,
    feelingOverall: 'good',
    verbsInMessage: ['Hello', 'world']
  }
  const exampleValidValue2 = {
    message: null,
    numberOfWordsInMessage: 3,
    feelingGoodPercentage: 0.5,
    feelingGood: false,
    feelingOverall: 'bad',
    verbsInMessage: ['Hello', 'world']
  }
  const exampleInvalidValue = {
    message: 'Hello, world!',
    numberOfWordsInMessage: 3,
    feelingGoodPercentage: 0.5,
    feelingGood: true,
    feelingOverall: 'good',
    verbsInMessage: ['Hello', 10]
  }
  const exampleInvalidValue2 = {
    message: 'Hello, world!',
    numberOfWordsInMessage: 3,
    feelingGoodPercentage: 0.5,
    feelingGood: true,
    feelingOverall: 'average',
    verbsInMessage: ['Hello', 'world']
  }
  const exampleInvalidValue3 = {
    message: 'Hello, world!',
    numberOfWordsInMessage: 3,
    feelingGoodPercentage: 0.5,
    feelingGood: true,
    feelingOverall: 'good',
    verbsInMessage: ['Hello', 'world', true]
  }
  const exampleInvalidValue4 = {
    message: 'Hello, world!',
    numberOfWordsInMessage: 3,
    feelingGoodPercentage: 0.5,
    feelingGood: true,
    feelingOverall: 'good',
    verbsInMessage: ['Hello', 'world', {}]
  }
  const exampleInvalidValue5 = {
    message: 'Hello, world!',
    numberOfWordsInMessage: 3,
    feelingGoodPercentage: 0.5,
    feelingGood: true,
    feelingOverall: 'good',
    verbsInMessage: ['Hello', 'world', null]
  }
  const exampleInvalidValue6 = {
    message: false,
    numberOfWordsInMessage: 3,
    feelingGoodPercentage: 0.5,
    feelingGood: true,
    feelingOverall: 'good',
    verbsInMessage: ['Hello', 'world']
  }

  // eslint-disable-next-line
  t.equal(grammar.grammar, 'root ::= "{" whitespace-new-lines-rule "\\"message\\"" ":" [ ]? rule0 "," whitespace-new-lines-rule "\\"numberOfWordsInMessage\\"" ":" [ ]? integer-number-rule "," whitespace-new-lines-rule "\\"feelingGoodPercentage\\"" ":" [ ]? fractional-number-rule "," whitespace-new-lines-rule "\\"feelingGood\\"" ":" [ ]? boolean-rule "," whitespace-new-lines-rule "\\"feelingOverall\\"" ":" [ ]? rule5 "," whitespace-new-lines-rule "\\"verbsInMessage\\"" ":" [ ]? rule6 whitespace-new-lines-rule "}" [\\n] [\\n] [\\n] [\\n] [\\n]*\nwhitespace-new-lines-rule ::= [\\n]? [ \\t]* [\\n]?\nstring-rule ::= "\\"\\" ( [^"\\\\] | "\\"\\\\" (["\\\\/bfnrt] | "u" [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F]))* "\\"\\\"\nnull-rule ::= "null"\nrule0 ::= ( string-rule | null-rule )\ninteger-number-rule ::= ("-"? ([0-9] | [1-9][0-9]*))\nfractional-number-rule ::= ("-"? ([0-9] | [1-9][0-9]*)) ("." [0-9]+)\nboolean-rule ::= "true" | "false"\nrule5 ::= "good" | "bad"\nrule6 ::= "[" whitespace-new-lines-rule ( string-rule [ ]? "," whitespace-new-lines-rule )* string-rule [ ]? "]"\n')
  t.deepEqual(grammar.typeOfSchema, schemaType)
  t.deepEqual(grammar.validate(exampleValidValue), [])
  t.deepEqual(grammar.validate(exampleValidValue2), [])
  t.deepEqual(grammar.validate(exampleInvalidValue), [
    'message: Expected type string, null, but got integer',
    'verbsInMessage: Expected type string, but got integer'
  ])
  t.deepEqual(grammar.validate(exampleInvalidValue2), [
    'feelingOverall: Expected type "good" or "bad", but got "average"'
  ])
  t.deepEqual(grammar.validate(exampleInvalidValue3), [
    'verbsInMessage: Expected type string, but got boolean'
  ])
  t.deepEqual(grammar.validate(exampleInvalidValue4), [
    'verbsInMessage: Expected type string, but got object'
  ])
  t.deepEqual(grammar.validate(exampleInvalidValue5), [
    'verbsInMessage: Expected type string, but got null'
  ])
  t.deepEqual(grammar.validate(exampleInvalidValue6), [
    'message: Expected type string, null, but got boolean'
  ])
})
