/**
 * @module Language
 *
 * A module for querying ISO 639-1 language names and codes and working with
 * RFC 5646 language tags.
 */
import Enumeration from './enumeration.js'
import { toProperCase } from './util.js'

/**
 * Normalizes a language name to be proper cased, space or '-' separated.
 * @ignore
 * @param {string} value
 * @return {string}
 */
function normalizeName (value) {
  return value
    .split(' ')
    .map(toProperCase)
    .join(' ')
    .split('-')
    .map(toProperCase)
    .join('-')
}

/**
 * @ignore
 * @param {string[]|Enumeration} items
 * @param {string} query
 * @param {boolean=} [caseSensitive = false]
 * @param {boolean=} [strict = false]
 * @return {{ index: number, value: string }[]}
 */
function filterItems (items, query, caseSensitive = false, strict = false) {
  const filtered = []
  const escaped = query.replace('(', '\\(').replace(')', '\\)')
  const regex = new RegExp(
    `^(${escaped})${strict ? '$' : ''}`,
    caseSensitive ? '' : 'i'
  )

  for (const item of items) {
    if (regex.test(item)) {
      const index = items[item]
      filtered.push({ index, value: item })
    }
  }

  return filtered
}

/**
 * A list of ISO 639-1 language names.
 * @type {string[]}
 */
export const names = Enumeration.from([
  'Abkhazian',
  'Afar',
  'Afrikaans',
  'Albanian',
  'Amharic',
  'Arabic',
  'Armenian',
  'Assamese',
  'Aymara',
  'Azerbaijani',
  'Bashkir',
  'Basque',
  'Bengali, Bangla',
  'Bhutani',
  'Bihari',
  'Bislama',
  'Breton',
  'Bulgarian',
  'Burmese',
  'Byelorussian',
  'Cambodian',
  'Catalan',
  'Chinese',
  'Corsican',
  'Croatian',
  'Czech',
  'Danish',
  'Dutch',
  'English, American',
  'Esperanto',
  'Estonian',
  'Faeroese',
  'Fiji',
  'Finnish',
  'French',
  'Frisian',
  'Gaelic (Scots Gaelic)',
  'Galician',
  'Georgian',
  'German',
  'Greek',
  'Greenlandic',
  'Guarani',
  'Gujarati',
  'Hausa',
  'Hebrew',
  'Hindi',
  'Hungarian',
  'Icelandic',
  'Indonesian',
  'Interlingua',
  'Interlingue',
  'Inupiak',
  'Irish',
  'Italian',
  'Japanese',
  'Javanese',
  'Kannada',
  'Kashmiri',
  'Kazakh',
  'Kinyarwanda',
  'Kirghiz',
  'Kirundi',
  'Korean',
  'Kurdish',
  'Laothian',
  'Latin',
  'Latvian, Lettish',
  'Lingala',
  'Lithuanian',
  'Macedonian',
  'Malagasy',
  'Malay',
  'Malayalam',
  'Maltese',
  'Maori',
  'Marathi',
  'Moldavian',
  'Mongolian',
  'Nauru',
  'Nepali',
  'Norwegian',
  'Occitan',
  'Oriya',
  'Oromo, Afan',
  'Pashto, Pushto',
  'Persian',
  'Polish',
  'Portuguese',
  'Punjabi',
  'Quechua',
  'Rhaeto-Romance',
  'Romanian',
  'Russian',
  'Samoan',
  'Sangro',
  'Sanskrit',
  'Serbian',
  'Serbo-Croatian',
  'Sesotho',
  'Setswana',
  'Shona',
  'Sindhi',
  'Singhalese',
  'Siswati',
  'Slovak',
  'Slovenian',
  'Somali',
  'Spanish',
  'Sudanese',
  'Swahili',
  'Swedish',
  'Tagalog',
  'Tajik',
  'Tamil',
  'Tatar',
  'Tegulu',
  'Thai',
  'Tibetan',
  'Tigrinya',
  'Tonga',
  'Tsonga',
  'Turkish',
  'Turkmen',
  'Twi',
  'Ukrainian',
  'Urdu',
  'Uzbek',
  'Vietnamese',
  'Volapuk',
  'Welsh',
  'Wolof',
  'Xhosa',
  'Yiddish',
  'Yoruba',
  'Zulu'
])

