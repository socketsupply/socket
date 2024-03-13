/**
 * @module Signal
 */
import { signal as constants } from './os/constants.js'
import { SignalEvent } from './internal/events.js'
import os from './os.js'

/**
 * @typedef {import('./os/constants.js').signal} signal
 */

export { constants }

export const channel = new BroadcastChannel('socket.runtime.signal')

export const SIGHUP = constants.SIGHUP
export const SIGINT = constants.SIGINT
export const SIGQUIT = constants.SIGQUIT
export const SIGILL = constants.SIGILL
export const SIGTRAP = constants.SIGTRAP
export const SIGABRT = constants.SIGABRT
export const SIGIOT = constants.SIGIOT
export const SIGBUS = constants.SIGBUS
export const SIGFPE = constants.SIGFPE
export const SIGKILL = constants.SIGKILL
export const SIGUSR1 = constants.SIGUSR1
export const SIGSEGV = constants.SIGSEGV
export const SIGUSR2 = constants.SIGUSR2
export const SIGPIPE = constants.SIGPIPE
export const SIGALRM = constants.SIGALRM
export const SIGTERM = constants.SIGTERM
export const SIGCHLD = constants.SIGCHLD
export const SIGCONT = constants.SIGCONT
export const SIGSTOP = constants.SIGSTOP
export const SIGTSTP = constants.SIGTSTP
export const SIGTTIN = constants.SIGTTIN
export const SIGTTOU = constants.SIGTTOU
export const SIGURG = constants.SIGURG
export const SIGXCPU = constants.SIGXCPU
export const SIGXFSZ = constants.SIGXFSZ
export const SIGVTALRM = constants.SIGVTALRM
export const SIGPROF = constants.SIGPROF
export const SIGWINCH = constants.SIGWINCH
export const SIGIO = constants.SIGIO
export const SIGINFO = constants.SIGINFO
export const SIGSYS = constants.SIGSYS

export const strings = {
  [SIGHUP]: 'Terminal line hangup',
  [SIGINT]: 'Interrupt program',
  [SIGQUIT]: 'Quit program',
  [SIGILL]: 'Illegal instruction',
  [SIGTRAP]: 'Trace trap',
  [SIGABRT]: 'Abort program',
  [SIGIOT]: 'Abort program',
  [SIGBUS]: 'Bus error',
  [SIGFPE]: 'Floating-point exception',
  [SIGKILL]: 'Kill program',
  [SIGUSR1]: 'User defined signal 1',
  [SIGSEGV]: 'Segmentation violation',
  [SIGUSR2]: 'User defined signal 2',
  [SIGPIPE]: 'Write on a pipe with no reader',
  [SIGALRM]: 'Real-time timer expired',
  [SIGTERM]: 'Software termination signal',
  [SIGCHLD]: 'Child status has changed',
  [SIGCONT]: 'Continue after stop',
  [SIGSTOP]: 'Stop signal',
  [SIGTSTP]: 'Stop signal generated from keyboard',
  [SIGTTIN]: ' Background read attempted from control terminal',
  [SIGTTOU]: 'Background write attempted to control terminal',
  [SIGURG]: 'Urgent condition present on socket',
  [SIGXCPU]: 'Urgent condition present on socket',
  [SIGXFSZ]: 'File size limit exceeded',
  [SIGVTALRM]: 'Virtual time alarm',
  [SIGPROF]: 'Profiling timer alarm',
  [SIGWINCH]: 'Window size change',
  [SIGIO]: 'I/O is possible on a descriptor',
  [SIGINFO]: 'Status request from keyboard',
  [SIGSYS]: 'Non-existent system call invoked'
}

/**
 * Converts an `signal` code to its corresponding string message.
 * @param {import('./os/constants.js').signal} {code}
 * @return {string}
 */
export function toString (code) {
  return strings[code] ?? ''
}

/**
 * Gets the code for a given 'signal' name.
 * @param {string|number} name
 * @return {signal}
 */
export function getCode (name) {
  if (typeof name !== 'string') {
    name = name.toString()
  }

  name = name.toUpperCase()
  for (const key in constants) {
    if (name === key) {
      return constants[key]
    }
  }

  return 0
}

/**
 * Gets the name for a given 'signal' code
 * @return {string}
 * @param {string|number} code
 */
export function getName (code) {
  if (typeof code === 'string') {
    code = getCode(code)
  }

  for (const key in constants) {
    const value = constants[key]
    if (value === code) {
      return key
    }
  }

  return ''
}

/**
 * Gets the message for a 'signal' code.
 * @param {number|string} code
 * @return {string}
 */
export function getMessage (code) {
  if (typeof code === 'string') {
    code = getCode(code)
  }

  return toString(code)
}

/**
 * Add a signal event listener.
 * @param {string|number} signal
 * @param {function(SignalEvent)} callback
 * @param {{ once?: boolean }=} [options]
 */
export function addEventListener (signalName, callback, options = null) {
  const name = getName(signalName)
  globalThis.addEventListener(name, callback, options)
}

/**
 * Remove a signal event listener.
 * @param {string|number} signal
 * @param {function(SignalEvent)} callback
 * @param {{ once?: boolean }=} [options]
 */
export function removeEventListener (signalName, callback, options = null) {
  const name = getName(signalName)
  return globalThis.removeEventListener(name, callback, options)
}

if (!/android|ios/i.test(os.platform())) {
  channel.addEventListener('message', (event) => {
    onSignal(event.data.signal)
  })

  globalThis.addEventListener('signal', (event) => {
    onSignal(event.detail.signal)
    channel.postMessage(event.detail)
  })
}

function onSignal (code) {
  const name = getName(code)
  const message = getMessage(code)
  globalThis.dispatchEvent(new SignalEvent(name, {
    code,
    message
  }))
}

export default {
  addEventListener,
  removeEventListener,
  constants,
  channel,
  strings,
  toString,
  getName,
  getCode,
  getMessage,

  // constants
  SIGHUP,
  SIGINT,
  SIGQUIT,
  SIGILL,
  SIGTRAP,
  SIGABRT,
  SIGIOT,
  SIGBUS,
  SIGFPE,
  SIGKILL,
  SIGUSR1,
  SIGSEGV,
  SIGUSR2,
  SIGPIPE,
  SIGALRM,
  SIGTERM,
  SIGCHLD,
  SIGCONT,
  SIGSTOP,
  SIGTSTP,
  SIGTTIN,
  SIGTTOU,
  SIGURG,
  SIGXCPU,
  SIGXFSZ,
  SIGVTALRM,
  SIGPROF,
  SIGWINCH,
  SIGIO,
  SIGINFO,
  SIGSYS
}
