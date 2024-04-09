import serialize from './serialize.js'

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
  return serialize(message)
}

export default null