/**
 * A list of ISO 639-1 language codes.
 * @type {string[]}
 */
export const codes = Enumeration.from([
  'AB',
  'AA',
  'AF',
  'SQ',
  'AM',
  'AR',
  'HY',
  'AS',
  'AY',
  'AZ',
  'BA',
  'EU',
  'BN',
  'DZ',
  'BH',
  'BI',
  'BR',
  'BG',
  'MY',
  'BE',
  'KM',
  'CA',
  'ZH',
  'CO',
  'HR',
  'CS',
  'DA',
  'NL',
  'EN',
  'EO',
  'ET',
  'FO',
  'FJ',
  'FI',
  'FR',
  'FY',
  'GD',
  'GL',
  'KA',
  'DE',
  'EL',
  'KL',
  'GN',
  'GU',
  'HA',
  'IW',
  'HI',
  'HU',
  'IS',
  'IN',
  'IA',
  'IE',
  'IK',
  'GA',
  'IT',
  'JA',
  'JW',
  'KN',
  'KS',
  'KK',
  'RW',
  'KY',
  'RN',
  'KO',
  'KU',
  'LO',
  'LA',
  'LV',
  'LN',
  'LT',
  'MK',
  'MG',
  'MS',
  'ML',
  'MT',
  'MI',
  'MR',
  'MO',
  'MN',
  'NA',
  'NE',
  'NO',
  'OC',
  'OR',
  'OM',
  'PS',
  'FA',
  'PL',
  'PT',
  'PA',
  'QU',
  'RM',
  'RO',
  'RU',
  'SM',
  'SG',
  'SA',
  'SR',
  'SH',
  'ST',
  'TN',
  'SN',
  'SD',
  'SI',
  'SS',
  'SK',
  'SL',
  'SO',
  'ES',
  'SU',
  'SW',
  'SV',
  'TL',
  'TG',
  'TA',
  'TT',
  'TE',
  'TH',
  'BO',
  'TI',
  'TO',
  'TS',
  'TR',
  'TK',
  'TW',
  'UK',
  'UR',
  'UZ',
  'VI',
  'VO',
  'CY',
  'WO',
  'XH',
  'JI',
  'YO',
  'ZU'
])

/**
 * A list of RFC 5646 language tag identifiers.
 * @see {@link http://tools.ietf.org/html/rfc5646}
 */
