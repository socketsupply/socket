/* eslint-disable */
// The MIT License (MIT)
//
// Copyright (c) 2015 Mathias Rasmussen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Query String Utilities - https://github.com/mathiasvr/querystring

import { Buffer } from './buffer.js'

const HEX_TABLE = new Array(256)

for (var i = 0; i < 256; ++i) {
  HEX_TABLE[i] = '%' + ((i < 16 ? '0' : '') + i.toString(16)).toUpperCase()
}


// a safe fast alternative to decodeURIComponent
export function unescapeBuffer (s, decodeSpaces) {
  var out = new Buffer(s.length);
  var state = 0;
  var n, m, hexchar;

  for (var inIndex = 0, outIndex = 0; inIndex <= s.length; inIndex++) {
    var c = inIndex < s.length ? s.charCodeAt(inIndex) : NaN;
    switch (state) {
      case 0: // Any character
        switch (c) {
          case 37: // '%'
            n = 0;
            m = 0;
            state = 1;
            break;
          case 43: // '+'
            if (decodeSpaces)
              c = 32; // ' '
            // falls through
          default:
            out[outIndex++] = c;
            break;
        }
        break;

      case 1: // First hex digit
        hexchar = c;
        if (c >= 48/*0*/ && c <= 57/*9*/) {
          n = c - 48/*0*/;
        } else if (c >= 65/*A*/ && c <= 70/*F*/) {
          n = c - 65/*A*/ + 10;
        } else if (c >= 97/*a*/ && c <= 102/*f*/) {
          n = c - 97/*a*/ + 10;
        } else {
          out[outIndex++] = 37/*%*/;
          out[outIndex++] = c;
          state = 0;
          break;
        }
        state = 2;
        break;

      case 2: // Second hex digit
        state = 0;
        if (c >= 48/*0*/ && c <= 57/*9*/) {
          m = c -  48/*0*/;
        } else if (c >= 65/*A*/ && c <= 70/*F*/) {
          m = c - 65/*A*/ + 10;
        } else if (c >= 97/*a*/ && c <= 102/*f*/) {
          m = c - 97/*a*/ + 10;
        } else {
          out[outIndex++] = 37/*%*/;
          out[outIndex++] = hexchar;
          out[outIndex++] = c;
          break;
        }
        out[outIndex++] = 16 * n + m;
        break;
    }
  }

  return out.slice(0, outIndex - 1);
}

export function unescape (s, decodeSpaces) {
  try {
    return decodeURIComponent(s);
  } catch (e) {
    return unescapeBuffer(s, decodeSpaces).toString();
  }
}

export function escape (str) {
  // replaces encodeURIComponent
  // http://www.ecma-international.org/ecma-262/5.1/#sec-15.1.3.4
  if (typeof str !== 'string')
    str += '';
  var out = '';
  var lastPos = 0;

  for (var i = 0; i < str.length; ++i) {
    var c = str.charCodeAt(i);

    // These characters do not need escaping (in order):
    // ! - . _ ~
    // ' ( ) *
    // digits
    // alpha (uppercase)
    // alpha (lowercase)
    if (c === 0x21 || c === 0x2D || c === 0x2E || c === 0x5F || c === 0x7E ||
        (c >= 0x27 && c <= 0x2A) ||
        (c >= 0x30 && c <= 0x39) ||
        (c >= 0x41 && c <= 0x5A) ||
        (c >= 0x61 && c <= 0x7A)) {
      continue;
    }

    if (i - lastPos > 0)
      out += str.slice(lastPos, i);

    // Other ASCII characters
    if (c < 0x80) {
      lastPos = i + 1;
      out += HEX_TABLE[c];
      continue;
    }

    // Multi-byte characters ...
    if (c < 0x800) {
      lastPos = i + 1;
      out += HEX_TABLE[0xC0 | (c >> 6)] + HEX_TABLE[0x80 | (c & 0x3F)];
      continue;
    }
    if (c < 0xD800 || c >= 0xE000) {
      lastPos = i + 1;
      out += HEX_TABLE[0xE0 | (c >> 12)] +
             HEX_TABLE[0x80 | ((c >> 6) & 0x3F)] +
             HEX_TABLE[0x80 | (c & 0x3F)];
      continue;
    }
    // Surrogate pair
    ++i;
    var c2;
    if (i < str.length)
      c2 = str.charCodeAt(i) & 0x3FF;
    else
      throw new URIError('URI malformed');
    lastPos = i + 1;
    c = 0x10000 + (((c & 0x3FF) << 10) | c2);
    out += HEX_TABLE[0xF0 | (c >> 18)] +
           HEX_TABLE[0x80 | ((c >> 12) & 0x3F)] +
           HEX_TABLE[0x80 | ((c >> 6) & 0x3F)] +
           HEX_TABLE[0x80 | (c & 0x3F)];
  }
  if (lastPos === 0)
    return str;
  if (lastPos < str.length)
    return out + str.slice(lastPos);

  return out;
}

