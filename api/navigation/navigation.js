// node_modules/@virtualstate/navigation/esnext/global-navigation.js
var globalNavigation = void 0;
if (typeof window !== "undefined" && window.navigation) {
  const navigation3 = window.navigation;
  assertNavigation(navigation3);
  globalNavigation = navigation3;
}
function assertNavigation(value) {
  if (!value) {
    throw new Error("Expected Navigation");
  }
}

// node_modules/@virtualstate/navigation/esnext/event-target/event.js
function isEvent(value) {
  function isLike(value2) {
    return !!value2;
  }
  return isLike(value) && (typeof value.type === "string" || typeof value.type === "symbol");
}
function assertEvent(value, type) {
  if (!isEvent(value)) {
    throw new Error("Expected event");
  }
  if (typeof type !== "undefined" && value.type !== type) {
    throw new Error(`Expected event type ${String(type)}, got ${value.type.toString()}`);
  }
}

// node_modules/@virtualstate/navigation/esnext/event-target/parallel-event.js
function isParallelEvent(value) {
  return isEvent(value) && value.parallel !== false;
}

// node_modules/@virtualstate/navigation/esnext/navigation-errors.js
var AbortError = class extends Error {
  constructor(message) {
    super(`AbortError${message ? `: ${message}` : ""}`);
    this.name = "AbortError";
  }
};
function isAbortError(error) {
  return error instanceof Error && error.name === "AbortError";
}
var InvalidStateError = class extends Error {
  constructor(message) {
    super(`InvalidStateError${message ? `: ${message}` : ""}`);
    this.name = "InvalidStateError";
  }
};
function isInvalidStateError(error) {
  return error instanceof Error && error.name === "InvalidStateError";
}

// node_modules/@virtualstate/navigation/esnext/event-target/signal-event.js
function isAbortSignal(value) {
  function isAbortSignalLike(value2) {
    return typeof value2 === "object";
  }
  return isAbortSignalLike(value) && typeof value.aborted === "boolean" && typeof value.addEventListener === "function";
}
function isSignalEvent(value) {
  function isSignalEventLike(value2) {
    return value2.hasOwnProperty("signal");
  }
  return isEvent(value) && isSignalEventLike(value) && isAbortSignal(value.signal);
}
function isSignalHandled(event, error) {
  if (isSignalEvent(event) && event.signal.aborted && error instanceof Error && isAbortError(error)) {
    return true;
  }
}

// node_modules/@virtualstate/navigation/esnext/event-target/event-target-options.js
var EventTargetListeners = Symbol.for("@opennetwork/environment/events/target/listeners");
var EventTargetListenersIgnore = Symbol.for("@opennetwork/environment/events/target/listeners/ignore");
var EventTargetListenersMatch = Symbol.for("@opennetwork/environment/events/target/listeners/match");
var EventTargetListenersThis = Symbol.for("@opennetwork/environment/events/target/listeners/this");

// node_modules/@virtualstate/navigation/esnext/event-target/descriptor.js
var EventDescriptorSymbol = Symbol.for("@opennetwork/environment/events/descriptor");

// node_modules/@virtualstate/navigation/esnext/event-target/callback.js
function matchEventCallback(type, callback, options) {
  const optionsDescriptor = isOptionsDescriptor(options) ? options : void 0;
  return (descriptor) => {
    if (optionsDescriptor) {
      return optionsDescriptor === descriptor;
    }
    return (!callback || callback === descriptor.callback) && type === descriptor.type;
  };
  function isOptionsDescriptor(options2) {
    function isLike(options3) {
      return !!options3;
    }
    return isLike(options2) && options2[EventDescriptorSymbol] === true;
  }
}

// node_modules/@virtualstate/navigation/esnext/event-target/event-target-listeners.js
function isFunctionEventCallback(fn) {
  return typeof fn === "function";
}
var EventTargetDescriptors = Symbol.for("@virtualstate/navigation/event-target/descriptors");
var EventTargetListeners2 = class {
  [EventTargetDescriptors] = [];
  [EventTargetListenersIgnore] = /* @__PURE__ */ new WeakSet();
  get [EventTargetListeners]() {
    return [...this[EventTargetDescriptors] ?? []];
  }
  [EventTargetListenersMatch](type) {
    const external = this[EventTargetListeners];
    const matched = [
      .../* @__PURE__ */ new Set([...external ?? [], ...this[EventTargetDescriptors] ?? []])
    ].filter((descriptor) => descriptor.type === type || descriptor.type === "*").filter((descriptor) => !this[EventTargetListenersIgnore]?.has(descriptor));
    const listener = typeof type === "string" ? this[`on${type}`] : void 0;
    if (typeof listener === "function" && isFunctionEventCallback(listener)) {
      matched.push({
        type,
        callback: listener,
        [EventDescriptorSymbol]: true
      });
    }
    return matched;
  }
  addEventListener(type, callback, options) {
    const listener = {
      ...options,
      isListening: () => !!this[EventTargetDescriptors]?.find(matchEventCallback(type, callback)),
      descriptor: {
        [EventDescriptorSymbol]: true,
        ...options,
        type,
        callback
      },
      timestamp: Date.now()
    };
    if (listener.isListening()) {
      return;
    }
    this[EventTargetDescriptors]?.push(listener.descriptor);
  }
  removeEventListener(type, callback, options) {
    if (!isFunctionEventCallback(callback)) {
      return;
    }
    const externalListeners = this[EventTargetListeners] ?? this[EventTargetDescriptors] ?? [];
    const externalIndex = externalListeners.findIndex(matchEventCallback(type, callback, options));
    if (externalIndex === -1) {
      return;
    }
    const index = this[EventTargetDescriptors]?.findIndex(matchEventCallback(type, callback, options)) ?? -1;
    if (index !== -1) {
      this[EventTargetDescriptors]?.splice(index, 1);
    }
    const descriptor = externalListeners[externalIndex];
    if (descriptor) {
      this[EventTargetListenersIgnore]?.add(descriptor);
    }
  }
  hasEventListener(type, callback) {
    if (callback && !isFunctionEventCallback(callback)) {
      return false;
    }
    const foundIndex = this[EventTargetDescriptors]?.findIndex(matchEventCallback(type, callback)) ?? -1;
    return foundIndex > -1;
  }
};

// node_modules/@virtualstate/navigation/esnext/event-target/async-event-target.js
var AsyncEventTarget = class extends EventTargetListeners2 {
  [EventTargetListenersThis];
  constructor(thisValue = void 0) {
    super();
    this[EventTargetListenersThis] = thisValue;
  }
  async dispatchEvent(event) {
    const listeners = this[EventTargetListenersMatch]?.(event.type) ?? [];
    if (isSignalEvent(event) && event.signal.aborted) {
      throw new AbortError();
    }
    const parallel = isParallelEvent(event);
    const promises = [];
    for (let index = 0; index < listeners.length; index += 1) {
      const descriptor = listeners[index];
      const promise = (async () => {
        if (descriptor.once) {
          this.removeEventListener(descriptor.type, descriptor.callback, descriptor);
        }
        await descriptor.callback.call(this[EventTargetListenersThis] ?? this, event);
      })();
      if (!parallel) {
        try {
          await promise;
        } catch (error) {
          if (!isSignalHandled(event, error)) {
            await Promise.reject(error);
          }
        }
        if (isSignalEvent(event) && event.signal.aborted) {
          return;
        }
      } else {
        promises.push(promise);
      }
    }
    if (promises.length) {
      const results = await Promise.allSettled(promises);
      const rejected = results.filter((result) => {
        return result.status === "rejected";
      });
      if (rejected.length) {
        let unhandled = rejected;
        if (isSignalEvent(event) && event.signal.aborted) {
          unhandled = unhandled.filter((result) => !isSignalHandled(event, result.reason));
        }
        if (unhandled.length === 1) {
          await Promise.reject(unhandled[0].reason);
          throw unhandled[0].reason;
        } else if (unhandled.length > 1) {
          throw new AggregateError(unhandled.map(({ reason }) => reason));
        }
      }
    }
  }
};

// node_modules/@virtualstate/navigation/esnext/event-target/event-target.js
var defaultEventTargetModule = {
  EventTarget: AsyncEventTarget,
  AsyncEventTarget,
  SyncEventTarget: AsyncEventTarget
};
var eventTargetModule = defaultEventTargetModule;
var EventTargetImplementation = eventTargetModule.EventTarget || eventTargetModule.SyncEventTarget || eventTargetModule.AsyncEventTarget;
function assertEventTarget(target) {
  if (typeof target !== "function") {
    throw new Error("Could not load EventTarget implementation");
  }
}
var EventTarget = class extends AsyncEventTarget {
  constructor(...args) {
    super();
    if (EventTargetImplementation) {
      assertEventTarget(EventTargetImplementation);
      const { dispatchEvent } = new EventTargetImplementation(...args);
      this.dispatchEvent = dispatchEvent;
    }
  }
};

// node_modules/@virtualstate/navigation/esnext/event-target/sync-event-target.js
var SyncEventTarget = class extends EventTargetListeners2 {
  [EventTargetListenersThis];
  constructor(thisValue = void 0) {
    super();
    this[EventTargetListenersThis] = thisValue;
  }
  dispatchEvent(event) {
    const listeners = this[EventTargetListenersMatch]?.(event.type) ?? [];
    if (isSignalEvent(event) && event.signal.aborted) {
      throw new AbortError();
    }
    for (let index = 0; index < listeners.length; index += 1) {
      const descriptor = listeners[index];
      if (descriptor.once) {
        this.removeEventListener(descriptor.type, descriptor.callback, descriptor);
      }
      try {
        descriptor.callback.call(this[EventTargetListenersThis] ?? this, event);
      } catch (error) {
        if (!isSignalHandled(event, error)) {
          throw error;
        }
      }
      if (isSignalEvent(event) && event.signal.aborted) {
        throw new AbortError();
      }
    }
  }
};