export const tags = Enumeration.from([
  'af',
  'af-ZA',
  'ar',
  'ar-AE',
  'ar-BH',
  'ar-DZ',
  'ar-EG',
  'ar-IQ',
  'ar-JO',
  'ar-KW',
  'ar-LB',
  'ar-LY',
  'ar-MA',
  'ar-OM',
  'ar-QA',
  'ar-SA',
  'ar-SY',
  'ar-TN',
  'ar-YE',
  'az',
  'az-AZ',
  'az-Cyrl-AZ',
  'be',
  'be-BY',
  'bg',
  'bg-BG',
  'bs-BA',
  'ca',
  'ca-ES',
  'cs',
  'cs-CZ',
  'cy',
  'cy-GB',
  'da',
  'da-DK',
  'de',
  'de-AT',
  'de-CH',
  'de-DE',
  'de-LI',
  'de-LU',
  'dv',
  'dv-MV',
  'el',
  'el-GR',
  'en',
  'en-AU',
  'en-BZ',
  'en-CA',
  'en-CB',
  'en-GB',
  'en-IE',
  'en-JM',
  'en-NZ',
  'en-PH',
  'en-TT',
  'en-US',
  'en-ZA',
  'en-ZW',
  'eo',
  'es',
  'es-AR',
  'es-BO',
  'es-CL',
  'es-CO',
  'es-CR',
  'es-DO',
  'es-EC',
  'es-ES',
  'es-GT',
  'es-HN',
  'es-MX',
  'es-NI',
  'es-PA',
  'es-PE',
  'es-PR',
  'es-PY',
  'es-SV',
  'es-UY',
  'es-VE',
  'et',
  'et-EE',
  'eu',
  'eu-ES',
  'fa',
  'fa-IR',
  'fi',
  'fi-FI',
  'fo',
  'fo-FO',
  'fr',
  'fr-BE',
  'fr-CA',
  'fr-CH',
  'fr-FR',
  'fr-LU',
  'fr-MC',
  'gl',
  'gl-ES',
  'gu',
  'gu-IN',
  'he',
  'he-IL',
  'hi',
  'hi-IN',
  'hr',
  'hr-BA',
  'hr-HR',
  'hu',
  'hu-HU',
  'hy',
  'hy-AM',
  'id',
  'id-ID',
  'is',
  'is-IS',
  'it',
  'it-CH',
  'it-IT',
  'ja',
  'ja-JP',
  'ka',
  'ka-GE',
  'kk',
  'kk-KZ',
  'kn',
  'kn-IN',
  'ko',
  'ko-KR',
  'kok',
  'kok-IN',
  'ky',
  'ky-KG',
  'lt',
  'lt-LT',
  'lv',
  'lv-LV',
  'mi',
  'mi-NZ',
  'mk',
  'mk-MK',
  'mn',
  'mn-MN',
  'mr',
  'mr-IN',
  'ms',
  'ms-BN',
  'ms-MY',
  'mt',
  'mt-MT',
  'nb',
  'nb-NO',
  'nl',
  'nl-BE',
  'nl-NL',
  'nn-NO',
  'ns',
  'ns-ZA',
  'pa',
  'pa-IN',
  'pl',
  'pl-PL',
  'ps',
  'ps-AR',
  'pt',
  'pt-BR',
  'pt-PT',
  'qu',
  'qu-BO',
  'qu-EC',
  'qu-PE',
  'ro',
  'ro-RO',
  'ru',
  'ru-RU',
  'sa',
  'sa-IN',
  'se',
  'se-FI',
  'se-NO',
  'se-SE',
  'sk',
  'sk-SK',
  'sl',
  'sl-SI',
  'sq',
  'sq-AL',
  'sr-BA',
  'sr-Cyrl-BA',
  'sr-SP',
  'sr-Cyrl-SP',
  'sv',
  'sv-FI',
  'sv-SE',
  'sw',
  'sw-KE',
  'syr',
  'syr-SY',
  'ta',
  'ta-IN',
  'te',
  'te-IN',
  'th',
  'th-TH',
  'tl',
  'tl-PH',
  'tn',
  'tn-ZA',
  'tr',
  'tr-TR',
  'tt',
  'tt-RU',
  'ts',
  'uk',
  'uk-UA',
  'ur',
  'ur-PK',
  'uz',
  'uz-UZ',
  'uz-Cyrl-UZ',
  'vi',
  'vi-VN',
  'xh',
  'xh-ZA',
  'zh',
  'zh-CN',
  'zh-HK',
  'zh-MO',
  'zh-SG',
  'zh-TW',
  'zu',
  'zu-ZA'
])

/**
 * A list of RFC 5646 language tag titles corresponding
 * to language tags.
 * @see {@link http://tools.ietf.org/html/rfc5646}
 */