function stringifyPrimitive (v) {
  if (typeof v === 'string')
    return v;
  if (typeof v === 'number' && isFinite(v))
    return '' + v;
  if (typeof v === 'boolean')
    return v ? 'true' : 'false';
  return '';
}

export function stringify (obj, sep, eq, options) {
  sep = sep || '&';
  eq = eq || '=';

  var encode = escape;
  if (options && typeof options.encodeURIComponent === 'function') {
    encode = options.encodeURIComponent;
  }

  if (obj !== null && typeof obj === 'object') {
    var keys = Object.keys(obj);
    var len = keys.length;
    var flast = len - 1;
    var fields = '';
    for (var i = 0; i < len; ++i) {
      var k = keys[i];
      var v = obj[k];
      var ks = encode(stringifyPrimitive(k)) + eq;

      if (Array.isArray(v)) {
        var vlen = v.length;
        var vlast = vlen - 1;
        for (var j = 0; j < vlen; ++j) {
          fields += ks + encode(stringifyPrimitive(v[j]));
          if (j < vlast)
            fields += sep;
        }
        if (vlen && i < flast)
          fields += sep;
      } else {
        fields += ks + encode(stringifyPrimitive(v));
        if (i < flast)
          fields += sep;
      }
    }
    return fields;
  }
  return '';
}