// node_modules/@virtualstate/navigation/esnext/navigation-event-target.js
var NavigationEventTarget = class extends EventTarget {
  addEventListener(type, listener, options) {
    assertEventCallback(listener);
    return super.addEventListener(type, listener, typeof options === "boolean" ? { once: options } : options);
    function assertEventCallback(listener2) {
      if (typeof listener2 !== "function")
        throw new Error("Please us the function variant of event listener");
    }
  }
  removeEventListener(type, listener, options) {
    assertEventCallback(listener);
    return super.removeEventListener(type, listener);
    function assertEventCallback(listener2) {
      if (typeof listener2 !== "function")
        throw new Error("Please us the function variant of event listener");
    }
  }
};

// node_modules/@virtualstate/navigation/esnext/util/uuid-or-random.js
var isWebCryptoSupported = "crypto" in globalThis && typeof globalThis.crypto.randomUUID === "function";
var v4 = isWebCryptoSupported ? globalThis.crypto.randomUUID.bind(globalThis.crypto) : () => Array.from({ length: 5 }, () => `${Math.random()}`.replace(/^0\./, "")).join("-").replace(".", "");

// node_modules/@virtualstate/navigation/esnext/navigation-entry.js
var NavigationGetState = Symbol.for("@virtualstate/navigation/getState");
var NavigationHistoryEntryNavigationType = Symbol.for("@virtualstate/navigation/entry/navigationType");
var NavigationHistoryEntryKnownAs = Symbol.for("@virtualstate/navigation/entry/knownAs");
var NavigationHistoryEntrySetState = Symbol.for("@virtualstate/navigation/entry/setState");
function isPrimitiveValue(state) {
  return typeof state === "number" || typeof state === "boolean" || typeof state === "symbol" || typeof state === "bigint" || typeof state === "string";
}
function isValue(state) {
  return !!(state || isPrimitiveValue(state));
}
var NavigationHistoryEntry = class extends NavigationEventTarget {
  #index;
  #state;
  get index() {
    return typeof this.#index === "number" ? this.#index : this.#index();
  }
  key;
  id;
  url;
  sameDocument;
  get [NavigationHistoryEntryNavigationType]() {
    return this.#options.navigationType;
  }
  get [NavigationHistoryEntryKnownAs]() {
    const set = new Set(this.#options[NavigationHistoryEntryKnownAs]);
    set.add(this.id);
    return set;
  }
  #options;
  get [EventTargetListeners]() {
    return [
      ...super[EventTargetListeners] ?? [],
      ...this.#options[EventTargetListeners] ?? []
    ];
  }
  constructor(init) {
    super();
    this.#options = init;
    this.key = init.key || v4();
    this.id = v4();
    this.url = init.url ?? void 0;
    this.#index = init.index;
    this.sameDocument = init.sameDocument ?? true;
    this.#state = init.state ?? void 0;
  }
  [NavigationGetState]() {
    return this.#options?.getState?.(this);
  }
  getState() {
    let state = this.#state;
    if (!isValue(state)) {
      const external = this[NavigationGetState]();
      if (isValue(external)) {
        state = this.#state = external;
      }
    }
    if (typeof state === "undefined" || isPrimitiveValue(state)) {
      return state;
    }
    if (typeof state === "function") {
      console.warn("State passed to Navigation.navigate was a function, this may be unintentional");
      console.warn("Unless a state value is primitive, with a standard implementation of Navigation");
      console.warn("your state value will be serialized and deserialized before this point, meaning");
      console.warn("a function would not be usable.");
    }
    return {
      ...state
    };
  }
  [NavigationHistoryEntrySetState](state) {
    this.#state = state;
  }
};

// node_modules/@virtualstate/navigation/esnext/util/deferred.js
function deferred(handleCatch) {
  let resolve = void 0, reject = void 0;
  const promise = new Promise((resolveFn, rejectFn) => {
    resolve = resolveFn;
    reject = rejectFn;
  });
  ok(resolve);
  ok(reject);
  return {
    resolve,
    reject,
    promise: handleCatch ? promise.catch(handleCatch) : promise
  };
}
function ok(value) {
  if (!value) {
    throw new Error("Value not provided");
  }
}

// node_modules/@virtualstate/navigation/esnext/global-abort-controller.js
var GlobalAbortController = typeof AbortController !== "undefined" ? AbortController : void 0;

// node_modules/@virtualstate/navigation/esnext/import-abort-controller.js
if (!GlobalAbortController) {
  throw new Error("AbortController expected to be available or polyfilled");
}
var AbortController2 = GlobalAbortController;

// node_modules/@virtualstate/navigation/esnext/is.js
function isPromise(value) {
  return like(value) && typeof value.then === "function";
}
function ok2(value, message = "Expected value") {
  if (!value) {
    throw new Error(message);
  }
}
function isPromiseRejectedResult(value) {
  return value.status === "rejected";
}
function like(value) {
  return !!value;
}

// node_modules/@virtualstate/navigation/esnext/navigation-transition.js
var Rollback = Symbol.for("@virtualstate/navigation/rollback");
var Unset = Symbol.for("@virtualstate/navigation/unset");
var NavigationTransitionParentEventTarget = Symbol.for("@virtualstate/navigation/transition/parentEventTarget");
var NavigationTransitionFinishedDeferred = Symbol.for("@virtualstate/navigation/transition/deferred/finished");
var NavigationTransitionCommittedDeferred = Symbol.for("@virtualstate/navigation/transition/deferred/committed");
var NavigationTransitionNavigationType = Symbol.for("@virtualstate/navigation/transition/navigationType");
var NavigationTransitionInitialEntries = Symbol.for("@virtualstate/navigation/transition/entries/initial");
var NavigationTransitionFinishedEntries = Symbol.for("@virtualstate/navigation/transition/entries/finished");
var NavigationTransitionInitialIndex = Symbol.for("@virtualstate/navigation/transition/index/initial");
var NavigationTransitionFinishedIndex = Symbol.for("@virtualstate/navigation/transition/index/finished");
var NavigationTransitionEntry = Symbol.for("@virtualstate/navigation/transition/entry");
var NavigationTransitionIsCommitted = Symbol.for("@virtualstate/navigation/transition/isCommitted");
var NavigationTransitionIsFinished = Symbol.for("@virtualstate/navigation/transition/isFinished");
var NavigationTransitionIsRejected = Symbol.for("@virtualstate/navigation/transition/isRejected");
var NavigationTransitionKnown = Symbol.for("@virtualstate/navigation/transition/known");
var NavigationTransitionPromises = Symbol.for("@virtualstate/navigation/transition/promises");
var NavigationIntercept = Symbol.for("@virtualstate/navigation/intercept");
var NavigationTransitionIsOngoing = Symbol.for("@virtualstate/navigation/transition/isOngoing");
var NavigationTransitionIsPending = Symbol.for("@virtualstate/navigation/transition/isPending");
var NavigationTransitionIsAsync = Symbol.for("@virtualstate/navigation/transition/isAsync");
var NavigationTransitionWait = Symbol.for("@virtualstate/navigation/transition/wait");
var NavigationTransitionPromiseResolved = Symbol.for("@virtualstate/navigation/transition/promise/resolved");
var NavigationTransitionRejected = Symbol.for("@virtualstate/navigation/transition/rejected");
var NavigationTransitionCommit = Symbol.for("@virtualstate/navigation/transition/commit");
var NavigationTransitionFinish = Symbol.for("@virtualstate/navigation/transition/finish");
var NavigationTransitionStart = Symbol.for("@virtualstate/navigation/transition/start");
var NavigationTransitionStartDeadline = Symbol.for("@virtualstate/navigation/transition/start/deadline");
var NavigationTransitionError = Symbol.for("@virtualstate/navigation/transition/error");
var NavigationTransitionFinally = Symbol.for("@virtualstate/navigation/transition/finally");
var NavigationTransitionAbort = Symbol.for("@virtualstate/navigation/transition/abort");
var NavigationTransitionInterceptOptionsCommit = Symbol.for("@virtualstate/navigation/transition/intercept/options/commit");
var NavigationTransitionCommitIsManual = Symbol.for("@virtualstate/navigation/transition/commit/isManual");
var NavigationTransition = class extends EventTarget {
  finished;
  committed;
  from;
  navigationType;
  [NavigationTransitionIsAsync] = false;
  [NavigationTransitionInterceptOptionsCommit];
  #options;
  [NavigationTransitionFinishedDeferred] = deferred();
  [NavigationTransitionCommittedDeferred] = deferred();
  get [NavigationTransitionIsPending]() {
    return !!this.#promises.size;
  }
  get [NavigationTransitionNavigationType]() {
    return this.#options[NavigationTransitionNavigationType];
  }
  get [NavigationTransitionInitialEntries]() {
    return this.#options[NavigationTransitionInitialEntries];
  }
  get [NavigationTransitionInitialIndex]() {
    return this.#options[NavigationTransitionInitialIndex];
  }
  get [NavigationTransitionCommitIsManual]() {
    return !!(this[NavigationTransitionInterceptOptionsCommit]?.includes("after-transition") || this[NavigationTransitionInterceptOptionsCommit]?.includes("manual"));
  }
  [NavigationTransitionFinishedEntries];
  [NavigationTransitionFinishedIndex];
  [NavigationTransitionIsCommitted] = false;
  [NavigationTransitionIsFinished] = false;
  [NavigationTransitionIsRejected] = false;
  [NavigationTransitionIsOngoing] = false;
  [NavigationTransitionKnown] = /* @__PURE__ */ new Set();
  [NavigationTransitionEntry];
  #promises = /* @__PURE__ */ new Set();
  #rolledBack = false;
  #abortController = new AbortController2();
  get signal() {
    return this.#abortController.signal;
  }
  get [NavigationTransitionPromises]() {
    return this.#promises;
  }
  constructor(init) {
    super();
    this[NavigationTransitionInterceptOptionsCommit] = [];
    this[NavigationTransitionFinishedDeferred] = init[NavigationTransitionFinishedDeferred] ?? this[NavigationTransitionFinishedDeferred];
    this[NavigationTransitionCommittedDeferred] = init[NavigationTransitionCommittedDeferred] ?? this[NavigationTransitionCommittedDeferred];
    this.#options = init;
    const finished = this.finished = this[NavigationTransitionFinishedDeferred].promise;
    const committed = this.committed = this[NavigationTransitionCommittedDeferred].promise;
    void finished.catch((error) => error);
    void committed.catch((error) => error);
    this.from = init.from;
    this.navigationType = init.navigationType;
    this[NavigationTransitionFinishedEntries] = init[NavigationTransitionFinishedEntries];
    this[NavigationTransitionFinishedIndex] = init[NavigationTransitionFinishedIndex];
    const known = init[NavigationTransitionKnown];
    if (known) {
      for (const entry of known) {
        this[NavigationTransitionKnown].add(entry);
      }
    }
    this[NavigationTransitionEntry] = init[NavigationTransitionEntry];
    {
      {
        this.addEventListener(NavigationTransitionCommit, this.#onCommitPromise, { once: true });
        this.addEventListener(NavigationTransitionFinish, this.#onFinishPromise, { once: true });
      }
      {
        this.addEventListener(NavigationTransitionCommit, this.#onCommitSetProperty, { once: true });
        this.addEventListener(NavigationTransitionFinish, this.#onFinishSetProperty, { once: true });
      }
      {
        this.addEventListener(NavigationTransitionError, this.#onError, {
          once: true
        });
        this.addEventListener(NavigationTransitionAbort, () => {
          if (!this[NavigationTransitionIsFinished]) {
            return this[NavigationTransitionRejected](new AbortError());
          }
        });
      }
      {
        this.addEventListener("*", this[NavigationTransitionEntry].dispatchEvent.bind(this[NavigationTransitionEntry]));
        this.addEventListener("*", init[NavigationTransitionParentEventTarget].dispatchEvent.bind(init[NavigationTransitionParentEventTarget]));
      }
    }
  }
  rollback = (options) => {
    if (this.#rolledBack) {
      throw new InvalidStateError("Rollback invoked multiple times: Please raise an issue at https://github.com/virtualstate/navigation with the use case where you want to use a rollback multiple times, this may have been unexpected behaviour");
    }
    this.#rolledBack = true;
    return this.#options.rollback(options);
  };
  #onCommitSetProperty = () => {
    this[NavigationTransitionIsCommitted] = true;
  };
  #onFinishSetProperty = () => {
    this[NavigationTransitionIsFinished] = true;
  };
  #onFinishPromise = () => {
    this[NavigationTransitionFinishedDeferred].resolve(this[NavigationTransitionEntry]);
  };
  #onCommitPromise = () => {
    if (this.signal.aborted) {
    } else {
      this[NavigationTransitionCommittedDeferred].resolve(this[NavigationTransitionEntry]);
    }
  };
  #onError = (event) => {
    return this[NavigationTransitionRejected](event.error);
  };
  [NavigationTransitionPromiseResolved] = (...promises) => {
    for (const promise of promises) {
      this.#promises.delete(promise);
    }
  };
  [NavigationTransitionRejected] = async (reason) => {
    if (this[NavigationTransitionIsRejected])
      return;
    this[NavigationTransitionIsRejected] = true;
    this[NavigationTransitionAbort]();
    const navigationType = this[NavigationTransitionNavigationType];
    if (typeof navigationType === "string" || navigationType === Rollback) {
      await this.dispatchEvent({
        type: "navigateerror",
        error: reason,
        get message() {
          if (reason instanceof Error) {
            return reason.message;
          }
          return `${reason}`;
        }
      });
      if (navigationType !== Rollback && !(isInvalidStateError(reason) || isAbortError(reason))) {
        try {
          await this.rollback()?.finished;
        } catch (error) {
          throw new InvalidStateError("Failed to rollback, please raise an issue at https://github.com/virtualstate/navigation/issues");
        }
      }
    }
    this[NavigationTransitionCommittedDeferred].reject(reason);
    this[NavigationTransitionFinishedDeferred].reject(reason);
  };
  [NavigationIntercept] = (options) => {
    const transition = this;
    const promise = parseOptions();
    this[NavigationTransitionIsOngoing] = true;
    if (!promise)
      return;
    this[NavigationTransitionIsAsync] = true;
    const statusPromise = promise.then(() => ({
      status: "fulfilled",
      value: void 0
    })).catch(async (reason) => {
      await this[NavigationTransitionRejected](reason);
      return {
        status: "rejected",
        reason
      };
    });
    this.#promises.add(statusPromise);
    function parseOptions() {
      if (!options)
        return void 0;
      if (isPromise(options)) {
        return options;
      }
      if (typeof options === "function") {
        return options();
      }
      const { handler, commit } = options;
      if (commit && typeof commit === "string") {
        transition[NavigationTransitionInterceptOptionsCommit].push(commit);
      }
      if (typeof handler !== "function") {
        return;
      }
      return handler();
    }
  };
  [NavigationTransitionWait] = async () => {
    if (!this.#promises.size)
      return this[NavigationTransitionEntry];
    try {
      const captured = [...this.#promises];
      const results = await Promise.all(captured);
      const rejected = results.filter((result) => result.status === "rejected");
      if (rejected.length) {
        if (rejected.length === 1) {
          throw rejected[0].reason;
        }
        if (typeof AggregateError !== "undefined") {
          throw new AggregateError(rejected.map(({ reason }) => reason));
        }
        throw new Error();
      }
      this[NavigationTransitionPromiseResolved](...captured);
      if (this[NavigationTransitionIsPending]) {
        return this[NavigationTransitionWait]();
      }
      return this[NavigationTransitionEntry];
    } catch (error) {
      await this.#onError(error);
      throw await Promise.reject(error);
    } finally {
      await this[NavigationTransitionFinish]();
    }
  };
  [NavigationTransitionAbort]() {
    if (this.#abortController.signal.aborted)
      return;
    this.#abortController.abort();
    this.dispatchEvent({
      type: NavigationTransitionAbort,
      transition: this,
      entry: this[NavigationTransitionEntry]
    });
  }
  [NavigationTransitionFinish] = async () => {
    if (this[NavigationTransitionIsFinished]) {
      return;
    }
    await this.dispatchEvent({
      type: NavigationTransitionFinish,
      transition: this,
      entry: this[NavigationTransitionEntry],
      intercept: this[NavigationIntercept]
    });
  };
};

// node_modules/@virtualstate/navigation/esnext/base-url.js
function getWindowBaseURL() {
  try {
    if (typeof window !== "undefined" && window.location) {
      return window.location.href;
    }
  } catch {
  }
}
function getBaseURL(url) {
  const baseURL = getWindowBaseURL() ?? "https://html.spec.whatwg.org/";
  return new URL(
    (url ?? "").toString(),
    baseURL
  );
}

// node_modules/@virtualstate/navigation/esnext/defer.js
function defer() {
  let resolve = void 0, reject = void 0, settled = false, status = "pending";
  const promise = new Promise((resolveFn, rejectFn) => {
    resolve = (value) => {
      status = "fulfilled";
      settled = true;
      resolveFn(value);
    };
    reject = (reason) => {
      status = "rejected";
      settled = true;
      rejectFn(reason);
    };
  });
  ok2(resolve);
  ok2(reject);
  return {
    get settled() {
      return settled;
    },
    get status() {
      return status;
    },
    resolve,
    reject,
    promise
  };
}

// node_modules/@virtualstate/navigation/esnext/events/navigation-current-entry-change-event.js
var NavigationCurrentEntryChangeEvent = class {
  type;
  from;
  navigationType;
  constructor(type, init) {
    this.type = type;
    if (!init) {
      throw new TypeError("init required");
    }
    if (!init.from) {
      throw new TypeError("from required");
    }
    this.from = init.from;
    this.navigationType = init.navigationType ?? void 0;
  }
};

// node_modules/@virtualstate/navigation/esnext/events/navigate-event.js
var NavigateEvent = class {
  type;
  canIntercept;
  canTransition;
  destination;
  downloadRequest;
  formData;
  hashChange;
  info;
  signal;
  userInitiated;
  navigationType;
  constructor(type, init) {
    this.type = type;
    if (!init) {
      throw new TypeError("init required");
    }
    if (!init.destination) {
      throw new TypeError("destination required");
    }
    if (!init.signal) {
      throw new TypeError("signal required");
    }
    this.canIntercept = init.canIntercept ?? false;
    this.canTransition = init.canIntercept ?? false;
    this.destination = init.destination;
    this.downloadRequest = init.downloadRequest;
    this.formData = init.formData;
    this.hashChange = init.hashChange ?? false;
    this.info = init.info;
    this.signal = init.signal;
    this.userInitiated = init.userInitiated ?? false;
    this.navigationType = init.navigationType ?? "push";
  }
  commit() {
    throw new Error("Not implemented");
  }
  intercept(options) {
    throw new Error("Not implemented");
  }
  preventDefault() {
    throw new Error("Not implemented");
  }
  reportError(reason) {
    throw new Error("Not implemented");
  }
  scroll() {
    throw new Error("Not implemented");
  }
  transitionWhile(options) {
    return this.intercept(options);
  }
};

// node_modules/@virtualstate/navigation/esnext/create-navigation-transition.js
var NavigationFormData = Symbol.for("@virtualstate/navigation/formData");
var NavigationDownloadRequest = Symbol.for("@virtualstate/navigation/downloadRequest");
var NavigationCanIntercept = Symbol.for("@virtualstate/navigation/canIntercept");
var NavigationUserInitiated = Symbol.for("@virtualstate/navigation/userInitiated");
var NavigationOriginalEvent = Symbol.for("@virtualstate/navigation/originalEvent");
var EventAbortController = Symbol.for("@virtualstate/navigation/event/abortController");
function noop() {
  return void 0;
}
function getEntryIndex(entries, entry) {
  const knownIndex = entry.index;
  if (knownIndex !== -1) {
    return knownIndex;
  }
  return -1;
}
function createNavigationTransition(context) {
  const { commit: transitionCommit, currentIndex, options, known: initialKnown, currentEntry, transition, transition: { [NavigationTransitionInitialEntries]: previousEntries, [NavigationTransitionEntry]: entry, [NavigationIntercept]: intercept }, reportError } = context;
  let { transition: { [NavigationTransitionNavigationType]: navigationType } } = context;
  let resolvedEntries = [...previousEntries];
  const known = new Set(initialKnown);
  let destinationIndex = -1, nextIndex = currentIndex;
  if (navigationType === Rollback) {
    const { index } = options ?? { index: void 0 };
    if (typeof index !== "number")
      throw new InvalidStateError("Expected index to be provided for rollback");
    destinationIndex = index;
    nextIndex = index;
  } else if (navigationType === "traverse" || navigationType === "reload") {
    destinationIndex = getEntryIndex(previousEntries, entry);
    nextIndex = destinationIndex;
  } else if (navigationType === "replace") {
    if (currentIndex === -1) {
      navigationType = "push";
      destinationIndex = currentIndex + 1;
      nextIndex = destinationIndex;
    } else {
      destinationIndex = currentIndex;
      nextIndex = currentIndex;
    }
  } else {
    destinationIndex = currentIndex + 1;
    nextIndex = destinationIndex;
  }
  if (typeof destinationIndex !== "number" || destinationIndex === -1) {
    throw new InvalidStateError("Could not resolve next index");
  }
  if (!entry.url) {
    console.trace({ navigationType, entry, options });
    throw new InvalidStateError("Expected entry url");
  }
  const destination = {
    url: entry.url,
    key: entry.key,
    index: destinationIndex,
    sameDocument: entry.sameDocument,
    getState() {
      return entry.getState();
    }
  };
  let hashChange = false;
  const currentUrlInstance = getBaseURL(currentEntry?.url);
  const destinationUrlInstance = new URL(destination.url);
  const currentHash = currentUrlInstance.hash;
  const destinationHash = destinationUrlInstance.hash;
  if (currentHash !== destinationHash) {
    const currentUrlInstanceWithoutHash = new URL(currentUrlInstance.toString());
    currentUrlInstanceWithoutHash.hash = "";
    const destinationUrlInstanceWithoutHash = new URL(destinationUrlInstance.toString());
    destinationUrlInstanceWithoutHash.hash = "";
    hashChange = currentUrlInstanceWithoutHash.toString() === destinationUrlInstanceWithoutHash.toString();
  }
  let contextToCommit;
  const { resolve: resolveCommit, promise: waitForCommit } = defer();
  function commit() {
    ok2(contextToCommit, "Expected contextToCommit");
    resolveCommit(transitionCommit(contextToCommit));
  }
  const abortController = new AbortController2();
  const event = new NavigateEvent("navigate", {
    signal: abortController.signal,
    info: void 0,
    ...options,
    canIntercept: options?.[NavigationCanIntercept] ?? true,
    formData: options?.[NavigationFormData] ?? void 0,
    downloadRequest: options?.[NavigationDownloadRequest] ?? void 0,
    hashChange,
    navigationType: options?.navigationType ?? (typeof navigationType === "string" ? navigationType : "replace"),
    userInitiated: options?.[NavigationUserInitiated] ?? false,
    destination
  });
  const originalEvent = options?.[NavigationOriginalEvent];
  const preventDefault = transition[NavigationTransitionAbort].bind(transition);
  if (originalEvent) {
    const definedEvent = originalEvent;
    event.intercept = function originalEventIntercept(options2) {
      definedEvent.preventDefault();
      return intercept(options2);
    };
    event.preventDefault = function originalEventPreventDefault() {
      definedEvent.preventDefault();
      return preventDefault();
    };
  } else {
    event.intercept = intercept;
    event.preventDefault = preventDefault;
  }
  event.transitionWhile = event.intercept;
  event.commit = commit;
  if (reportError) {
    event.reportError = reportError;
  }
  event.scroll = noop;
  if (originalEvent) {
    event.originalEvent = originalEvent;
  }
  const currentEntryChange = new NavigationCurrentEntryChangeEvent("currententrychange", {
    from: currentEntry,
    navigationType: event.navigationType
  });
  let updatedEntries = [], removedEntries = [], addedEntries = [];
  const previousKeys = previousEntries.map((entry2) => entry2.key);
  if (navigationType === Rollback) {
    const { entries } = options ?? { entries: void 0 };
    if (!entries)
      throw new InvalidStateError("Expected entries to be provided for rollback");
    resolvedEntries = entries;
    resolvedEntries.forEach((entry2) => known.add(entry2));
    const keys = resolvedEntries.map((entry2) => entry2.key);
    removedEntries = previousEntries.filter((entry2) => !keys.includes(entry2.key));
    addedEntries = resolvedEntries.filter((entry2) => !previousKeys.includes(entry2.key));
  } else if (navigationType === "replace" || navigationType === "traverse" || navigationType === "reload") {
    resolvedEntries[destination.index] = entry;
    if (navigationType !== "traverse") {
      updatedEntries.push(entry);
    }
    if (navigationType === "replace") {
      resolvedEntries = resolvedEntries.slice(0, destination.index + 1);
    }
    const keys = resolvedEntries.map((entry2) => entry2.key);
    removedEntries = previousEntries.filter((entry2) => !keys.includes(entry2.key));
    if (previousKeys.includes(entry.id)) {
      addedEntries = [entry];
    }
  } else if (navigationType === "push") {
    let removed = false;
    if (resolvedEntries[destination.index]) {
      resolvedEntries = resolvedEntries.slice(0, destination.index);
      removed = true;
    }
    resolvedEntries.push(entry);
    addedEntries = [entry];
    if (removed) {
      const keys = resolvedEntries.map((entry2) => entry2.key);
      removedEntries = previousEntries.filter((entry2) => !keys.includes(entry2.key));
    }
  }
  known.add(entry);
  let entriesChange = void 0;
  if (updatedEntries.length || addedEntries.length || removedEntries.length) {
    entriesChange = {
      updatedEntries,
      addedEntries,
      removedEntries
    };
  }
  contextToCommit = {
    entries: resolvedEntries,
    index: nextIndex,
    known,
    entriesChange
  };
  return {
    entries: resolvedEntries,
    known,
    index: nextIndex,
    currentEntryChange,
    destination,
    navigate: event,
    navigationType,
    waitForCommit,
    commit,
    abortController
  };
}

// node_modules/@virtualstate/navigation/esnext/event-target/create-event.js
function createEvent(event) {
  if (typeof CustomEvent !== "undefined" && typeof event.type === "string") {
    if (event instanceof CustomEvent) {
      return event;
    }
    const { type, detail, ...rest } = event;
    const customEvent = new CustomEvent(type, {
      detail: detail ?? rest
    });
    Object.assign(customEvent, rest);
    assertEvent(customEvent, event.type);
    return customEvent;
  }
  return event;
}

// node_modules/@virtualstate/navigation/esnext/navigation.js
var NavigationSetOptions = Symbol.for("@virtualstate/navigation/setOptions");
var NavigationSetEntries = Symbol.for("@virtualstate/navigation/setEntries");
var NavigationSetCurrentIndex = Symbol.for("@virtualstate/navigation/setCurrentIndex");
var NavigationSetCurrentKey = Symbol.for("@virtualstate/navigation/setCurrentKey");
var NavigationGetState2 = Symbol.for("@virtualstate/navigation/getState");
var NavigationSetState = Symbol.for("@virtualstate/navigation/setState");
var NavigationDisposeState = Symbol.for("@virtualstate/navigation/disposeState");
function isNavigationNavigationType(value) {
  return value === "reload" || value === "push" || value === "replace" || value === "traverse";
}
var Navigation = class extends NavigationEventTarget {
  #transitionInProgressCount = 0;
  #entries = [];
  #known = /* @__PURE__ */ new Set();
  #currentIndex = -1;
  #activeTransition;
  #knownTransitions = /* @__PURE__ */ new WeakSet();
  #baseURL = "";
  #initialEntry = void 0;
  #options = void 0;
  get canGoBack() {
    return !!this.#entries[this.#currentIndex - 1];
  }
  get canGoForward() {
    return !!this.#entries[this.#currentIndex + 1];
  }
  get currentEntry() {
    if (this.#currentIndex === -1) {
      if (!this.#initialEntry) {
        this.#initialEntry = new NavigationHistoryEntry({
          getState: this[NavigationGetState2],
          navigationType: "push",
          index: -1,
          sameDocument: false,
          url: this.#baseURL.toString()
        });
      }
      return this.#initialEntry;
    }
    return this.#entries[this.#currentIndex];
  }
  get transition() {
    const transition = this.#activeTransition;
    return transition?.signal.aborted ? void 0 : transition;
  }
  constructor(options = {}) {
    super();
    this[NavigationSetOptions](options);
  }
  [NavigationSetOptions](options) {
    this.#options = options;
    this.#baseURL = getBaseURL(options?.baseURL);
    this.#entries = [];
    if (options.entries) {
      this[NavigationSetEntries](options.entries);
    }
    if (options.currentKey) {
      this[NavigationSetCurrentKey](options.currentKey);
    } else if (typeof options.currentIndex === "number") {
      this[NavigationSetCurrentIndex](options.currentIndex);
    }
  }
  [NavigationSetCurrentKey](key) {
    const index = this.#entries.findIndex((entry) => entry.key === key);
    if (index === -1)
      return;
    this.#currentIndex = index;
  }
  [NavigationSetCurrentIndex](index) {
    if (index <= -1)
      return;
    if (index >= this.#entries.length)
      return;
    this.#currentIndex = index;
  }
  [NavigationSetEntries](entries) {
    this.#entries = entries.map(({ key, url, navigationType, state, sameDocument }, index) => new NavigationHistoryEntry({
      getState: this[NavigationGetState2],
      navigationType: isNavigationNavigationType(navigationType) ? navigationType : "push",
      sameDocument: sameDocument ?? true,
      index,
      url,
      key,
      state
    }));
    if (this.#currentIndex === -1 && this.#entries.length) {
      this.#currentIndex = 0;
    }
  }
  [NavigationGetState2] = (entry) => {
    return this.#options?.getState?.(entry) ?? void 0;
  };
  [NavigationSetState] = (entry) => {
    return this.#options?.setState?.(entry);
  };
  [NavigationDisposeState] = (entry) => {
    return this.#options?.disposeState?.(entry);
  };
  back(options) {
    if (!this.canGoBack)
      throw new InvalidStateError("Cannot go back");
    const entry = this.#entries[this.#currentIndex - 1];
    return this.#pushEntry("traverse", this.#cloneNavigationHistoryEntry(entry, {
      ...options,
      navigationType: "traverse"
    }));
  }
  entries() {
    return [...this.#entries];
  }
  forward(options) {
    if (!this.canGoForward)
      throw new InvalidStateError();
    const entry = this.#entries[this.#currentIndex + 1];
    return this.#pushEntry("traverse", this.#cloneNavigationHistoryEntry(entry, {
      ...options,
      navigationType: "traverse"
    }));
  }
  goTo(key, options) {
    return this.traverseTo(key, options);
  }
  traverseTo(key, options) {
    const found = this.#entries.find((entry) => entry.key === key);
    if (found) {
      return this.#pushEntry("traverse", this.#cloneNavigationHistoryEntry(found, {
        ...options,
        navigationType: "traverse"
      }));
    }
    throw new InvalidStateError();
  }
  #isSameDocument = (url) => {
    function isSameOrigins(a, b) {
      return a.origin === b.origin;
    }
    const currentEntryUrl = this.currentEntry?.url;
    if (!currentEntryUrl)
      return true;
    return isSameOrigins(new URL(currentEntryUrl), new URL(url));
  };
  navigate(url, options) {
    let baseURL = this.#baseURL;
    if (this.currentEntry?.url) {
      baseURL = this.currentEntry?.url;
    }
    const nextUrl = new URL(url, baseURL).toString();
    let navigationType = "push";
    if (options?.history === "push" || options?.history === "replace") {
      navigationType = options?.history;
    }
    const entry = this.#createNavigationHistoryEntry({
      getState: this[NavigationGetState2],
      url: nextUrl,
      ...options,
      sameDocument: this.#isSameDocument(nextUrl),
      navigationType
    });
    return this.#pushEntry(navigationType, entry, void 0, options);
  }
  #cloneNavigationHistoryEntry = (entry, options) => {
    return this.#createNavigationHistoryEntry({
      ...entry,
      getState: this[NavigationGetState2],
      index: entry?.index ?? void 0,
      state: options?.state ?? entry?.getState(),
      navigationType: entry?.[NavigationHistoryEntryNavigationType] ?? (typeof options?.navigationType === "string" ? options.navigationType : "replace"),
      ...options,
      get [NavigationHistoryEntryKnownAs]() {
        return entry?.[NavigationHistoryEntryKnownAs];
      },
      get [EventTargetListeners]() {
        return entry?.[EventTargetListeners];
      }
    });
  };
  #createNavigationHistoryEntry = (options) => {
    const entry = new NavigationHistoryEntry({
      ...options,
      index: options.index ?? (() => {
        return this.#entries.indexOf(entry);
      })
    });
    return entry;
  };
  #pushEntry = (navigationType, entry, transition, options) => {
    if (entry === this.currentEntry)
      throw new InvalidStateError();
    const existingPosition = this.#entries.findIndex((existing) => existing.id === entry.id);
    if (existingPosition > -1) {
      throw new InvalidStateError();
    }
    return this.#commitTransition(navigationType, entry, transition, options);
  };
  #commitTransition = (givenNavigationType, entry, transition, options) => {
    const nextTransition = transition ?? new NavigationTransition({
      from: entry,
      navigationType: typeof givenNavigationType === "string" ? givenNavigationType : "replace",
      rollback: (options2) => {
        return this.#rollback(nextTransition, options2);
      },
      [NavigationTransitionNavigationType]: givenNavigationType,
      [NavigationTransitionInitialEntries]: [...this.#entries],
      [NavigationTransitionInitialIndex]: this.#currentIndex,
      [NavigationTransitionKnown]: [...this.#known],
      [NavigationTransitionEntry]: entry,
      [NavigationTransitionParentEventTarget]: this
    });
    const { finished, committed } = nextTransition;
    const handler = () => {
      return this.#immediateTransition(givenNavigationType, entry, nextTransition, options);
    };
    this.#queueTransition(nextTransition);
    void handler().catch((error) => void 0);
    return { committed, finished };
  };
  #queueTransition = (transition) => {
    this.#knownTransitions.add(transition);
  };
  #immediateTransition = (givenNavigationType, entry, transition, options) => {
    try {
      this.#transitionInProgressCount += 1;
      return this.#transition(givenNavigationType, entry, transition, options);
    } finally {
      this.#transitionInProgressCount -= 1;
    }
  };
  #rollback = (rollbackTransition, options) => {
    const previousEntries = rollbackTransition[NavigationTransitionInitialEntries];
    const previousIndex = rollbackTransition[NavigationTransitionInitialIndex];
    const previousCurrent = previousEntries[previousIndex];
    const entry = previousCurrent ? this.#cloneNavigationHistoryEntry(previousCurrent, options) : void 0;
    const nextOptions = {
      ...options,
      index: previousIndex,
      known: /* @__PURE__ */ new Set([...this.#known, ...previousEntries]),
      navigationType: entry?.[NavigationHistoryEntryNavigationType] ?? "replace",
      entries: previousEntries
    };
    const resolvedNavigationType = entry ? Rollback : Unset;
    const resolvedEntry = entry ?? this.#createNavigationHistoryEntry({
      getState: this[NavigationGetState2],
      navigationType: "replace",
      index: nextOptions.index,
      sameDocument: true,
      ...options
    });
    return this.#pushEntry(resolvedNavigationType, resolvedEntry, void 0, nextOptions);
  };
  #transition = (givenNavigationType, entry, transition, options) => {
    let navigationType = givenNavigationType;
    const performance2 = getPerformance();
    if (performance2 && entry.sameDocument && typeof navigationType === "string") {
      performance2?.mark?.(`same-document-navigation:${entry.id}`);
    }
    let currentEntryChangeEvent = false, committedCurrentEntryChange = false;
    const { currentEntry } = this;
    void this.#activeTransition?.finished?.catch((error) => error);
    void this.#activeTransition?.[NavigationTransitionFinishedDeferred]?.promise?.catch((error) => error);
    void this.#activeTransition?.[NavigationTransitionCommittedDeferred]?.promise?.catch((error) => error);
    this.#activeTransition?.[NavigationTransitionAbort]();
    this.#activeTransition = transition;
    const startEventPromise = transition.dispatchEvent({
      type: NavigationTransitionStart,
      transition,
      entry
    });
    const syncCommit = ({ entries, index, known }) => {
      if (transition.signal.aborted)
        return;
      this.#entries = entries;
      if (known) {
        this.#known = /* @__PURE__ */ new Set([...this.#known, ...known]);
      }
      this.#currentIndex = index;
      this[NavigationSetState](this.currentEntry);
    };
    const asyncCommit = async (commit) => {
      if (committedCurrentEntryChange) {
        return;
      }
      committedCurrentEntryChange = true;
      syncCommit(commit);
      const { entriesChange } = commit;
      const promises = [
        transition.dispatchEvent(createEvent({
          type: NavigationTransitionCommit,
          transition,
          entry
        }))
      ];
      if (entriesChange) {
        promises.push(this.dispatchEvent(createEvent({
          type: "entrieschange",
          ...entriesChange
        })));
      }
      await Promise.all(promises);
    };
    const unsetTransition = async () => {
      await startEventPromise;
      if (!(typeof options?.index === "number" && options.entries))
        throw new InvalidStateError();
      const previous = this.entries();
      const previousKeys = previous.map((entry2) => entry2.key);
      const keys = options.entries.map((entry2) => entry2.key);
      const removedEntries = previous.filter((entry2) => !keys.includes(entry2.key));
      const addedEntries = options.entries.filter((entry2) => !previousKeys.includes(entry2.key));
      await asyncCommit({
        entries: options.entries,
        index: options.index,
        known: options.known,
        entriesChange: removedEntries.length || addedEntries.length ? {
          removedEntries,
          addedEntries,
          updatedEntries: []
        } : void 0
      });
      await this.dispatchEvent(createEvent({
        type: "currententrychange"
      }));
      currentEntryChangeEvent = true;
      return entry;
    };
    const completeTransition = () => {
      if (givenNavigationType === Unset) {
        return unsetTransition();
      }
      const transitionResult = createNavigationTransition({
        currentEntry,
        currentIndex: this.#currentIndex,
        options,
        transition,
        known: this.#known,
        commit: asyncCommit,
        reportError: transition[NavigationTransitionRejected]
      });
      const microtask = new Promise(queueMicrotask);
      let promises = [];
      const iterator = transitionSteps(transitionResult)[Symbol.iterator]();
      const iterable = {
        [Symbol.iterator]: () => ({ next: () => iterator.next() })
      };
      async function syncTransition() {
        for (const promise of iterable) {
          if (isPromise(promise)) {
            promises.push(Promise.allSettled([
              promise
            ]).then(([result]) => result));
          }
          if (transition[NavigationTransitionCommitIsManual] || currentEntryChangeEvent && transition[NavigationTransitionIsAsync]) {
            return asyncTransition().then(syncTransition);
          }
          if (transition.signal.aborted) {
            break;
          }
        }
        if (promises.length) {
          return asyncTransition();
        }
      }
      async function asyncTransition() {
        const captured = [...promises];
        if (captured.length) {
          promises = [];
          const results = await Promise.all(captured);
          const rejected = results.filter(isPromiseRejectedResult);
          if (rejected.length === 1) {
            throw await Promise.reject(rejected[0]);
          } else if (rejected.length) {
            throw new AggregateError(rejected, rejected[0].reason?.message);
          }
        } else if (!transition[NavigationTransitionIsOngoing]) {
          await microtask;
        }
      }
      return syncTransition().then(() => transition[NavigationTransitionIsOngoing] ? void 0 : microtask).then(() => entry);
    };
    const dispose = async () => this.#dispose();
    function* transitionSteps(transitionResult) {
      const microtask = new Promise(queueMicrotask);
      const { currentEntryChange, navigate, waitForCommit, commit, abortController } = transitionResult;
      const navigateAbort = abortController.abort.bind(abortController);
      transition.signal.addEventListener("abort", navigateAbort, {
        once: true
      });
      if (typeof navigationType === "string" || navigationType === Rollback) {
        const promise = currentEntry?.dispatchEvent(createEvent({
          type: "navigatefrom",
          intercept: transition[NavigationIntercept],
          transitionWhile: transition[NavigationIntercept]
        }));
        if (promise)
          yield promise;
      }
      if (typeof navigationType === "string") {
        yield transition.dispatchEvent(navigate);
      }
      if (!transition[NavigationTransitionCommitIsManual]) {
        commit();
      }
      yield waitForCommit;
      if (entry.sameDocument) {
        yield transition.dispatchEvent(currentEntryChange);
      }
      currentEntryChangeEvent = true;
      if (typeof navigationType === "string") {
        yield entry.dispatchEvent(createEvent({
          type: "navigateto",
          intercept: transition[NavigationIntercept],
          transitionWhile: transition[NavigationIntercept]
        }));
      }
      yield dispose();
      if (!transition[NavigationTransitionPromises].size) {
        yield microtask;
      }
      yield transition.dispatchEvent({
        type: NavigationTransitionStartDeadline,
        transition,
        entry
      });
      yield transition[NavigationTransitionWait]();
      transition.signal.removeEventListener("abort", navigateAbort);
      yield transition[NavigationTransitionFinish]();
      if (typeof navigationType === "string") {
        yield transition.dispatchEvent(createEvent({
          type: "finish",
          intercept: transition[NavigationIntercept],
          transitionWhile: transition[NavigationIntercept]
        }));
        yield transition.dispatchEvent(createEvent({
          type: "navigatesuccess",
          intercept: transition[NavigationIntercept],
          transitionWhile: transition[NavigationIntercept]
        }));
      }
    }
    const maybeSyncTransition = () => {
      try {
        return completeTransition();
      } catch (error) {
        return Promise.reject(error);
      }
    };
    return Promise.allSettled([maybeSyncTransition()]).then(async ([detail]) => {
      if (detail.status === "rejected") {
        await transition.dispatchEvent({
          type: NavigationTransitionError,
          error: detail.reason,
          transition,
          entry
        });
      }
      await dispose();
      await transition.dispatchEvent({
        type: NavigationTransitionFinally,
        transition,
        entry
      });
      await transition[NavigationTransitionWait]();
      if (this.#activeTransition === transition) {
        this.#activeTransition = void 0;
      }
      if (entry.sameDocument && typeof navigationType === "string") {
        performance2.mark(`same-document-navigation-finish:${entry.id}`);
        performance2.measure(`same-document-navigation:${entry.url}`, `same-document-navigation:${entry.id}`, `same-document-navigation-finish:${entry.id}`);
      }
    }).then(() => entry);
  };
  #dispose = async () => {
    for (const known of this.#known) {
      const index = this.#entries.findIndex((entry) => entry.key === known.key);
      if (index !== -1) {
        continue;
      }
      this.#known.delete(known);
      const event = createEvent({
        type: "dispose",
        entry: known
      });
      this[NavigationDisposeState](known);
      await known.dispatchEvent(event);
      await this.dispatchEvent(event);
    }
  };
  reload(options) {
    const { currentEntry } = this;
    if (!currentEntry)
      throw new InvalidStateError();
    const entry = this.#cloneNavigationHistoryEntry(currentEntry, options);
    return this.#pushEntry("reload", entry, void 0, options);
  }
  updateCurrentEntry(options) {
    const { currentEntry } = this;
    if (!currentEntry) {
      throw new InvalidStateError("Expected current entry");
    }
    currentEntry[NavigationHistoryEntrySetState](options.state);
    this[NavigationSetState](currentEntry);
    const currentEntryChange = new NavigationCurrentEntryChangeEvent("currententrychange", {
      from: currentEntry,
      navigationType: void 0
    });
    const entriesChange = createEvent({
      type: "entrieschange",
      addedEntries: [],
      removedEntries: [],
      updatedEntries: [
        currentEntry
      ]
    });
    return Promise.all([
      this.dispatchEvent(currentEntryChange),
      this.dispatchEvent(entriesChange)
    ]);
  }
};
function getPerformance() {
  if (typeof performance !== "undefined") {
    return performance;
  }
  return {
    now() {
      return Date.now();
    },
    mark() {
    },
    measure() {
    }
  };
}