export const descriptions = Enumeration.from([
  'Afrikaans',
  'Afrikaans (South Africa)',
  'Arabic',
  'Arabic (U.A.E.)',
  'Arabic (Bahrain)',
  'Arabic (Algeria)',
  'Arabic (Egypt)',
  'Arabic (Iraq)',
  'Arabic (Jordan)',
  'Arabic (Kuwait)',
  'Arabic (Lebanon)',
  'Arabic (Libya)',
  'Arabic (Morocco)',
  'Arabic (Oman)',
  'Arabic (Qatar)',
  'Arabic (Saudi Arabia)',
  'Arabic (Syria)',
  'Arabic (Tunisia)',
  'Arabic (Yemen)',
  'Azeri (Latin)',
  'Azeri (Latin) (Azerbaijan)',
  'Azeri (Cyrillic) (Azerbaijan)',
  'Belarusian',
  'Belarusian (Belarus)',
  'Bulgarian',
  'Bulgarian (Bulgaria)',
  'Bosnian (Bosnia and Herzegovina)',
  'Catalan',
  'Catalan (Spain)',
  'Czech',
  'Czech (Czech Republic)',
  'Welsh',
  'Welsh (United Kingdom)',
  'Danish',
  'Danish (Denmark)',
  'German',
  'German (Austria)',
  'German (Switzerland)',
  'German (Germany)',
  'German (Liechtenstein)',
  'German (Luxembourg)',
  'Divehi',
  'Divehi (Maldives)',
  'Greek',
  'Greek (Greece)',
  'English',
  'English (Australia)',
  'English (Belize)',
  'English (Canada)',
  'English (Caribbean)',
  'English (United Kingdom)',
  'English (Ireland)',
  'English (Jamaica)',
  'English (New Zealand)',
  'English (Republic of the Philippines)',
  'English (Trinidad and Tobago)',
  'English (United States)',
  'English (South Africa)',
  'English (Zimbabwe)',
  'Esperanto',
  'Spanish',
  'Spanish (Argentina)',
  'Spanish (Bolivia)',
  'Spanish (Chile)',
  'Spanish (Colombia)',
  'Spanish (Costa Rica)',
  'Spanish (Dominican Republic)',
  'Spanish (Ecuador)',
  'Spanish (Spain)',
  'Spanish (Guatemala)',
  'Spanish (Honduras)',
  'Spanish (Mexico)',
  'Spanish (Nicaragua)',
  'Spanish (Panama)',
  'Spanish (Peru)',
  'Spanish (Puerto Rico)',
  'Spanish (Paraguay)',
  'Spanish (El Salvador)',
  'Spanish (Uruguay)',
  'Spanish (Venezuela)',
  'Estonian',
  'Estonian (Estonia)',
  'Basque',
  'Basque (Spain)',
  'Farsi',
  'Farsi (Iran)',
  'Finnish',
  'Finnish (Finland)',
  'Faroese',
  'Faroese (Faroe Islands)',
  'French',
  'French (Belgium)',
  'French (Canada)',
  'French (Switzerland)',
  'French (France)',
  'French (Luxembourg)',
  'French (Principality of Monaco)',
  'Galician',
  'Galician (Spain)',
  'Gujarati',
  'Gujarati (India)',
  'Hebrew',
  'Hebrew (Israel)',
  'Hindi',
  'Hindi (India)',
  'Croatian',
  'Croatian (Bosnia and Herzegovina)',
  'Croatian (Croatia)',
  'Hungarian',
  'Hungarian (Hungary)',
  'Armenian',
  'Armenian (Armenia)',
  'Indonesian',
  'Indonesian (Indonesia)',
  'Icelandic',
  'Icelandic (Iceland)',
  'Italian',
  'Italian (Switzerland)',
  'Italian (Italy)',
  'Japanese',
  'Japanese (Japan)',
  'Georgian',
  'Georgian (Georgia)',
  'Kazakh',
  'Kazakh (Kazakhstan)',
  'Kannada',
  'Kannada (India)',
  'Korean',
  'Korean (Korea)',
  'Konkani',
  'Konkani (India)',
  'Kyrgyz',
  'Kyrgyz (Kyrgyzstan)',
  'Lithuanian',
  'Lithuanian (Lithuania)',
  'Latvian',
  'Latvian (Latvia)',
  'Maori',
  'Maori (New Zealand)',
  'FYRO Macedonian',
  'FYRO Macedonian (Former Yugoslav Republic of Macedonia)',
  'Mongolian',
  'Mongolian (Mongolia)',
  'Marathi',
  'Marathi (India)',
  'Malay',
  'Malay (Brunei Darussalam)',
  'Malay (Malaysia)',
  'Maltese',
  'Maltese (Malta)',
  'Norwegian (Bokm?l)',
  'Norwegian (Bokm?l) (Norway)',
  'Dutch',
  'Dutch (Belgium)',
  'Dutch (Netherlands)',
  'Norwegian (Nynorsk) (Norway)',
  'Northern Sotho',
  'Northern Sotho (South Africa)',
  'Punjabi',
  'Punjabi (India)',
  'Polish',
  'Polish (Poland)',
  'Pashto',
  'Pashto (Afghanistan)',
  'Portuguese',
  'Portuguese (Brazil)',
  'Portuguese (Portugal)',
  'Quechua',
  'Quechua (Bolivia)',
  'Quechua (Ecuador)',
  'Quechua (Peru)',
  'Romanian',
  'Romanian (Romania)',
  'Russian',
  'Russian (Russia)',
  'Sanskrit',
  'Sanskrit (India)',
  'Sami',
  'Sami (Finland)',
  'Sami (Norway)',
  'Sami (Sweden)',
  'Slovak',
  'Slovak (Slovakia)',
  'Slovenian',
  'Slovenian (Slovenia)',
  'Albanian',
  'Albanian (Albania)',
  'Serbian (Latin) (Bosnia and Herzegovina)',
  'Serbian (Cyrillic) (Bosnia and Herzegovina)',
  'Serbian (Latin) (Serbia and Montenegro)',
  'Serbian (Cyrillic) (Serbia and Montenegro)',
  'Swedish',
  'Swedish (Finland)',
  'Swedish (Sweden)',
  'Swahili',
  'Swahili (Kenya)',
  'Syriac',
  'Syriac (Syria)',
  'Tamil',
  'Tamil (India)',
  'Telugu',
  'Telugu (India)',
  'Thai',
  'Thai (Thailand)',
  'Tagalog',
  'Tagalog (Philippines)',
  'Tswana',
  'Tswana (South Africa)',
  'Turkish',
  'Turkish (Turkey)',
  'Tatar',
  'Tatar (Russia)',
  'Tsonga',
  'Ukrainian',
  'Ukrainian (Ukraine)',
  'Urdu',
  'Urdu (Islamic Republic of Pakistan)',
  'Uzbek (Latin)',
  'Uzbek (Latin) (Uzbekistan)',
  'Uzbek (Cyrillic) (Uzbekistan)',
  'Vietnamese',
  'Vietnamese (Viet Nam)',
  'Xhosa',
  'Xhosa (South Africa)',
  'Chinese',
  'Chinese (S)',
  'Chinese (Hong Kong)',
  'Chinese (Macau)',
  'Chinese (Singapore)',
  'Chinese (T)',
  'Zulu',
  'Zulu (South Africa)'
])

