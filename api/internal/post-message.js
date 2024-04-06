const platform = {
  BroadcastChannelPostMessage: globalThis.BroadcastChannel.prototype.postMessage,
  MessageChannelPostMessage: globalThis.MessageChannel.prototype.postMessage,
  GlobalPostMessage: globalThis.postMessage
}

globalThis.BroadcastChannel.prototype.postMessage = function (message, ...args) {
  message = handlePostMessage(message)
  return platform.BroadcastChannelPostMessage.call(this, message, ...args)
}

globalThis.MessageChannel.prototype.postMessage = function (message, ...args) {
  return platform.MessageChannelPostMessage.call(this, handlePostMessage(message), ...args)
}

globalThis.postMessage = function (message, ...args) {
  return platform.GlobalPostMessage.call(this, handlePostMessage(message), ...args)
}

function handlePostMessage (message) {
  if (!message || typeof message !== 'object') {
    return message
  }

  return map(message, (value) => {
    if (typeof value[Symbol.serialize] !== 'function') {
      return value
    }

    return value[Symbol.serialize]()
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

export default null