// node_modules/@virtualstate/navigation/esnext/get-navigation.js
var navigation;
function getNavigation() {
  if (globalNavigation) {
    return globalNavigation;
  }
  if (navigation) {
    return navigation;
  }
  return navigation = new Navigation();
}

// node_modules/@virtualstate/navigation/esnext/util/serialization.js
var GLOBAL_SERIALIZER = JSON;
function setSerializer(serializer) {
  GLOBAL_SERIALIZER = serializer;
}
function stringify(value) {
  return GLOBAL_SERIALIZER.stringify(value);
}
function parse(value) {
  return GLOBAL_SERIALIZER.parse(value);
}

// node_modules/@virtualstate/navigation/esnext/location.js
var AppLocationCheckChange = Symbol.for("@virtualstate/navigation/location/checkChange");
var AppLocationAwaitFinished = Symbol.for("@virtualstate/navigation/location/awaitFinished");
var AppLocationTransitionURL = Symbol.for("@virtualstate/navigation/location/transitionURL");
var AppLocationUrl = Symbol.for("@virtualstate/navigation/location/url");
var NAVIGATION_LOCATION_DEFAULT_URL = "https://html.spec.whatwg.org/";
var NavigationLocation = class {
  #options;
  #navigation;
  constructor(options) {
    this.#options = options;
    this.#navigation = options.navigation;
    const reset = () => {
      this.#transitioningURL = void 0;
      this.#baseURL = void 0;
    };
    this.#navigation.addEventListener("navigate", () => {
      const transition = this.#navigation.transition;
      if (transition && isCommittedAvailable(transition)) {
        transition[NavigationTransitionCommittedDeferred].promise.then(reset, reset);
      }
      function isCommittedAvailable(transition2) {
        return NavigationTransitionCommittedDeferred in transition2;
      }
    });
    this.#navigation.addEventListener("currententrychange", reset);
  }
  #urls = /* @__PURE__ */ new WeakMap();
  #transitioningURL;
  #baseURL;
  get [AppLocationUrl]() {
    if (this.#transitioningURL) {
      return this.#transitioningURL;
    }
    const { currentEntry } = this.#navigation;
    if (!currentEntry) {
      this.#baseURL = getBaseURL(this.#options.baseURL);
      return this.#baseURL;
    }
    const existing = this.#urls.get(currentEntry);
    if (existing)
      return existing;
    const next = new URL(currentEntry.url ?? NAVIGATION_LOCATION_DEFAULT_URL);
    this.#urls.set(currentEntry, next);
    return next;
  }
  get hash() {
    return this[AppLocationUrl].hash;
  }
  set hash(value) {
    this.#setUrlValue("hash", value);
  }
  get host() {
    return this[AppLocationUrl].host;
  }
  set host(value) {
    this.#setUrlValue("host", value);
  }
  get hostname() {
    return this[AppLocationUrl].hostname;
  }
  set hostname(value) {
    this.#setUrlValue("hostname", value);
  }
  get href() {
    return this[AppLocationUrl].href;
  }
  set href(value) {
    this.#setUrlValue("href", value);
  }
  get origin() {
    return this[AppLocationUrl].origin;
  }
  get pathname() {
    return this[AppLocationUrl].pathname;
  }
  set pathname(value) {
    this.#setUrlValue("pathname", value);
  }
  get port() {
    return this[AppLocationUrl].port;
  }
  set port(value) {
    this.#setUrlValue("port", value);
  }
  get protocol() {
    return this[AppLocationUrl].protocol;
  }
  set protocol(value) {
    this.#setUrlValue("protocol", value);
  }
  get search() {
    return this[AppLocationUrl].search;
  }
  set search(value) {
    this.#setUrlValue("search", value);
  }
  #setUrlValue = (key, value) => {
    const currentUrlString = this[AppLocationUrl].toString();
    let nextUrl;
    if (key === "href") {
      nextUrl = new URL(value, currentUrlString);
    } else {
      nextUrl = new URL(currentUrlString);
      nextUrl[key] = value;
    }
    const nextUrlString = nextUrl.toString();
    if (currentUrlString === nextUrlString) {
      return;
    }
    void this.#transitionURL(nextUrl, () => this.#navigation.navigate(nextUrlString));
  };
  replace(url) {
    return this.#transitionURL(url, (url2) => this.#navigation.navigate(url2.toString(), {
      history: "replace"
    }));
  }
  reload() {
    return this.#awaitFinished(this.#navigation.reload());
  }
  assign(url) {
    return this.#transitionURL(url, (url2) => this.#navigation.navigate(url2.toString()));
  }
  [AppLocationTransitionURL](url, fn) {
    return this.#transitionURL(url, fn);
  }
  #transitionURL = async (url, fn) => {
    const instance = this.#transitioningURL = typeof url === "string" ? new URL(url, this[AppLocationUrl].toString()) : url;
    try {
      await this.#awaitFinished(fn(instance));
    } finally {
      if (this.#transitioningURL === instance) {
        this.#transitioningURL = void 0;
      }
    }
  };
  [AppLocationAwaitFinished](result) {
    return this.#awaitFinished(result);
  }
  #awaitFinished = async (result) => {
    this.#baseURL = void 0;
    if (!result)
      return;
    const { committed, finished } = result;
    await Promise.all([
      committed || Promise.resolve(void 0),
      finished || Promise.resolve(void 0)
    ]);
  };
  #triggerIfUrlChanged = () => {
    const current = this[AppLocationUrl];
    const currentUrl = current.toString();
    const expectedUrl = this.#navigation.currentEntry?.url;
    if (currentUrl !== expectedUrl) {
      return this.#transitionURL(current, () => this.#navigation.navigate(currentUrl));
    }
  };
  [AppLocationCheckChange]() {
    return this.#triggerIfUrlChanged();
  }
};