/**
 * A container for a language query response containing an ISO language
 * name and code.
 * @see {@link https://www.sitepoint.com/iso-2-letter-language-codes}
 */
export class LanguageQueryResult {
  #code = null
  #name = null
  #tags = null

  /**
   * `LanguageQueryResult` class constructor.
   * @param {string} code
   * @param {string} name
   * @param {string[]} [tags]
   */
  constructor (code, name, tags) {
    this.#code = code.toUpperCase()
    this.#name = normalizeName(name)
    this.#tags = Enumeration.from(tags)
  }

  /**
   * The language code corresponding to the query.
   * @type {string}
   */
  get code () {
    return this.#code
  }

  /**
   * The language name corresponding to the query.
   * @type {string}
   */
  get name () {
    return this.#name
  }

  /**
   * The language tags corresponding to the query.
   * @type {string[]}
   */
  get tags () {
    return Array.from(this.#tags)
  }

  /**
   * JSON represenation of a `LanguageQueryResult` instance.
   * @return {{
   *   code: string,
   *   name: string,
   *   tags: string[]
   * }}
   */
  toJSON () {
    const { code, name, tags } = this
    return { code, name, tags }
  }

  /**
   * Internal inspect function.
   * @ignore
   * @return {LanguageQueryResult}
   */
  inspect () {
    // eslint-disable-next-line
    return Object.assign(new class LanguageQueryResult {}, this.toJSON())
  }

  /**
   * @ignore
   */
  [Symbol.for('socket.util.inspect.custom')] () {
    return this.inspect()
  }

  /**
   * @ignore
   */
  [Symbol.for('nodejs.util.inspect.custom')] () {
    return this.inspect()
  }
}

/**
 * A container for a language code, tag, and description.
 */
export class LanguageDescription {
  #code = null
  #tag = null
  #description = null

