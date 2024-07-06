import serialize from './serialize.js'

const {
  BroadcastChannel,
  MessagePort,
  postMessage
} = globalThis

const platform = {
  BroadcastChannelPostMessage: BroadcastChannel.prototype.postMessage,
  MessagePortPostMessage: MessagePort.prototype.postMessage,
  GlobalPostMessage: postMessage
}

BroadcastChannel.prototype.postMessage = function (message, ...args) {
  return platform.BroadcastChannelPostMessage.call(
    this,
    handlePostMessage(message),
    ...args
  )
}

MessagePort.prototype.postMessage = function (message, ...args) {
  return platform.MessagePortPostMessage.call(
    this,
    handlePostMessage(message),
    ...args
  )
}

globalThis.postMessage = function (message, ...args) {
  return platform.GlobalPostMessage.call(
    this,
    handlePostMessage(message),
    ...args
  )
}

function handlePostMessage (message) {
  return serialize(message)
}

export default null