// Parse a key/val string.
export function parse (qs, sep, eq, options) {
  sep = sep || '&';
  eq = eq || '=';

  var obj = {}

  if (typeof qs !== 'string' || qs.length === 0) {
    return obj;
  }

  if (typeof sep !== 'string')
    sep += '';

  var eqLen = eq.length;
  var sepLen = sep.length;

  var maxKeys = 1000;
  if (options && typeof options.maxKeys === 'number') {
    maxKeys = options.maxKeys;
  }

  var pairs = Infinity;
  if (maxKeys > 0)
    pairs = maxKeys;

  var decode = unescape;
  if (options && typeof options.decodeURIComponent === 'function') {
    decode = options.decodeURIComponent;
  }
  var customDecode = (decode !== unescape);

  var keys = [];
  var lastPos = 0;
  var sepIdx = 0;
  var eqIdx = 0;
  var key = '';
  var value = '';
  var keyEncoded = customDecode;
  var valEncoded = customDecode;
  var encodeCheck = 0;
  for (var i = 0; i < qs.length; ++i) {
    var code = qs.charCodeAt(i);

    // Try matching key/value pair separator (e.g. '&')
    if (code === sep.charCodeAt(sepIdx)) {
      if (++sepIdx === sepLen) {
        // Key/value pair separator match!
        var end = i - sepIdx + 1;
        if (eqIdx < eqLen) {
          // If we didn't find the key/value separator, treat the substring as
          // part of the key instead of the value
          if (lastPos < end)
            key += qs.slice(lastPos, end);
        } else if (lastPos < end)
          value += qs.slice(lastPos, end);
        if (keyEncoded)
          key = decodeStr(key, decode);
        if (valEncoded)
          value = decodeStr(value, decode);
        // Use a key array lookup instead of using hasOwnProperty(), which is
        // slower
        if (keys.indexOf(key) === -1) {
          obj[key] = value;
          keys[keys.length] = key;
        } else {
          var curValue = obj[key];
          // `instanceof Array` is used instead of Array.isArray() because it
          // is ~15-20% faster with v8 4.7 and is safe to use because we are
          // using it with values being created within this function
          if (curValue instanceof Array)
            curValue[curValue.length] = value;
          else
            obj[key] = [curValue, value];
        }
        if (--pairs === 0)
          break;
        keyEncoded = valEncoded = customDecode;
        encodeCheck = 0;
        key = value = '';
        lastPos = i + 1;
        sepIdx = eqIdx = 0;
      }
      continue;
    } else {
      sepIdx = 0;
      if (!valEncoded) {
        // Try to match an (valid) encoded byte (once) to minimize unnecessary
        // calls to string decoding functions
        if (code === 37/*%*/) {
          encodeCheck = 1;
        } else if (encodeCheck > 0 &&
                   ((code >= 48/*0*/ && code <= 57/*9*/) ||
                    (code >= 65/*A*/ && code <= 70/*Z*/) ||
                    (code >= 97/*a*/ && code <= 102/*z*/))) {
          if (++encodeCheck === 3)
            valEncoded = true;
        } else {
          encodeCheck = 0;
        }
      }
    }

    // Try matching key/value separator (e.g. '=') if we haven't already
    if (eqIdx < eqLen) {
      if (code === eq.charCodeAt(eqIdx)) {
        if (++eqIdx === eqLen) {
          // Key/value separator match!
          var end = i - eqIdx + 1;
          if (lastPos < end)
            key += qs.slice(lastPos, end);
          encodeCheck = 0;
          lastPos = i + 1;
        }
        continue;
      } else {
        eqIdx = 0;
        if (!keyEncoded) {
          // Try to match an (valid) encoded byte once to minimize unnecessary
          // calls to string decoding functions
          if (code === 37/*%*/) {
            encodeCheck = 1;
          } else if (encodeCheck > 0 &&
                     ((code >= 48/*0*/ && code <= 57/*9*/) ||
                      (code >= 65/*A*/ && code <= 70/*Z*/) ||
                      (code >= 97/*a*/ && code <= 102/*z*/))) {
            if (++encodeCheck === 3)
              keyEncoded = true;
          } else {
            encodeCheck = 0;
          }
        }
      }
    }

    if (code === 43/*+*/) {
      if (eqIdx < eqLen) {
        if (i - lastPos > 0)
          key += qs.slice(lastPos, i);
        key += '%20';
        keyEncoded = true;
      } else {
        if (i - lastPos > 0)
          value += qs.slice(lastPos, i);
        value += '%20';
        valEncoded = true;
      }
      lastPos = i + 1;
    }
  }

  // Check if we have leftover key or value data
  if (pairs > 0 && (lastPos < qs.length || eqIdx > 0)) {
    if (lastPos < qs.length) {
      if (eqIdx < eqLen)
        key += qs.slice(lastPos);
      else if (sepIdx < sepLen)
        value += qs.slice(lastPos);
    }
    if (keyEncoded)
      key = decodeStr(key, decode);
    if (valEncoded)
      value = decodeStr(value, decode);
    // Use a key array lookup instead of using hasOwnProperty(), which is
    // slower
    if (keys.indexOf(key) === -1) {
      obj[key] = value;
      keys[keys.length] = key;
    } else {
      var curValue = obj[key];
      // `instanceof Array` is used instead of Array.isArray() because it
      // is ~15-20% faster with v8 4.7 and is safe to use because we are
      // using it with values being created within this function
      if (curValue instanceof Array)
        curValue[curValue.length] = value;
      else
        obj[key] = [curValue, value];
    }
  }

  return obj;
}

export const decode = parse
export const encode = stringify

export default {
  decode,
  encode,
  parse,
  stringify,
  escape,
  unescape
}

function decodeStr(s, decoder) {
  try {
    return decoder(s);
  } catch (e) {
    return unescape(s, true);
  }
}
