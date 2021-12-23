#ifndef PRELOAD_HH
#define PRELOAD_HH

constexpr auto gPreload = R"JS(
  const IPC = window._ipc = { nextSeq: 1, streams: {} }

  window._ipc.resolve = async (seq, status, value) => {
    try {
      value = decodeURIComponent(value)
    } catch (err) {
      console.error(`${err.message} (${value})`)
      return
    }

    try {
      value = JSON.parse(value)
    } catch (err) {
      console.error(`${err.message} (${value})`)
      return
    }

    if (!window._ipc[seq]) {
      console.error('inbound IPC message with unknown sequence:', seq, value)
      return
    }

    if (status === 0) {
      await window._ipc[seq].resolve(value)
    } else {
      const err = new Error(JSON.stringify(value))
      await window._ipc[seq].reject(err);
    }
    delete window._ipc[seq];
  }

  window._ipc.send = (name, value) => {
    const seq = window._ipc.nextSeq++

    const promise = new Promise((resolve, reject) => {
      window._ipc[seq] = {
        resolve: resolve,
        reject: reject
      }
    })

    try {
      let rawValue = value
      if (typeof value === 'object') {
        value = JSON.stringify(value)
      }

      value = new URLSearchParams({
        ...rawValue,
        index: window.process.index,
        seq,
        value
      }).toString()

      value = value.replace(/\+/g, '%20')

    } catch (err) {
      console.error(`${err.message} (${value})`)
      return Promise.reject(err.message)
    }

    window.external.invoke(`ipc://${name}?${value}`)
    return promise
  }

  window._ipc.emit = (name, value) => {
    let detail

    try {
      detail = JSON.parse(decodeURIComponent(value))
    } catch (err) {
      console.error(`${err.message} (${value})`)
      return
    }

    if (detail.event && detail.data && (detail.serverId || detail.clientId)) {
      const stream = window._ipc.streams[detail.serverId || detail.clientId]
      if (!stream) {
        console.error('inbound IPC message with unknown serverId/clientId:', detail)
        return
      }

      const value = detail.event === 'data' ? atob(detail.data) : detail.data

      if (details.event === 'data') {
        stream.__write(detail.event, value)
      } else {
        stream.emit(detail.event, value)
      }
    }

    const event = new window.CustomEvent(name, { detail })
    window.dispatchEvent(event)
  }
)JS";

constexpr auto gPreloadDesktop = R"JS(
  window.system.send = o => window._ipc.send('send', o)
  window.system.exit = o => window._ipc.send('exit', o)
  window.system.openExternal = o => window._ipc.send('external', o)
  window.system.setTitle = o => window._ipc.send('title', o)
  window.system.hide = o => window._ipc.send('hide', o)

  window.system.dialog = async o => {
    const files = await window._ipc.send('dialog', o);
    return files.split('\n');
  }

  window.system.setContextMenu = o => {
    o = Object
      .entries(o)
      .flatMap(a => a.join(':'))
      .join('_')
    return window._ipc.send('context', o)
  };
)JS";

constexpr auto gPreloadMobile = R"JS(
  () => {
    window.system.tcp = {}

    class Server extends EventEmitter {
      constructor (...args) {
        super()

        Object.assign(this, args)
      }

      onconnection () {
        self.emit('connection', data)
      }

      listen () {
        const connect = async () => {
          const { err, data } = await window._ipc.send('tcpCreateServer', o)

          if (err && cb) return cb(err)
          if (err) return server.emit('error', err)

          this.port = data.port
          this.address = data.address
          this.family = data.family

          window._ipc.streams[data.serverId] = this

          if (cb) return cb(null, data)
          server.emit('listening', data)
        }

        connect()

        return this
      }

      async close (cb) {
        const { err, data } = await window._ipc.send('tcpClose', data)
        delete window._ipc.streams[data.serverId]

        if (err && cb) return cb(err)
        if (err) this.emit('error', err)
      }

      address () {
        return {
          port: this.port,
          family: this.family,
          address: this.address
        }
      }

      getConnections () {

      }

      unref () {
        return this
      }
    }

    class Socket extends Duplex {
      constructor (...args) {
        Object.assign(this, args)

        this._server = null

        this.port = null
        this.family = null
        this.address = null

        this.on('end', () => {
          if (!this.allowHalfOpen) this.write = _writeAfterFIN;
        })
      }

      setNoDelay () {
        const params = {
          clientId: this.clientId
        }

        const { err } = await window._ipc.send('tcpSetNoDelay', params)

        if (err) {
          throw new Error(err)
        }
      }

      setKeepAlive () {
        const params = {
          clientId: this.clientId
        }

        const { err } = await window._ipc.send('tcpSetKeepAlive', params)

        if (err) {
          throw new Error(err)
        }
      }

      setTimeout () {
        const params = {
          clientId: this.clientId
        }

        const { err } = await window._ipc.send('tcpSetTimeout', params)

        if (err) {
          throw new Error(err)
        }
      }

      address () {
        return {
          port: this.port,
          family: this.family,
          address: this.address
        }
      }

      _writeAfterFIN (chunk, encoding, cb) {
        if (!this.writableEnded) {
          return Duplex.prototype.write.call(this, chunk, encoding, cb)
        }

        if (typeof encoding === 'function') {
          cb = encoding
          encoding = null
        }

        // eslint-disable-next-line no-restricted-syntax
        const err = new Error('Socket has been ended by the other party')
        err.code = 'EPIPE';

        if (typeof cb === 'function') {
          cb(err)
        }

        this.destroy(er)

        return false
      }

      async _final (cb) {
        if (this.pending) {
          return this.once('connect', () => this._final(cb))
        }

        const { err, data } = await window._ipc.send('tcpClose', data)

        if (err && cb) return cb(err)
        if (cb) return cb(null, data)

        return data
      }

      async _destroy (exception, cb) {
        if (this.destroyed) return

        cb(exception)

        if (this._server) {
          this._server._connections--

          if (this._server._emitCloseIfDrained) {
            this._server._emitCloseIfDrained()
          }
        }
      }

      destroySoon () {
        if (this.writable) this.end()

        if (this.writableFinished) {
          this.destroy()
        } else {
          this.once('finish', this.destroy)
        }
      }

      async _writev (chunks, cb) {
        const allBuffers = data.allBuffers

        let chunks;
        if (allBuffers) {
          chunks = data
          for (let i = 0; i < data.length; i++) {
            data[i] = data[i].chunk
          }
        } else {
          chunks = new Array(data.length << 1)

          for (let i = 0; i < data.length; i++) {
            const entry = data[i]
            chunks[i * 2] = entry.chunk
            chunks[i * 2 + 1] = entry.encoding
          }
        }

        const requests = []

        for (const chunk of chunks) {
          const params = {
            clientId: this.clientId,
            data: chunk
          }

          requests.push(window._ipc.send('tcpSend', params))
        }

        await Promise.all(requests)
        return cb()
      }

      async _write (data, encoding, cb) {
        const params = {
          clientId: this.clientId,
          encoding,
          data
        }

        const { err } = await window._ipc.send('tcpSend', params)

        if (err) {
          this.destroy(err)
          return cb(err)
        }

        cb()
      }

      __write (data) {
        if (data.length && !stream.destroyed) {
          this.push(data)
        } else {
          stream.push(null)
          stream.read(0)
        }
      }

      _read (n) {
        const params = {
          bytes: n,
          clientId: this.clientId
        }

        const { err } = await window._ipc.send('tcpRead', params)

        if (err) {
          socket.destroy()
        }
      }

      connect (port, address, cb) {
        if (typeof address === 'function') {
          cb = address
          address = null
        }

        const { err, data } = await window._ipc.send('tcpConnect', { port, address })

        if (err && cb) return cb(err)
        if (err) return this.emit('error', err)

        this.address = data.address
        this.port = data.port
        this.clientId = data.clientId

        window._ipc.streams[data.serverId] = this

        if (cb) cb(null, data)
      }

      end (data, encoding, cb) {
        Duplex.prototype.end.call(this, data)

        const params = {
          clientId: this.clientId,
          encoding
        }

        const { err, data } = await window._ipc.send('tcpClose', params)
        delete window._ipc.streams[data.clientId]

        if (err && cb) return cb(err)
        if (err) this.emit('error', err)
        this.emit('closed', !!err)
        if (cb) return cb(null, data)
      }

      unref () {
        return this // for compatibility with the net module
      }
    }

    window.system.tcp.Socket = Socket
    window.system.tcp.Server = Server

    window.system.tcp.connect = (value, cb) => {
      let options = {}
      if (typeof value === 'number') {
        options.port = value
      } else if (typeof value === 'object') {
        options = value
      } else {
        throw new Error('Invalid argument')
      }

      const socket = new Socket(options)

      if (options.timeout) {
        socket.setTimeout(options.timeout);
      }

      return socket.connect(...args)
    }

    window.system.tcp.createServer = (...args) => {
      return new Server(...args)
    }

    window.system.utp = {}
    window.system.getNetworkInterfaces = o => window._ipc.send('getNetworkInterfaces', o)
    window.system.openExternal = o => window._ipc.send('external', o)
  })();
)JS";

