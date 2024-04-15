import Buffer from '../buffer.js'

export default function serialize (value) {
  if (!value || typeof value !== 'object') {
    return value
  }

  return map(value, (value) => {
    if (Buffer.isBuffer(value)) return value

    if (typeof value[Symbol.serialize] === 'function') {
      return value[Symbol.serialize]()
    }

    if (typeof value.toJSON === 'function') {
      return value.toJSON()
    }

    return value
  })
}

function map (object, callback) {
  if (Array.isArray(object)) {
    for (let i = 0; i < object.length; ++i) {
      object[i] = map(object[i], callback)
    }
  } else if (object && typeof object === 'object') {
    object = callback(object)
    for (const key in object) {
      const descriptor = Object.getOwnPropertyDescriptor(object, key)
      if (descriptor && descriptor.writable) {
        object[key] = map(object[key], callback)
      }
    }
  }

  if (object && typeof object === 'object') {
    return callback(object)
  } else {
    return object
  }
}