// node_modules/@virtualstate/navigation/esnext/history.js
var State = Symbol.for("@virtualstate/navigation/history/state");
var NavigationHistory = class extends NavigationLocation {
  #options;
  #navigation;
  constructor(options) {
    super(options);
    this.#options = options;
    this.#navigation = options.navigation;
  }
  get length() {
    return this.#navigation.entries().length;
  }
  scrollRestoration = "manual";
  get state() {
    const currentState = this.#navigation.currentEntry?.getState();
    if (typeof currentState === "string" || typeof currentState === "number" || typeof currentState === "boolean") {
      return currentState;
    }
    return this.#options[State] ?? void 0;
  }
  back() {
    const entries = this.#navigation.entries();
    const index = this.#navigation.currentEntry?.index ?? -1;
    const back = entries[index - 1];
    const url = back?.url;
    if (!url)
      throw new InvalidStateError("Cannot go back");
    return this[AppLocationTransitionURL](url, () => this.#navigation.back());
  }
  forward() {
    const entries = this.#navigation.entries();
    const index = this.#navigation.currentEntry?.index ?? -1;
    const forward = entries[index + 1];
    const url = forward?.url;
    if (!url)
      throw new InvalidStateError("Cannot go forward");
    return this[AppLocationTransitionURL](url, () => this.#navigation.forward());
  }
  go(delta) {
    if (typeof delta !== "number" || delta === 0 || isNaN(delta)) {
      return this[AppLocationAwaitFinished](this.#navigation.reload());
    }
    const entries = this.#navigation.entries();
    const { currentEntry } = this.#navigation;
    if (!currentEntry) {
      throw new Error(`Could not go ${delta}`);
    }
    const nextIndex = currentEntry.index + delta;
    const nextEntry = entries[nextIndex];
    if (!nextEntry) {
      throw new Error(`Could not go ${delta}`);
    }
    const nextEntryKey = nextEntry.key;
    return this[AppLocationAwaitFinished](this.#navigation.traverseTo(nextEntryKey));
  }
  replaceState(data, unused, url) {
    if (url) {
      return this[AppLocationTransitionURL](url, (url2) => this.#navigation.navigate(url2.toString(), {
        state: data,
        history: "replace"
      }));
    } else {
      return this.#navigation.updateCurrentEntry({
        state: data
      });
    }
  }
  pushState(data, unused, url) {
    if (url) {
      return this[AppLocationTransitionURL](url, (url2) => this.#navigation.navigate(url2.toString(), {
        state: data
      }));
    } else {
      return this.#navigation.updateCurrentEntry({
        state: data
      });
    }
  }
};