constexpr auto gEventEmitter = R"JS(
;(() => {
  // Copyright Joyent, Inc. and other Node contributors.
  //
  // Permission is hereby granted, free of charge, to any person obtaining a
  // copy of this software and associated documentation files (the
  // "Software"), to deal in the Software without restriction, including
  // without limitation the rights to use, copy, modify, merge, publish,
  // distribute, sublicense, and/or sell copies of the Software, and to permit
  // persons to whom the Software is furnished to do so, subject to the
  // following conditions:
  //
  // The above copyright notice and this permission notice shall be included
  // in all copies or substantial portions of the Software.
  //
  // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  // OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  // MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
  // NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  // DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
  // OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
  // USE OR OTHER DEALINGS IN THE SOFTWARE.

  'use strict';

  var R = typeof Reflect === 'object' ? Reflect : null
  var ReflectApply = R && typeof R.apply === 'function'
    ? R.apply
    : function ReflectApply(target, receiver, args) {
      return Function.prototype.apply.call(target, receiver, args);
    }

  var ReflectOwnKeys
  if (R && typeof R.ownKeys === 'function') {
    ReflectOwnKeys = R.ownKeys
  } else if (Object.getOwnPropertySymbols) {
    ReflectOwnKeys = function ReflectOwnKeys(target) {
      return Object.getOwnPropertyNames(target)
        .concat(Object.getOwnPropertySymbols(target));
    };
  } else {
    ReflectOwnKeys = function ReflectOwnKeys(target) {
      return Object.getOwnPropertyNames(target);
    };
  }

  function ProcessEmitWarning(warning) {
    if (console && console.warn) console.warn(warning);
  }

  var NumberIsNaN = Number.isNaN || function NumberIsNaN(value) {
    return value !== value;
  }

  function EventEmitter() {
    EventEmitter.init.call(this);
  }

  Object.assign(window, {
    EventEmitter,
    once
  });

  // Backwards-compat with node 0.10.x
  EventEmitter.EventEmitter = EventEmitter;

  EventEmitter.prototype._events = undefined;
  EventEmitter.prototype._eventsCount = 0;
  EventEmitter.prototype._maxListeners = undefined;

  // By default EventEmitters will print a warning if more than 10 listeners are
  // added to it. This is a useful default which helps finding memory leaks.
  var defaultMaxListeners = 10;

  function checkListener(listener) {
    if (typeof listener !== 'function') {
      throw new TypeError('The "listener" argument must be of type Function. Received type ' + typeof listener);
    }
  }

  Object.defineProperty(EventEmitter, 'defaultMaxListeners', {
    enumerable: true,
    get: function() {
      return defaultMaxListeners;
    },
    set: function(arg) {
      if (typeof arg !== 'number' || arg < 0 || NumberIsNaN(arg)) {
        throw new RangeError('The value of "defaultMaxListeners" is out of range. It must be a non-negative number. Received ' + arg + '.');
      }
      defaultMaxListeners = arg;
    }
  });

  EventEmitter.init = function() {

    if (this._events === undefined ||
        this._events === Object.getPrototypeOf(this)._events) {
      this._events = Object.create(null);
      this._eventsCount = 0;
    }

    this._maxListeners = this._maxListeners || undefined;
  };

  // Obviously not all Emitters should be limited to 10. This function allows
  // that to be increased. Set to zero for unlimited.
  EventEmitter.prototype.setMaxListeners = function setMaxListeners(n) {
    if (typeof n !== 'number' || n < 0 || NumberIsNaN(n)) {
      throw new RangeError('The value of "n" is out of range. It must be a non-negative number. Received ' + n + '.');
    }
    this._maxListeners = n;
    return this;
  };

  function _getMaxListeners(that) {
    if (that._maxListeners === undefined)
      return EventEmitter.defaultMaxListeners;
    return that._maxListeners;
  }

  EventEmitter.prototype.getMaxListeners = function getMaxListeners() {
    return _getMaxListeners(this);
  };

  EventEmitter.prototype.emit = function emit(type) {
    var args = [];
    for (var i = 1; i < arguments.length; i++) args.push(arguments[i]);
    var doError = (type === 'error');

    var events = this._events;
    if (events !== undefined)
      doError = (doError && events.error === undefined);
    else if (!doError)
      return false;

    // If there is no 'error' event listener then throw.
    if (doError) {
      var er;
      if (args.length > 0)
        er = args[0];
      if (er instanceof Error) {
        // Note: The comments on the `throw` lines are intentional, they show
        // up in Node's output if this results in an unhandled exception.
        throw er; // Unhandled 'error' event
      }
      // At least give some kind of context to the user
      var err = new Error('Unhandled error.' + (er ? ' (' + er.message + ')' : ''));
      err.context = er;
      throw err; // Unhandled 'error' event
    }

    var handler = events[type];

    if (handler === undefined)
      return false;

    if (typeof handler === 'function') {
      ReflectApply(handler, this, args);
    } else {
      var len = handler.length;
      var listeners = arrayClone(handler, len);
      for (var i = 0; i < len; ++i)
        ReflectApply(listeners[i], this, args);
    }

    return true;
  };

  function _addListener(target, type, listener, prepend) {
    var m;
    var events;
    var existing;

    checkListener(listener);

    events = target._events;
    if (events === undefined) {
      events = target._events = Object.create(null);
      target._eventsCount = 0;
    } else {
      // To avoid recursion in the case that type === "newListener"! Before
      // adding it to the listeners, first emit "newListener".
      if (events.newListener !== undefined) {
        target.emit('newListener', type,
                    listener.listener ? listener.listener : listener);

        // Re-assign `events` because a newListener handler could have caused the
        // this._events to be assigned to a new object
        events = target._events;
      }
      existing = events[type];
    }

    if (existing === undefined) {
      // Optimize the case of one listener. Don't need the extra array object.
      existing = events[type] = listener;
      ++target._eventsCount;
    } else {
      if (typeof existing === 'function') {
        // Adding the second element, need to change to array.
        existing = events[type] =
          prepend ? [listener, existing] : [existing, listener];
        // If we've already got an array, just append.
      } else if (prepend) {
        existing.unshift(listener);
      } else {
        existing.push(listener);
      }

      // Check for listener leak
      m = _getMaxListeners(target);
      if (m > 0 && existing.length > m && !existing.warned) {
        existing.warned = true;
        // No error code for this since it is a Warning
        // eslint-disable-next-line no-restricted-syntax
        var w = new Error('Possible EventEmitter memory leak detected. ' +
                            existing.length + ' ' + String(type) + ' listeners ' +
                            'added. Use emitter.setMaxListeners() to ' +
                            'increase limit');
        w.name = 'MaxListenersExceededWarning';
        w.emitter = target;
        w.type = type;
        w.count = existing.length;
        ProcessEmitWarning(w);
      }
    }

    return target;
  }

  EventEmitter.prototype.addListener = function addListener(type, listener) {
    return _addListener(this, type, listener, false);
  };

  EventEmitter.prototype.on = EventEmitter.prototype.addListener;

  EventEmitter.prototype.prependListener =
      function prependListener(type, listener) {
        return _addListener(this, type, listener, true);
      };

  function onceWrapper() {
    if (!this.fired) {
      this.target.removeListener(this.type, this.wrapFn);
      this.fired = true;
      if (arguments.length === 0)
        return this.listener.call(this.target);
      return this.listener.apply(this.target, arguments);
    }
  }

  function _onceWrap(target, type, listener) {
    var state = { fired: false, wrapFn: undefined, target: target, type: type, listener: listener };
    var wrapped = onceWrapper.bind(state);
    wrapped.listener = listener;
    state.wrapFn = wrapped;
    return wrapped;
  }

  EventEmitter.prototype.once = function once(type, listener) {
    checkListener(listener);
    this.on(type, _onceWrap(this, type, listener));
    return this;
  };

  EventEmitter.prototype.prependOnceListener =
      function prependOnceListener(type, listener) {
        checkListener(listener);
        this.prependListener(type, _onceWrap(this, type, listener));
        return this;
      };

  // Emits a 'removeListener' event if and only if the listener was removed.
  EventEmitter.prototype.removeListener =
      function removeListener(type, listener) {
        var list, events, position, i, originalListener;

        checkListener(listener);

        events = this._events;
        if (events === undefined)
          return this;

        list = events[type];
        if (list === undefined)
          return this;

        if (list === listener || list.listener === listener) {
          if (--this._eventsCount === 0)
            this._events = Object.create(null);
          else {
            delete events[type];
            if (events.removeListener)
              this.emit('removeListener', type, list.listener || listener);
          }
        } else if (typeof list !== 'function') {
          position = -1;

          for (i = list.length - 1; i >= 0; i--) {
            if (list[i] === listener || list[i].listener === listener) {
              originalListener = list[i].listener;
              position = i;
              break;
            }
          }

          if (position < 0)
            return this;

          if (position === 0)
            list.shift();
          else {
            spliceOne(list, position);
          }

          if (list.length === 1)
            events[type] = list[0];

          if (events.removeListener !== undefined)
            this.emit('removeListener', type, originalListener || listener);
        }

        return this;
      };

  EventEmitter.prototype.off = EventEmitter.prototype.removeListener;

  EventEmitter.prototype.removeAllListeners =
      function removeAllListeners(type) {
        var listeners, events, i;

        events = this._events;
        if (events === undefined)
          return this;

        // not listening for removeListener, no need to emit
        if (events.removeListener === undefined) {
          if (arguments.length === 0) {
            this._events = Object.create(null);
            this._eventsCount = 0;
          } else if (events[type] !== undefined) {
            if (--this._eventsCount === 0)
              this._events = Object.create(null);
            else
              delete events[type];
          }
          return this;
        }

        // emit removeListener for all listeners on all events
        if (arguments.length === 0) {
          var keys = Object.keys(events);
          var key;
          for (i = 0; i < keys.length; ++i) {
            key = keys[i];
            if (key === 'removeListener') continue;
            this.removeAllListeners(key);
          }
          this.removeAllListeners('removeListener');
          this._events = Object.create(null);
          this._eventsCount = 0;
          return this;
        }

        listeners = events[type];

        if (typeof listeners === 'function') {
          this.removeListener(type, listeners);
        } else if (listeners !== undefined) {
          // LIFO order
          for (i = listeners.length - 1; i >= 0; i--) {
            this.removeListener(type, listeners[i]);
          }
        }

        return this;
      };

  function _listeners(target, type, unwrap) {
    var events = target._events;

    if (events === undefined)
      return [];

    var evlistener = events[type];
    if (evlistener === undefined)
      return [];

    if (typeof evlistener === 'function')
      return unwrap ? [evlistener.listener || evlistener] : [evlistener];

    return unwrap ?
      unwrapListeners(evlistener) : arrayClone(evlistener, evlistener.length);
  }

  EventEmitter.prototype.listeners = function listeners(type) {
    return _listeners(this, type, true);
  };

  EventEmitter.prototype.rawListeners = function rawListeners(type) {
    return _listeners(this, type, false);
  };

  EventEmitter.listenerCount = function(emitter, type) {
    if (typeof emitter.listenerCount === 'function') {
      return emitter.listenerCount(type);
    } else {
      return listenerCount.call(emitter, type);
    }
  };

  EventEmitter.prototype.listenerCount = listenerCount;
  function listenerCount(type) {
    var events = this._events;

    if (events !== undefined) {
      var evlistener = events[type];

      if (typeof evlistener === 'function') {
        return 1;
      } else if (evlistener !== undefined) {
        return evlistener.length;
      }
    }

    return 0;
  }

  EventEmitter.prototype.eventNames = function eventNames() {
    return this._eventsCount > 0 ? ReflectOwnKeys(this._events) : [];
  };

  function arrayClone(arr, n) {
    var copy = new Array(n);
    for (var i = 0; i < n; ++i)
      copy[i] = arr[i];
    return copy;
  }

  function spliceOne(list, index) {
    for (; index + 1 < list.length; index++)
      list[index] = list[index + 1];
    list.pop();
  }

  function unwrapListeners(arr) {
    var ret = new Array(arr.length);
    for (var i = 0; i < ret.length; ++i) {
      ret[i] = arr[i].listener || arr[i];
    }
    return ret;
  }

  function once(emitter, name) {
    return new Promise(function (resolve, reject) {
      function errorListener(err) {
        emitter.removeListener(name, resolver);
        reject(err);
      }

      function resolver() {
        if (typeof emitter.removeListener === 'function') {
          emitter.removeListener('error', errorListener);
        }
        resolve([].slice.call(arguments));
      };

      eventTargetAgnosticAddListener(emitter, name, resolver, { once: true });
      if (name !== 'error') {
        addErrorHandlerIfEventEmitter(emitter, errorListener, { once: true });
      }
    });
  }

  function addErrorHandlerIfEventEmitter(emitter, handler, flags) {
    if (typeof emitter.on === 'function') {
      eventTargetAgnosticAddListener(emitter, 'error', handler, flags);
    }
  }

  function eventTargetAgnosticAddListener(emitter, name, listener, flags) {
    if (typeof emitter.on === 'function') {
      if (flags.once) {
        emitter.once(name, listener);
      } else {
        emitter.on(name, listener);
      }
    } else if (typeof emitter.addEventListener === 'function') {
      // EventTarget does not have `error` event semantics like Node
      // EventEmitters, we do not listen for `error` events here.
      emitter.addEventListener(name, function wrapListener(arg) {
        // IE does not have builtin `{ once: true }` support so we
        // have to do it manually.
        if (flags.once) {
          emitter.removeEventListener(name, wrapListener);
        }
        listener(arg);
      });
    } else {
      throw new TypeError('The "emitter" argument must be of type EventEmitter. Received type ' + typeof emitter);
    }
  }
})();
)JS";