  /**
   * `LanguageDescription` class constructor.
   * @param {string} code
   * @param {string} tag
   * @param {string} description
   */
  constructor (code, tag, description) {
    this.#code = code
    this.#tag = tag
    this.#description = description
  }

  /**
   * The language code corresponding to the language
   * @type {string}
   */
  get code () {
    return this.#code
  }

  /**
   * The language tag corresponding to the language.
   * @type {string}
   */
  get tag () {
    return this.#tag
  }

  /**
   * The language description corresponding to the language.
   * @type {string}
   */
  get description () {
    return this.#description
  }

  /**
   * JSON represenation of a `LanguageDescription` instance.
   * @return {{
   *   code: string,
   *   tag: string,
   *   description: string
   * }}
   */
  toJSON () {
    const { code, tag, description } = this
    return { code, tag, description }
  }

  /**
   * Internal inspect function.
   * @ignore
   * @return {LanguageDescription}
   */
  inspect () {
    // eslint-disable-next-line
    return Object.assign(new class LanguageDescription {}, this.toJSON())
  }

  /**
   * @ignore
   */
  [Symbol.for('socket.util.inspect.custom')] () {
    return this.inspect()
  }

  /**
   * @ignore
   */
  [Symbol.for('nodejs.util.inspect.custom')] () {
    return this.inspect()
  }
}

/**
 * Look up a language name or code by query.
 * @param {string} query
 * @param {object=} [options]
 * @param {boolean=} [options.strict = false]
 * @return {?LanguageQueryResult[]}
 */
export function lookup (query, options = { strict: false }) {
  const results = []

  if (arguments.length === 0) {
    throw new TypeError(
      'Failed to lookup query:' +
      '1 argument required, but only 0 present.'
    )
  }

  if (typeof query !== 'string') {
    throw new TypeError(
      'Failed to lookup query: Expecting string as first argument.'
    )
  }

  if (query.length === 0) {
    throw new TypeError(
      'Failed to lookup query: Expecting string to not be empty.'
    )
  }

  if (query.length < 2) {
    throw new TypeError(
      'Failed to lookup query: Expecting string to at least 2 characters.'
    )
  }

  const queried = {
    names: filterItems(names, query, false, options?.strict),
    codes: filterItems(codes, query.toUpperCase(), true, options?.strict),
    tags: filterItems(tags, query)
  }

  if (queried.codes.length === 0) {
    for (const tag of queried.tags) {
      const [prefix] = tag.value.split('-')
      queried.codes.push(...filterItems(codes, prefix))
    }

    if (queried.codes.length) {
      queried.names = []
    }
  }

  if (queried.codes.length === 0 && queried.names.length) {
    for (const name of queried.names) {
      queried.codes.push({ index: name.index, value: codes[name.index] })
    }
  }

  if (queried.names.length === 0) {
    for (const code of queried.codes) {
      queried.names.push({ index: code.index, value: names[code.index] })
    }
  }

  if (queried.tags.length === 0 && queried.codes.length) {
    for (const code of queried.codes) {
      queried.tags.push(...filterItems(tags, code.value))
    }

    if (queried.tags.length === 0) {
      queried.tags.push(...queried.codes.map((c) => ({ value: c.value.toLowerCase() })))
    }
  }

  for (const code of queried.codes) {
    const name = queried.names.find((n) => n.index === code.index)?.value

    if (!name) {
      continue
    }

    const regex = new RegExp(`^(${code.value}-?)`, 'i')
    const tags = queried.tags.filter((t) => regex.test(t.value))

    results.push(new LanguageQueryResult(
      code.value,
      name,
      tags.map((t) => t.value)
    ))
  }

  return results
}

/**
 * Describe a language by tag
 * @param {string} query
 * @param {object=} [options]
 * @param {boolean=} [options.strict = true]
 * @return {?LanguageDescription[]}
 */
export function describe (query, options = { strict: true }) {
  const queried = lookup(query, options)
  const results = []

  for (const item of queried) {
    for (const tag of item.tags) {
      const { code } = item
      const description = descriptions[tags[tag]] || names[codes[code]]
      results.push(new LanguageDescription(code, tag, description))
    }
  }

  return results
}

export default {
  codes,
  describe,
  lookup,
  names,
  tags
}