// node_modules/@virtualstate/navigation/esnext/global-window.js
var globalWindow = typeof window === "undefined" ? void 0 : window;

// node_modules/@virtualstate/navigation/esnext/global-self.js
var globalSelf = typeof self === "undefined" ? void 0 : self;

// node_modules/@virtualstate/navigation/esnext/get-polyfill.js
var NavigationKey = "__@virtualstate/navigation/key";
var NavigationMeta = "__@virtualstate/navigation/meta";
function getWindowHistory(givenWindow = globalWindow) {
  if (typeof givenWindow === "undefined")
    return void 0;
  return givenWindow.history;
}
function isStateHistoryMeta(state) {
  return like(state) && state[NavigationMeta] === true;
}
function isStateHistoryWithMeta(state) {
  return like(state) && isStateHistoryMeta(state[NavigationKey]);
}
function disposeHistoryState(entry, persist) {
  if (!persist)
    return;
  if (typeof sessionStorage === "undefined")
    return;
  sessionStorage.removeItem(entry.key);
}
function getEntries(navigation3, limit = DEFAULT_POLYFILL_OPTIONS.limit) {
  let entries = navigation3.entries();
  if (typeof limit === "number") {
    entries = entries.slice(-limit);
  }
  return entries.map(({ id, key, url, sameDocument }) => ({
    id,
    key,
    url,
    sameDocument
  }));
}
function getNavigationEntryMeta(navigation3, entry, limit = DEFAULT_POLYFILL_OPTIONS.limit) {
  return {
    [NavigationMeta]: true,
    currentIndex: entry.index,
    key: entry.key,
    entries: getEntries(navigation3, limit),
    state: entry.getState()
  };
}
function getNavigationEntryWithMeta(navigation3, entry, limit = DEFAULT_POLYFILL_OPTIONS.limit) {
  return {
    [NavigationKey]: getNavigationEntryMeta(navigation3, entry, limit)
  };
}
function setHistoryState(navigation3, history, entry, persist, limit) {
  setStateInSession();
  function getSerializableState() {
    return getNavigationEntryWithMeta(navigation3, entry, limit);
  }
  function setStateInSession() {
    if (typeof sessionStorage === "undefined")
      return;
    try {
      const raw = stringify(getSerializableState());
      sessionStorage.setItem(entry.key, raw);
    } catch {
    }
  }
}
function getHistoryState(history, entry) {
  return getStateFromHistoryIfMatchingKey() ?? getStateFromSession();
  function getStateFromHistoryDirectly() {
    try {
      return history.state;
    } catch {
      return void 0;
    }
  }
  function getBaseState() {
    const value = history.originalState ?? getStateFromHistoryDirectly();
    return like(value) ? value : void 0;
  }
  function getStateFromHistoryIfMatchingKey() {
    const state = getBaseState();
    if (!isStateHistoryWithMeta(state))
      return void 0;
    if (state[NavigationKey].key !== entry.key)
      return void 0;
    return state[NavigationKey].state;
  }
  function getStateFromSession() {
    if (typeof sessionStorage === "undefined")
      return void 0;
    try {
      const raw = sessionStorage.getItem(entry.key);
      if (!raw)
        return void 0;
      const state = parse(raw);
      if (!like(state))
        return void 0;
      if (!isStateHistoryWithMeta(state))
        return void 0;
      return state[NavigationKey].state;
    } catch {
      return void 0;
    }
  }
}
var DEFAULT_POLYFILL_OPTIONS = Object.freeze({
  persist: true,
  persistState: true,
  history: true,
  limit: 50,
  patch: true,
  interceptEvents: true
});
function isNavigationPolyfill(navigation3) {
  return like(navigation3) && typeof navigation3[NavigationSetEntries] === "function" && typeof navigation3[NavigationSetCurrentKey] === "function";
}
function getNavigationOnlyPolyfill(givenNavigation) {
  const entries = [
    {
      key: v4()
    }
  ];
  const navigation3 = givenNavigation ?? new Navigation({
    entries
  });
  const history = new NavigationHistory({
    navigation: navigation3
  });
  return {
    navigation: navigation3,
    history,
    apply() {
      if (isNavigationPolyfill(givenNavigation) && !navigation3.entries().length) {
        givenNavigation[NavigationSetEntries](entries);
      }
    }
  };
}
function interceptWindowClicks(navigation3, window2) {
  function clickCallback(ev, aEl) {
    process();
    function process() {
      if (!isAppNavigation(ev))
        return;
      ok2(ev);
      const options = {
        history: "auto",
        [NavigationUserInitiated]: true,
        [NavigationDownloadRequest]: aEl.download,
        [NavigationOriginalEvent]: ev
      };
      navigation3.navigate(aEl.href, options);
    }
  }
  function submitCallback(ev, form) {
    process();
    function process() {
      if (ev.defaultPrevented)
        return;
      const method = ev.submitter && "formMethod" in ev.submitter && ev.submitter.formMethod ? ev.submitter.formMethod : form.method;
      if (method === "dialog")
        return;
      const action = ev.submitter && "formAction" in ev.submitter && ev.submitter.formAction ? ev.submitter.formAction : form.action;
      let formData;
      try {
        formData = new FormData(form);
      } catch {
        formData = new FormData(void 0);
      }
      const params = method === "get" ? new URLSearchParams([...formData].map(([k, v]) => v instanceof File ? [k, v.name] : [k, v])) : void 0;
      const navFormData = method === "post" ? formData : void 0;
      const url = new URL(action, navigation3.currentEntry.url);
      if (params)
        url.search = params.toString();
      const unknownEvent = ev;
      ok2(unknownEvent);
      const options = {
        history: "auto",
        [NavigationUserInitiated]: true,
        [NavigationFormData]: navFormData,
        [NavigationOriginalEvent]: unknownEvent
      };
      navigation3.navigate(url.href, options);
    }
  }
  window2.addEventListener("click", (ev) => {
    if (ev.target?.ownerDocument === window2.document) {
      const aEl = getAnchorFromEvent(ev);
      if (like(aEl)) {
        clickCallback(ev, aEl);
      }
    }
  });
  window2.addEventListener("submit", (ev) => {
    if (ev.target?.ownerDocument === window2.document) {
      const form = getFormFromEvent(ev);
      if (like(form)) {
        submitCallback(ev, form);
      }
    }
  });
}
function getAnchorFromEvent(event) {
  return matchesAncestor(getComposedPathTarget(event), "a[href]:not([data-navigation-ignore])");
}
function getFormFromEvent(event) {
  return matchesAncestor(getComposedPathTarget(event), "form:not([data-navigation-ignore])");
}
function getComposedPathTarget(event) {
  if (!event.composedPath) {
    return event.target;
  }
  const targets = event.composedPath();
  return targets[0] ?? event.target;
}
function patchGlobalScope(window2, history, navigation3) {
  patchGlobals();
  patchPopState();
  patchHistory();
  function patchWindow(window3) {
    try {
      Object.defineProperty(window3, "navigation", {
        value: navigation3
      });
    } catch (e) {
    }
    if (!window3.history) {
      try {
        Object.defineProperty(window3, "history", {
          value: history
        });
      } catch (e) {
      }
    }
  }
  function patchGlobals() {
    patchWindow(window2);
    if (window2 !== globalWindow)
      return;
    if (globalSelf) {
      try {
        Object.defineProperty(globalSelf, "navigation", {
          value: navigation3
        });
      } catch (e) {
      }
    }
    if (typeof globalThis !== "undefined") {
      try {
        Object.defineProperty(globalThis, "navigation", {
          value: navigation3
        });
      } catch (e) {
      }
    }
  }
  function patchHistory() {
    if (history instanceof NavigationHistory) {
      return;
    }
    const polyfillHistory = new NavigationHistory({
      navigation: navigation3
    });
    const pushState = polyfillHistory.pushState.bind(polyfillHistory);
    const replaceState = polyfillHistory.replaceState.bind(polyfillHistory);
    const go = polyfillHistory.go.bind(polyfillHistory);
    const back = polyfillHistory.back.bind(polyfillHistory);
    const forward = polyfillHistory.forward.bind(polyfillHistory);
    const prototype = Object.getPrototypeOf(history);
    const descriptor = {
      pushState: {
        ...Object.getOwnPropertyDescriptor(prototype, "pushState"),
        value: pushState
      },
      replaceState: {
        ...Object.getOwnPropertyDescriptor(prototype, "replaceState"),
        value: replaceState
      },
      go: {
        ...Object.getOwnPropertyDescriptor(prototype, "go"),
        value: go
      },
      back: {
        ...Object.getOwnPropertyDescriptor(prototype, "back"),
        value: back
      },
      forward: {
        ...Object.getOwnPropertyDescriptor(prototype, "forward"),
        value: forward
      }
    };
    Object.defineProperties(prototype, descriptor);
    const stateDescriptor = Object.getOwnPropertyDescriptor(Object.getPrototypeOf(history), "state");
    Object.defineProperty(history, "state", {
      ...stateDescriptor,
      get() {
        return polyfillHistory.state;
      }
    });
    Object.defineProperty(history, "originalState", {
      ...stateDescriptor
    });
  }
  function patchPopState() {
    if (!window2.PopStateEvent)
      return;
    const popStateEventPrototype = window2.PopStateEvent.prototype;
    if (!popStateEventPrototype)
      return;
    const descriptor = Object.getOwnPropertyDescriptor(popStateEventPrototype, "state");
    Object.defineProperty(popStateEventPrototype, "state", {
      ...descriptor,
      get() {
        const original = descriptor.get.call(this);
        if (!isStateHistoryWithMeta(original))
          return original;
        return original[NavigationKey].state;
      }
    });
    Object.defineProperty(popStateEventPrototype, "originalState", {
      ...descriptor
    });
  }
}
function getCompletePolyfill(options = DEFAULT_POLYFILL_OPTIONS) {
  const { persist: PERSIST_ENTRIES, persistState: PERSIST_ENTRIES_STATE, history: givenHistory, limit: patchLimit, patch: PATCH_HISTORY, interceptEvents: INTERCEPT_EVENTS, window: givenWindow = globalWindow, navigation: givenNavigation } = {
    ...DEFAULT_POLYFILL_OPTIONS,
    ...options
  };
  const IS_PERSIST = PERSIST_ENTRIES || PERSIST_ENTRIES_STATE;
  const window2 = givenWindow ?? globalWindow;
  const history = options.history && typeof options.history !== "boolean" ? options.history : getWindowHistory(window2);
  if (!history) {
    return getNavigationOnlyPolyfill();
  }
  ok2(window2, "window required when using polyfill with history, this shouldn't be seen");
  const historyInitialState = history?.state;
  let initialMeta = {
    [NavigationMeta]: true,
    currentIndex: -1,
    entries: [],
    key: "",
    state: void 0
  };
  if (isStateHistoryWithMeta(historyInitialState)) {
    initialMeta = historyInitialState[NavigationKey];
  }
  let initialEntries = initialMeta.entries;
  const HISTORY_INTEGRATION = !!((givenWindow || givenHistory) && history);
  if (!initialEntries.length) {
    let url = void 0;
    if (window2.location?.href) {
      url = window2.location.href;
    }
    let state = void 0;
    if (!isStateHistoryWithMeta(historyInitialState) && !isStateHistoryMeta(historyInitialState)) {
      state = historyInitialState;
    }
    const key = v4();
    initialEntries = [
      {
        key,
        state,
        url
      }
    ];
    initialMeta.key = key;
    initialMeta.currentIndex = 0;
  }
  const navigationOptions = {
    entries: initialEntries,
    currentIndex: initialMeta?.currentIndex,
    currentKey: initialMeta?.key,
    getState(entry) {
      if (!HISTORY_INTEGRATION)
        return;
      return getHistoryState(history, entry);
    },
    setState(entry) {
      if (!HISTORY_INTEGRATION)
        return;
      if (!entry.sameDocument)
        return;
      setHistoryState(navigation3, history, entry, IS_PERSIST, patchLimit);
    },
    disposeState(entry) {
      if (!HISTORY_INTEGRATION)
        return;
      disposeHistoryState(entry, IS_PERSIST);
    }
  };
  const navigation3 = givenNavigation ?? new Navigation(navigationOptions);
  const pushState = history?.pushState.bind(history);
  const replaceState = history?.replaceState.bind(history);
  const historyGo = history?.go.bind(history);
  return {
    navigation: navigation3,
    history,
    apply() {
      if (isNavigationPolyfill(navigation3)) {
        navigation3[NavigationSetOptions](navigationOptions);
      }
      if (HISTORY_INTEGRATION) {
        const ignorePopState = /* @__PURE__ */ new Set();
        const ignoreCurrentEntryChange = /* @__PURE__ */ new Set();
        navigation3.addEventListener("navigate", (event) => {
          if (event.destination.sameDocument) {
            return;
          }
          event.intercept({
            commit: "after-transition",
            async handler() {
              queueMicrotask(() => {
                if (event.signal.aborted)
                  return;
                submit();
              });
            }
          });
          function submit() {
            if (like(event.originalEvent)) {
              const anchor = getAnchorFromEvent(event.originalEvent);
              if (anchor) {
                return submitAnchor(anchor);
              } else {
                const form = getFormFromEvent(event.originalEvent);
                if (form) {
                  return submitForm(form);
                }
              }
            }
            location.href = event.destination.url;
          }
          function submitAnchor(element) {
            const cloned = element.cloneNode();
            cloned.setAttribute("data-navigation-ignore", "1");
            cloned.click();
          }
          function submitForm(element) {
            const cloned = element.cloneNode();
            cloned.setAttribute("data-navigation-ignore", "1");
            cloned.submit();
          }
        });
        navigation3.addEventListener("currententrychange", ({ navigationType, from }) => {
          const { currentEntry } = navigation3;
          if (!currentEntry)
            return;
          const { key, url } = currentEntry;
          if (ignoreCurrentEntryChange.delete(key) || !currentEntry?.sameDocument)
            return;
          const historyState = getNavigationEntryWithMeta(navigation3, currentEntry, patchLimit);
          switch (navigationType || "replace") {
            case "push":
              return pushState(historyState, "", url);
            case "replace":
              return replaceState(historyState, "", url);
            case "traverse":
              const delta = currentEntry.index - from.index;
              ignorePopState.add(key);
              return historyGo(delta);
            case "reload":
          }
        });
        window2.addEventListener("popstate", (event) => {
          const { state, originalState } = event;
          const foundState = originalState ?? state;
          if (!isStateHistoryWithMeta(foundState))
            return;
          const { [NavigationKey]: { key } } = foundState;
          if (ignorePopState.delete(key))
            return;
          ignoreCurrentEntryChange.add(key);
          let committed;
          try {
            committed = navigation3.traverseTo(key).committed;
          } catch (error) {
            if (error instanceof InvalidStateError && !PERSIST_ENTRIES) {
              return;
            }
            throw error;
          }
          if (PERSIST_ENTRIES || PERSIST_ENTRIES_STATE) {
            committed.then((entry) => {
              const historyState = getNavigationEntryWithMeta(navigation3, entry, patchLimit);
              replaceState(historyState, "", entry.url);
            }).catch(() => {
            });
          }
        });
      }
      if (INTERCEPT_EVENTS) {
        interceptWindowClicks(navigation3, window2);
      }
      if (PATCH_HISTORY) {
        patchGlobalScope(window2, history, navigation3);
      }
      if (!history.state) {
        const historyState = getNavigationEntryWithMeta(navigation3, navigation3.currentEntry, patchLimit);
        replaceState(historyState, "", navigation3.currentEntry.url);
      }
    }
  };
}
function isAppNavigation(evt) {
  return evt.button === 0 && !evt.defaultPrevented && !evt.metaKey && !evt.altKey && !evt.ctrlKey && !evt.shiftKey;
}
function matchesAncestor(givenElement, selector) {
  let element = getDefaultElement();
  while (element) {
    if (element.matches(selector)) {
      ok2(element);
      return element;
    }
    element = element.parentElement ?? element.getRootNode()?.host;
  }
  return void 0;
  function getDefaultElement() {
    if (!givenElement)
      return void 0;
    if (givenElement.matches instanceof Function)
      return givenElement;
    return givenElement.parentElement;
  }
}

// node_modules/@virtualstate/navigation/esnext/apply-polyfill.js
function applyPolyfill(options = DEFAULT_POLYFILL_OPTIONS) {
  const { apply, navigation: navigation3 } = getCompletePolyfill(options);
  apply();
  return navigation3;
}
function shouldApplyPolyfill(navigation3 = getNavigation()) {
  return navigation3 !== globalNavigation && !globalNavigation && typeof window !== "undefined";
}

// node_modules/@virtualstate/navigation/esnext/polyfill.js
var navigation2 = getNavigation();
if (shouldApplyPolyfill(navigation2)) {
  try {
    applyPolyfill({
      navigation: navigation2
    });
  } catch (error) {
    console.error("Failed to apply polyfill");
    console.error(error);
  }
}
export {
  setSerializer
};