constexpr auto gStreams = R"JS(
;(() => {
  const STREAM_DESTROYED = new Error('Stream was destroyed')
  const PREMATURE_CLOSE = new Error('Premature close')

  class FixedFIFO {
    constructor (hwm) {
      if (!(hwm > 0) || ((hwm - 1) & hwm) !== 0) throw new Error('Max size for a FixedFIFO should be a power of two')
      this.buffer = new Array(hwm)
      this.mask = hwm - 1
      this.top = 0
      this.btm = 0
      this.next = null
    }

    push (data) {
      if (this.buffer[this.top] !== undefined) return false
      this.buffer[this.top] = data
      this.top = (this.top + 1) & this.mask
      return true
    }

    shift () {
      const last = this.buffer[this.btm]
      if (last === undefined) return undefined
      this.buffer[this.btm] = undefined
      this.btm = (this.btm + 1) & this.mask
      return last
    }

    isEmpty () {
      return this.buffer[this.btm] === undefined
    }
  }

  class FastFIFO {
    constructor (hwm) {
      this.hwm = hwm || 16
      this.head = new FixedFIFO(this.hwm)
      this.tail = this.head
    }

    push (val) {
      if (!this.head.push(val)) {
        const prev = this.head
        this.head = prev.next = new FixedFIFO(2 * this.head.buffer.length)
        this.head.push(val)
      }
    }

    shift () {
      const val = this.tail.shift()
      if (val === undefined && this.tail.next) {
        const next = this.tail.next
        this.tail.next = null
        this.tail = next
        return this.tail.shift()
      }
      return val
    }

    isEmpty () {
      return this.head.isEmpty()
    }
  }

  /* eslint-disable no-multi-spaces */

  const MAX = ((1 << 25) - 1)

  // Shared state
  const OPENING     = 0b001
  const DESTROYING  = 0b010
  const DESTROYED   = 0b100

  const NOT_OPENING = MAX ^ OPENING

  // Read state
  const READ_ACTIVE           = 0b0000000000001 << 3
  const READ_PRIMARY          = 0b0000000000010 << 3
  const READ_SYNC             = 0b0000000000100 << 3
  const READ_QUEUED           = 0b0000000001000 << 3
  const READ_RESUMED          = 0b0000000010000 << 3
  const READ_PIPE_DRAINED     = 0b0000000100000 << 3
  const READ_ENDING           = 0b0000001000000 << 3
  const READ_EMIT_DATA        = 0b0000010000000 << 3
  const READ_EMIT_READABLE    = 0b0000100000000 << 3
  const READ_EMITTED_READABLE = 0b0001000000000 << 3
  const READ_DONE             = 0b0010000000000 << 3
  const READ_NEXT_TICK        = 0b0100000000001 << 3 // also active
  const READ_NEEDS_PUSH       = 0b1000000000000 << 3

  const READ_NOT_ACTIVE             = MAX ^ READ_ACTIVE
  const READ_NON_PRIMARY            = MAX ^ READ_PRIMARY
  const READ_NON_PRIMARY_AND_PUSHED = MAX ^ (READ_PRIMARY | READ_NEEDS_PUSH)
  const READ_NOT_SYNC               = MAX ^ READ_SYNC
  const READ_PUSHED                 = MAX ^ READ_NEEDS_PUSH
  const READ_PAUSED                 = MAX ^ READ_RESUMED
  const READ_NOT_QUEUED             = MAX ^ (READ_QUEUED | READ_EMITTED_READABLE)
  const READ_NOT_ENDING             = MAX ^ READ_ENDING
  const READ_PIPE_NOT_DRAINED       = MAX ^ (READ_RESUMED | READ_PIPE_DRAINED)
  const READ_NOT_NEXT_TICK          = MAX ^ READ_NEXT_TICK

  // Write state
  const WRITE_ACTIVE     = 0b000000001 << 16
  const WRITE_PRIMARY    = 0b000000010 << 16
  const WRITE_SYNC       = 0b000000100 << 16
  const WRITE_QUEUED     = 0b000001000 << 16
  const WRITE_UNDRAINED  = 0b000010000 << 16
  const WRITE_DONE       = 0b000100000 << 16
  const WRITE_EMIT_DRAIN = 0b001000000 << 16
  const WRITE_NEXT_TICK  = 0b010000001 << 16 // also active
  const WRITE_FINISHING  = 0b100000000 << 16

  const WRITE_NOT_ACTIVE    = MAX ^ WRITE_ACTIVE
  const WRITE_NOT_SYNC      = MAX ^ WRITE_SYNC
  const WRITE_NON_PRIMARY   = MAX ^ WRITE_PRIMARY
  const WRITE_NOT_FINISHING = MAX ^ WRITE_FINISHING
  const WRITE_DRAINED       = MAX ^ WRITE_UNDRAINED
  const WRITE_NOT_QUEUED    = MAX ^ WRITE_QUEUED
  const WRITE_NOT_NEXT_TICK = MAX ^ WRITE_NEXT_TICK

  // Combined shared state
  const ACTIVE = READ_ACTIVE | WRITE_ACTIVE
  const NOT_ACTIVE = MAX ^ ACTIVE
  const DONE = READ_DONE | WRITE_DONE
  const DESTROY_STATUS = DESTROYING | DESTROYED
  const OPEN_STATUS = DESTROY_STATUS | OPENING
  const AUTO_DESTROY = DESTROY_STATUS | DONE
  const NON_PRIMARY = WRITE_NON_PRIMARY & READ_NON_PRIMARY
  const TICKING = (WRITE_NEXT_TICK | READ_NEXT_TICK) & NOT_ACTIVE
  const ACTIVE_OR_TICKING = ACTIVE | TICKING
  const IS_OPENING = OPEN_STATUS | TICKING

  // Combined read state
  const READ_PRIMARY_STATUS = OPEN_STATUS | READ_ENDING | READ_DONE
  const READ_STATUS = OPEN_STATUS | READ_DONE | READ_QUEUED
  const READ_FLOWING = READ_RESUMED | READ_PIPE_DRAINED
  const READ_ACTIVE_AND_SYNC = READ_ACTIVE | READ_SYNC
  const READ_ACTIVE_AND_SYNC_AND_NEEDS_PUSH = READ_ACTIVE | READ_SYNC | READ_NEEDS_PUSH
  const READ_PRIMARY_AND_ACTIVE = READ_PRIMARY | READ_ACTIVE
  const READ_ENDING_STATUS = OPEN_STATUS | READ_ENDING | READ_QUEUED
  const READ_EMIT_READABLE_AND_QUEUED = READ_EMIT_READABLE | READ_QUEUED
  const READ_READABLE_STATUS = OPEN_STATUS | READ_EMIT_READABLE | READ_QUEUED | READ_EMITTED_READABLE
  const SHOULD_NOT_READ = OPEN_STATUS | READ_ACTIVE | READ_ENDING | READ_DONE | READ_NEEDS_PUSH
  const READ_BACKPRESSURE_STATUS = DESTROY_STATUS | READ_ENDING | READ_DONE

  // Combined write state
  const WRITE_PRIMARY_STATUS = OPEN_STATUS | WRITE_FINISHING | WRITE_DONE
  const WRITE_QUEUED_AND_UNDRAINED = WRITE_QUEUED | WRITE_UNDRAINED
  const WRITE_QUEUED_AND_ACTIVE = WRITE_QUEUED | WRITE_ACTIVE
  const WRITE_DRAIN_STATUS = WRITE_QUEUED | WRITE_UNDRAINED | OPEN_STATUS | WRITE_ACTIVE
  const WRITE_STATUS = OPEN_STATUS | WRITE_ACTIVE | WRITE_QUEUED
  const WRITE_PRIMARY_AND_ACTIVE = WRITE_PRIMARY | WRITE_ACTIVE
  const WRITE_ACTIVE_AND_SYNC = WRITE_ACTIVE | WRITE_SYNC
  const WRITE_FINISHING_STATUS = OPEN_STATUS | WRITE_FINISHING | WRITE_QUEUED
  const WRITE_BACKPRESSURE_STATUS = WRITE_UNDRAINED | DESTROY_STATUS | WRITE_FINISHING | WRITE_DONE

  const asyncIterator = Symbol.asyncIterator || Symbol('asyncIterator')

  class WritableState {
    constructor (stream, { highWaterMark = 16384, map = null, mapWritable, byteLength, byteLengthWritable } = {}) {
      this.stream = stream
      this.queue = new FIFO()
      this.highWaterMark = highWaterMark
      this.buffered = 0
      this.error = null
      this.pipeline = null
      this.byteLength = byteLengthWritable || byteLength || defaultByteLength
      this.map = mapWritable || map
      this.afterWrite = afterWrite.bind(this)
      this.afterUpdateNextTick = updateWriteNT.bind(this)
    }

    get ended () {
      return (this.stream._duplexState & WRITE_DONE) !== 0
    }

    push (data) {
      if (this.map !== null) data = this.map(data)

      this.buffered += this.byteLength(data)
      this.queue.push(data)

      if (this.buffered < this.highWaterMark) {
        this.stream._duplexState |= WRITE_QUEUED
        return true
      }

      this.stream._duplexState |= WRITE_QUEUED_AND_UNDRAINED
      return false
    }

    shift () {
      const data = this.queue.shift()
      const stream = this.stream

      this.buffered -= this.byteLength(data)
      if (this.buffered === 0) stream._duplexState &= WRITE_NOT_QUEUED

      return data
    }

    end (data) {
      if (typeof data === 'function') this.stream.once('finish', data)
      else if (data !== undefined && data !== null) this.push(data)
      this.stream._duplexState = (this.stream._duplexState | WRITE_FINISHING) & WRITE_NON_PRIMARY
    }

    autoBatch (data, cb) {
      const buffer = []
      const stream = this.stream

      buffer.push(data)
      while ((stream._duplexState & WRITE_STATUS) === WRITE_QUEUED_AND_ACTIVE) {
        buffer.push(stream._writableState.shift())
      }

      if ((stream._duplexState & OPEN_STATUS) !== 0) return cb(null)
      stream._writev(buffer, cb)
    }

    update () {
      const stream = this.stream

      while ((stream._duplexState & WRITE_STATUS) === WRITE_QUEUED) {
        const data = this.shift()
        stream._duplexState |= WRITE_ACTIVE_AND_SYNC
        stream._write(data, this.afterWrite)
        stream._duplexState &= WRITE_NOT_SYNC
      }

      if ((stream._duplexState & WRITE_PRIMARY_AND_ACTIVE) === 0) this.updateNonPrimary()
    }

    updateNonPrimary () {
      const stream = this.stream

      if ((stream._duplexState & WRITE_FINISHING_STATUS) === WRITE_FINISHING) {
        stream._duplexState = (stream._duplexState | WRITE_ACTIVE) & WRITE_NOT_FINISHING
        stream._final(afterFinal.bind(this))
        return
      }

      if ((stream._duplexState & DESTROY_STATUS) === DESTROYING) {
        if ((stream._duplexState & ACTIVE_OR_TICKING) === 0) {
          stream._duplexState |= ACTIVE
          stream._destroy(afterDestroy.bind(this))
        }
        return
      }

      if ((stream._duplexState & IS_OPENING) === OPENING) {
        stream._duplexState = (stream._duplexState | ACTIVE) & NOT_OPENING
        stream._open(afterOpen.bind(this))
      }
    }

    updateNextTick () {
      if ((this.stream._duplexState & WRITE_NEXT_TICK) !== 0) return
      this.stream._duplexState |= WRITE_NEXT_TICK
      queueMicrotask(this.afterUpdateNextTick)
    }
  }

  class ReadableState {
    constructor (stream, { highWaterMark = 16384, map = null, mapReadable, byteLength, byteLengthReadable } = {}) {
      this.stream = stream
      this.queue = new FIFO()
      this.highWaterMark = highWaterMark
      this.buffered = 0
      this.error = null
      this.pipeline = null
      this.byteLength = byteLengthReadable || byteLength || defaultByteLength
      this.map = mapReadable || map
      this.pipeTo = null
      this.afterRead = afterRead.bind(this)
      this.afterUpdateNextTick = updateReadNT.bind(this)
    }

    get ended () {
      return (this.stream._duplexState & READ_DONE) !== 0
    }

    pipe (pipeTo, cb) {
      if (this.pipeTo !== null) throw new Error('Can only pipe to one destination')

      this.stream._duplexState |= READ_PIPE_DRAINED
      this.pipeTo = pipeTo
      this.pipeline = new Pipeline(this.stream, pipeTo, cb || null)

      if (cb) this.stream.on('error', noop) // We already error handle this so supress crashes

      if (isStreamx(pipeTo)) {
        pipeTo._writableState.pipeline = this.pipeline
        if (cb) pipeTo.on('error', noop) // We already error handle this so supress crashes
        pipeTo.on('finish', this.pipeline.finished.bind(this.pipeline)) // TODO: just call finished from pipeTo itself
      } else {
        const onerror = this.pipeline.done.bind(this.pipeline, pipeTo)
        const onclose = this.pipeline.done.bind(this.pipeline, pipeTo, null) // onclose has a weird bool arg
        pipeTo.on('error', onerror)
        pipeTo.on('close', onclose)
        pipeTo.on('finish', this.pipeline.finished.bind(this.pipeline))
      }

      pipeTo.on('drain', afterDrain.bind(this))
      this.stream.emit('piping', pipeTo)
      pipeTo.emit('pipe', this.stream)
    }

    push (data) {
      const stream = this.stream

      if (data === null) {
        this.highWaterMark = 0
        stream._duplexState = (stream._duplexState | READ_ENDING) & READ_NON_PRIMARY_AND_PUSHED
        return false
      }

      if (this.map !== null) data = this.map(data)
      this.buffered += this.byteLength(data)
      this.queue.push(data)

      stream._duplexState = (stream._duplexState | READ_QUEUED) & READ_PUSHED

      return this.buffered < this.highWaterMark
    }

    shift () {
      const data = this.queue.shift()

      this.buffered -= this.byteLength(data)
      if (this.buffered === 0) this.stream._duplexState &= READ_NOT_QUEUED
      return data
    }

    unshift (data) {
      let tail
      const pending = []

      while ((tail = this.queue.shift()) !== undefined) {
        pending.push(tail)
      }

      this.push(data)

      for (let i = 0; i < pending.length; i++) {
        this.queue.push(pending[i])
      }
    }

    read () {
      const stream = this.stream

      if ((stream._duplexState & READ_STATUS) === READ_QUEUED) {
        const data = this.shift()
        if (this.pipeTo !== null && this.pipeTo.write(data) === false) stream._duplexState &= READ_PIPE_NOT_DRAINED
        if ((stream._duplexState & READ_EMIT_DATA) !== 0) stream.emit('data', data)
        return data
      }

      return null
    }

    drain () {
      const stream = this.stream

      while ((stream._duplexState & READ_STATUS) === READ_QUEUED && (stream._duplexState & READ_FLOWING) !== 0) {
        const data = this.shift()
        if (this.pipeTo !== null && this.pipeTo.write(data) === false) stream._duplexState &= READ_PIPE_NOT_DRAINED
        if ((stream._duplexState & READ_EMIT_DATA) !== 0) stream.emit('data', data)
      }
    }

    update () {
      const stream = this.stream

      this.drain()

      while (this.buffered < this.highWaterMark && (stream._duplexState & SHOULD_NOT_READ) === 0) {
        stream._duplexState |= READ_ACTIVE_AND_SYNC_AND_NEEDS_PUSH
        stream._read(this.afterRead)
        stream._duplexState &= READ_NOT_SYNC
        if ((stream._duplexState & READ_ACTIVE) === 0) this.drain()
      }

      if ((stream._duplexState & READ_READABLE_STATUS) === READ_EMIT_READABLE_AND_QUEUED) {
        stream._duplexState |= READ_EMITTED_READABLE
        stream.emit('readable')
      }

      if ((stream._duplexState & READ_PRIMARY_AND_ACTIVE) === 0) this.updateNonPrimary()
    }

    updateNonPrimary () {
      const stream = this.stream

      if ((stream._duplexState & READ_ENDING_STATUS) === READ_ENDING) {
        stream._duplexState = (stream._duplexState | READ_DONE) & READ_NOT_ENDING
        stream.emit('end')
        if ((stream._duplexState & AUTO_DESTROY) === DONE) stream._duplexState |= DESTROYING
        if (this.pipeTo !== null) this.pipeTo.end()
      }

      if ((stream._duplexState & DESTROY_STATUS) === DESTROYING) {
        if ((stream._duplexState & ACTIVE_OR_TICKING) === 0) {
          stream._duplexState |= ACTIVE
          stream._destroy(afterDestroy.bind(this))
        }
        return
      }

      if ((stream._duplexState & IS_OPENING) === OPENING) {
        stream._duplexState = (stream._duplexState | ACTIVE) & NOT_OPENING
        stream._open(afterOpen.bind(this))
      }
    }

    updateNextTick () {
      if ((this.stream._duplexState & READ_NEXT_TICK) !== 0) return
      this.stream._duplexState |= READ_NEXT_TICK
      queueMicrotask(this.afterUpdateNextTick)
    }
  }

  class TransformState {
    constructor (stream) {
      this.data = null
      this.afterTransform = afterTransform.bind(stream)
      this.afterFinal = null
    }
  }

  class Pipeline {
    constructor (src, dst, cb) {
      this.from = src
      this.to = dst
      this.afterPipe = cb
      this.error = null
      this.pipeToFinished = false
    }

    finished () {
      this.pipeToFinished = true
    }

    done (stream, err) {
      if (err) this.error = err

      if (stream === this.to) {
        this.to = null

        if (this.from !== null) {
          if ((this.from._duplexState & READ_DONE) === 0 || !this.pipeToFinished) {
            this.from.destroy(this.error || new Error('Writable stream closed prematurely'))
          }
          return
        }
      }

      if (stream === this.from) {
        this.from = null

        if (this.to !== null) {
          if ((stream._duplexState & READ_DONE) === 0) {
            this.to.destroy(this.error || new Error('Readable stream closed before ending'))
          }
          return
        }
      }

      if (this.afterPipe !== null) this.afterPipe(this.error)
      this.to = this.from = this.afterPipe = null
    }
  }

  function afterDrain () {
    this.stream._duplexState |= READ_PIPE_DRAINED
    if ((this.stream._duplexState & READ_ACTIVE_AND_SYNC) === 0) this.updateNextTick()
  }

  function afterFinal (err) {
    const stream = this.stream
    if (err) stream.destroy(err)
    if ((stream._duplexState & DESTROY_STATUS) === 0) {
      stream._duplexState |= WRITE_DONE
      stream.emit('finish')
    }
    if ((stream._duplexState & AUTO_DESTROY) === DONE) {
      stream._duplexState |= DESTROYING
    }

    stream._duplexState &= WRITE_NOT_ACTIVE
    this.update()
  }

  function afterDestroy (err) {
    const stream = this.stream

    if (!err && this.error !== STREAM_DESTROYED) err = this.error
    if (err) stream.emit('error', err)
    stream._duplexState |= DESTROYED
    stream.emit('close')

    const rs = stream._readableState
    const ws = stream._writableState

    if (rs !== null && rs.pipeline !== null) rs.pipeline.done(stream, err)
    if (ws !== null && ws.pipeline !== null) ws.pipeline.done(stream, err)
  }

  function afterWrite (err) {
    const stream = this.stream

    if (err) stream.destroy(err)
    stream._duplexState &= WRITE_NOT_ACTIVE

    if ((stream._duplexState & WRITE_DRAIN_STATUS) === WRITE_UNDRAINED) {
      stream._duplexState &= WRITE_DRAINED
      if ((stream._duplexState & WRITE_EMIT_DRAIN) === WRITE_EMIT_DRAIN) {
        stream.emit('drain')
      }
    }

    if ((stream._duplexState & WRITE_SYNC) === 0) this.update()
  }

  function afterRead (err) {
    if (err) this.stream.destroy(err)
    this.stream._duplexState &= READ_NOT_ACTIVE
    if ((this.stream._duplexState & READ_SYNC) === 0) this.update()
  }

  function updateReadNT () {
    this.stream._duplexState &= READ_NOT_NEXT_TICK
    this.update()
  }

  function updateWriteNT () {
    this.stream._duplexState &= WRITE_NOT_NEXT_TICK
    this.update()
  }

  function afterOpen (err) {
    const stream = this.stream

    if (err) stream.destroy(err)

    if ((stream._duplexState & DESTROYING) === 0) {
      if ((stream._duplexState & READ_PRIMARY_STATUS) === 0) stream._duplexState |= READ_PRIMARY
      if ((stream._duplexState & WRITE_PRIMARY_STATUS) === 0) stream._duplexState |= WRITE_PRIMARY
      stream.emit('open')
    }

    stream._duplexState &= NOT_ACTIVE

    if (stream._writableState !== null) {
      stream._writableState.update()
    }

    if (stream._readableState !== null) {
      stream._readableState.update()
    }
  }

  function afterTransform (err, data) {
    if (data !== undefined && data !== null) this.push(data)
    this._writableState.afterWrite(err)
  }

  class Stream extends EventEmitter {
    constructor (opts) {
      super()

      this._duplexState = 0
      this._readableState = null
      this._writableState = null

      if (opts) {
        if (opts.open) this._open = opts.open
        if (opts.destroy) this._destroy = opts.destroy
        if (opts.predestroy) this._predestroy = opts.predestroy
        if (opts.signal) {
          opts.signal.addEventListener('abort', abort.bind(this))
        }
      }
    }

    _open (cb) {
      cb(null)
    }

    _destroy (cb) {
      cb(null)
    }

    _predestroy () {
      // does nothing
    }

    get readable () {
      return this._readableState !== null ? true : undefined
    }

    get writable () {
      return this._writableState !== null ? true : undefined
    }

    get destroyed () {
      return (this._duplexState & DESTROYED) !== 0
    }

    get destroying () {
      return (this._duplexState & DESTROY_STATUS) !== 0
    }

    destroy (err) {
      if ((this._duplexState & DESTROY_STATUS) === 0) {
        if (!err) err = STREAM_DESTROYED
        this._duplexState = (this._duplexState | DESTROYING) & NON_PRIMARY
        if (this._readableState !== null) {
          this._readableState.error = err
          this._readableState.updateNextTick()
        }
        if (this._writableState !== null) {
          this._writableState.error = err
          this._writableState.updateNextTick()
        }
        this._predestroy()
      }
    }

    on (name, fn) {
      if (this._readableState !== null) {
        if (name === 'data') {
          this._duplexState |= (READ_EMIT_DATA | READ_RESUMED)
          this._readableState.updateNextTick()
        }
        if (name === 'readable') {
          this._duplexState |= READ_EMIT_READABLE
          this._readableState.updateNextTick()
        }
      }

      if (this._writableState !== null) {
        if (name === 'drain') {
          this._duplexState |= WRITE_EMIT_DRAIN
          this._writableState.updateNextTick()
        }
      }

      return super.on(name, fn)
    }
  }

  class Readable extends Stream {
    constructor (opts) {
      super(opts)

      this._duplexState |= OPENING | WRITE_DONE
      this._readableState = new ReadableState(this, opts)

      if (opts) {
        if (opts.read) this._read = opts.read
        if (opts.eagerOpen) this.resume().pause()
      }
    }

    _read (cb) {
      cb(null)
    }

    pipe (dest, cb) {
      this._readableState.pipe(dest, cb)
      this._readableState.updateNextTick()
      return dest
    }

    read () {
      this._readableState.updateNextTick()
      return this._readableState.read()
    }

    push (data) {
      this._readableState.updateNextTick()
      return this._readableState.push(data)
    }

    unshift (data) {
      this._readableState.updateNextTick()
      return this._readableState.unshift(data)
    }

    resume () {
      this._duplexState |= READ_RESUMED
      this._readableState.updateNextTick()
      return this
    }

    pause () {
      this._duplexState &= READ_PAUSED
      return this
    }

    static _fromAsyncIterator (ite, opts) {
      let destroy

      const rs = new Readable({
        ...opts,
        read (cb) {
          ite.next().then(push).then(cb.bind(null, null)).catch(cb)
        },
        predestroy () {
          destroy = ite.return()
        },
        destroy (cb) {
          destroy.then(cb.bind(null, null)).catch(cb)
        }
      })

      return rs

      function push (data) {
        if (data.done) rs.push(null)
        else rs.push(data.value)
      }
    }

    static from (data, opts) {
      if (isReadStreamx(data)) return data
      if (data[asyncIterator]) return this._fromAsyncIterator(data[asyncIterator](), opts)
      if (!Array.isArray(data)) data = data === undefined ? [] : [data]

      let i = 0
      return new Readable({
        ...opts,
        read (cb) {
          this.push(i === data.length ? null : data[i++])
          cb(null)
        }
      })
    }

    static isBackpressured (rs) {
      return (rs._duplexState & READ_BACKPRESSURE_STATUS) !== 0 || rs._readableState.buffered >= rs._readableState.highWaterMark
    }

    static isPaused (rs) {
      return (rs._duplexState & READ_RESUMED) === 0
    }

    [asyncIterator] () {
      const stream = this

      let error = null
      let promiseResolve = null
      let promiseReject = null

      this.on('error', (err) => { error = err })
      this.on('readable', onreadable)
      this.on('close', onclose)

      return {
        [asyncIterator] () {
          return this
        },
        next () {
          return new Promise(function (resolve, reject) {
            promiseResolve = resolve
            promiseReject = reject
            const data = stream.read()
            if (data !== null) ondata(data)
            else if ((stream._duplexState & DESTROYED) !== 0) ondata(null)
          })
        },
        return () {
          return destroy(null)
        },
        throw (err) {
          return destroy(err)
        }
      }

      function onreadable () {
        if (promiseResolve !== null) ondata(stream.read())
      }

      function onclose () {
        if (promiseResolve !== null) ondata(null)
      }

      function ondata (data) {
        if (promiseReject === null) return
        if (error) promiseReject(error)
        else if (data === null && (stream._duplexState & READ_DONE) === 0) promiseReject(STREAM_DESTROYED)
        else promiseResolve({ value: data, done: data === null })
        promiseReject = promiseResolve = null
      }

      function destroy (err) {
        stream.destroy(err)
        return new Promise((resolve, reject) => {
          if (stream._duplexState & DESTROYED) return resolve({ value: undefined, done: true })
          stream.once('close', function () {
            if (err) reject(err)
            else resolve({ value: undefined, done: true })
          })
        })
      }
    }
  }

  class Writable extends Stream {
    constructor (opts) {
      super(opts)

      this._duplexState |= OPENING | READ_DONE
      this._writableState = new WritableState(this, opts)

      if (opts) {
        if (opts.writev) this._writev = opts.writev
        if (opts.write) this._write = opts.write
        if (opts.final) this._final = opts.final
      }
    }

    _writev (batch, cb) {
      cb(null)
    }

    _write (data, cb) {
      this._writableState.autoBatch(data, cb)
    }

    _final (cb) {
      cb(null)
    }

    static isBackpressured (ws) {
      return (ws._duplexState & WRITE_BACKPRESSURE_STATUS) !== 0
    }

    write (data) {
      this._writableState.updateNextTick()
      return this._writableState.push(data)
    }

    end (data) {
      this._writableState.updateNextTick()
      this._writableState.end(data)
      return this
    }
  }

  class Duplex extends Readable { // and Writable
    constructor (opts) {
      super(opts)

      this._duplexState = OPENING
      this._writableState = new WritableState(this, opts)

      if (opts) {
        if (opts.writev) this._writev = opts.writev
        if (opts.write) this._write = opts.write
        if (opts.final) this._final = opts.final
      }
    }

    _writev (batch, cb) {
      cb(null)
    }

    _write (data, cb) {
      this._writableState.autoBatch(data, cb)
    }

    _final (cb) {
      cb(null)
    }

    write (data) {
      this._writableState.updateNextTick()
      return this._writableState.push(data)
    }

    end (data) {
      this._writableState.updateNextTick()
      this._writableState.end(data)
      return this
    }
  }

  class Transform extends Duplex {
    constructor (opts) {
      super(opts)
      this._transformState = new TransformState(this)

      if (opts) {
        if (opts.transform) this._transform = opts.transform
        if (opts.flush) this._flush = opts.flush
      }
    }

    _write (data, cb) {
      if (this._readableState.buffered >= this._readableState.highWaterMark) {
        this._transformState.data = data
      } else {
        this._transform(data, this._transformState.afterTransform)
      }
    }

    _read (cb) {
      if (this._transformState.data !== null) {
        const data = this._transformState.data
        this._transformState.data = null
        cb(null)
        this._transform(data, this._transformState.afterTransform)
      } else {
        cb(null)
      }
    }

    _transform (data, cb) {
      cb(null, data)
    }

    _flush (cb) {
      cb(null)
    }

    _final (cb) {
      this._transformState.afterFinal = cb
      this._flush(transformAfterFlush.bind(this))
    }
  }

  class PassThrough extends Transform {}

  function transformAfterFlush (err, data) {
    const cb = this._transformState.afterFinal
    if (err) return cb(err)
    if (data !== null && data !== undefined) this.push(data)
    this.push(null)
    cb(null)
  }

  function pipelinePromise (...streams) {
    return new Promise((resolve, reject) => {
      return pipeline(...streams, (err) => {
        if (err) return reject(err)
        resolve()
      })
    })
  }

  function pipeline (stream, ...streams) {
    const all = Array.isArray(stream) ? [...stream, ...streams] : [stream, ...streams]
    const done = (all.length && typeof all[all.length - 1] === 'function') ? all.pop() : null

    if (all.length < 2) throw new Error('Pipeline requires at least 2 streams')

    let src = all[0]
    let dest = null
    let error = null

    for (let i = 1; i < all.length; i++) {
      dest = all[i]

      if (isStreamx(src)) {
        src.pipe(dest, onerror)
      } else {
        errorHandle(src, true, i > 1, onerror)
        src.pipe(dest)
      }

      src = dest
    }

    if (done) {
      let fin = false

      dest.on('finish', () => { fin = true })
      dest.on('error', err => { error = error || err })
      dest.on('close', () => done(error || (fin ? null : PREMATURE_CLOSE)))
    }

    return dest

    function errorHandle (s, rd, wr, onerror) {
      s.on('error', onerror)
      s.on('close', onclose)

      function onclose () {
        if (rd && s._readableState && !s._readableState.ended) return onerror(PREMATURE_CLOSE)
        if (wr && s._writableState && !s._writableState.ended) return onerror(PREMATURE_CLOSE)
      }
    }

    function onerror (err) {
      if (!err || error) return
      error = err

      for (const s of all) {
        s.destroy(err)
      }
    }
  }

  function isStream (stream) {
    return !!stream._readableState || !!stream._writableState
  }

  function isStreamx (stream) {
    return typeof stream._duplexState === 'number' && isStream(stream)
  }

  function isReadStreamx (stream) {
    return isStreamx(stream) && stream.readable
  }

  function isTypedArray (data) {
    return typeof data === 'object' && data !== null && typeof data.byteLength === 'number'
  }

  function defaultByteLength (data) {
    return isTypedArray(data) ? data.byteLength : 1024
  }

  function noop () {}

  function abort () {
    this.destroy(new Error('Stream aborted.'))
  }

  Object.assign(window, {
    pipeline,
    pipelinePromise,
    isStream,
    isStreamx,
    Stream,
    Writable,
    Readable,
    Duplex,
    Transform
  })
})();
)JS";

#endif

