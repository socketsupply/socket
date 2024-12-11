
declare module "socket:async/context" {
    /**
     * @module async.context
     *
     * Async Context for JavaScript based on the TC39 proposal.
     *
     * Example usage:
     * ```js
     * // `AsyncContext` is also globally available as `globalThis.AsyncContext`
     * import AsyncContext from 'socket:async/context'
     *
     * const var = new AsyncContext.Variable()
     * var.run('top', () => {
     *   console.log(var.get()) // 'top'
     *   queueMicrotask(() => {
     *     var.run('nested', () => {
     *       console.log(var.get()) // 'nested'
     *     })
     *   })
     * })
     * ```
     *
     * @see {@link https://tc39.es/proposal-async-context}
     * @see {@link https://github.com/tc39/proposal-async-context}
     */
    /**
     * @template T
     * @typedef {{
     *   name?: string,
     *   defaultValue?: T
     * }} VariableOptions
     */
    /**
     * @callback AnyFunc
     * @template T
     * @this T
     * @param {...any} args
     * @returns {any}
     */
    /**
     * `FrozenRevert` holds a frozen Mapping that will be simply restored
     * when the revert is run.
     * @see {@link https://github.com/tc39/proposal-async-context/blob/master/src/fork.ts}
     */
    export class FrozenRevert {
        /**
         * `FrozenRevert` class constructor.
         * @param {Mapping} mapping
         */
        constructor(mapping: Mapping);
        /**
         * Restores (unchaged) mapping from this `FrozenRevert`. This function is
         * called by `AsyncContext.Storage` when it reverts a current mapping to the
         * previous state before a "fork".
         * @param {Mapping=} [unused]
         * @return {Mapping}
         */
        restore(unused?: Mapping | undefined): Mapping;
        #private;
    }
    /**
     * Revert holds the state on how to revert a change to the
     * `AsyncContext.Storage` current `Mapping`
     * @see {@link https://github.com/tc39/proposal-async-context/blob/master/src/fork.ts}
     * @template T
     */
    export class Revert<T> {
        /**
         * `Revert` class constructor.
         * @param {Mapping} mapping
         * @param {Variable<T>} key
         */
        constructor(mapping: Mapping, key: Variable<T>);
        /**
         * @type {T|undefined}
         */
        get previousVariable(): T;
        /**
         * Restores a mapping from this `Revert`. This function is called by
         * `AsyncContext.Storage` when it reverts a current mapping to the
         * previous state before a "fork".
         * @param {Mapping} current
         * @return {Mapping}
         */
        restore(current: Mapping): Mapping;
        #private;
    }
    /**
     * A container for all `AsyncContext.Variable` instances and snapshot state.
     * @see {@link https://github.com/tc39/proposal-async-context/blob/master/src/mapping.ts}
     */
    export class Mapping {
        /**
         * `Mapping` class constructor.
         * @param {Map<Variable<any>, any>} data
         */
        constructor(data: Map<Variable<any>, any>);
        /**
         * Freezes the `Mapping` preventing `AsyncContext.Variable` modifications with
         * `set()` and `delete()`.
         */
        freeze(): void;
        /**
         * Returns `true` if the `Mapping` is frozen, otherwise `false`.
         * @return {boolean}
         */
        isFrozen(): boolean;
        /**
         * Optionally returns a new `Mapping` if the current one is "frozen",
         * otherwise it just returns the current instance.
         * @return {Mapping}
         */
        fork(): Mapping;
        /**
         * Returns `true` if the `Mapping` has a `AsyncContext.Variable` at `key`,
         * otherwise `false.
         * @template T
         * @param {Variable<T>} key
         * @return {boolean}
         */
        has<T>(key: Variable<T>): boolean;
        /**
         * Gets an `AsyncContext.Variable` value at `key`. If not set, this function
         * returns `undefined`.
         * @template T
         * @param {Variable<T>} key
         * @return {boolean}
         */
        get<T>(key: Variable<T>): boolean;
        /**
         * Sets an `AsyncContext.Variable` value at `key`. If the `Mapping` is frozen,
         * then a "forked" (new) instance with the value set on it is returned,
         * otherwise the current instance.
         * @template T
         * @param {Variable<T>} key
         * @param {T} value
         * @return {Mapping}
         */
        set<T>(key: Variable<T>, value: T): Mapping;
        /**
         * Delete  an `AsyncContext.Variable` value at `key`.
         * If the `Mapping` is frozen, then a "forked" (new) instance is returned,
         * otherwise the current instance.
         * @template T
         * @param {Variable<T>} key
         * @param {T} value
         * @return {Mapping}
         */
        delete<T>(key: Variable<T>): Mapping;
        #private;
    }
    /**
     * A container of all `AsyncContext.Variable` data.
     * @ignore
     * @see {@link https://github.com/tc39/proposal-async-context/blob/master/src/storage.ts}
     */
    export class Storage {
        /**
         * The current `Mapping` for this `AsyncContext`.
         * @type {Mapping}
         */
        static "__#4@#current": Mapping;
        /**
         * Returns `true` if the current `Mapping` has a
         * `AsyncContext.Variable` at `key`,
         * otherwise `false.
         * @template T
         * @param {Variable<T>} key
         * @return {boolean}
         */
        static has<T>(key: Variable<T>): boolean;
        /**
         * Gets an `AsyncContext.Variable` value at `key` for the current `Mapping`.
         * If not set, this function returns `undefined`.
         * @template T
         * @param {Variable<T>} key
         * @return {T|undefined}
         */
        static get<T>(key: Variable<T>): T | undefined;
        /**
         * Set updates the `AsyncContext.Variable` with a new value and returns a
         * revert action that allows the modification to be reversed in the future.
         * @template T
         * @param {Variable<T>} key
         * @param {T} value
         * @return {Revert<T>|FrozenRevert}
         */
        static set<T>(key: Variable<T>, value: T): Revert<T> | FrozenRevert;
        /**
         * "Freezes" the current storage `Mapping`, and returns a new `FrozenRevert`
         * or `Revert` which can restore the storage state to the state at
         * the time of the snapshot.
         * @return {FrozenRevert}
         */
        static snapshot(): FrozenRevert;
        /**
         * Restores the storage `Mapping` state to state at the time the
         * "revert" (`FrozenRevert` or `Revert`) was created.
         * @template T
         * @param {Revert<T>|FrozenRevert} revert
         */
        static restore<T>(revert: Revert<T> | FrozenRevert): void;
        /**
         * Switches storage `Mapping` state to the state at the time of a
         * "snapshot".
         * @param {FrozenRevert} snapshot
         * @return {FrozenRevert}
         */
        static switch(snapshot: FrozenRevert): FrozenRevert;
    }
    /**
     * `AsyncContext.Variable` is a container for a value that is associated with
     * the current execution flow. The value is propagated through async execution
     * flows, and can be snapshot and restored with Snapshot.
     * @template T
     * @see {@link https://github.com/tc39/proposal-async-context/blob/master/README.md#asynccontextvariable}
     */
    export class Variable<T> {
        /**
         * `Variable` class constructor.
         * @param {VariableOptions<T>=} [options]
         */
        constructor(options?: VariableOptions<T> | undefined);
        set defaultValue(defaultValue: T);
        /**
         * @ignore
         */
        get defaultValue(): T;
        /**
         * @ignore
         */
        get revert(): FrozenRevert | Revert<T>;
        /**
         * The name of this async context variable.
         * @type {string}
         */
        get name(): string;
        /**
         * Executes a function `fn` with specified arguments,
         * setting a new value to the current context before the call,
         * and ensuring the environment is reverted back afterwards.
         * The function allows for the modification of a specific context's
         * state in a controlled manner, ensuring that any changes can be undone.
         * @template T, F extends AnyFunc<null>
         * @param {T} value
         * @param {F} fn
         * @param {...Parameters<F>} args
         * @returns {ReturnType<F>}
         */
        run<T_1, F>(value: T_1, fn: F, ...args: Parameters<F>[]): ReturnType<F>;
        /**
         * Get the `AsyncContext.Variable` value.
         * @template T
         * @return {T|undefined}
         */
        get<T_1>(): T_1 | undefined;
        #private;
    }
    /**
     * `AsyncContext.Snapshot` allows you to opaquely capture the current values of
     * all `AsyncContext.Variable` instances and execute a function at a later time
     * as if those values were still the current values (a snapshot and restore).
     * @see {@link https://github.com/tc39/proposal-async-context/blob/master/README.md#asynccontextsnapshot}
     */
    export class Snapshot {
        /**
         * Wraps a given function `fn` with additional logic to take a snapshot of
         * `Storage` before invoking `fn`. Returns a new function with the same
         * signature as `fn` that when called, will invoke `fn` with the current
         * `this` context and provided arguments, after restoring the `Storage`
         * snapshot.
         *
         * `AsyncContext.Snapshot.wrap` is a helper which captures the current values
         * of all Variables and returns a wrapped function. When invoked, this
         * wrapped function restores the state of all Variables and executes the
         * inner function.
         *
         * @see {@link https://github.com/tc39/proposal-async-context/blob/master/README.md#asynccontextsnapshotwrap}
         *
         * @template F
         * @param {F} fn
         * @returns {F}
         */
        static wrap<F>(fn: F): F;
        /**
         * Runs the given function `fn` with arguments `args`, using a `null`
         * context and the current snapshot.
         *
         * @template F extends AnyFunc<null>
         * @param {F} fn
         * @param {...Parameters<F>} args
         * @returns {ReturnType<F>}
         */
        run<F>(fn: F, ...args: Parameters<F>[]): ReturnType<F>;
        #private;
    }
    /**
     * `AsyncContext` container.
     */
    export class AsyncContext {
        /**
         * `AsyncContext.Variable` is a container for a value that is associated with
         * the current execution flow. The value is propagated through async execution
         * flows, and can be snapshot and restored with Snapshot.
         * @see {@link https://github.com/tc39/proposal-async-context/blob/master/README.md#asynccontextvariable}
         * @type {typeof Variable}
         */
        static Variable: typeof Variable;
        /**
         * `AsyncContext.Snapshot` allows you to opaquely capture the current values of
         * all `AsyncContext.Variable` instances and execute a function at a later time
         * as if those values were still the current values (a snapshot and restore).
         * @see {@link https://github.com/tc39/proposal-async-context/blob/master/README.md#asynccontextsnapshot}
         * @type {typeof Snapshot}
         */
        static Snapshot: typeof Snapshot;
    }
    export default AsyncContext;
    export type VariableOptions<T> = {
        name?: string;
        defaultValue?: T;
    };
    export type AnyFunc = () => any;
}

declare module "socket:events" {
    export const Event: {
        new (type: string, eventInitDict?: EventInit): Event;
        prototype: Event;
        readonly NONE: 0;
        readonly CAPTURING_PHASE: 1;
        readonly AT_TARGET: 2;
        readonly BUBBLING_PHASE: 3;
    } | {
        new (): {};
    };
    export const EventTarget: {
        new (): {};
    };
    export const CustomEvent: {
        new <T>(type: string, eventInitDict?: CustomEventInit<T>): CustomEvent<T>;
        prototype: CustomEvent;
    } | {
        new (type: any, options: any): {
            "__#7@#detail": any;
            readonly detail: any;
        };
    };
    export const MessageEvent: {
        new <T>(type: string, eventInitDict?: MessageEventInit<T>): MessageEvent<T>;
        prototype: MessageEvent;
    } | {
        new (type: any, options: any): {
            "__#8@#detail": any;
            "__#8@#data": any;
            readonly detail: any;
            readonly data: any;
        };
    };
    export const ErrorEvent: {
        new (type: string, eventInitDict?: ErrorEventInit): ErrorEvent;
        prototype: ErrorEvent;
    } | {
        new (type: any, options: any): {
            "__#9@#detail": any;
            "__#9@#error": any;
            readonly detail: any;
            readonly error: any;
        };
    };
    export default EventEmitter;
    export function EventEmitter(): void;
    export class EventEmitter {
        _events: any;
        _contexts: any;
        _eventsCount: number;
        _maxListeners: number;
        setMaxListeners(n: any): this;
        getMaxListeners(): any;
        emit(type: any, ...args: any[]): boolean;
        addListener(type: any, listener: any): any;
        on(arg0: any, arg1: any): any;
        prependListener(type: any, listener: any): any;
        once(type: any, listener: any): this;
        prependOnceListener(type: any, listener: any): this;
        removeListener(type: any, listener: any): this;
        off(type: any, listener: any): this;
        removeAllListeners(type: any, ...args: any[]): this;
        listeners(type: any): any[];
        rawListeners(type: any): any[];
        listenerCount(type: any): any;
        eventNames(): (string | symbol)[];
    }
    export namespace EventEmitter {
        export { EventEmitter };
        export let defaultMaxListeners: number;
        export function init(): void;
        export function listenerCount(emitter: any, type: any): any;
        export { once };
    }
    export function once(emitter: any, name: any): Promise<any>;
}

declare module "socket:url/urlpattern/urlpattern" {
    export { me as URLPattern };
    var me: {
        new (t: {}, r: any, n: any): {
            "__#11@#i": any;
            "__#11@#n": {};
            "__#11@#t": {};
            "__#11@#e": {};
            "__#11@#s": {};
            "__#11@#l": boolean;
            test(t: {}, r: any): boolean;
            exec(t: {}, r: any): {
                inputs: any[] | {}[];
            };
            readonly protocol: any;
            readonly username: any;
            readonly password: any;
            readonly hostname: any;
            readonly port: any;
            readonly pathname: any;
            readonly search: any;
            readonly hash: any;
            readonly hasRegExpGroups: boolean;
        };
        compareComponent(t: any, r: any, n: any): number;
    };
}

declare module "socket:url/url/url" {
    const _default: any;
    export default _default;
}

declare module "socket:buffer" {
    export default Buffer;
    export const File: {
        new (fileBits: BlobPart[], fileName: string, options?: FilePropertyBag): File;
        prototype: File;
    };
    export const Blob: {
        new (blobParts?: BlobPart[], options?: BlobPropertyBag): Blob;
        prototype: Blob;
    };
    export namespace constants {
        export { kMaxLength as MAX_LENGTH };
        export { kMaxLength as MAX_STRING_LENGTH };
    }
    export const btoa: any;
    export const atob: any;
    /**
     * The Buffer constructor returns instances of `Uint8Array` that have their
     * prototype changed to `Buffer.prototype`. Furthermore, `Buffer` is a subclass of
     * `Uint8Array`, so the returned instances will have all the node `Buffer` methods
     * and the `Uint8Array` methods. Square bracket notation works as expected -- it
     * returns a single octet.
     *
     * The `Uint8Array` prototype remains unmodified.
     */
    /**
     * @name Buffer
     * @extends {Uint8Array}
     */
    export function Buffer(arg: any, encodingOrOffset: any, length: any): any;
    export class Buffer {
        /**
         * The Buffer constructor returns instances of `Uint8Array` that have their
         * prototype changed to `Buffer.prototype`. Furthermore, `Buffer` is a subclass of
         * `Uint8Array`, so the returned instances will have all the node `Buffer` methods
         * and the `Uint8Array` methods. Square bracket notation works as expected -- it
         * returns a single octet.
         *
         * The `Uint8Array` prototype remains unmodified.
         */
        /**
         * @name Buffer
         * @extends {Uint8Array}
         */
        constructor(arg: any, encodingOrOffset: any, length: any);
        get parent(): any;
        get offset(): any;
        _isBuffer: boolean;
        swap16(): this;
        swap32(): this;
        swap64(): this;
        toString(...args: any[]): any;
        toLocaleString: any;
        equals(b: any): boolean;
        inspect(): string;
        compare(target: any, start: any, end: any, thisStart: any, thisEnd: any): 0 | 1 | -1;
        includes(val: any, byteOffset: any, encoding: any): boolean;
        indexOf(val: any, byteOffset: any, encoding: any): any;
        lastIndexOf(val: any, byteOffset: any, encoding: any): any;
        write(string: any, offset: any, length: any, encoding: any): number;
        toJSON(): {
            type: string;
            data: any;
        };
        slice(start: any, end: any): any;
        readUintLE: (offset: any, byteLength: any, noAssert: any) => any;
        readUIntLE(offset: any, byteLength: any, noAssert: any): any;
        readUintBE: (offset: any, byteLength: any, noAssert: any) => any;
        readUIntBE(offset: any, byteLength: any, noAssert: any): any;
        readUint8: (offset: any, noAssert: any) => any;
        readUInt8(offset: any, noAssert: any): any;
        readUint16LE: (offset: any, noAssert: any) => number;
        readUInt16LE(offset: any, noAssert: any): number;
        readUint16BE: (offset: any, noAssert: any) => number;
        readUInt16BE(offset: any, noAssert: any): number;
        readUint32LE: (offset: any, noAssert: any) => number;
        readUInt32LE(offset: any, noAssert: any): number;
        readUint32BE: (offset: any, noAssert: any) => number;
        readUInt32BE(offset: any, noAssert: any): number;
        readBigUInt64LE: any;
        readBigUInt64BE: any;
        readIntLE(offset: any, byteLength: any, noAssert: any): any;
        readIntBE(offset: any, byteLength: any, noAssert: any): any;
        readInt8(offset: any, noAssert: any): any;
        readInt16LE(offset: any, noAssert: any): number;
        readInt16BE(offset: any, noAssert: any): number;
        readInt32LE(offset: any, noAssert: any): number;
        readInt32BE(offset: any, noAssert: any): number;
        readBigInt64LE: any;
        readBigInt64BE: any;
        readFloatLE(offset: any, noAssert: any): number;
        readFloatBE(offset: any, noAssert: any): number;
        readDoubleLE(offset: any, noAssert: any): number;
        readDoubleBE(offset: any, noAssert: any): number;
        writeUintLE: (value: any, offset: any, byteLength: any, noAssert: any) => any;
        writeUIntLE(value: any, offset: any, byteLength: any, noAssert: any): any;
        writeUintBE: (value: any, offset: any, byteLength: any, noAssert: any) => any;
        writeUIntBE(value: any, offset: any, byteLength: any, noAssert: any): any;
        writeUint8: (value: any, offset: any, noAssert: any) => any;
        writeUInt8(value: any, offset: any, noAssert: any): any;
        writeUint16LE: (value: any, offset: any, noAssert: any) => any;
        writeUInt16LE(value: any, offset: any, noAssert: any): any;
        writeUint16BE: (value: any, offset: any, noAssert: any) => any;
        writeUInt16BE(value: any, offset: any, noAssert: any): any;
        writeUint32LE: (value: any, offset: any, noAssert: any) => any;
        writeUInt32LE(value: any, offset: any, noAssert: any): any;
        writeUint32BE: (value: any, offset: any, noAssert: any) => any;
        writeUInt32BE(value: any, offset: any, noAssert: any): any;
        writeBigUInt64LE: any;
        writeBigUInt64BE: any;
        writeIntLE(value: any, offset: any, byteLength: any, noAssert: any): any;
        writeIntBE(value: any, offset: any, byteLength: any, noAssert: any): any;
        writeInt8(value: any, offset: any, noAssert: any): any;
        writeInt16LE(value: any, offset: any, noAssert: any): any;
        writeInt16BE(value: any, offset: any, noAssert: any): any;
        writeInt32LE(value: any, offset: any, noAssert: any): any;
        writeInt32BE(value: any, offset: any, noAssert: any): any;
        writeBigInt64LE: any;
        writeBigInt64BE: any;
        writeFloatLE(value: any, offset: any, noAssert: any): any;
        writeFloatBE(value: any, offset: any, noAssert: any): any;
        writeDoubleLE(value: any, offset: any, noAssert: any): any;
        writeDoubleBE(value: any, offset: any, noAssert: any): any;
        copy(target: any, targetStart: any, start: any, end: any): number;
        fill(val: any, start: any, end: any, encoding: any): this;
    }
    export namespace Buffer {
        export let TYPED_ARRAY_SUPPORT: boolean;
        export let poolSize: number;
        /**
         * Functionally equivalent to Buffer(arg, encoding) but throws a TypeError
         * if value is a number.
         * Buffer.from(str[, encoding])
         * Buffer.from(array)
         * Buffer.from(buffer)
         * Buffer.from(arrayBuffer[, byteOffset[, length]])
         **/
        export function from(value: any, encodingOrOffset?: any, length?: any): any;
        /**
         * Creates a new filled Buffer instance.
         * alloc(size[, fill[, encoding]])
         **/
        export function alloc(size: any, fill: any, encoding: any): Uint8Array;
        /**
         * Equivalent to Buffer(num), by default creates a non-zero-filled Buffer instance.
         * */
        export function allocUnsafe(size: any): Uint8Array;
        /**
         * Equivalent to SlowBuffer(num), by default creates a non-zero-filled Buffer instance.
         */
        export function allocUnsafeSlow(size: any): Uint8Array;
        export function isBuffer(b: any): boolean;
        export function compare(a: any, b: any): 0 | 1 | -1;
        export function isEncoding(encoding: any): boolean;
        export function concat(list: any, length?: any): Uint8Array;
        export { byteLength };
    }
    export const kMaxLength: 2147483647;
    export function SlowBuffer(length: any): Uint8Array;
    export const INSPECT_MAX_BYTES: 50;
    function byteLength(string: any, encoding: any, ...args: any[]): any;
}

declare module "socket:querystring" {
    export function unescapeBuffer(s: any, decodeSpaces: any): any;
    export function unescape(s: any, decodeSpaces: any): any;
    export function escape(str: any): any;
    export function stringify(obj: any, sep: any, eq: any, options: any): string;
    export function parse(qs: any, sep: any, eq: any, options: any): {};
    export function decode(qs: any, sep: any, eq: any, options: any): {};
    export function encode(obj: any, sep: any, eq: any, options: any): string;
    namespace _default {
        export { decode };
        export { encode };
        export { parse };
        export { stringify };
        export { escape };
        export { unescape };
    }
    export default _default;
}

declare module "socket:url/index" {
    export function parse(input: any, options?: any): {
        hash: any;
        host: any;
        hostname: any;
        origin: any;
        auth: string;
        password: any;
        pathname: any;
        path: any;
        port: any;
        protocol: any;
        search: any;
        searchParams: any;
        username: any;
        [Symbol.toStringTag]: string;
    };
    export function resolve(from: any, to: any): any;
    export function format(input: any): any;
    export function fileURLToPath(url: any): any;
    /**
     * @type {Set & { handlers: Set<string> }}
     */
    export const protocols: Set<any> & {
        handlers: Set<string>;
    };
    export default URL;
    export class URL {
        private constructor();
    }
    export const URLSearchParams: any;
    export const parseURL: any;
    import { URLPattern } from "socket:url/urlpattern/urlpattern";
    export { URLPattern };
}

declare module "socket:url" {
    export * from "socket:url/index";
    export default URL;
    import URL from "socket:url/index";
}

declare module "socket:fs/bookmarks" {
    /**
     * A map of known absolute file paths to file IDs that
     * have been granted access outside of the sandbox.
     * XXX(@jwerle): this is currently only used on linux, but valaues may
     * be added for all platforms, likely from a file system picker dialog.
     * @type {Map<string, string>}
     */
    export const temporary: Map<string, string>;
    namespace _default {
        export { temporary };
    }
    export default _default;
}

declare module "socket:internal/serialize" {
    export default function serialize(value: any): any;
}

declare module "socket:location" {
    export class Location {
        get url(): URL;
        get protocol(): string;
        get host(): string;
        get hostname(): string;
        get port(): string;
        get pathname(): string;
        get search(): string;
        get origin(): string;
        get href(): string;
        get hash(): string;
        toString(): string;
    }
    const _default: Location;
    export default _default;
}

declare module "socket:internal/symbols" {
    export const dispose: any;
    export const serialize: any;
    namespace _default {
        export { dispose };
        export { serialize };
    }
    export default _default;
}

declare module "socket:gc" {
    /**
     * Track `object` ref to call `Symbol.for('socket.runtime.gc.finalize')` method when
     * environment garbage collects object.
     * @param {object} object
     * @return {boolean}
     */
    export function ref(object: object, ...args: any[]): boolean;
    /**
     * Stop tracking `object` ref to call `Symbol.for('socket.runtime.gc.finalize')` method when
     * environment garbage collects object.
     * @param {object} object
     * @return {boolean}
     */
    export function unref(object: object): boolean;
    /**
     * An alias for `unref()`
     * @param {object} object}
     * @return {boolean}
     */
    export function retain(object: object): boolean;
    /**
     * Call finalize on `object` for `gc.finalizer` implementation.
     * @param {object} object]
     * @return {Promise<boolean>}
     */
    export function finalize(object: object, ...args: any[]): Promise<boolean>;
    /**
     * Calls all pending finalization handlers forcefully. This function
     * may have unintended consequences as objects be considered finalized
     * and still strongly held (retained) somewhere.
     */
    export function release(): Promise<void>;
    export const finalizers: WeakMap<object, any>;
    export const kFinalizer: unique symbol;
    export const finalizer: symbol;
    /**
     * @type {Set<WeakRef>}
     */
    export const pool: Set<WeakRef<any>>;
    /**
     * Static registry for objects to clean up underlying resources when they
     * are gc'd by the environment. There is no guarantee that the `finalizer()`
     * is called at any time.
     */
    export const registry: FinalizationRegistry<Finalizer>;
    /**
     * Default exports which also acts a retained value to persist bound
     * `Finalizer#handle()` functions from being gc'd before the
     * `FinalizationRegistry` callback is called because `heldValue` must be
     * strongly held (retained) in order for the callback to be called.
     */
    export const gc: any;
    export default gc;
    /**
     * A container for strongly (retain) referenced finalizer function
     * with arguments weakly referenced to an object that will be
     * garbage collected.
     */
    export class Finalizer {
        /**
         * Creates a `Finalizer` from input.
         */
        static from(handler: any): Finalizer;
        /**
         * `Finalizer` class constructor.
         * @private
         * @param {array} args
         * @param {function} handle
         */
        private constructor();
        args: any[];
        handle: any;
    }
}

declare module "socket:ipc" {
    export function maybeMakeError(error: any, caller: any): any;
    /**
     * Parses `seq` as integer value
     * @param {string|number} seq
     * @param {object=} [options]
     * @param {boolean} [options.bigint = false]
     * @ignore
     */
    export function parseSeq(seq: string | number, options?: object | undefined): number | bigint;
    /**
     * If `debug.enabled === true`, then debug output will be printed to console.
     * @param {(boolean)} [enable]
     * @return {boolean}
     * @ignore
     */
    export function debug(enable?: (boolean)): boolean;
    export namespace debug {
        let enabled: boolean;
        function log(...args: any[]): any;
    }
    /**
     * Find transfers for an in worker global `postMessage`
     * that is proxied to the main thread.
     * @ignore
     */
    export function findMessageTransfers(transfers: any, object: any): any;
    /**
     * @ignore
     */
    export function postMessage(message: any, ...args: any[]): any;
    /**
     * Waits for the native IPC layer to be ready and exposed on the
     * global window object.
     * @ignore
     */
    export function ready(): Promise<any>;
    /**
     * Sends a synchronous IPC command over XHR returning a `Result`
     * upon success or error.
     * @param {string} command
     * @param {any?} [value]
     * @param {object?} [options]
     * @return {Result}
     * @ignore
     */
    export function sendSync(command: string, value?: any | null, options?: object | null, buffer?: any): Result;
    /**
     * Emit event to be dispatched on `window` object.
     * @param {string} name
     * @param {any} value
     * @param {EventTarget=} [target = window]
     * @param {Object=} options
     */
    export function emit(name: string, value: any, target?: EventTarget | undefined, options?: any | undefined): Promise<void>;
    /**
     * Resolves a request by `seq` with possible value.
     * @param {string} seq
     * @param {any} value
     * @ignore
     */
    export function resolve(seq: string, value: any): Promise<void>;
    /**
     * Sends an async IPC command request with parameters.
     * @param {string} command
     * @param {any=} value
     * @param {object=} [options]
     * @param {boolean=} [options.cache=false]
     * @param {boolean=} [options.bytes=false]
     * @return {Promise<Result>}
     */
    export function send(command: string, value?: any | undefined, options?: object | undefined): Promise<Result>;
    /**
     * Sends an async IPC command request with parameters and buffered bytes.
     * @param {string} command
     * @param {any=} value
     * @param {(Buffer|Uint8Array|ArrayBuffer|string|Array)=} buffer
     * @param {object=} options
     * @ignore
     */
    export function write(command: string, value?: any | undefined, buffer?: (Buffer | Uint8Array | ArrayBuffer | string | any[]) | undefined, options?: object | undefined): Promise<any>;
    /**
     * Sends an async IPC command request with parameters requesting a response
     * with buffered bytes.
     * @param {string} command
     * @param {any=} value
     * @param {object=} options
     * @ignore
     */
    export function request(command: string, value?: any | undefined, options?: object | undefined): Promise<any>;
    /**
     * Factory for creating a proxy based IPC API.
     * @param {string} domain
     * @param {(function|object)=} ctx
     * @param {string=} [ctx.default]
     * @return {Proxy}
     * @ignore
     */
    export function createBinding(domain: string, ctx?: (Function | object) | undefined): ProxyConstructor;
    export function inflateIPCMessageTransfers(object: any, types?: Map<any, any>): any;
    export function findIPCMessageTransfers(transfers: any, object: any): any;
    /**
     * Represents an OK IPC status.
     * @ignore
     */
    export const OK: 0;
    /**
     * Represents an ERROR IPC status.
     * @ignore
     */
    export const ERROR: 1;
    /**
     * Timeout in milliseconds for IPC requests.
     * @ignore
     */
    export const TIMEOUT: number;
    /**
     * Symbol for the `ipc.debug.enabled` property
     * @ignore
     */
    export const kDebugEnabled: unique symbol;
    /**
     * @ignore
     */
    export class Headers extends globalThis.Headers {
        /**
         * @ignore
         */
        static from(input: any): Headers;
        /**
         * @ignore
         */
        get length(): number;
        /**
         * @ignore
         */
        toJSON(): {
            [k: string]: string;
        };
    }
    const Message_base: any;
    /**
     * A container for a IPC message based on a `ipc://` URI scheme.
     * @ignore
     */
    export class Message extends Message_base {
        [x: string]: any;
        /**
         * The expected protocol for an IPC message.
         * @ignore
         */
        static get PROTOCOL(): string;
        /**
         * Creates a `Message` instance from a variety of input.
         * @param {string|URL|Message|Buffer|object} input
         * @param {(object|string|URLSearchParams)=} [params]
         * @param {(ArrayBuffer|Uint8Array|string)?} [bytes]
         * @return {Message}
         * @ignore
         */
        static from(input: string | URL | Message | Buffer | object, params?: (object | string | URLSearchParams) | undefined, bytes?: (ArrayBuffer | Uint8Array | string) | null): Message;
        /**
         * Predicate to determine if `input` is valid for constructing
         * a new `Message` instance.
         * @param {string|URL|Message|Buffer|object} input
         * @return {boolean}
         * @ignore
         */
        static isValidInput(input: string | URL | Message | Buffer | object): boolean;
        /**
         * `Message` class constructor.
         * @protected
         * @param {string|URL} input
         * @param {(object|Uint8Array)?} [bytes]
         * @ignore
         */
        protected constructor();
        /**
         *  @type {Uint8Array?}
         *  @ignore
         */
        bytes: Uint8Array | null;
        /**
         * Computed IPC message name.
         * @type {string}
         * @ignore
         */
        get command(): string;
        /**
         * Computed IPC message name.
         * @type {string}
         * @ignore
         */
        get name(): string;
        /**
         * Computed `id` value for the command.
         * @type {string}
         * @ignore
         */
        get id(): string;
        /**
         * Computed `seq` (sequence) value for the command.
         * @type {string}
         * @ignore
         */
        get seq(): string;
        /**
         * Computed message value potentially given in message parameters.
         * This value is automatically decoded, but not treated as JSON.
         * @type {string}
         * @ignore
         */
        get value(): string;
        /**
         * Computed `index` value for the command potentially referring to
         * the window index the command is scoped to or originating from. If not
         * specified in the message parameters, then this value defaults to `-1`.
         * @type {number}
         * @ignore
         */
        get index(): number;
        /**
         * Computed value parsed as JSON. This value is `null` if the value is not present
         * or it is invalid JSON.
         * @type {object?}
         * @ignore
         */
        get json(): any;
        /**
         * Computed readonly object of message parameters.
         * @type {object}
         * @ignore
         */
        get params(): any;
        /**
         * Gets unparsed message parameters.
         * @type {Array<Array<string>>}
         * @ignore
         */
        get rawParams(): string[][];
        /**
         * Returns computed parameters as entries
         * @return {Array<Array<any>>}
         * @ignore
         */
        entries(): Array<Array<any>>;
        /**
         * Set a parameter `value` by `key`.
         * @param {string} key
         * @param {any} value
         * @ignore
         */
        set(key: string, value: any): any;
        /**
         * Get a parameter value by `key`.
         * @param {string} key
         * @param {any=} [defaultValue]
         * @return {any}
         * @ignore
         */
        get(key: string, defaultValue?: any | undefined): any;
        /**
         * Delete a parameter by `key`.
         * @param {string} key
         * @return {boolean}
         * @ignore
         */
        delete(key: string): boolean;
        /**
         * Computed parameter keys.
         * @return {Array<string>}
         * @ignore
         */
        keys(): Array<string>;
        /**
         * Computed parameter values.
         * @return {Array<any>}
         * @ignore
         */
        values(): Array<any>;
        /**
         * Predicate to determine if parameter `key` is present in parameters.
         * @param {string} key
         * @return {boolean}
         * @ignore
         */
        has(key: string): boolean;
    }
    /**
     * A result type used internally for handling
     * IPC result values from the native layer that are in the form
     * of `{ err?, data? }`. The `data` and `err` properties on this
     * type of object are in tuple form and be accessed at `[data?,err?]`
     * @ignore
     */
    export class Result {
        /**
         * Creates a `Result` instance from input that may be an object
         * like `{ err?, data? }`, an `Error` instance, or just `data`.
         * @param {(object|Error|any)?} result
         * @param {Error|object} [maybeError]
         * @param {string} [maybeSource]
         * @param {object|string|Headers} [maybeHeaders]
         * @return {Result}
         * @ignore
         */
        static from(result: (object | Error | any) | null, maybeError?: Error | object, maybeSource?: string, maybeHeaders?: object | string | Headers): Result;
        /**
         * `Result` class constructor.
         * @private
         * @param {string?} [id = null]
         * @param {Error?} [err = null]
         * @param {object?} [data = null]
         * @param {string?} [source = null]
         * @param {(object|string|Headers)?} [headers = null]
         * @ignore
         */
        private constructor();
        /**
         * The unique ID for this result.
         * @type {string}
         * @ignore
         */
        id: string;
        /**
         * An optional error in the result.
         * @type {Error?}
         * @ignore
         */
        err: Error | null;
        /**
         * Result data if given.
         * @type {(string|object|Uint8Array)?}
         * @ignore
         */
        data: (string | object | Uint8Array) | null;
        /**
         * The source of this result.
         * @type {string?}
         * @ignore
         */
        source: string | null;
        /**
         * Result headers, if given.
         * @type {Headers?}
         * @ignore
         */
        headers: Headers | null;
        /**
         * Computed result length.
         * @ignore
         */
        get length(): any;
        /**
         * @ignore
         */
        toJSON(): {
            headers: {
                [k: string]: string;
            };
            source: string;
            data: any;
            err: {
                name: string;
                message: string;
                stack?: string;
                cause?: unknown;
                type: any;
                code: any;
            };
        };
        /**
         * Generator for an `Iterable` interface over this instance.
         * @ignore
         */
        [Symbol.iterator](): Generator<any, void, unknown>;
    }
    export class IPCSearchParams extends URLSearchParams {
        constructor(params: any, nonce?: any);
    }
    /**
     * @ignore
     */
    export const primordials: any;
    export const ports: Map<any, any>;
    export class IPCMessagePort extends MessagePort {
        static from(options?: any): any;
        static create(options?: any): any;
        get id(): any;
        get started(): any;
        get closed(): any;
        set onmessage(onmessage: any);
        get onmessage(): any;
        set onmessageerror(onmessageerror: any);
        get onmessageerror(): any;
        close(purge?: boolean): void;
        postMessage(message: any, optionsOrTransferList: any): void;
        addEventListener(...args: any[]): any;
        removeEventListener(...args: any[]): any;
        dispatchEvent(event: any): any;
    }
    export class IPCMessageChannel extends MessageChannel {
        constructor(options?: any);
        get id(): any;
        get port1(): any;
        get port2(): any;
        get channel(): any;
        #private;
    }
    export default exports;
    import { Buffer } from "socket:buffer";
    import { URL } from "socket:url/index";
    import * as exports from "socket:ipc";
    
}

declare module "socket:os/constants" {
    export type errno = number;
    /**
     * @typedef {number} errno
     * @typedef {number} signal
     */
    /**
     * A container for all known "errno" constant values.
     * Unsupported values have a default value of `0`.
     */
    export const errno: any;
    export type signal = number;
    /**
     * A container for all known "signal" constant values.
     * Unsupported values have a default value of `0`.
     */
    export const signal: any;
    namespace _default {
        export { errno };
        export { signal };
    }
    export default _default;
}

declare module "socket:errno" {
    /**
     * Converts an `errno` code to its corresponding string message.
     * @param {import('./os/constants.js').errno} {code}
     * @return {string}
     */
    export function toString(code: any): string;
    /**
     * Gets the code for a given 'errno' name.
     * @param {string|number} name
     * @return {errno}
     */
    export function getCode(name: string | number): errno;
    /**
     * Gets the name for a given 'errno' code
     * @return {string}
     * @param {string|number} code
     */
    export function getName(code: string | number): string;
    /**
     * Gets the message for a 'errno' code.
     * @param {number|string} code
     * @return {string}
     */
    export function getMessage(code: number | string): string;
    /**
     * @typedef {import('./os/constants.js').errno} errno
     */
    export const E2BIG: any;
    export const EACCES: any;
    export const EADDRINUSE: any;
    export const EADDRNOTAVAIL: any;
    export const EAFNOSUPPORT: any;
    export const EAGAIN: any;
    export const EALREADY: any;
    export const EBADF: any;
    export const EBADMSG: any;
    export const EBUSY: any;
    export const ECANCELED: any;
    export const ECHILD: any;
    export const ECONNABORTED: any;
    export const ECONNREFUSED: any;
    export const ECONNRESET: any;
    export const EDEADLK: any;
    export const EDESTADDRREQ: any;
    export const EDOM: any;
    export const EDQUOT: any;
    export const EEXIST: any;
    export const EFAULT: any;
    export const EFBIG: any;
    export const EHOSTUNREACH: any;
    export const EIDRM: any;
    export const EILSEQ: any;
    export const EINPROGRESS: any;
    export const EINTR: any;
    export const EINVAL: any;
    export const EIO: any;
    export const EISCONN: any;
    export const EISDIR: any;
    export const ELOOP: any;
    export const EMFILE: any;
    export const EMLINK: any;
    export const EMSGSIZE: any;
    export const EMULTIHOP: any;
    export const ENAMETOOLONG: any;
    export const ENETDOWN: any;
    export const ENETRESET: any;
    export const ENETUNREACH: any;
    export const ENFILE: any;
    export const ENOBUFS: any;
    export const ENODATA: any;
    export const ENODEV: any;
    export const ENOENT: any;
    export const ENOEXEC: any;
    export const ENOLCK: any;
    export const ENOLINK: any;
    export const ENOMEM: any;
    export const ENOMSG: any;
    export const ENOPROTOOPT: any;
    export const ENOSPC: any;
    export const ENOSR: any;
    export const ENOSTR: any;
    export const ENOSYS: any;
    export const ENOTCONN: any;
    export const ENOTDIR: any;
    export const ENOTEMPTY: any;
    export const ENOTSOCK: any;
    export const ENOTSUP: any;
    export const ENOTTY: any;
    export const ENXIO: any;
    export const EOPNOTSUPP: any;
    export const EOVERFLOW: any;
    export const EPERM: any;
    export const EPIPE: any;
    export const EPROTO: any;
    export const EPROTONOSUPPORT: any;
    export const EPROTOTYPE: any;
    export const ERANGE: any;
    export const EROFS: any;
    export const ESPIPE: any;
    export const ESRCH: any;
    export const ESTALE: any;
    export const ETIME: any;
    export const ETIMEDOUT: any;
    export const ETXTBSY: any;
    export const EWOULDBLOCK: any;
    export const EXDEV: any;
    export const strings: any;
    export { constants };
    namespace _default {
        export { constants };
        export { strings };
        export { toString };
        export { getCode };
        export { getMessage };
    }
    export default _default;
    export type errno = import("socket:os/constants").errno;
    import { errno as constants } from "socket:os/constants";
}

declare module "socket:errors" {
    export default exports;
    export const ABORT_ERR: any;
    export const ENCODING_ERR: any;
    export const INVALID_ACCESS_ERR: any;
    export const INDEX_SIZE_ERR: any;
    export const NETWORK_ERR: any;
    export const NOT_ALLOWED_ERR: any;
    export const NOT_FOUND_ERR: any;
    export const NOT_SUPPORTED_ERR: any;
    export const OPERATION_ERR: any;
    export const SECURITY_ERR: any;
    export const TIMEOUT_ERR: any;
    /**
     * An `AbortError` is an error type thrown in an `onabort()` level 0
     * event handler on an `AbortSignal` instance.
     */
    export class AbortError extends Error {
        /**
         * The code given to an `ABORT_ERR` `DOMException`
         * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): any;
        /**
         * `AbortError` class constructor.
         * @param {AbortSignal|string} reasonOrSignal
         * @param {AbortSignal=} [signal]
         */
        constructor(reason: any, signal?: AbortSignal | undefined, ...args: any[]);
        signal: AbortSignal;
        get name(): string;
        get code(): string;
    }
    /**
     * An `BadRequestError` is an error type thrown in an `onabort()` level 0
     * event handler on an `BadRequestSignal` instance.
     */
    export class BadRequestError extends Error {
        /**
         * The default code given to a `BadRequestError`
         */
        static get code(): number;
        /**
         * `BadRequestError` class constructor.
         * @param {string} message
         * @param {number} [code]
         */
        constructor(message: string, ...args: any[]);
        get name(): string;
        get code(): string;
    }
    /**
     * An `EncodingError` is an error type thrown when an internal exception
     * has occurred, such as in the native IPC layer.
     */
    export class EncodingError extends Error {
        /**
         * The code given to an `ENCODING_ERR` `DOMException`.
         */
        static get code(): any;
        /**
         * `EncodingError` class constructor.
         * @param {string} message
         * @param {number} [code]
         */
        constructor(message: string, ...args: any[]);
        get name(): string;
        get code(): string;
    }
    /**
     * An error type derived from an `errno` code.
     */
    export class ErrnoError extends Error {
        static get code(): string;
        static errno: any;
        /**
         * `ErrnoError` class constructor.
         * @param {import('./errno').errno|string} code
         */
        constructor(code: import("socket:errno").errno | string, message?: any, ...args: any[]);
        get name(): string;
        get code(): number;
        #private;
    }
    /**
     * An `FinalizationRegistryCallbackError` is an error type thrown when an internal exception
     * has occurred, such as in the native IPC layer.
     */
    export class FinalizationRegistryCallbackError extends Error {
        /**
         * The default code given to an `FinalizationRegistryCallbackError`
         */
        static get code(): number;
        /**
         * `FinalizationRegistryCallbackError` class constructor.
         * @param {string} message
         * @param {number} [code]
         */
        constructor(message: string, ...args: any[]);
        get name(): string;
        get code(): string;
    }
    /**
     * An `IllegalConstructorError` is an error type thrown when a constructor is
     * called for a class constructor when it shouldn't be.
     */
    export class IllegalConstructorError extends TypeError {
        /**
         * The default code given to an `IllegalConstructorError`
         */
        static get code(): number;
        /**
         * `IllegalConstructorError` class constructor.
         * @param {string} message
         * @param {number} [code]
         */
        constructor(message: string, ...args: any[]);
        get name(): string;
        get code(): string;
    }
    /**
     * An `IndexSizeError` is an error type thrown when an internal exception
     * has occurred, such as in the native IPC layer.
     */
    export class IndexSizeError extends Error {
        /**
         * The code given to an `INDEX_SIZE_ERR` `DOMException`
         */
        static get code(): any;
        /**
         * `IndexSizeError` class constructor.
         * @param {string} message
         * @param {number} [code]
         */
        constructor(message: string, ...args: any[]);
        get name(): string;
        get code(): string;
    }
    export const kInternalErrorCode: unique symbol;
    /**
     * An `InternalError` is an error type thrown when an internal exception
     * has occurred, such as in the native IPC layer.
     */
    export class InternalError extends Error {
        /**
         * The default code given to an `InternalError`
         */
        static get code(): number;
        /**
         * `InternalError` class constructor.
         * @param {string} message
         * @param {number} [code]
         */
        constructor(message: string, code?: number, ...args: any[]);
        get name(): string;
        /**
         * @param {number|string}
         */
        set code(code: string | number);
        /**
         * @type {number|string}
         */
        get code(): string | number;
        [exports.kInternalErrorCode]: number;
    }
    /**
     * An `InvalidAccessError` is an error type thrown when an internal exception
     * has occurred, such as in the native IPC layer.
     */
    export class InvalidAccessError extends Error {
        /**
         * The code given to an `INVALID_ACCESS_ERR` `DOMException`
         * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): any;
        /**
         * `InvalidAccessError` class constructor.
         * @param {string} message
         * @param {number} [code]
         */
        constructor(message: string, ...args: any[]);
        get name(): string;
        get code(): string;
    }
    /**
     * An `NetworkError` is an error type thrown when an internal exception
     * has occurred, such as in the native IPC layer.
     */
    export class NetworkError extends Error {
        /**
         * The code given to an `NETWORK_ERR` `DOMException`
         * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): any;
        /**
         * `NetworkError` class constructor.
         * @param {string} message
         * @param {number} [code]
         */
        constructor(message: string, ...args: any[]);
        get name(): string;
        get code(): string;
    }
    /**
     * An `NotAllowedError` is an error type thrown when an internal exception
     * has occurred, such as in the native IPC layer.
     */
    export class NotAllowedError extends Error {
        /**
         * The code given to an `NOT_ALLOWED_ERR` `DOMException`
         * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): any;
        /**
         * `NotAllowedError` class constructor.
         * @param {string} message
         * @param {number} [code]
         */
        constructor(message: string, ...args: any[]);
        get name(): string;
        get code(): string;
    }
    /**
     * An `NotFoundError` is an error type thrown when an internal exception
     * has occurred, such as in the native IPC layer.
     */
    export class NotFoundError extends Error {
        /**
         * The code given to an `NOT_FOUND_ERR` `DOMException`
         * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): any;
        /**
         * `NotFoundError` class constructor.
         * @param {string} message
         * @param {number} [code]
         */
        constructor(message: string, ...args: any[]);
        get name(): string;
        get code(): string;
    }
    /**
     * An `NotSupportedError` is an error type thrown when an internal exception
     * has occurred, such as in the native IPC layer.
     */
    export class NotSupportedError extends Error {
        /**
         * The code given to an `NOT_SUPPORTED_ERR` `DOMException`
         * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): any;
        /**
         * `NotSupportedError` class constructor.
         * @param {string} message
         * @param {number} [code]
         */
        constructor(message: string, ...args: any[]);
        get name(): string;
        get code(): string;
    }
    /**
     * An `ModuleNotFoundError` is an error type thrown when an imported or
     * required module is not found.
     */
    export class ModuleNotFoundError extends exports.NotFoundError {
        /**
         * `ModuleNotFoundError` class constructor.
         * @param {string} message
         * @param {string[]=} [requireStack]
         */
        constructor(message: string, requireStack?: string[] | undefined);
        requireStack: string[];
    }
    /**
     * An `OperationError` is an error type thrown when an internal exception
     * has occurred, such as in the native IPC layer.
     */
    export class OperationError extends Error {
        /**
         * The code given to an `OPERATION_ERR` `DOMException`
         */
        static get code(): any;
        /**
         * `OperationError` class constructor.
         * @param {string} message
         * @param {number} [code]
         */
        constructor(message: string, ...args: any[]);
        get name(): string;
        get code(): string;
    }
    /**
     * An `SecurityError` is an error type thrown when an internal exception
     * has occurred, such as in the native IPC layer.
     */
    export class SecurityError extends Error {
        /**
         * The code given to an `SECURITY_ERR` `DOMException`
         */
        static get code(): any;
        /**
         * `SecurityError` class constructor.
         * @param {string} message
         * @param {number} [code]
         */
        constructor(message: string, ...args: any[]);
        get name(): string;
        get code(): string;
    }
    /**
     * An `TimeoutError` is an error type thrown when an operation timesout.
     */
    export class TimeoutError extends Error {
        /**
         * The code given to an `TIMEOUT_ERR` `DOMException`
         * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): any;
        /**
         * `TimeoutError` class constructor.
         * @param {string} message
         */
        constructor(message: string, ...args: any[]);
        get name(): string;
        get code(): string;
    }
    import * as exports from "socket:errors";
    
}

declare module "socket:util/types" {
    /**
     * Returns `true` if input is a plan `Object` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isPlainObject(input: any): boolean;
    /**
     * Returns `true` if input is an `AsyncFunction`
     * @param {any} input
     * @return {boolean}
     */
    export function isAsyncFunction(input: any): boolean;
    /**
     * Returns `true` if input is an `Function`
     * @param {any} input
     * @return {boolean}
     */
    export function isFunction(input: any): boolean;
    /**
     * Returns `true` if input is an `AsyncFunction` object.
     * @param {any} input
     * @return {boolean}
     */
    export function isAsyncFunctionObject(input: any): boolean;
    /**
     * Returns `true` if input is an `Function` object.
     * @param {any} input
     * @return {boolean}
     */
    export function isFunctionObject(input: any): boolean;
    /**
     * Always returns `false`.
     * @param {any} input
     * @return {boolean}
     */
    export function isExternal(input: any): boolean;
    /**
     * Returns `true` if input is a `Date` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isDate(input: any): boolean;
    /**
     * Returns `true` if input is an `arguments` object.
     * @param {any} input
     * @return {boolean}
     */
    export function isArgumentsObject(input: any): boolean;
    /**
     * Returns `true` if input is a `BigInt` object.
     * @param {any} input
     * @return {boolean}
     */
    export function isBigIntObject(input: any): boolean;
    /**
     * Returns `true` if input is a `Boolean` object.
     * @param {any} input
     * @return {boolean}
     */
    export function isBooleanObject(input: any): boolean;
    /**
     * Returns `true` if input is a `Number` object.
     * @param {any} input
     * @return {boolean}
     */
    export function isNumberObject(input: any): boolean;
    /**
     * Returns `true` if input is a `String` object.
     * @param {any} input
     * @return {boolean}
     */
    export function isStringObject(input: any): boolean;
    /**
     * Returns `true` if input is a `Symbol` object.
     * @param {any} input
     * @return {boolean}
     */
    export function isSymbolObject(input: any): boolean;
    /**
     * Returns `true` if input is native `Error` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isNativeError(input: any): boolean;
    /**
     * Returns `true` if input is a `RegExp` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isRegExp(input: any): boolean;
    /**
     * Returns `true` if input is a `GeneratorFunction`.
     * @param {any} input
     * @return {boolean}
     */
    export function isGeneratorFunction(input: any): boolean;
    /**
     * Returns `true` if input is an `AsyncGeneratorFunction`.
     * @param {any} input
     * @return {boolean}
     */
    export function isAsyncGeneratorFunction(input: any): boolean;
    /**
     * Returns `true` if input is an instance of a `Generator`.
     * @param {any} input
     * @return {boolean}
     */
    export function isGeneratorObject(input: any): boolean;
    /**
     * Returns `true` if input is a `Promise` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isPromise(input: any): boolean;
    /**
     * Returns `true` if input is a `Map` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isMap(input: any): boolean;
    /**
     * Returns `true` if input is a `Set` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isSet(input: any): boolean;
    /**
     * Returns `true` if input is an instance of an `Iterator`.
     * @param {any} input
     * @return {boolean}
     */
    export function isIterator(input: any): boolean;
    /**
     * Returns `true` if input is an instance of an `AsyncIterator`.
     * @param {any} input
     * @return {boolean}
     */
    export function isAsyncIterator(input: any): boolean;
    /**
     * Returns `true` if input is an instance of a `MapIterator`.
     * @param {any} input
     * @return {boolean}
     */
    export function isMapIterator(input: any): boolean;
    /**
     * Returns `true` if input is an instance of a `SetIterator`.
     * @param {any} input
     * @return {boolean}
     */
    export function isSetIterator(input: any): boolean;
    /**
     * Returns `true` if input is a `WeakMap` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isWeakMap(input: any): boolean;
    /**
     * Returns `true` if input is a `WeakSet` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isWeakSet(input: any): boolean;
    /**
     * Returns `true` if input is an `ArrayBuffer` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isArrayBuffer(input: any): boolean;
    /**
     * Returns `true` if input is an `DataView` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isDataView(input: any): boolean;
    /**
     * Returns `true` if input is a `SharedArrayBuffer`.
     * This will always return `false` if a `SharedArrayBuffer`
     * type is not available.
     * @param {any} input
     * @return {boolean}
     */
    export function isSharedArrayBuffer(input: any): boolean;
    /**
     * Not supported. This function will return `false` always.
     * @param {any} input
     * @return {boolean}
     */
    export function isProxy(input: any): boolean;
    /**
     * Returns `true` if input looks like a module namespace object.
     * @param {any} input
     * @return {boolean}
     */
    export function isModuleNamespaceObject(input: any): boolean;
    /**
     * Returns `true` if input is an `ArrayBuffer` of `SharedArrayBuffer`.
     * @param {any} input
     * @return {boolean}
     */
    export function isAnyArrayBuffer(input: any): boolean;
    /**
     * Returns `true` if input is a "boxed" primitive.
     * @param {any} input
     * @return {boolean}
     */
    export function isBoxedPrimitive(input: any): boolean;
    /**
     * Returns `true` if input is an `ArrayBuffer` view.
     * @param {any} input
     * @return {boolean}
     */
    export function isArrayBufferView(input: any): boolean;
    /**
     * Returns `true` if input is a `TypedArray` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isTypedArray(input: any): boolean;
    /**
     * Returns `true` if input is an `Uint8Array` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isUint8Array(input: any): boolean;
    /**
     * Returns `true` if input is an `Uint8ClampedArray` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isUint8ClampedArray(input: any): boolean;
    /**
     * Returns `true` if input is an `Uint16Array` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isUint16Array(input: any): boolean;
    /**
     * Returns `true` if input is an `Uint32Array` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isUint32Array(input: any): boolean;
    /**
     * Returns `true` if input is an Int8Array`` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isInt8Array(input: any): boolean;
    /**
     * Returns `true` if input is an `Int16Array` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isInt16Array(input: any): boolean;
    /**
     * Returns `true` if input is an `Int32Array` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isInt32Array(input: any): boolean;
    /**
     * Returns `true` if input is an `Float32Array` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isFloat32Array(input: any): boolean;
    /**
     * Returns `true` if input is an `Float64Array` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isFloat64Array(input: any): boolean;
    /**
     * Returns `true` if input is an `BigInt64Array` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isBigInt64Array(input: any): boolean;
    /**
     * Returns `true` if input is an `BigUint64Array` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isBigUint64Array(input: any): boolean;
    /**
     * @ignore
     * @param {any} input
     * @return {boolean}
     */
    export function isKeyObject(input: any): boolean;
    /**
     * Returns `true` if input is a `CryptoKey` instance.
     * @param {any} input
     * @return {boolean}
     */
    export function isCryptoKey(input: any): boolean;
    /**
     * Returns `true` if input is an `Array`.
     * @param {any} input
     * @return {boolean}
     */
    export const isArray: any;
    export default exports;
    import * as exports from "socket:util/types";
    
}

declare module "socket:mime/index" {
    /**
     * Look up a MIME type in various MIME databases.
     * @param {string} query
     * @return {Promise<DatabaseQueryResult[]>}
     */
    export function lookup(query: string): Promise<DatabaseQueryResult[]>;
    /**
     * Look up a MIME type in various MIME databases synchronously.
     * @param {string} query
     * @return {DatabaseQueryResult[]}
     */
    export function lookupSync(query: string): DatabaseQueryResult[];
    /**
     * A container for a database lookup query.
     */
    export class DatabaseQueryResult {
        /**
         * `DatabaseQueryResult` class constructor.
         * @ignore
         * @param {Database|null} database
         * @param {string} name
         * @param {string} mime
         */
        constructor(database: Database | null, name: string, mime: string);
        /**
         * @type {string}
         */
        name: string;
        /**
         * @type {string}
         */
        mime: string;
        /**
         * @type {Database?}
         */
        database: Database | null;
    }
    /**
     * A container for MIME types by class (audio, video, text, etc)
     * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml}
     */
    export class Database {
        /**
         * `Database` class constructor.
         * @param {string} name
         */
        constructor(name: string);
        /**
         * The name of the MIME database.
         * @type {string}
         */
        name: string;
        /**
         * The URL of the MIME database.
         * @type {URL}
         */
        url: URL;
        /**
         * The mapping of MIME name to the MIME "content type"
         * @type {Map}
         */
        map: Map<any, any>;
        /**
         * An index of MIME "content type" to the MIME name.
         * @type {Map}
         */
        index: Map<any, any>;
        /**
         * An enumeration of all database entries.
         * @return {Array<Array<string>>}
         */
        entries(): Array<Array<string>>;
        /**
         * Loads database MIME entries into internal map.
         * @return {Promise}
         */
        load(): Promise<any>;
        /**
         * Loads database MIME entries synchronously into internal map.
         */
        loadSync(): void;
        /**
         * Lookup MIME type by name or content type
         * @param {string} query
         * @return {Promise<DatabaseQueryResult[]>}
         */
        lookup(query: string): Promise<DatabaseQueryResult[]>;
        /**
         * Lookup MIME type by name or content type synchronously.
         * @param {string} query
         * @return {Promise<DatabaseQueryResult[]>}
         */
        lookupSync(query: string): Promise<DatabaseQueryResult[]>;
        /**
         * Queries database map and returns an array of results
         * @param {string} query
         * @return {DatabaseQueryResult[]}
         */
        query(query: string): DatabaseQueryResult[];
    }
    /**
     * A database of MIME types for 'application/' content types
     * @type {Database}
     * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#application}
     */
    export const application: Database;
    /**
     * A database of MIME types for 'audio/' content types
     * @type {Database}
     * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#audio}
     */
    export const audio: Database;
    /**
     * A database of MIME types for 'font/' content types
     * @type {Database}
     * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#font}
     */
    export const font: Database;
    /**
     * A database of MIME types for 'image/' content types
     * @type {Database}
     * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#image}
     */
    export const image: Database;
    /**
     * A database of MIME types for 'model/' content types
     * @type {Database}
     * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#model}
     */
    export const model: Database;
    /**
     * A database of MIME types for 'multipart/' content types
     * @type {Database}
     * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#multipart}
     */
    export const multipart: Database;
    /**
     * A database of MIME types for 'text/' content types
     * @type {Database}
     * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#text}
     */
    export const text: Database;
    /**
     * A database of MIME types for 'video/' content types
     * @type {Database}
     * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#video}
     */
    export const video: Database;
    /**
     * An array of known MIME databases. Custom databases can be added to this
     * array in userspace for lookup with `mime.lookup()`
     * @type {Database[]}
     */
    export const databases: Database[];
    export class MIMEParams extends Map<any, any> {
        constructor();
        constructor(entries?: readonly (readonly [any, any])[]);
        constructor();
        constructor(iterable?: Iterable<readonly [any, any]>);
    }
    export class MIMEType {
        constructor(input: any);
        set type(value: any);
        get type(): any;
        set subtype(value: any);
        get subtype(): any;
        get essence(): string;
        get params(): any;
        toString(): string;
        toJSON(): string;
        #private;
    }
    namespace _default {
        export { Database };
        export { databases };
        export { lookup };
        export { lookupSync };
        export { MIMEParams };
        export { MIMEType };
        export { application };
        export { audio };
        export { font };
        export { image };
        export { model };
        export { multipart };
        export { text };
        export { video };
    }
    export default _default;
}

declare module "socket:mime" {
    export * from "socket:mime/index";
    export default exports;
    import * as exports from "socket:mime/index";
}

declare module "socket:util" {
    export function debug(section: any): {
        (...args: any[]): void;
        enabled: boolean;
    };
    export function hasOwnProperty(object: any, property: any): any;
    export function isDate(object: any): boolean;
    export function isTypedArray(object: any): boolean;
    export function isArrayLike(input: any): boolean;
    export function isError(object: any): boolean;
    export function isSymbol(value: any): value is symbol;
    export function isNumber(value: any): boolean;
    export function isBoolean(value: any): boolean;
    export function isArrayBufferView(buf: any): boolean;
    export function isAsyncFunction(object: any): boolean;
    export function isArgumentsObject(object: any): boolean;
    export function isEmptyObject(object: any): boolean;
    export function isObject(object: any): boolean;
    export function isUndefined(value: any): boolean;
    export function isNull(value: any): boolean;
    export function isNullOrUndefined(value: any): boolean;
    export function isPrimitive(value: any): boolean;
    export function isRegExp(value: any): boolean;
    export function isPlainObject(object: any): boolean;
    export function isArrayBuffer(object: any): boolean;
    export function isBufferLike(object: any): boolean;
    export function isFunction(value: any): boolean;
    export function isErrorLike(error: any): boolean;
    export function isClass(value: any): boolean;
    export function isBuffer(value: any): boolean;
    export function isPromiseLike(object: any): boolean;
    export function toString(object: any): any;
    export function toBuffer(object: any, encoding?: any): any;
    export function toProperCase(string: any): any;
    export function splitBuffer(buffer: any, highWaterMark: any): any[];
    export function clamp(value: any, min: any, max: any): number;
    export function promisify(original: any): any;
    export function inspect(value: any, options: any): any;
    export namespace inspect {
        let ignore: symbol;
        let custom: symbol;
    }
    export function format(format: any, ...args: any[]): string;
    export function parseJSON(string: any): any;
    export function parseHeaders(headers: any): string[][];
    export function noop(): void;
    export function isValidPercentageValue(input: any): boolean;
    export function compareBuffers(a: any, b: any): any;
    export function inherits(Constructor: any, Super: any): void;
    /**
     * @ignore
     * @param {string} source
     * @return {boolean}
     */
    export function isESMSource(source: string): boolean;
    export function deprecate(...args: any[]): void;
    export { types };
    export const TextDecoder: {
        new (label?: string, options?: TextDecoderOptions): TextDecoder;
        prototype: TextDecoder;
    };
    export const TextEncoder: {
        new (): TextEncoder;
        prototype: TextEncoder;
    };
    export const isArray: any;
    export const inspectSymbols: symbol[];
    export class IllegalConstructor {
    }
    export const ESM_TEST_REGEX: RegExp;
    export const MIMEType: typeof mime.MIMEType;
    export const MIMEParams: typeof mime.MIMEParams;
    export default exports;
    import types from "socket:util/types";
    import mime from "socket:mime";
    import * as exports from "socket:util";
    
}

declare module "socket:async/wrap" {
    /**
     * Returns `true` if a given function `fn` has the "async" wrapped tag,
     * meaning it was "tagged" in a `wrap(fn)` call before, otherwise this
     * function will return `false`.
     * @ignore
     * @param {function} fn
     * @param {boolean}
     */
    export function isTagged(fn: Function): boolean;
    /**
     * Tags a function `fn` as being "async wrapped" so subsequent calls to
     * `wrap(fn)` do not wrap an already wrapped function.
     * @ignore
     * @param {function} fn
     * @return {function}
     */
    export function tag(fn: Function): Function;
    /**
     * Wraps a function `fn` that captures a snapshot of the current async
     * context. This function is idempotent and will not wrap a function more
     * than once.
     * @ignore
     * @param {function} fn
     * @return {function}
     */
    export function wrap(fn: Function): Function;
    export const symbol: unique symbol;
    export default wrap;
}

declare module "socket:internal/async/hooks" {
    export function dispatch(hook: any, asyncId: any, type: any, triggerAsyncId: any, resource: any): void;
    export function getNextAsyncResourceId(): number;
    export function executionAsyncResource(): any;
    export function executionAsyncId(): any;
    export function triggerAsyncId(): any;
    export function getDefaultExecutionAsyncId(): any;
    export function wrap(callback: any, type: any, asyncId?: number, triggerAsyncId?: any, resource?: any): (...args: any[]) => any;
    export function getTopLevelAsyncResourceName(): any;
    /**
     * The default top level async resource ID
     * @type {number}
     */
    export const TOP_LEVEL_ASYNC_RESOURCE_ID: number;
    export namespace state {
        let defaultExecutionAsyncId: number;
    }
    export namespace hooks {
        let init: any[];
        let before: any[];
        let after: any[];
        let destroy: any[];
        let promiseResolve: any[];
    }
    /**
     * A base class for the `AsyncResource` class or other higher level async
     * resource classes.
     */
    export class CoreAsyncResource {
        /**
         * `CoreAsyncResource` class constructor.
         * @param {string} type
         * @param {object|number=} [options]
         */
        constructor(type: string, options?: (object | number) | undefined);
        /**
         * The `CoreAsyncResource` type.
         * @type {string}
         */
        get type(): string;
        /**
         * `true` if the `CoreAsyncResource` was destroyed, otherwise `false`. This
         * value is only set to `true` if `emitDestroy()` was called, likely from
         * destroying the resource manually.
         * @type {boolean}
         */
        get destroyed(): boolean;
        /**
         * The unique async resource ID.
         * @return {number}
         */
        asyncId(): number;
        /**
         * The trigger async resource ID.
         * @return {number}
         */
        triggerAsyncId(): number;
        /**
         * Manually emits destroy hook for the resource.
         * @return {CoreAsyncResource}
         */
        emitDestroy(): CoreAsyncResource;
        /**
         * Binds function `fn` with an optional this `thisArg` binding to run
         * in the execution context of this `CoreAsyncResource`.
         * @param {function} fn
         * @param {object=} [thisArg]
         * @return {function}
         */
        bind(fn: Function, thisArg?: object | undefined): Function;
        /**
         * Runs function `fn` in the execution context of this `CoreAsyncResource`.
         * @param {function} fn
         * @param {object=} [thisArg]
         * @param {...any} [args]
         * @return {any}
         */
        runInAsyncScope(fn: Function, thisArg?: object | undefined, ...args?: any[]): any;
        #private;
    }
    export class TopLevelAsyncResource extends CoreAsyncResource {
    }
    export const asyncContextVariable: Variable<any>;
    export const topLevelAsyncResource: TopLevelAsyncResource;
    export default hooks;
    import { Variable } from "socket:async/context";
}

declare module "socket:async/resource" {
    /**
     * @typedef {{
     *   triggerAsyncId?: number,
     *   requireManualDestroy?: boolean
     * }} AsyncResourceOptions
     */
    /**
     * A container that should be extended that represents a resource with
     * an asynchronous execution context.
     */
    export class AsyncResource extends CoreAsyncResource {
        /**
         * Binds function `fn` with an optional this `thisArg` binding to run
         * in the execution context of an anonymous `AsyncResource`.
         * @param {function} fn
         * @param {object|string=} [type]
         * @param {object=} [thisArg]
         * @return {function}
         */
        static bind(fn: Function, type?: (object | string) | undefined, thisArg?: object | undefined): Function;
        /**
         * `AsyncResource` class constructor.
         * @param {string} type
         * @param {AsyncResourceOptions|number=} [options]
         */
        constructor(type: string, options?: (AsyncResourceOptions | number) | undefined);
    }
    export default AsyncResource;
    export type AsyncResourceOptions = {
        triggerAsyncId?: number;
        requireManualDestroy?: boolean;
    };
    import { executionAsyncResource } from "socket:internal/async/hooks";
    import { executionAsyncId } from "socket:internal/async/hooks";
    import { triggerAsyncId } from "socket:internal/async/hooks";
    import { CoreAsyncResource } from "socket:internal/async/hooks";
    export { executionAsyncResource, executionAsyncId, triggerAsyncId };
}

declare module "socket:async/hooks" {
    /**
     * Factory for creating a `AsyncHook` instance.
     * @param {AsyncHookCallbackOptions|AsyncHookCallbacks=} [callbacks]
     * @return {AsyncHook}
     */
    export function createHook(callbacks?: (AsyncHookCallbackOptions | AsyncHookCallbacks) | undefined): AsyncHook;
    /**
     * A container for `AsyncHooks` callbacks.
     * @ignore
     */
    export class AsyncHookCallbacks {
        /**
         * `AsyncHookCallbacks` class constructor.
         * @ignore
         * @param {AsyncHookCallbackOptions} [options]
         */
        constructor(options?: AsyncHookCallbackOptions);
        init(asyncId: any, type: any, triggerAsyncId: any, resource: any): void;
        before(asyncId: any): void;
        after(asyncId: any): void;
        destroy(asyncId: any): void;
        promiseResolve(asyncId: any): void;
    }
    /**
     * A container for registering various callbacks for async resource hooks.
     */
    export class AsyncHook {
        /**
         * @param {AsyncHookCallbackOptions|AsyncHookCallbacks=} [options]
         */
        constructor(callbacks?: any);
        /**
         * @type {boolean}
         */
        get enabled(): boolean;
        /**
         * Enable the async hook.
         * @return {AsyncHook}
         */
        enable(): AsyncHook;
        /**
         * Disables the async hook
         * @return {AsyncHook}
         */
        disable(): AsyncHook;
        #private;
    }
    export default createHook;
    import { executionAsyncResource } from "socket:internal/async/hooks";
    import { executionAsyncId } from "socket:internal/async/hooks";
    import { triggerAsyncId } from "socket:internal/async/hooks";
    export { executionAsyncResource, executionAsyncId, triggerAsyncId };
}

declare module "socket:async/storage" {
    /**
     * A container for storing values that remain present during
     * asynchronous operations.
     */
    export class AsyncLocalStorage {
        /**
         * Binds function `fn` to run in the execution context of an
         * anonymous `AsyncResource`.
         * @param {function} fn
         * @return {function}
         */
        static bind(fn: Function): Function;
        /**
         * Captures the current async context and returns a function that runs
         * a function in that execution context.
         * @return {function}
         */
        static snapshot(): Function;
        /**
         * @type {boolean}
         */
        get enabled(): boolean;
        /**
         * Disables the `AsyncLocalStorage` instance. When disabled,
         * `getStore()` will always return `undefined`.
         */
        disable(): void;
        /**
         * Enables the `AsyncLocalStorage` instance.
         */
        enable(): void;
        /**
         * Enables and sets the `AsyncLocalStorage` instance default store value.
         * @param {any} store
         */
        enterWith(store: any): void;
        /**
         * Runs function `fn` in the current asynchronous execution context with
         * a given `store` value and arguments given to `fn`.
         * @param {any} store
         * @param {function} fn
         * @param {...any} args
         * @return {any}
         */
        run(store: any, fn: Function, ...args: any[]): any;
        exit(fn: any, ...args: any[]): any;
        /**
         * If the `AsyncLocalStorage` instance is enabled, it returns the current
         * store value for this asynchronous execution context.
         * @return {any|undefined}
         */
        getStore(): any | undefined;
        #private;
    }
    export default AsyncLocalStorage;
}

declare module "socket:async/deferred" {
    /**
     * Dispatched when a `Deferred` internal promise is resolved.
     */
    export class DeferredResolveEvent extends Event {
        /**
         * `DeferredResolveEvent` class constructor
         * @ignore
         * @param {string=} [type]
         * @param {any=} [result]
         */
        constructor(type?: string | undefined, result?: any | undefined);
        /**
         * The `Deferred` promise result value.
         * @type {any?}
         */
        result: any | null;
    }
    /**
     * Dispatched when a `Deferred` internal promise is rejected.
     */
    export class DeferredRejectEvent {
        /**
         * `DeferredRejectEvent` class constructor
         * @ignore
         * @param {string=} [type]
         * @param {Error=} [error]
         */
        constructor(type?: string | undefined, error?: Error | undefined);
    }
    /**
     * A utility class for creating deferred promises.
     */
    export class Deferred extends EventTarget {
        /**
         * `Deferred` class constructor.
         * @param {Deferred|Promise?} [promise]
         */
        constructor(promise?: Deferred | (Promise<any> | null));
        /**
         * Function to resolve the associated promise.
         * @type {function}
         */
        resolve: Function;
        /**
         * Function to reject the associated promise.
         * @type {function}
         */
        reject: Function;
        /**
         * Attaches a fulfillment callback and a rejection callback to the promise,
         * and returns a new promise resolving to the return value of the called
         * callback.
         * @param {function(any)=} [resolve]
         * @param {function(Error)=} [reject]
         */
        then(resolve?: ((arg0: any) => any) | undefined, reject?: ((arg0: Error) => any) | undefined): Promise<any>;
        /**
         * Attaches a rejection callback to the promise, and returns a new promise
         * resolving to the return value of the callback if it is called, or to its
         * original fulfillment value if the promise is instead fulfilled.
         * @param {function(Error)=} [callback]
         */
        catch(callback?: ((arg0: Error) => any) | undefined): Promise<any>;
        /**
         * Attaches a callback for when the promise is settled (fulfilled or rejected).
         * @param {function(any?)} [callback]
         */
        finally(callback?: (arg0: any | null) => any): Promise<any>;
        /**
         * The promise associated with this Deferred instance.
         * @type {Promise<any>}
         */
        get promise(): Promise<any>;
        /**
         * A string representation of this Deferred instance.
         * @type {string}
         * @ignore
         */
        get [Symbol.toStringTag](): string;
        #private;
    }
    export default Deferred;
}

declare module "socket:async" {
    export default exports;
    import AsyncLocalStorage from "socket:async/storage";
    import AsyncResource from "socket:async/resource";
    import AsyncContext from "socket:async/context";
    import Deferred from "socket:async/deferred";
    import { executionAsyncResource } from "socket:async/hooks";
    import { executionAsyncId } from "socket:async/hooks";
    import { triggerAsyncId } from "socket:async/hooks";
    import { createHook } from "socket:async/hooks";
    import { AsyncHook } from "socket:async/hooks";
    import * as exports from "socket:async";
    
    export { AsyncLocalStorage, AsyncResource, AsyncContext, Deferred, executionAsyncResource, executionAsyncId, triggerAsyncId, createHook, AsyncHook };
}

declare module "socket:application/menu" {
    /**
     * Internal IPC for setting an application menu
     * @ignore
     */
    export function setMenu(options: any, type: any): Promise<ipc.Result>;
    /**
     * Internal IPC for setting an application context menu
     * @ignore
     */
    export function setContextMenu(options: any): Promise<any>;
    /**
     * A `Menu` is base class for a `ContextMenu`, `SystemMenu`, or `TrayMenu`.
     */
    export class Menu extends EventTarget {
        /**
         * `Menu` class constructor.
         * @ignore
         * @param {string} type
         */
        constructor(type: string);
        /**
         * The broadcast channel for this menu.
         * @ignore
         * @type {BroadcastChannel}
         */
        get channel(): BroadcastChannel;
        /**
         * The `Menu` instance type.
         * @type {('context'|'system'|'tray')?}
         */
        get type(): "tray" | "system" | "context";
        /**
         * Setter for the level 1 'error'` event listener.
         * @ignore
         * @type {function(ErrorEvent)?}
         */
        set onerror(onerror: (arg0: ErrorEvent) => any);
        /**
         * Level 1 'error'` event listener.
         * @type {function(ErrorEvent)?}
         */
        get onerror(): (arg0: ErrorEvent) => any;
        /**
         * Setter for the level 1 'menuitem'` event listener.
         * @ignore
         * @type {function(MenuItemEvent)?}
         */
        set onmenuitem(onmenuitem: (arg0: menuitemEvent) => any);
        /**
         * Level 1 'menuitem'` event listener.
         * @type {function(menuitemEvent)?}
         */
        get onmenuitem(): (arg0: menuitemEvent) => any;
        /**
         * Set the menu layout for this `Menu` instance.
         * @param {string|object} layoutOrOptions
         * @param {object=} [options]
         */
        set(layoutOrOptions: string | object, options?: object | undefined): Promise<any>;
        #private;
    }
    /**
     * A container for various `Menu` instances.
     */
    export class MenuContainer extends EventTarget {
        /**
         * `MenuContainer` class constructor.
         * @param {EventTarget} [sourceEventTarget]
         * @param {object=} [options]
         */
        constructor(sourceEventTarget?: EventTarget, options?: object | undefined);
        /**
         * Setter for the level 1 'error'` event listener.
         * @ignore
         * @type {function(ErrorEvent)?}
         */
        set onerror(onerror: (arg0: ErrorEvent) => any);
        /**
         * Level 1 'error'` event listener.
         * @type {function(ErrorEvent)?}
         */
        get onerror(): (arg0: ErrorEvent) => any;
        /**
         * Setter for the level 1 'menuitem'` event listener.
         * @ignore
         * @type {function(MenuItemEvent)?}
         */
        set onmenuitem(onmenuitem: (arg0: menuitemEvent) => any);
        /**
         * Level 1 'menuitem'` event listener.
         * @type {function(menuitemEvent)?}
         */
        get onmenuitem(): (arg0: menuitemEvent) => any;
        /**
         * The `TrayMenu` instance for the application.
         * @type {TrayMenu}
         */
        get tray(): TrayMenu;
        /**
         * The `SystemMenu` instance for the application.
         * @type {SystemMenu}
         */
        get system(): SystemMenu;
        /**
         * The `ContextMenu` instance for the application.
         * @type {ContextMenu}
         */
        get context(): ContextMenu;
        #private;
    }
    /**
     * A `Menu` instance that represents a context menu.
     */
    export class ContextMenu extends Menu {
        constructor();
    }
    /**
     * A `Menu` instance that represents the system menu.
     */
    export class SystemMenu extends Menu {
        constructor();
    }
    /**
     * A `Menu` instance that represents the tray menu.
     */
    export class TrayMenu extends Menu {
        constructor();
    }
    /**
     * The application tray menu.
     * @type {TrayMenu}
     */
    export const tray: TrayMenu;
    /**
     * The application system menu.
     * @type {SystemMenu}
     */
    export const system: SystemMenu;
    /**
     * The application context menu.
     * @type {ContextMenu}
     */
    export const context: ContextMenu;
    /**
     * The application menus container.
     * @type {MenuContainer}
     */
    export const container: MenuContainer;
    export default container;
    import ipc from "socket:ipc";
}

declare module "socket:internal/events" {
    /**
     * An event dispatched when an application URL is opening the application.
     */
    export class ApplicationURLEvent extends Event {
        /**
         * `ApplicationURLEvent` class constructor.
         * @param {string=} [type]
         * @param {object=} [options]
         */
        constructor(type?: string | undefined, options?: object | undefined);
        /**
         * `true` if the application URL is valid (parses correctly).
         * @type {boolean}
         */
        get isValid(): boolean;
        /**
         * Data associated with the `ApplicationURLEvent`.
         * @type {?any}
         */
        get data(): any;
        /**
         * The original source URI
         * @type {?string}
         */
        get source(): string;
        /**
         * The `URL` for the `ApplicationURLEvent`.
         * @type {?URL}
         */
        get url(): URL;
        /**
         * String tag name for an `ApplicationURLEvent` instance.
         * @type {string}
         */
        get [Symbol.toStringTag](): string;
        #private;
    }
    /**
     * An event dispacted for a registered global hotkey expression.
     */
    export class HotKeyEvent extends MessageEvent<any> {
        /**
         * `HotKeyEvent` class constructor.
         * @ignore
         * @param {string=} [type]
         * @param {object=} [data]
         */
        constructor(type?: string | undefined, data?: object | undefined);
        /**
         * The global unique ID for this hotkey binding.
         * @type {number?}
         */
        get id(): number;
        /**
         * The computed hash for this hotkey binding.
         * @type {number?}
         */
        get hash(): number;
        /**
         * The normalized hotkey expression as a sequence of tokens.
         * @type {string[]}
         */
        get sequence(): string[];
        /**
         * The original expression of the hotkey binding.
         * @type {string?}
         */
        get expression(): string;
    }
    /**
     * An event dispacted when a menu item is selected.
     */
    export class MenuItemEvent extends MessageEvent<any> {
        /**
         * `MenuItemEvent` class constructor
         * @ignore
         * @param {string=} [type]
         * @param {object=} [data]
         * @param {import('../application/menu.js').Menu} menu
         */
        constructor(type?: string | undefined, data?: object | undefined, menu?: import("socket:application/menu").Menu);
        /**
         * The `Menu` this event has been dispatched for.
         * @type {import('../application/menu.js').Menu?}
         */
        get menu(): import("socket:application/menu").Menu;
        /**
         * The title of the menu item.
         * @type {string?}
         */
        get title(): string;
        /**
         * An optional tag value for the menu item that may also be the
         * parent menu item title.
         * @type {string?}
         */
        get tag(): string;
        /**
         * The parent title of the menu item.
         * @type {string?}
         */
        get parent(): string;
        #private;
    }
    /**
     * An event dispacted when the application receives an OS signal
     */
    export class SignalEvent extends MessageEvent<any> {
        /**
         * `SignalEvent` class constructor
         * @ignore
         * @param {string=} [type]
         * @param {object=} [options]
         */
        constructor(type?: string | undefined, options?: object | undefined);
        /**
         * The code of the signal.
         * @type {import('../process/signal.js').signal}
         */
        get code(): any;
        /**
         * The name of the signal.
         * @type {string}
         */
        get name(): string;
        /**
         * An optional message describing the signal
         * @type {string}
         */
        get message(): string;
        #private;
    }
    namespace _default {
        export { ApplicationURLEvent };
        export { MenuItemEvent };
        export { SignalEvent };
        export { HotKeyEvent };
    }
    export default _default;
}

declare module "socket:path/well-known" {
    /**
     * Well known path to the user's "Downloads" folder.
     * @type {?string}
     */
    export const DOWNLOADS: string | null;
    /**
     * Well known path to the user's "Documents" folder.
     * @type {?string}
     */
    export const DOCUMENTS: string | null;
    /**
     * Well known path to the user's "Pictures" folder.
     * @type {?string}
     */
    export const PICTURES: string | null;
    /**
     * Well known path to the user's "Desktop" folder.
     * @type {?string}
     */
    export const DESKTOP: string | null;
    /**
     * Well known path to the user's "Videos" folder.
     * @type {?string}
     */
    export const VIDEOS: string | null;
    /**
     * Well known path to the user's "Music" folder.
     * @type {?string}
     */
    export const MUSIC: string | null;
    /**
     * Well known path to the application's "resources" folder.
     * @type {?string}
     */
    export const RESOURCES: string | null;
    /**
     * Well known path to the application's "config" folder.
     * @type {?string}
     */
    export const CONFIG: string | null;
    /**
     * Well known path to the application's public "media" folder.
     * @type {?string}
     */
    export const MEDIA: string | null;
    /**
     * Well known path to the application's "data" folder.
     * @type {?string}
     */
    export const DATA: string | null;
    /**
     * Well known path to the application's "log" folder.
     * @type {?string}
     */
    export const LOG: string | null;
    /**
     * Well known path to the application's "tmp" folder.
     * @type {?string}
     */
    export const TMP: string | null;
    /**
     * Well known path to the application's "home" folder.
     * This may be the user's HOME directory or the application container sandbox.
     * @type {?string}
     */
    export const HOME: string | null;
    namespace _default {
        export { DOWNLOADS };
        export { DOCUMENTS };
        export { RESOURCES };
        export { PICTURES };
        export { DESKTOP };
        export { VIDEOS };
        export { CONFIG };
        export { MEDIA };
        export { MUSIC };
        export { HOME };
        export { DATA };
        export { LOG };
        export { TMP };
    }
    export default _default;
}

declare module "socket:os" {
    /**
     * Returns the operating system CPU architecture for which Socket was compiled.
     * @returns {string} - 'arm64', 'ia32', 'x64', or 'unknown'
     */
    export function arch(): string;
    /**
     * Returns an array of objects containing information about each CPU/core.
     * @returns {Array<object>} cpus - An array of objects containing information about each CPU/core.
     * The properties of the objects are:
     * - model `<string>` - CPU model name.
     * - speed `<number>` - CPU clock speed (in MHz).
     * - times `<object>` - An object containing the fields user, nice, sys, idle, irq representing the number of milliseconds the CPU has spent in each mode.
     *   - user `<number>` - Time spent by this CPU or core in user mode.
     *   - nice `<number>` - Time spent by this CPU or core in user mode with low priority (nice).
     *   - sys `<number>` - Time spent by this CPU or core in system mode.
     *   - idle `<number>` - Time spent by this CPU or core in idle mode.
     *   - irq `<number>` - Time spent by this CPU or core in IRQ mode.
     * @see {@link https://nodejs.org/api/os.html#os_os_cpus}
     */
    export function cpus(): Array<object>;
    /**
     * Returns an object containing network interfaces that have been assigned a network address.
     * @returns {object}  - An object containing network interfaces that have been assigned a network address.
     * Each key on the returned object identifies a network interface. The associated value is an array of objects that each describe an assigned network address.
     * The properties available on the assigned network address object include:
     * - address `<string>` - The assigned IPv4 or IPv6 address.
     * - netmask `<string>` - The IPv4 or IPv6 network mask.
     * - family `<string>` - The address family ('IPv4' or 'IPv6').
     * - mac `<string>` - The MAC address of the network interface.
     * - internal `<boolean>` - Indicates whether the network interface is a loopback interface.
     * - scopeid `<number>` - The numeric scope ID (only specified when family is 'IPv6').
     * - cidr `<string>` - The CIDR notation of the interface.
     * @see {@link https://nodejs.org/api/os.html#os_os_networkinterfaces}
     */
    export function networkInterfaces(): object;
    /**
     * Returns the operating system platform.
     * @returns {string} - 'android', 'cygwin', 'freebsd', 'linux', 'darwin', 'ios', 'openbsd', 'win32', or 'unknown'
     * @see {@link https://nodejs.org/api/os.html#os_os_platform}
     * The returned value is equivalent to `process.platform`.
     */
    export function platform(): string;
    /**
     * Returns the operating system name.
     * @returns {string} - 'CYGWIN_NT', 'Mac', 'Darwin', 'FreeBSD', 'Linux', 'OpenBSD', 'Windows_NT', 'Win32', or 'Unknown'
     * @see {@link https://nodejs.org/api/os.html#os_os_type}
     */
    export function type(): string;
    /**
     * @returns {boolean} - `true` if the operating system is Windows.
     */
    export function isWindows(): boolean;
    /**
     * @returns {string} - The operating system's default directory for temporary files.
     */
    export function tmpdir(): string;
    /**
     * Get resource usage.
     */
    export function rusage(): any;
    /**
     * Returns the system uptime in seconds.
     * @returns {number} - The system uptime in seconds.
     */
    export function uptime(): number;
    /**
     * Returns the operating system name.
     * @returns {string} - The operating system name.
     */
    export function uname(): string;
    /**
     * It's implemented in process.hrtime.bigint()
     * @ignore
     */
    export function hrtime(): any;
    /**
     * Node.js doesn't have this method.
     * @ignore
     */
    export function availableMemory(): any;
    /**
     * The host operating system. This value can be one of:
     * - android
     * - android-emulator
     * - iphoneos
     * - iphone-simulator
     * - linux
     * - macosx
     * - unix
     * - unknown
     * - win32
     * @ignore
     * @return {'android'|'android-emulator'|'iphoneos'|iphone-simulator'|'linux'|'macosx'|unix'|unknown'|win32'}
     */
    export function host(): "android" | "android-emulator" | "iphoneos" | iphone;
    /**
     * Returns the home directory of the current user.
     * @return {string}
     */
    export function homedir(): string;
    export { constants };
    /**
     * @type {string}
     * The operating system's end-of-line marker. `'\r\n'` on Windows and `'\n'` on POSIX.
     */
    export const EOL: string;
    export default exports;
    import constants from "socket:os/constants";
    import * as exports from "socket:os";
    
}

declare module "socket:process/signal" {
    /**
     * Converts an `signal` code to its corresponding string message.
     * @param {import('./os/constants.js').signal} {code}
     * @return {string}
     */
    export function toString(code: any): string;
    /**
     * Gets the code for a given 'signal' name.
     * @param {string|number} name
     * @return {signal}
     */
    export function getCode(name: string | number): signal;
    /**
     * Gets the name for a given 'signal' code
     * @return {string}
     * @param {string|number} code
     */
    export function getName(code: string | number): string;
    /**
     * Gets the message for a 'signal' code.
     * @param {number|string} code
     * @return {string}
     */
    export function getMessage(code: number | string): string;
    /**
     * Add a signal event listener.
     * @param {string|number} signal
     * @param {function(SignalEvent)} callback
     * @param {{ once?: boolean }=} [options]
     */
    export function addEventListener(signalName: any, callback: (arg0: SignalEvent) => any, options?: {
        once?: boolean;
    } | undefined): void;
    /**
     * Remove a signal event listener.
     * @param {string|number} signal
     * @param {function(SignalEvent)} callback
     * @param {{ once?: boolean }=} [options]
     */
    export function removeEventListener(signalName: any, callback: (arg0: SignalEvent) => any, options?: {
        once?: boolean;
    } | undefined): void;
    export { constants };
    export const channel: BroadcastChannel;
    export const SIGHUP: any;
    export const SIGINT: any;
    export const SIGQUIT: any;
    export const SIGILL: any;
    export const SIGTRAP: any;
    export const SIGABRT: any;
    export const SIGIOT: any;
    export const SIGBUS: any;
    export const SIGFPE: any;
    export const SIGKILL: any;
    export const SIGUSR1: any;
    export const SIGSEGV: any;
    export const SIGUSR2: any;
    export const SIGPIPE: any;
    export const SIGALRM: any;
    export const SIGTERM: any;
    export const SIGCHLD: any;
    export const SIGCONT: any;
    export const SIGSTOP: any;
    export const SIGTSTP: any;
    export const SIGTTIN: any;
    export const SIGTTOU: any;
    export const SIGURG: any;
    export const SIGXCPU: any;
    export const SIGXFSZ: any;
    export const SIGVTALRM: any;
    export const SIGPROF: any;
    export const SIGWINCH: any;
    export const SIGIO: any;
    export const SIGINFO: any;
    export const SIGSYS: any;
    export const strings: {
        [x: number]: string;
    };
    namespace _default {
        export { addEventListener };
        export { removeEventListener };
        export { constants };
        export { channel };
        export { strings };
        export { toString };
        export { getName };
        export { getCode };
        export { getMessage };
        export { SIGHUP };
        export { SIGINT };
        export { SIGQUIT };
        export { SIGILL };
        export { SIGTRAP };
        export { SIGABRT };
        export { SIGIOT };
        export { SIGBUS };
        export { SIGFPE };
        export { SIGKILL };
        export { SIGUSR1 };
        export { SIGSEGV };
        export { SIGUSR2 };
        export { SIGPIPE };
        export { SIGALRM };
        export { SIGTERM };
        export { SIGCHLD };
        export { SIGCONT };
        export { SIGSTOP };
        export { SIGTSTP };
        export { SIGTTIN };
        export { SIGTTOU };
        export { SIGURG };
        export { SIGXCPU };
        export { SIGXFSZ };
        export { SIGVTALRM };
        export { SIGPROF };
        export { SIGWINCH };
        export { SIGIO };
        export { SIGINFO };
        export { SIGSYS };
    }
    export default _default;
    export type signal = any;
    import { SignalEvent } from "socket:internal/events";
    import { signal as constants } from "socket:os/constants";
}

declare module "socket:internal/streams/web" {
    export class ByteLengthQueuingStrategy {
        constructor(e: any);
        _byteLengthQueuingStrategyHighWaterMark: any;
        get highWaterMark(): any;
        get size(): (e: any) => any;
    }
    export class CountQueuingStrategy {
        constructor(e: any);
        _countQueuingStrategyHighWaterMark: any;
        get highWaterMark(): any;
        get size(): () => number;
    }
    export class ReadableByteStreamController {
        get byobRequest(): any;
        get desiredSize(): number;
        close(): void;
        enqueue(e: any): void;
        error(e?: any): void;
        _pendingPullIntos: v;
        [T](e: any): any;
        [C](e: any): any;
        [P](): void;
    }
    export class ReadableStream {
        static from(e: any): any;
        constructor(e?: {}, t?: {});
        get locked(): boolean;
        cancel(e?: any): any;
        getReader(e?: any): ReadableStreamBYOBReader | ReadableStreamDefaultReader;
        pipeThrough(e: any, t?: {}): any;
        pipeTo(e: any, t?: {}): any;
        tee(): any;
        values(e?: any): any;
    }
    export class ReadableStreamBYOBReader {
        constructor(e: any);
        _readIntoRequests: v;
        get closed(): any;
        cancel(e?: any): any;
        read(e: any, t?: {}): any;
        releaseLock(): void;
    }
    export class ReadableStreamBYOBRequest {
        get view(): any;
        respond(e: any): void;
        respondWithNewView(e: any): void;
    }
    export class ReadableStreamDefaultController {
        get desiredSize(): number;
        close(): void;
        enqueue(e?: any): void;
        error(e?: any): void;
        [T](e: any): any;
        [C](e: any): void;
        [P](): void;
    }
    export class ReadableStreamDefaultReader {
        constructor(e: any);
        _readRequests: v;
        get closed(): any;
        cancel(e?: any): any;
        read(): any;
        releaseLock(): void;
    }
    export class TransformStream {
        constructor(e?: {}, t?: {}, r?: {});
        get readable(): any;
        get writable(): any;
    }
    export class TransformStreamDefaultController {
        get desiredSize(): number;
        enqueue(e?: any): void;
        error(e?: any): void;
        terminate(): void;
    }
    export class WritableStream {
        constructor(e?: {}, t?: {});
        get locked(): boolean;
        abort(e?: any): any;
        close(): any;
        getWriter(): WritableStreamDefaultWriter;
    }
    export class WritableStreamDefaultController {
        get abortReason(): any;
        get signal(): any;
        error(e?: any): void;
        [w](e: any): any;
        [R](): void;
    }
    export class WritableStreamDefaultWriter {
        constructor(e: any);
        _ownerWritableStream: any;
        get closed(): any;
        get desiredSize(): number;
        get ready(): any;
        abort(e?: any): any;
        close(): any;
        releaseLock(): void;
        write(e?: any): any;
    }
    class v {
        _cursor: number;
        _size: number;
        _front: {
            _elements: any[];
            _next: any;
        };
        _back: {
            _elements: any[];
            _next: any;
        };
        get length(): number;
        push(e: any): void;
        shift(): any;
        forEach(e: any): void;
        peek(): any;
    }
    const T: unique symbol;
    const C: unique symbol;
    const P: unique symbol;
    const w: unique symbol;
    const R: unique symbol;
    export {};
}

declare module "socket:internal/streams" {
    const _default: any;
    export default _default;
    import { ReadableStream } from "socket:internal/streams/web";
    import { ReadableStreamBYOBReader } from "socket:internal/streams/web";
    import { ReadableByteStreamController } from "socket:internal/streams/web";
    import { ReadableStreamBYOBRequest } from "socket:internal/streams/web";
    import { ReadableStreamDefaultController } from "socket:internal/streams/web";
    import { ReadableStreamDefaultReader } from "socket:internal/streams/web";
    import { WritableStream } from "socket:internal/streams/web";
    import { WritableStreamDefaultController } from "socket:internal/streams/web";
    import { WritableStreamDefaultWriter } from "socket:internal/streams/web";
    import { TransformStream } from "socket:internal/streams/web";
    import { TransformStreamDefaultController } from "socket:internal/streams/web";
    import { ByteLengthQueuingStrategy } from "socket:internal/streams/web";
    import { CountQueuingStrategy } from "socket:internal/streams/web";
    export { ReadableStream, ReadableStreamBYOBReader, ReadableByteStreamController, ReadableStreamBYOBRequest, ReadableStreamDefaultController, ReadableStreamDefaultReader, WritableStream, WritableStreamDefaultController, WritableStreamDefaultWriter, TransformStream, TransformStreamDefaultController, ByteLengthQueuingStrategy, CountQueuingStrategy };
}

declare module "socket:stream/web" {
    export const TextEncoderStream: typeof UnsupportedStreamInterface;
    export const TextDecoderStream: {
        new (label?: string, options?: TextDecoderOptions): TextDecoderStream;
        prototype: TextDecoderStream;
    } | typeof UnsupportedStreamInterface;
    export const CompressionStream: {
        new (format: CompressionFormat): CompressionStream;
        prototype: CompressionStream;
    } | typeof UnsupportedStreamInterface;
    export const DecompressionStream: {
        new (format: CompressionFormat): DecompressionStream;
        prototype: DecompressionStream;
    } | typeof UnsupportedStreamInterface;
    export default exports;
    import { ReadableStream } from "socket:internal/streams";
    import { ReadableStreamBYOBReader } from "socket:internal/streams";
    import { ReadableByteStreamController } from "socket:internal/streams";
    import { ReadableStreamBYOBRequest } from "socket:internal/streams";
    import { ReadableStreamDefaultController } from "socket:internal/streams";
    import { ReadableStreamDefaultReader } from "socket:internal/streams";
    import { WritableStream } from "socket:internal/streams";
    import { WritableStreamDefaultController } from "socket:internal/streams";
    import { WritableStreamDefaultWriter } from "socket:internal/streams";
    import { TransformStream } from "socket:internal/streams";
    import { TransformStreamDefaultController } from "socket:internal/streams";
    import { ByteLengthQueuingStrategy } from "socket:internal/streams";
    import { CountQueuingStrategy } from "socket:internal/streams";
    class UnsupportedStreamInterface {
    }
    import * as exports from "socket:stream/web";
    
    export { ReadableStream, ReadableStreamBYOBReader, ReadableByteStreamController, ReadableStreamBYOBRequest, ReadableStreamDefaultController, ReadableStreamDefaultReader, WritableStream, WritableStreamDefaultController, WritableStreamDefaultWriter, TransformStream, TransformStreamDefaultController, ByteLengthQueuingStrategy, CountQueuingStrategy };
}

declare module "socket:stream" {
    export function pipelinePromise(...streams: any[]): Promise<any>;
    export function pipeline(stream: any, ...streams: any[]): any;
    export function isStream(stream: any): boolean;
    export function isStreamx(stream: any): boolean;
    export function getStreamError(stream: any): any;
    export function isReadStreamx(stream: any): any;
    export { web };
    export class FixedFIFO {
        constructor(hwm: any);
        buffer: any[];
        mask: number;
        top: number;
        btm: number;
        next: any;
        clear(): void;
        push(data: any): boolean;
        shift(): any;
        peek(): any;
        isEmpty(): boolean;
    }
    export class FIFO {
        constructor(hwm: any);
        hwm: any;
        head: FixedFIFO;
        tail: FixedFIFO;
        length: number;
        clear(): void;
        push(val: any): void;
        shift(): any;
        peek(): any;
        isEmpty(): boolean;
    }
    export class WritableState {
        constructor(stream: any, { highWaterMark, map, mapWritable, byteLength, byteLengthWritable }?: {
            highWaterMark?: number;
            map?: any;
            mapWritable: any;
            byteLength: any;
            byteLengthWritable: any;
        });
        stream: any;
        queue: FIFO;
        highWaterMark: number;
        buffered: number;
        error: any;
        pipeline: any;
        drains: any;
        byteLength: any;
        map: any;
        afterWrite: any;
        afterUpdateNextTick: any;
        get ended(): boolean;
        push(data: any): boolean;
        shift(): any;
        end(data: any): void;
        autoBatch(data: any, cb: any): any;
        update(): void;
        updateNonPrimary(): void;
        continueUpdate(): boolean;
        updateCallback(): void;
        updateNextTick(): void;
    }
    export class ReadableState {
        constructor(stream: any, { highWaterMark, map, mapReadable, byteLength, byteLengthReadable }?: {
            highWaterMark?: number;
            map?: any;
            mapReadable: any;
            byteLength: any;
            byteLengthReadable: any;
        });
        stream: any;
        queue: FIFO;
        highWaterMark: number;
        buffered: number;
        readAhead: boolean;
        error: any;
        pipeline: Pipeline;
        byteLength: any;
        map: any;
        pipeTo: any;
        afterRead: any;
        afterUpdateNextTick: any;
        get ended(): boolean;
        pipe(pipeTo: any, cb: any): void;
        push(data: any): boolean;
        shift(): any;
        unshift(data: any): void;
        read(): any;
        drain(): void;
        update(): void;
        updateNonPrimary(): void;
        continueUpdate(): boolean;
        updateCallback(): void;
        updateNextTick(): void;
    }
    export class TransformState {
        constructor(stream: any);
        data: any;
        afterTransform: any;
        afterFinal: any;
    }
    export class Pipeline {
        constructor(src: any, dst: any, cb: any);
        from: any;
        to: any;
        afterPipe: any;
        error: any;
        pipeToFinished: boolean;
        finished(): void;
        done(stream: any, err: any): void;
    }
    export class Stream extends EventEmitter {
        constructor(opts: any);
        _duplexState: number;
        _readableState: any;
        _writableState: any;
        _open(cb: any): void;
        _destroy(cb: any): void;
        _predestroy(): void;
        get readable(): boolean;
        get writable(): boolean;
        get destroyed(): boolean;
        get destroying(): boolean;
        destroy(err: any): void;
    }
    export class Readable extends Stream {
        static _fromAsyncIterator(ite: any, opts: any): Readable;
        static from(data: any, opts: any): any;
        static isBackpressured(rs: any): boolean;
        static isPaused(rs: any): boolean;
        _readableState: ReadableState;
        _read(cb: any): void;
        pipe(dest: any, cb: any): any;
        read(): any;
        push(data: any): boolean;
        unshift(data: any): void;
        resume(): this;
        pause(): this;
    }
    export class Writable extends Stream {
        static isBackpressured(ws: any): boolean;
        static drained(ws: any): Promise<any>;
        _writableState: WritableState;
        _writev(batch: any, cb: any): void;
        _write(data: any, cb: any): void;
        _final(cb: any): void;
        write(data: any): boolean;
        end(data: any): this;
    }
    export class Duplex extends Readable {
        _writableState: WritableState;
        _writev(batch: any, cb: any): void;
        _write(data: any, cb: any): void;
        _final(cb: any): void;
        write(data: any): boolean;
        end(data: any): this;
    }
    export class Transform extends Duplex {
        _transformState: TransformState;
        _transform(data: any, cb: any): void;
        _flush(cb: any): void;
    }
    export class PassThrough extends Transform {
    }
    const _default: typeof Stream & {
        web: typeof web;
        Readable: typeof Readable;
        Writable: typeof Writable;
        Duplex: typeof Duplex;
        Transform: typeof Transform;
        PassThrough: typeof PassThrough;
        pipeline: typeof pipeline & {
            [x: symbol]: typeof pipelinePromise;
        };
    };
    export default _default;
    import web from "socket:stream/web";
    import { EventEmitter } from "socket:events";
}

declare module "socket:tty" {
    export function WriteStream(fd: any): Writable;
    export function ReadStream(fd: any): Readable;
    export function isatty(fd: any): boolean;
    namespace _default {
        export { WriteStream };
        export { ReadStream };
        export { isatty };
    }
    export default _default;
    import { Writable } from "socket:stream";
    import { Readable } from "socket:stream";
}

declare module "socket:process" {
    /**
     * Adds callback to the 'nextTick' queue.
     * @param {Function} callback
     */
    export function nextTick(callback: Function, ...args: any[]): void;
    /**
     * Computed high resolution time as a `BigInt`.
     * @param {Array<number>?} [time]
     * @return {bigint}
     */
    export function hrtime(time?: Array<number> | null): bigint;
    export namespace hrtime {
        function bigint(): any;
    }
    /**
     * @param {number=} [code=0] - The exit code. Default: 0.
     */
    export function exit(code?: number | undefined): Promise<void>;
    /**
     * Returns an object describing the memory usage of the Node.js process measured in bytes.
     * @returns {Object}
     */
    export function memoryUsage(): any;
    export namespace memoryUsage {
        function rss(): any;
    }
    export class ProcessEnvironmentEvent extends Event {
        constructor(type: any, key: any, value: any);
        key: any;
        value: any;
    }
    export class ProcessEnvironment extends EventTarget {
        get [Symbol.toStringTag](): string;
    }
    export const env: ProcessEnvironment;
    export default process;
    const process: any;
}

declare module "socket:path/path" {
    /**
     * The path.resolve() method resolves a sequence of paths or path segments into an absolute path.
     * @param {strig} ...paths
     * @returns {string}
     * @see {@link https://nodejs.org/api/path.html#path_path_resolve_paths}
     */
    export function resolve(options: any, ...components: any[]): string;
    /**
     * Computes current working directory for a path
     * @param {object=} [opts]
     * @param {boolean=} [opts.posix] Set to `true` to force POSIX style path
     * @return {string}
     */
    export function cwd(opts?: object | undefined): string;
    /**
     * Computed location origin. Defaults to `socket:///` if not available.
     * @return {string}
     */
    export function origin(): string;
    /**
     * Computes the relative path from `from` to `to`.
     * @param {object} options
     * @param {PathComponent} from
     * @param {PathComponent} to
     * @return {string}
     */
    export function relative(options: object, from: PathComponent, to: PathComponent): string;
    /**
     * Joins path components. This function may not return an absolute path.
     * @param {object} options
     * @param {...PathComponent} components
     * @return {string}
     */
    export function join(options: object, ...components: PathComponent[]): string;
    /**
     * Computes directory name of path.
     * @param {object} options
     * @param {...PathComponent} components
     * @return {string}
     */
    export function dirname(options: object, path: any): string;
    /**
     * Computes base name of path.
     * @param {object} options
     * @param {...PathComponent} components
     * @return {string}
     */
    export function basename(options: object, path: any): string;
    /**
     * Computes extension name of path.
     * @param {object} options
     * @param {PathComponent} path
     * @return {string}
     */
    export function extname(options: object, path: PathComponent): string;
    /**
     * Computes normalized path
     * @param {object} options
     * @param {PathComponent} path
     * @return {string}
     */
    export function normalize(options: object, path: PathComponent): string;
    /**
     * Formats `Path` object into a string.
     * @param {object} options
     * @param {object|Path} path
     * @return {string}
     */
    export function format(options: object, path: object | Path): string;
    /**
     * Parses input `path` into a `Path` instance.
     * @param {PathComponent} path
     * @return {object}
     */
    export function parse(path: PathComponent): object;
    /**
     * @typedef {(string|Path|URL|{ pathname: string }|{ url: string)} PathComponent
     */
    /**
     * A container for a parsed Path.
     */
    export class Path {
        /**
         * Creates a `Path` instance from `input` and optional `cwd`.
         * @param {PathComponent} input
         * @param {string} [cwd]
         */
        static from(input: PathComponent, cwd?: string): any;
        /**
         * `Path` class constructor.
         * @protected
         * @param {string} pathname
         * @param {string} [cwd = Path.cwd()]
         */
        protected constructor();
        pattern: {
            "__#11@#i": any;
            "__#11@#n": {};
            "__#11@#t": {};
            "__#11@#e": {};
            "__#11@#s": {};
            "__#11@#l": boolean;
            test(t: {}, r: any): boolean;
            exec(t: {}, r: any): {
                inputs: any[] | {}[];
            };
            readonly protocol: any;
            readonly username: any;
            readonly password: any;
            readonly hostname: any;
            readonly port: any;
            readonly pathname: any;
            readonly search: any;
            readonly hash: any;
            readonly hasRegExpGroups: boolean;
        };
        url: any;
        get pathname(): any;
        get protocol(): any;
        get href(): any;
        /**
         * `true` if the path is relative, otherwise `false.
         * @type {boolean}
         */
        get isRelative(): boolean;
        /**
         * The working value of this path.
         */
        get value(): any;
        /**
         * The original source, unresolved.
         * @type {string}
         */
        get source(): string;
        /**
         * Computed parent path.
         * @type {string}
         */
        get parent(): string;
        /**
         * Computed root in path.
         * @type {string}
         */
        get root(): string;
        /**
         * Computed directory name in path.
         * @type {string}
         */
        get dir(): string;
        /**
         * Computed base name in path.
         * @type {string}
         */
        get base(): string;
        /**
         * Computed base name in path without path extension.
         * @type {string}
         */
        get name(): string;
        /**
         * Computed extension name in path.
         * @type {string}
         */
        get ext(): string;
        /**
         * The computed drive, if given in the path.
         * @type {string?}
         */
        get drive(): string;
        /**
         * @return {URL}
         */
        toURL(): URL;
        /**
         * Converts this `Path` instance to a string.
         * @return {string}
         */
        toString(): string;
        /**
         * @ignore
         */
        inspect(): {
            root: string;
            dir: string;
            base: string;
            ext: string;
            name: string;
        };
        /**
         * @ignore
         */
        [Symbol.toStringTag](): string;
        #private;
    }
    export default Path;
    export type PathComponent = (string | Path | URL | {
        pathname: string;
    } | {
        url: string;
    });
    import { URL } from "socket:url/index";
}

declare module "socket:path/mounts" {
    const _default: {};
    export default _default;
}

declare module "socket:path/win32" {
    /**
     * Computes current working directory for a path
     * @param {string}
     */
    export function cwd(): any;
    /**
     * Resolves path components to an absolute path.
     * @param {...PathComponent} components
     * @return {string}
     */
    export function resolve(...components: PathComponent[]): string;
    /**
     * Joins path components. This function may not return an absolute path.
     * @param {...PathComponent} components
     * @return {string}
     */
    export function join(...components: PathComponent[]): string;
    /**
     * Computes directory name of path.
     * @param {PathComponent} path
     * @return {string}
     */
    export function dirname(path: PathComponent): string;
    /**
     * Computes base name of path.
     * @param {PathComponent} path
     * @param {string=} [suffix]
     * @return {string}
     */
    export function basename(path: PathComponent, suffix?: string | undefined): string;
    /**
     * Computes extension name of path.
     * @param {PathComponent} path
     * @return {string}
     */
    export function extname(path: PathComponent): string;
    /**
     * Predicate helper to determine if path is absolute.
     * @param {PathComponent} path
     * @return {boolean}
     */
    export function isAbsolute(path: PathComponent): boolean;
    /**
     * Parses input `path` into a `Path` instance.
     * @param {PathComponent} path
     * @return {Path}
     */
    export function parse(path: PathComponent): Path;
    /**
     * Formats `Path` object into a string.
     * @param {object|Path} path
     * @return {string}
     */
    export function format(path: object | Path): string;
    /**
     * Normalizes `path` resolving `..` and `.\` preserving trailing
     * slashes.
     * @param {string} path
     */
    export function normalize(path: string): any;
    /**
     * Computes the relative path from `from` to `to`.
     * @param {string} from
     * @param {string} to
     * @return {string}
     */
    export function relative(from: string, to: string): string;
    export default exports;
    export namespace win32 {
        let sep: "\\";
        let delimiter: ";";
    }
    export type PathComponent = import("socket:path/path").PathComponent;
    import { Path } from "socket:path/path";
    import * as mounts from "socket:path/mounts";
    import * as posix from "socket:path/posix";
    import { DOWNLOADS } from "socket:path/well-known";
    import { DOCUMENTS } from "socket:path/well-known";
    import { RESOURCES } from "socket:path/well-known";
    import { PICTURES } from "socket:path/well-known";
    import { DESKTOP } from "socket:path/well-known";
    import { VIDEOS } from "socket:path/well-known";
    import { CONFIG } from "socket:path/well-known";
    import { MEDIA } from "socket:path/well-known";
    import { MUSIC } from "socket:path/well-known";
    import { HOME } from "socket:path/well-known";
    import { DATA } from "socket:path/well-known";
    import { LOG } from "socket:path/well-known";
    import { TMP } from "socket:path/well-known";
    import * as exports from "socket:path/win32";
    
    export { mounts, posix, Path, DOWNLOADS, DOCUMENTS, RESOURCES, PICTURES, DESKTOP, VIDEOS, CONFIG, MEDIA, MUSIC, HOME, DATA, LOG, TMP };
}

declare module "socket:path/posix" {
    /**
     * Computes current working directory for a path
     * @param {string}
     * @return {string}
     */
    export function cwd(): string;
    /**
     * Resolves path components to an absolute path.
     * @param {...PathComponent} components
     * @return {string}
     */
    export function resolve(...components: PathComponent[]): string;
    /**
     * Joins path components. This function may not return an absolute path.
     * @param {...PathComponent} components
     * @return {string}
     */
    export function join(...components: PathComponent[]): string;
    /**
     * Computes directory name of path.
     * @param {PathComponent} path
     * @return {string}
     */
    export function dirname(path: PathComponent): string;
    /**
     * Computes base name of path.
     * @param {PathComponent} path
     * @param {string=} [suffix]
     * @return {string}
     */
    export function basename(path: PathComponent, suffix?: string | undefined): string;
    /**
     * Computes extension name of path.
     * @param {PathComponent} path
     * @return {string}
     */
    export function extname(path: PathComponent): string;
    /**
     * Predicate helper to determine if path is absolute.
     * @param {PathComponent} path
     * @return {boolean}
     */
    export function isAbsolute(path: PathComponent): boolean;
    /**
     * Parses input `path` into a `Path` instance.
     * @param {PathComponent} path
     * @return {Path}
     */
    export function parse(path: PathComponent): Path;
    /**
     * Formats `Path` object into a string.
     * @param {object|Path} path
     * @return {string}
     */
    export function format(path: object | Path): string;
    /**
     * Normalizes `path` resolving `..` and `./` preserving trailing
     * slashes.
     * @param {string} path
     */
    export function normalize(path: string): any;
    /**
     * Computes the relative path from `from` to `to`.
     * @param {string} from
     * @param {string} to
     * @return {string}
     */
    export function relative(from: string, to: string): string;
    export default exports;
    export namespace posix {
        let sep: "/";
        let delimiter: ":";
    }
    export type PathComponent = import("socket:path/path").PathComponent;
    import { Path } from "socket:path/path";
    import * as mounts from "socket:path/mounts";
    import * as win32 from "socket:path/win32";
    import { DOWNLOADS } from "socket:path/well-known";
    import { DOCUMENTS } from "socket:path/well-known";
    import { RESOURCES } from "socket:path/well-known";
    import { PICTURES } from "socket:path/well-known";
    import { DESKTOP } from "socket:path/well-known";
    import { VIDEOS } from "socket:path/well-known";
    import { CONFIG } from "socket:path/well-known";
    import { MEDIA } from "socket:path/well-known";
    import { MUSIC } from "socket:path/well-known";
    import { HOME } from "socket:path/well-known";
    import { DATA } from "socket:path/well-known";
    import { LOG } from "socket:path/well-known";
    import { TMP } from "socket:path/well-known";
    import * as exports from "socket:path/posix";
    
    export { mounts, win32, Path, DOWNLOADS, DOCUMENTS, RESOURCES, PICTURES, DESKTOP, VIDEOS, CONFIG, MEDIA, MUSIC, HOME, DATA, LOG, TMP };
}

declare module "socket:path/index" {
    export default exports;
    import * as mounts from "socket:path/mounts";
    import * as posix from "socket:path/posix";
    import * as win32 from "socket:path/win32";
    import { Path } from "socket:path/path";
    import { DOWNLOADS } from "socket:path/well-known";
    import { DOCUMENTS } from "socket:path/well-known";
    import { RESOURCES } from "socket:path/well-known";
    import { PICTURES } from "socket:path/well-known";
    import { DESKTOP } from "socket:path/well-known";
    import { VIDEOS } from "socket:path/well-known";
    import { CONFIG } from "socket:path/well-known";
    import { MEDIA } from "socket:path/well-known";
    import { MUSIC } from "socket:path/well-known";
    import { HOME } from "socket:path/well-known";
    import { DATA } from "socket:path/well-known";
    import { LOG } from "socket:path/well-known";
    import { TMP } from "socket:path/well-known";
    import * as exports from "socket:path/index";
    
    export { mounts, posix, win32, Path, DOWNLOADS, DOCUMENTS, RESOURCES, PICTURES, DESKTOP, VIDEOS, CONFIG, MEDIA, MUSIC, HOME, DATA, LOG, TMP };
}

declare module "socket:path" {
    export const sep: "\\" | "/";
    export const delimiter: ":" | ";";
    export const resolve: typeof posix.win32.resolve;
    export const join: typeof posix.win32.join;
    export const dirname: typeof posix.win32.dirname;
    export const basename: typeof posix.win32.basename;
    export const extname: typeof posix.win32.extname;
    export const cwd: typeof posix.win32.cwd;
    export const isAbsolute: typeof posix.win32.isAbsolute;
    export const parse: typeof posix.win32.parse;
    export const format: typeof posix.win32.format;
    export const normalize: typeof posix.win32.normalize;
    export const relative: typeof posix.win32.relative;
    const _default: typeof posix | typeof posix.win32;
    export default _default;
    import { posix } from "socket:path/index";
    import { Path } from "socket:path/index";
    import { win32 } from "socket:path/index";
    import { mounts } from "socket:path/index";
    import { DOWNLOADS } from "socket:path/index";
    import { DOCUMENTS } from "socket:path/index";
    import { RESOURCES } from "socket:path/index";
    import { PICTURES } from "socket:path/index";
    import { DESKTOP } from "socket:path/index";
    import { VIDEOS } from "socket:path/index";
    import { CONFIG } from "socket:path/index";
    import { MEDIA } from "socket:path/index";
    import { MUSIC } from "socket:path/index";
    import { HOME } from "socket:path/index";
    import { DATA } from "socket:path/index";
    import { LOG } from "socket:path/index";
    import { TMP } from "socket:path/index";
    export { Path, posix, win32, mounts, DOWNLOADS, DOCUMENTS, RESOURCES, PICTURES, DESKTOP, VIDEOS, CONFIG, MEDIA, MUSIC, HOME, DATA, LOG, TMP };
}

declare module "socket:fs/constants" {
    /**
     * This flag can be used with uv_fs_copyfile() to return an error if the
     * destination already exists.
     * @type {number}
     */
    export const COPYFILE_EXCL: number;
    /**
     * This flag can be used with uv_fs_copyfile() to attempt to create a reflink.
     * If copy-on-write is not supported, a fallback copy mechanism is used.
     * @type {number}
     */
    export const COPYFILE_FICLONE: number;
    /**
     * This flag can be used with uv_fs_copyfile() to attempt to create a reflink.
     * If copy-on-write is not supported, an error is returned.
     * @type {number}
     */
    export const COPYFILE_FICLONE_FORCE: number;
    /**
     * A constant representing a directory entry whose type is unknown.
     * It indicates that the type of the file or directory cannot be determined.
     * @type {number}
     */
    export const UV_DIRENT_UNKNOWN: number;
    /**
     * A constant representing a directory entry of type file.
     * It indicates that the entry is a regular file.
     * @type {number}
     */
    export const UV_DIRENT_FILE: number;
    /**
     * A constant epresenting a directory entry of type directory.
     * It indicates that the entry is a directory.
     * @type {number}
     */
    export const UV_DIRENT_DIR: number;
    /**
     * A constant representing a directory entry of type symbolic link.
     * @type {number}
     */
    export const UV_DIRENT_LINK: number;
    /**
     * A constant representing a directory entry of type FIFO (named pipe).
     * @type {number}
     */
    export const UV_DIRENT_FIFO: number;
    /**
     * A constant representing a directory entry of type socket.
     * @type {number}
     */
    export const UV_DIRENT_SOCKET: number;
    /**
     * A constant representing a directory entry of type character device
     * @type {number}
     */
    export const UV_DIRENT_CHAR: number;
    /**
     * A constant representing a directory entry of type block device.
     * @type {number}
     */
    export const UV_DIRENT_BLOCK: number;
    /**
     * A constant representing a symlink should target a directory.
     * @type {number}
     */
    export const UV_FS_SYMLINK_DIR: number;
    /**
     * A constant representing a symlink should be created as a Windows junction.
     * @type {number}
     */
    export const UV_FS_SYMLINK_JUNCTION: number;
    /**
     * A constant representing an opened file for memory mapping on Windows systems.
     * @type {number}
     */
    export const UV_FS_O_FILEMAP: number;
    /**
     * Opens a file for read-only access.
     * @type {number}
     */
    export const O_RDONLY: number;
    /**
     * Opens a file for write-only access.
     * @type {number}
     */
    export const O_WRONLY: number;
    /**
     * Opens a file for both reading and writing.
     * @type {number}
     */
    export const O_RDWR: number;
    /**
     * Appends data to the file instead of overwriting.
     * @type {number}
     */
    export const O_APPEND: number;
    /**
     * Enables asynchronous I/O notifications.
     * @type {number}
     */
    export const O_ASYNC: number;
    /**
     * Ensures file descriptors are closed on `exec()` calls.
     * @type {number}
     */
    export const O_CLOEXEC: number;
    /**
     * Creates a new file if it does not exist.
     * @type {number}
     */
    export const O_CREAT: number;
    /**
     * Minimizes caching effects for file I/O.
     * @type {number}
     */
    export const O_DIRECT: number;
    /**
     * Ensures the opened file is a directory.
     * @type {number}
     */
    export const O_DIRECTORY: number;
    /**
     * Writes file data synchronously.
     * @type {number}
     */
    export const O_DSYNC: number;
    /**
     * Fails the operation if the file already exists.
     * @type {number}
     */
    export const O_EXCL: number;
    /**
     * Enables handling of large files.
     * @type {number}
     */
    export const O_LARGEFILE: number;
    /**
     * Prevents updating the file's last access time.
     * @type {number}
     */
    export const O_NOATIME: number;
    /**
     * Prevents becoming the controlling terminal for the process.
     * @type {number}
     */
    export const O_NOCTTY: number;
    /**
     * Does not follow symbolic links.
     * @type {number}
     */
    export const O_NOFOLLOW: number;
    /**
     * Opens the file in non-blocking mode.
     * @type {number}
     */
    export const O_NONBLOCK: number;
    /**
     * Alias for `O_NONBLOCK` on some systems.
     * @type {number}
     */
    export const O_NDELAY: number;
    /**
     * Obtains a file descriptor for a file but does not open it.
     * @type {number}
     */
    export const O_PATH: number;
    /**
     * Writes both file data and metadata synchronously.
     * @type {number}
     */
    export const O_SYNC: number;
    /**
     * Creates a temporary file that is not linked to a directory.
     * @type {number}
     */
    export const O_TMPFILE: number;
    /**
     * Truncates the file to zero length if it exists.
     * @type {number}
     */
    export const O_TRUNC: number;
    /**
     * Bitmask for extracting the file type from a mode.
     * @type {number}
     */
    export const S_IFMT: number;
    /**
     * Indicates a regular file.
     * @type {number}
     */
    export const S_IFREG: number;
    /**
     * Indicates a directory.
     * @type {number}
     */
    export const S_IFDIR: number;
    /**
     * Indicates a character device.
     * @type {number}
     */
    export const S_IFCHR: number;
    /**
     * Indicates a block device.
     * @type {number}
     */
    export const S_IFBLK: number;
    /**
     * Indicates a FIFO (named pipe).
     * @type {number}
     */
    export const S_IFIFO: number;
    /**
     * Indicates a symbolic link.
     * @type {number}
     */
    export const S_IFLNK: number;
    /**
     * Indicates a socket.
     * @type {number}
     */
    export const S_IFSOCK: number;
    /**
     * Grants read, write, and execute permissions for the file owner.
     * @type {number}
     */
    export const S_IRWXU: number;
    /**
     * Grants read permission for the file owner.
     * @type {number}
     */
    export const S_IRUSR: number;
    /**
     * Grants write permission for the file owner.
     * @type {number}
     */
    export const S_IWUSR: number;
    /**
     * Grants execute permission for the file owner.
     * @type {number}
     */
    export const S_IXUSR: number;
    /**
     * Grants read, write, and execute permissions for the group.
     * @type {number}
     */
    export const S_IRWXG: number;
    /**
     * Grants read permission for the group.
     * @type {number}
     */
    export const S_IRGRP: number;
    /**
     * Grants write permission for the group.
     * @type {number}
     */
    export const S_IWGRP: number;
    /**
     * Grants execute permission for the group.
     * @type {number}
     */
    export const S_IXGRP: number;
    /**
     * Grants read, write, and execute permissions for others.
     * @type {number}
     */
    export const S_IRWXO: number;
    /**
     * Grants read permission for others.
     * @type {number}
     */
    export const S_IROTH: number;
    /**
     * Grants write permission for others.
     * @type {number}
     */
    export const S_IWOTH: number;
    /**
     * Grants execute permission for others.
     * @type {number}
     */
    export const S_IXOTH: number;
    /**
     * Checks for the existence of a file.
     * @type {number}
     */
    export const F_OK: number;
    /**
     * Checks for read permission on a file.
     * @type {number}
     */
    export const R_OK: number;
    /**
     * Checks for write permission on a file.
     * @type {number}
     */
    export const W_OK: number;
    /**
     * Checks for execute permission on a file.
     * @type {number}
     */
    export const X_OK: number;
    export default exports;
    import * as exports from "socket:fs/constants";
    
}

declare module "socket:fs/stream" {
    export const DEFAULT_STREAM_HIGH_WATER_MARK: number;
    /**
     * @typedef {import('./handle.js').FileHandle} FileHandle
     */
    /**
     * A `Readable` stream for a `FileHandle`.
     */
    export class ReadStream extends Readable {
        end: any;
        start: any;
        handle: any;
        buffer: ArrayBuffer;
        signal: any;
        timeout: any;
        bytesRead: number;
        shouldEmitClose: boolean;
        /**
         * Sets file handle for the ReadStream.
         * @param {FileHandle} handle
         */
        setHandle(handle: FileHandle): void;
        /**
         * The max buffer size for the ReadStream.
         */
        get highWaterMark(): number;
        /**
         * Relative or absolute path of the underlying `FileHandle`.
         */
        get path(): any;
        /**
         * `true` if the stream is in a pending state.
         */
        get pending(): boolean;
        _open(callback: any): Promise<any>;
        _read(callback: any): Promise<any>;
    }
    export namespace ReadStream {
        export { DEFAULT_STREAM_HIGH_WATER_MARK as highWaterMark };
    }
    /**
     * A `Writable` stream for a `FileHandle`.
     */
    export class WriteStream extends Writable {
        start: any;
        handle: any;
        signal: any;
        timeout: any;
        bytesWritten: number;
        shouldEmitClose: boolean;
        /**
         * Sets file handle for the WriteStream.
         * @param {FileHandle} handle
         */
        setHandle(handle: FileHandle): void;
        /**
         * The max buffer size for the Writetream.
         */
        get highWaterMark(): number;
        /**
         * Relative or absolute path of the underlying `FileHandle`.
         */
        get path(): any;
        /**
         * `true` if the stream is in a pending state.
         */
        get pending(): boolean;
        _open(callback: any): Promise<any>;
        _write(buffer: any, callback: any): any;
    }
    export namespace WriteStream {
        export { DEFAULT_STREAM_HIGH_WATER_MARK as highWaterMark };
    }
    export const FileReadStream: typeof exports.ReadStream;
    export const FileWriteStream: typeof exports.WriteStream;
    export default exports;
    export type FileHandle = import("socket:fs/handle").FileHandle;
    import { Readable } from "socket:stream";
    import { Writable } from "socket:stream";
    import * as exports from "socket:fs/stream";
    
}

declare module "socket:fs/flags" {
    export function normalizeFlags(flags: any): number;
    export default exports;
    import * as exports from "socket:fs/flags";
    
}

declare module "socket:diagnostics/channels" {
    /**
     * Normalizes a channel name to lower case replacing white space,
     * hyphens (-), underscores (_), with dots (.).
     * @ignore
     */
    export function normalizeName(group: any, name: any): string;
    /**
     * Used to preallocate a minimum sized array of subscribers for
     * a channel.
     * @ignore
     */
    export const MIN_CHANNEL_SUBSCRIBER_SIZE: 64;
    /**
     * A general interface for diagnostic channels that can be subscribed to.
     */
    export class Channel {
        constructor(name: any);
        name: any;
        group: any;
        /**
         * Computed subscribers for all channels in this group.
         * @type {Array<function>}
         */
        get subscribers(): Function[];
        /**
         * Accessor for determining if channel has subscribers. This
         * is always `false` for `Channel instances and `true` for `ActiveChannel`
         * instances.
         */
        get hasSubscribers(): boolean;
        /**
         * Computed number of subscribers for this channel.
         */
        get length(): number;
        /**
         * Resets channel state.
         * @param {(boolean)} [shouldOrphan = false]
         */
        reset(shouldOrphan?: (boolean)): void;
        channel(name: any): Channel;
        /**
         * Adds an `onMessage` subscription callback to the channel.
         * @return {boolean}
         */
        subscribe(_: any, onMessage: any): boolean;
        /**
         * Removes an `onMessage` subscription callback from the channel.
         * @param {function} onMessage
         * @return {boolean}
         */
        unsubscribe(_: any, onMessage: Function): boolean;
        /**
         * A no-op for `Channel` instances. This function always returns `false`.
         * @param {string|object} name
         * @param {object=} [message]
         * @return Promise<boolean>
         */
        publish(name: string | object, message?: object | undefined): Promise<boolean>;
        /**
         * Returns a string representation of the `ChannelRegistry`.
         * @ignore
         */
        toString(): any;
        /**
         * Iterator interface
         * @ignore
         */
        get [Symbol.iterator](): any[];
        /**
         * The `Channel` string tag.
         * @ignore
         */
        [Symbol.toStringTag](): string;
        #private;
    }
    /**
     * An `ActiveChannel` is a prototype implementation for a `Channel`
     * that provides an interface what is considered an "active" channel. The
     * `hasSubscribers` accessor always returns `true` for this class.
     */
    export class ActiveChannel extends Channel {
        unsubscribe(onMessage: any): boolean;
        /**
         * @param {object|any} message
         * @return Promise<boolean>
         */
        publish(message: object | any): Promise<boolean>;
    }
    /**
     * A container for a grouping of channels that are named and owned
     * by this group. A `ChannelGroup` can also be a regular channel.
     */
    export class ChannelGroup extends Channel {
        /**
         * @param {Array<Channel>} channels
         * @param {string} name
         */
        constructor(name: string, channels: Array<Channel>);
        channels: Channel[];
        /**
         * Subscribe to a channel or selection of channels in this group.
         * @param {string} name
         * @return {boolean}
         */
        subscribe(name: string, onMessage: any): boolean;
        /**
         * Unsubscribe from a channel or selection of channels in this group.
         * @param {string} name
         * @return {boolean}
         */
        unsubscribe(name: string, onMessage: any): boolean;
        /**
         * Gets or creates a channel for this group.
         * @param {string} name
         * @return {Channel}
         */
        channel(name: string): Channel;
        /**
         * Select a test of channels from this group.
         * The following syntax is supported:
         *   - One Channel: `group.channel`
         *   - All Channels: `*`
         *   - Many Channel: `group.*`
         *   - Collections: `['group.a', 'group.b', 'group.c'] or `group.a,group.b,group.c`
         * @param {string|Array<string>} keys
         * @param {(boolean)} [hasSubscribers = false] - Enforce subscribers in selection
         * @return {Array<{name: string, channel: Channel}>}
         */
        select(keys: string | Array<string>, hasSubscribers?: (boolean)): Array<{
            name: string;
            channel: Channel;
        }>;
    }
    /**
     * An object mapping of named channels to `WeakRef<Channel>` instances.
     */
    export const registry: {
        /**
         * Subscribes callback `onMessage` to channel of `name`.
         * @param {string} name
         * @param {function} onMessage
         * @return {boolean}
         */
        subscribe(name: string, onMessage: Function): boolean;
        /**
         * Unsubscribes callback `onMessage` from channel of `name`.
         * @param {string} name
         * @param {function} onMessage
         * @return {boolean}
         */
        unsubscribe(name: string, onMessage: Function): boolean;
        /**
         * Predicate to determine if a named channel has subscribers.
         * @param {string} name
         */
        hasSubscribers(name: string): boolean;
        /**
         * Get or set a channel by `name`.
         * @param {string} name
         * @return {Channel}
         */
        channel(name: string): Channel;
        /**
         * Creates a `ChannelGroup` for a set of channels
         * @param {string} name
         * @param {Array<string>} [channels]
         * @return {ChannelGroup}
         */
        group(name: string, channels?: Array<string>): ChannelGroup;
        /**
         * Get a channel by name. The name is normalized.
         * @param {string} name
         * @return {Channel?}
         */
        get(name: string): Channel | null;
        /**
         * Checks if a channel is known by  name. The name is normalized.
         * @param {string} name
         * @return {boolean}
         */
        has(name: string): boolean;
        /**
         * Set a channel by name. The name is normalized.
         * @param {string} name
         * @param {Channel} channel
         * @return {Channel?}
         */
        set(name: string, channel: Channel): Channel | null;
        /**
         * Removes a channel by `name`
         * @return {boolean}
         */
        remove(name: any): boolean;
        /**
         * Returns a string representation of the `ChannelRegistry`.
         * @ignore
         */
        toString(): any;
        /**
         * Returns a JSON representation of the `ChannelRegistry`.
         * @return {object}
         */
        toJSON(): object;
        /**
         * The `ChannelRegistry` string tag.
         * @ignore
         */
        [Symbol.toStringTag](): string;
    };
    export default registry;
}

declare module "socket:diagnostics/metric" {
    export class Metric {
        init(): void;
        update(value: any): void;
        destroy(): void;
        toJSON(): {};
        toString(): string;
        [Symbol.iterator](): any;
        [Symbol.toStringTag](): string;
    }
    export default Metric;
}

declare module "socket:diagnostics/window" {
    export class RequestAnimationFrameMetric extends Metric {
        constructor(options: any);
        originalRequestAnimationFrame: typeof requestAnimationFrame;
        requestAnimationFrame(callback: any): any;
        sampleSize: any;
        sampleTick: number;
        channel: import("socket:diagnostics/channels").Channel;
        value: {
            rate: number;
            samples: number;
        };
        now: number;
        samples: Uint8Array;
        toJSON(): {
            sampleSize: any;
            sampleTick: number;
            samples: number[];
            rate: number;
            now: number;
        };
    }
    export class FetchMetric extends Metric {
        constructor(options: any);
        originalFetch: typeof fetch;
        channel: import("socket:diagnostics/channels").Channel;
        fetch(resource: any, options: any, extra: any): Promise<any>;
    }
    export class XMLHttpRequestMetric extends Metric {
        constructor(options: any);
        channel: import("socket:diagnostics/channels").Channel;
        patched: {
            open: {
                (method: string, url: string | URL): void;
                (method: string, url: string | URL, async: boolean, username?: string | null, password?: string | null): void;
            };
            send: (body?: Document | XMLHttpRequestBodyInit | null) => void;
        };
    }
    export class WorkerMetric extends Metric {
        constructor(options: any);
        GlobalWorker: {
            new (scriptURL: string | URL, options?: WorkerOptions): Worker;
            prototype: Worker;
        } | {
            new (): {};
        };
        channel: import("socket:diagnostics/channels").Channel;
        Worker: {
            new (url: any, options: any, ...args: any[]): {};
        };
    }
    export const metrics: {
        requestAnimationFrame: RequestAnimationFrameMetric;
        XMLHttpRequest: XMLHttpRequestMetric;
        Worker: WorkerMetric;
        fetch: FetchMetric;
        channel: import("socket:diagnostics/channels").ChannelGroup;
        subscribe(...args: any[]): boolean;
        unsubscribe(...args: any[]): boolean;
        start(which: any): void;
        stop(which: any): void;
    };
    namespace _default {
        export { metrics };
    }
    export default _default;
    import { Metric } from "socket:diagnostics/metric";
}

declare module "socket:diagnostics/runtime" {
    /**
     * Queries runtime diagnostics.
     * @return {Promise<QueryDiagnostic>}
     */
    export function query(type: any): Promise<QueryDiagnostic>;
    /**
     * A base container class for diagnostic information.
     */
    export class Diagnostic {
        /**
         * A container for handles related to the diagnostics
         */
        static Handles: {
            new (): {
                /**
                 * The nunmber of handles in this diagnostics.
                 * @type {number}
                 */
                count: number;
                /**
                 * A set of known handle IDs
                 * @type {string[]}
                 */
                ids: string[];
            };
        };
        /**
         * Known handles for this diagnostics.
         * @type {Diagnostic.Handles}
         */
        handles: {
            new (): {
                /**
                 * The nunmber of handles in this diagnostics.
                 * @type {number}
                 */
                count: number;
                /**
                 * A set of known handle IDs
                 * @type {string[]}
                 */
                ids: string[];
            };
        };
    }
    /**
     * A container for libuv diagnostics
     */
    export class UVDiagnostic extends Diagnostic {
        /**
         * A container for libuv metrics.
         */
        static Metrics: {
            new (): {
                /**
                 * The number of event loop iterations.
                 * @type {number}
                 */
                loopCount: number;
                /**
                 * Number of events that have been processed by the event handler.
                 * @type {number}
                 */
                events: number;
                /**
                 * Number of events that were waiting to be processed when the
                 * event provider was called.
                 * @type {number}
                 */
                eventsWaiting: number;
            };
        };
        /**
         * Known libuv metrics for this diagnostic.
         * @type {UVDiagnostic.Metrics}
         */
        metrics: {
            new (): {
                /**
                 * The number of event loop iterations.
                 * @type {number}
                 */
                loopCount: number;
                /**
                 * Number of events that have been processed by the event handler.
                 * @type {number}
                 */
                events: number;
                /**
                 * Number of events that were waiting to be processed when the
                 * event provider was called.
                 * @type {number}
                 */
                eventsWaiting: number;
            };
        };
        /**
         * The current idle time of the libuv loop
         * @type {number}
         */
        idleTime: number;
        /**
         * The number of active requests in the libuv loop
         * @type {number}
         */
        activeRequests: number;
    }
    /**
     * A container for Core Post diagnostics.
     */
    export class PostsDiagnostic extends Diagnostic {
    }
    /**
     * A container for child process diagnostics.
     */
    export class ChildProcessDiagnostic extends Diagnostic {
    }
    /**
     * A container for AI diagnostics.
     */
    export class AIDiagnostic extends Diagnostic {
        /**
         * A container for AI LLM diagnostics.
         */
        static LLMDiagnostic: {
            new (): {
                /**
                 * Known handles for this diagnostics.
                 * @type {Diagnostic.Handles}
                 */
                handles: {
                    new (): {
                        /**
                         * The nunmber of handles in this diagnostics.
                         * @type {number}
                         */
                        count: number;
                        /**
                         * A set of known handle IDs
                         * @type {string[]}
                         */
                        ids: string[];
                    };
                };
            };
            /**
             * A container for handles related to the diagnostics
             */
            Handles: {
                new (): {
                    /**
                     * The nunmber of handles in this diagnostics.
                     * @type {number}
                     */
                    count: number;
                    /**
                     * A set of known handle IDs
                     * @type {string[]}
                     */
                    ids: string[];
                };
            };
        };
        /**
         * Known AI LLM diagnostics.
         * @type {AIDiagnostic.LLMDiagnostic}
         */
        llm: {
            new (): {
                /**
                 * Known handles for this diagnostics.
                 * @type {Diagnostic.Handles}
                 */
                handles: {
                    new (): {
                        /**
                         * The nunmber of handles in this diagnostics.
                         * @type {number}
                         */
                        count: number;
                        /**
                         * A set of known handle IDs
                         * @type {string[]}
                         */
                        ids: string[];
                    };
                };
            };
            /**
             * A container for handles related to the diagnostics
             */
            Handles: {
                new (): {
                    /**
                     * The nunmber of handles in this diagnostics.
                     * @type {number}
                     */
                    count: number;
                    /**
                     * A set of known handle IDs
                     * @type {string[]}
                     */
                    ids: string[];
                };
            };
        };
    }
    /**
     * A container for various filesystem diagnostics.
     */
    export class FSDiagnostic extends Diagnostic {
        /**
         * A container for filesystem watcher diagnostics.
         */
        static WatchersDiagnostic: {
            new (): {
                /**
                 * Known handles for this diagnostics.
                 * @type {Diagnostic.Handles}
                 */
                handles: {
                    new (): {
                        /**
                         * The nunmber of handles in this diagnostics.
                         * @type {number}
                         */
                        count: number;
                        /**
                         * A set of known handle IDs
                         * @type {string[]}
                         */
                        ids: string[];
                    };
                };
            };
            /**
             * A container for handles related to the diagnostics
             */
            Handles: {
                new (): {
                    /**
                     * The nunmber of handles in this diagnostics.
                     * @type {number}
                     */
                    count: number;
                    /**
                     * A set of known handle IDs
                     * @type {string[]}
                     */
                    ids: string[];
                };
            };
        };
        /**
         * A container for filesystem descriptors diagnostics.
         */
        static DescriptorsDiagnostic: {
            new (): {
                /**
                 * Known handles for this diagnostics.
                 * @type {Diagnostic.Handles}
                 */
                handles: {
                    new (): {
                        /**
                         * The nunmber of handles in this diagnostics.
                         * @type {number}
                         */
                        count: number;
                        /**
                         * A set of known handle IDs
                         * @type {string[]}
                         */
                        ids: string[];
                    };
                };
            };
            /**
             * A container for handles related to the diagnostics
             */
            Handles: {
                new (): {
                    /**
                     * The nunmber of handles in this diagnostics.
                     * @type {number}
                     */
                    count: number;
                    /**
                     * A set of known handle IDs
                     * @type {string[]}
                     */
                    ids: string[];
                };
            };
        };
        /**
         * Known FS watcher diagnostics.
         * @type {FSDiagnostic.WatchersDiagnostic}
         */
        watchers: {
            new (): {
                /**
                 * Known handles for this diagnostics.
                 * @type {Diagnostic.Handles}
                 */
                handles: {
                    new (): {
                        /**
                         * The nunmber of handles in this diagnostics.
                         * @type {number}
                         */
                        count: number;
                        /**
                         * A set of known handle IDs
                         * @type {string[]}
                         */
                        ids: string[];
                    };
                };
            };
            /**
             * A container for handles related to the diagnostics
             */
            Handles: {
                new (): {
                    /**
                     * The nunmber of handles in this diagnostics.
                     * @type {number}
                     */
                    count: number;
                    /**
                     * A set of known handle IDs
                     * @type {string[]}
                     */
                    ids: string[];
                };
            };
        };
        /**
         * @type {FSDiagnostic.DescriptorsDiagnostic}
         */
        descriptors: {
            new (): {
                /**
                 * Known handles for this diagnostics.
                 * @type {Diagnostic.Handles}
                 */
                handles: {
                    new (): {
                        /**
                         * The nunmber of handles in this diagnostics.
                         * @type {number}
                         */
                        count: number;
                        /**
                         * A set of known handle IDs
                         * @type {string[]}
                         */
                        ids: string[];
                    };
                };
            };
            /**
             * A container for handles related to the diagnostics
             */
            Handles: {
                new (): {
                    /**
                     * The nunmber of handles in this diagnostics.
                     * @type {number}
                     */
                    count: number;
                    /**
                     * A set of known handle IDs
                     * @type {string[]}
                     */
                    ids: string[];
                };
            };
        };
    }
    /**
     * A container for various timers diagnostics.
     */
    export class TimersDiagnostic extends Diagnostic {
        /**
         * A container for core timeout timer diagnostics.
         */
        static TimeoutDiagnostic: {
            new (): {
                /**
                 * Known handles for this diagnostics.
                 * @type {Diagnostic.Handles}
                 */
                handles: {
                    new (): {
                        /**
                         * The nunmber of handles in this diagnostics.
                         * @type {number}
                         */
                        count: number;
                        /**
                         * A set of known handle IDs
                         * @type {string[]}
                         */
                        ids: string[];
                    };
                };
            };
            /**
             * A container for handles related to the diagnostics
             */
            Handles: {
                new (): {
                    /**
                     * The nunmber of handles in this diagnostics.
                     * @type {number}
                     */
                    count: number;
                    /**
                     * A set of known handle IDs
                     * @type {string[]}
                     */
                    ids: string[];
                };
            };
        };
        /**
         * A container for core interval timer diagnostics.
         */
        static IntervalDiagnostic: {
            new (): {
                /**
                 * Known handles for this diagnostics.
                 * @type {Diagnostic.Handles}
                 */
                handles: {
                    new (): {
                        /**
                         * The nunmber of handles in this diagnostics.
                         * @type {number}
                         */
                        count: number;
                        /**
                         * A set of known handle IDs
                         * @type {string[]}
                         */
                        ids: string[];
                    };
                };
            };
            /**
             * A container for handles related to the diagnostics
             */
            Handles: {
                new (): {
                    /**
                     * The nunmber of handles in this diagnostics.
                     * @type {number}
                     */
                    count: number;
                    /**
                     * A set of known handle IDs
                     * @type {string[]}
                     */
                    ids: string[];
                };
            };
        };
        /**
         * A container for core immediate timer diagnostics.
         */
        static ImmediateDiagnostic: {
            new (): {
                /**
                 * Known handles for this diagnostics.
                 * @type {Diagnostic.Handles}
                 */
                handles: {
                    new (): {
                        /**
                         * The nunmber of handles in this diagnostics.
                         * @type {number}
                         */
                        count: number;
                        /**
                         * A set of known handle IDs
                         * @type {string[]}
                         */
                        ids: string[];
                    };
                };
            };
            /**
             * A container for handles related to the diagnostics
             */
            Handles: {
                new (): {
                    /**
                     * The nunmber of handles in this diagnostics.
                     * @type {number}
                     */
                    count: number;
                    /**
                     * A set of known handle IDs
                     * @type {string[]}
                     */
                    ids: string[];
                };
            };
        };
        /**
         * @type {TimersDiagnostic.TimeoutDiagnostic}
         */
        timeout: {
            new (): {
                /**
                 * Known handles for this diagnostics.
                 * @type {Diagnostic.Handles}
                 */
                handles: {
                    new (): {
                        /**
                         * The nunmber of handles in this diagnostics.
                         * @type {number}
                         */
                        count: number;
                        /**
                         * A set of known handle IDs
                         * @type {string[]}
                         */
                        ids: string[];
                    };
                };
            };
            /**
             * A container for handles related to the diagnostics
             */
            Handles: {
                new (): {
                    /**
                     * The nunmber of handles in this diagnostics.
                     * @type {number}
                     */
                    count: number;
                    /**
                     * A set of known handle IDs
                     * @type {string[]}
                     */
                    ids: string[];
                };
            };
        };
        /**
         * @type {TimersDiagnostic.IntervalDiagnostic}
         */
        interval: {
            new (): {
                /**
                 * Known handles for this diagnostics.
                 * @type {Diagnostic.Handles}
                 */
                handles: {
                    new (): {
                        /**
                         * The nunmber of handles in this diagnostics.
                         * @type {number}
                         */
                        count: number;
                        /**
                         * A set of known handle IDs
                         * @type {string[]}
                         */
                        ids: string[];
                    };
                };
            };
            /**
             * A container for handles related to the diagnostics
             */
            Handles: {
                new (): {
                    /**
                     * The nunmber of handles in this diagnostics.
                     * @type {number}
                     */
                    count: number;
                    /**
                     * A set of known handle IDs
                     * @type {string[]}
                     */
                    ids: string[];
                };
            };
        };
        /**
         * @type {TimersDiagnostic.ImmediateDiagnostic}
         */
        immediate: {
            new (): {
                /**
                 * Known handles for this diagnostics.
                 * @type {Diagnostic.Handles}
                 */
                handles: {
                    new (): {
                        /**
                         * The nunmber of handles in this diagnostics.
                         * @type {number}
                         */
                        count: number;
                        /**
                         * A set of known handle IDs
                         * @type {string[]}
                         */
                        ids: string[];
                    };
                };
            };
            /**
             * A container for handles related to the diagnostics
             */
            Handles: {
                new (): {
                    /**
                     * The nunmber of handles in this diagnostics.
                     * @type {number}
                     */
                    count: number;
                    /**
                     * A set of known handle IDs
                     * @type {string[]}
                     */
                    ids: string[];
                };
            };
        };
    }
    /**
     * A container for UDP diagnostics.
     */
    export class UDPDiagnostic extends Diagnostic {
    }
    /**
     * A container for various queried runtime diagnostics.
     */
    export class QueryDiagnostic {
        posts: PostsDiagnostic;
        childProcess: ChildProcessDiagnostic;
        ai: AIDiagnostic;
        fs: FSDiagnostic;
        timers: TimersDiagnostic;
        udp: UDPDiagnostic;
        uv: UVDiagnostic;
    }
    namespace _default {
        export { query };
    }
    export default _default;
}

declare module "socket:diagnostics/index" {
    /**
     * @param {string} name
     * @return {import('./channels.js').Channel}
     */
    export function channel(name: string): import("socket:diagnostics/channels").Channel;
    export default exports;
    import * as exports from "socket:diagnostics/index";
    import channels from "socket:diagnostics/channels";
    import window from "socket:diagnostics/window";
    import runtime from "socket:diagnostics/runtime";
    
    export { channels, window, runtime };
}

declare module "socket:diagnostics" {
    export * from "socket:diagnostics/index";
    export default exports;
    import * as exports from "socket:diagnostics/index";
}

declare module "socket:fs/stats" {
    /**
     * A container for various stats about a file or directory.
     */
    export class Stats {
        /**
         * Creates a `Stats` instance from input, optionally with `BigInt` data types
         * @param {object|Stats} [stat]
         * @param {fromBigInt=} [fromBigInt = false]
         * @return {Stats}
         */
        static from(stat?: object | Stats, fromBigInt?: any | undefined): Stats;
        /**
         * `Stats` class constructor.
         * @param {object|Stats} stat
         */
        constructor(stat: object | Stats);
        dev: any;
        ino: any;
        mode: any;
        nlink: any;
        uid: any;
        gid: any;
        rdev: any;
        size: any;
        blksize: any;
        blocks: any;
        atimeMs: any;
        mtimeMs: any;
        ctimeMs: any;
        birthtimeMs: any;
        atime: Date;
        mtime: Date;
        ctime: Date;
        birthtime: Date;
        /**
         * Returns `true` if stats represents a directory.
         * @return {Boolean}
         */
        isDirectory(): boolean;
        /**
         * Returns `true` if stats represents a file.
         * @return {Boolean}
         */
        isFile(): boolean;
        /**
         * Returns `true` if stats represents a block device.
         * @return {Boolean}
         */
        isBlockDevice(): boolean;
        /**
         * Returns `true` if stats represents a character device.
         * @return {Boolean}
         */
        isCharacterDevice(): boolean;
        /**
         * Returns `true` if stats represents a symbolic link.
         * @return {Boolean}
         */
        isSymbolicLink(): boolean;
        /**
         * Returns `true` if stats represents a FIFO.
         * @return {Boolean}
         */
        isFIFO(): boolean;
        /**
         * Returns `true` if stats represents a socket.
         * @return {Boolean}
         */
        isSocket(): boolean;
    }
    export default exports;
    import * as exports from "socket:fs/stats";
    
}

declare module "socket:fs/fds" {
    const _default: {
        types: Map<any, any>;
        fds: Map<any, any>;
        ids: Map<any, any>;
        readonly size: number;
        get(id: any): any;
        syncOpenDescriptors(): Promise<void>;
        set(id: any, fd: any, type: any): void;
        has(id: any): boolean;
        fd(id: any): any;
        id(fd: any): any;
        release(id: any, closeDescriptor?: boolean): Promise<void>;
        retain(id: any): Promise<any>;
        delete(id: any): void;
        clear(): void;
        typeof(id: any): any;
        entries(): IterableIterator<[any, any]>;
    };
    export default _default;
}

declare module "socket:fs/handle" {
    export const kOpening: unique symbol;
    export const kClosing: unique symbol;
    export const kClosed: unique symbol;
    /**
     * A container for a descriptor tracked in `fds` and opened in the native layer.
     * This class implements the Node.js `FileHandle` interface
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#class-filehandle}
     */
    export class FileHandle extends EventEmitter {
        static get DEFAULT_ACCESS_MODE(): number;
        static get DEFAULT_OPEN_FLAGS(): string;
        static get DEFAULT_OPEN_MODE(): number;
        /**
         * Creates a `FileHandle` from a given `id` or `fd`
         * @param {string|number|FileHandle|object|FileSystemFileHandle} id
         * @return {FileHandle}
         */
        static from(id: string | number | FileHandle | object | FileSystemFileHandle): FileHandle;
        /**
         * Determines if access to `path` for `mode` is possible.
         * @param {string} path
         * @param {number} [mode = 0o666]
         * @param {object=} [options]
         * @return {Promise<boolean>}
         */
        static access(path: string, mode?: number, options?: object | undefined): Promise<boolean>;
        /**
         * Asynchronously open a file.
         * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromisesopenpath-flags-mode}
         * @param {string | Buffer | URL} path
         * @param {string=} [flags = 'r']
         * @param {string|number=} [mode = 0o666]
         * @param {object=} [options]
         * @return {Promise<FileHandle>}
         */
        static open(path: string | Buffer | URL, flags?: string | undefined, mode?: (string | number) | undefined, options?: object | undefined): Promise<FileHandle>;
        /**
         * `FileHandle` class constructor
         * @ignore
         * @param {object} options
         */
        constructor(options: object);
        flags: number;
        path: any;
        mode: any;
        id: string;
        fd: any;
        /**
         * `true` if the `FileHandle` instance has been opened.
         * @type {boolean}
         */
        get opened(): boolean;
        /**
         * `true` if the `FileHandle` is opening.
         * @type {boolean}
         */
        get opening(): boolean;
        /**
         * `true` if the `FileHandle` is closing.
         * @type {boolean}
         */
        get closing(): boolean;
        /**
         * `true` if the `FileHandle` is closed.
         */
        get closed(): boolean;
        /**
         * Appends to a file, if handle was opened with `O_APPEND`, otherwise this
         * method is just an alias to `FileHandle#writeFile()`.
         * @param {string|Buffer|TypedArray|Array} data
         * @param {object=} [options]
         * @param {string=} [options.encoding = 'utf8']
         * @param {object=} [options.signal]
         */
        appendFile(data: string | Buffer | TypedArray | any[], options?: object | undefined): Promise<TypeError | {
            buffer: any;
            bytesWritten: number;
        }>;
        /**
         * Change permissions of file handle.
         * @param {number} mode
         * @param {object=} [options]
         */
        chmod(mode: number, options?: object | undefined): Promise<TypeError>;
        /**
         * Change ownership of file handle.
         * @param {number} uid
         * @param {number} gid
         * @param {object=} [options]
         */
        chown(uid: number, gid: number, options?: object | undefined): Promise<TypeError>;
        /**
         * Close underlying file handle
         * @param {object=} [options]
         */
        close(options?: object | undefined): Promise<any>;
        /**
         * Creates a `ReadStream` for the underlying file.
         * @param {object=} [options] - An options object
         */
        createReadStream(options?: object | undefined): ReadStream;
        /**
         * Creates a `WriteStream` for the underlying file.
         * @param {object=} [options] - An options object
         */
        createWriteStream(options?: object | undefined): WriteStream;
        /**
         * @param {object=} [options]
         */
        datasync(): Promise<TypeError>;
        /**
         * Opens the underlying descriptor for the file handle.
         * @param {object=} [options]
         */
        open(options?: object | undefined): Promise<any>;
        /**
         * Reads `length` bytes starting from `position` into `buffer` at
         * `offset`.
         * @param {Buffer|object} buffer
         * @param {number=} [offset]
         * @param {number=} [length]
         * @param {number=} [position]
         * @param {object=} [options]
         */
        read(buffer: Buffer | object, offset?: number | undefined, length?: number | undefined, position?: number | undefined, options?: object | undefined): Promise<{
            bytesRead: number;
            buffer: any;
        }>;
        /**
         * Reads the entire contents of a file and returns it as a buffer or a string
         * specified of a given encoding specified at `options.encoding`.
         * @param {object=} [options]
         * @param {string=} [options.encoding = 'utf8']
         * @param {object=} [options.signal]
         */
        readFile(options?: object | undefined): Promise<string | Uint8Array>;
        /**
         * Returns the stats of the underlying file.
         * @param {object=} [options]
         * @return {Promise<Stats>}
         */
        stat(options?: object | undefined): Promise<Stats>;
        /**
         * Returns the stats of the underlying symbolic link.
         * @param {object=} [options]
         * @return {Promise<Stats>}
         */
        lstat(options?: object | undefined): Promise<Stats>;
        /**
         * Synchronize a file's in-core state with storage device
         * @return {Promise}
         */
        sync(): Promise<any>;
        /**
         * @param {number} [offset = 0]
         * @return {Promise}
         */
        truncate(offset?: number): Promise<any>;
        /**
         * Writes `length` bytes at `offset` in `buffer` to the underlying file
         * at `position`.
         * @param {Buffer|object} buffer
         * @param {number} offset
         * @param {number} length
         * @param {number} position
         * @param {object=} [options]
         */
        write(buffer: Buffer | object, offset: number, length: number, position: number, options?: object | undefined): Promise<TypeError | {
            buffer: any;
            bytesWritten: number;
        }>;
        /**
         * Writes `data` to file.
         * @param {string|Buffer|TypedArray|Array} data
         * @param {object=} [options]
         * @param {string=} [options.encoding = 'utf8']
         * @param {object=} [options.signal]
         */
        writeFile(data: string | Buffer | TypedArray | any[], options?: object | undefined): Promise<TypeError>;
        [exports.kOpening]: any;
        [exports.kClosing]: any;
        [exports.kClosed]: boolean;
        #private;
    }
    /**
     * A container for a directory handle tracked in `fds` and opened in the
     * native layer.
     */
    export class DirectoryHandle extends EventEmitter {
        /**
         * The max number of entries that can be bufferd with the `bufferSize`
         * option.
         */
        static get MAX_BUFFER_SIZE(): number;
        static get MAX_ENTRIES(): number;
        /**
         * The default number of entries `Dirent` that are buffered
         * for each read request.
         */
        static get DEFAULT_BUFFER_SIZE(): number;
        /**
         * Creates a `DirectoryHandle` from a given `id` or `fd`
         * @param {string|number|DirectoryHandle|object|FileSystemDirectoryHandle} id
         * @param {object} options
         * @return {DirectoryHandle}
         */
        static from(id: string | number | DirectoryHandle | object | FileSystemDirectoryHandle, options: object): DirectoryHandle;
        /**
         * Asynchronously open a directory.
         * @param {string | Buffer | URL} path
         * @param {object=} [options]
         * @return {Promise<DirectoryHandle>}
         */
        static open(path: string | Buffer | URL, options?: object | undefined): Promise<DirectoryHandle>;
        /**
         * `DirectoryHandle` class constructor
         * @private
         * @param {object} options
         */
        private constructor();
        id: string;
        path: any;
        bufferSize: number;
        /**
         * DirectoryHandle file descriptor id
         */
        get fd(): string;
        /**
         * `true` if the `DirectoryHandle` instance has been opened.
         * @type {boolean}
         */
        get opened(): boolean;
        /**
         * `true` if the `DirectoryHandle` is opening.
         * @type {boolean}
         */
        get opening(): boolean;
        /**
         * `true` if the `DirectoryHandle` is closing.
         * @type {boolean}
         */
        get closing(): boolean;
        /**
         * `true` if `DirectoryHandle` is closed.
         */
        get closed(): boolean;
        /**
         * Opens the underlying handle for a directory.
         * @param {object=} options
         * @return {Promise<boolean>}
         */
        open(options?: object | undefined): Promise<boolean>;
        /**
         * Close underlying directory handle
         * @param {object=} [options]
         */
        close(options?: object | undefined): Promise<any>;
        /**
         * Reads directory entries
         * @param {object=} [options]
         * @param {number=} [options.entries = DirectoryHandle.MAX_ENTRIES]
         */
        read(options?: object | undefined): Promise<any>;
        [exports.kOpening]: any;
        [exports.kClosing]: any;
        [exports.kClosed]: boolean;
        #private;
    }
    export default exports;
    export type TypedArray = Uint8Array | Int8Array;
    import { EventEmitter } from "socket:events";
    import { Buffer } from "socket:buffer";
    import { ReadStream } from "socket:fs/stream";
    import { WriteStream } from "socket:fs/stream";
    import { Stats } from "socket:fs/stats";
    import * as exports from "socket:fs/handle";
    
}

declare module "socket:fs/dir" {
    /**
     * Sorts directory entries
     * @param {string|Dirent} a
     * @param {string|Dirent} b
     * @return {number}
     */
    export function sortDirectoryEntries(a: string | Dirent, b: string | Dirent): number;
    export const kType: unique symbol;
    /**
     * A containerr for a directory and its entries. This class supports scanning
     * a directory entry by entry with a `read()` method. The `Symbol.asyncIterator`
     * interface is exposed along with an AsyncGenerator `entries()` method.
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#class-fsdir}
     */
    export class Dir {
        static from(fdOrHandle: any, options: any): exports.Dir;
        /**
         * `Dir` class constructor.
         * @param {DirectoryHandle} handle
         * @param {object=} options
         */
        constructor(handle: DirectoryHandle, options?: object | undefined);
        path: any;
        handle: DirectoryHandle;
        encoding: any;
        withFileTypes: boolean;
        /**
         * `true` if closed, otherwise `false`.
         * @ignore
         * @type {boolean}
         */
        get closed(): boolean;
        /**
         * `true` if closing, otherwise `false`.
         * @ignore
         * @type {boolean}
         */
        get closing(): boolean;
        /**
         * Closes container and underlying handle.
         * @param {object|function} options
         * @param {function=} callback
         */
        close(options?: object | Function, callback?: Function | undefined): Promise<any>;
        /**
         * Closes container and underlying handle
         * synchronously.
         * @param {object=} [options]
         */
        closeSync(options?: object | undefined): void;
        /**
         * Reads and returns directory entry.
         * @param {object|function} options
         * @param {function=} callback
         * @return {Promise<Dirent[]|string[]>}
         */
        read(options: object | Function, callback?: Function | undefined): Promise<Dirent[] | string[]>;
        /**
         * Reads and returns directory entry synchronously.
         * @param {object|function} options
         * @return {Dirent[]|string[]}
         */
        readSync(options?: object | Function): Dirent[] | string[];
        /**
         * AsyncGenerator which yields directory entries.
         * @param {object=} options
         */
        entries(options?: object | undefined): AsyncGenerator<string | exports.Dirent, void, unknown>;
        /**
         * `for await (...)` AsyncGenerator support.
         */
        get [Symbol.asyncIterator](): (options?: object | undefined) => AsyncGenerator<string | exports.Dirent, void, unknown>;
    }
    /**
     * A container for a directory entry.
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#class-fsdirent}
     */
    export class Dirent {
        static get UNKNOWN(): number;
        static get FILE(): number;
        static get DIR(): number;
        static get LINK(): number;
        static get FIFO(): number;
        static get SOCKET(): number;
        static get CHAR(): number;
        static get BLOCK(): number;
        /**
         * Creates `Dirent` instance from input.
         * @param {object|string} name
         * @param {(string|number)=} type
         */
        static from(name: object | string, type?: (string | number) | undefined): exports.Dirent;
        /**
         * `Dirent` class constructor.
         * @param {string} name
         * @param {string|number} type
         */
        constructor(name: string, type: string | number);
        name: string;
        /**
         * Read only type.
         */
        get type(): number;
        /**
         * `true` if `Dirent` instance is a directory.
         */
        isDirectory(): boolean;
        /**
         * `true` if `Dirent` instance is a file.
         */
        isFile(): boolean;
        /**
         * `true` if `Dirent` instance is a block device.
         */
        isBlockDevice(): boolean;
        /**
         * `true` if `Dirent` instance is a character device.
         */
        isCharacterDevice(): boolean;
        /**
         * `true` if `Dirent` instance is a symbolic link.
         */
        isSymbolicLink(): boolean;
        /**
         * `true` if `Dirent` instance is a FIFO.
         */
        isFIFO(): boolean;
        /**
         * `true` if `Dirent` instance is a socket.
         */
        isSocket(): boolean;
        [exports.kType]: number;
    }
    export default exports;
    import { DirectoryHandle } from "socket:fs/handle";
    import * as exports from "socket:fs/dir";
    
}

declare module "socket:hooks" {
    /**
     * Wait for a hook event to occur.
     * @template {Event | T extends Event}
     * @param {string|function} nameOrFunction
     * @return {Promise<T>}
     */
    export function wait(nameOrFunction: string | Function): Promise<T>;
    /**
     * Wait for the global Window, Document, and Runtime to be ready.
     * The callback function is called exactly once.
     * @param {function} callback
     * @return {function}
     */
    export function onReady(callback: Function): Function;
    /**
     * Wait for the global Window and Document to be ready. The callback
     * function is called exactly once.
     * @param {function} callback
     * @return {function}
     */
    export function onLoad(callback: Function): Function;
    /**
     * Wait for the runtime to be ready. The callback
     * function is called exactly once.
     * @param {function} callback
     * @return {function}
     */
    export function onInit(callback: Function): Function;
    /**
     * Calls callback when a global exception occurs.
     * 'error', 'messageerror', and 'unhandledrejection' events are handled here.
     * @param {function} callback
     * @return {function}
     */
    export function onError(callback: Function): Function;
    /**
     * Subscribes to the global data pipe calling callback when
     * new data is emitted on the global Window.
     * @param {function} callback
     * @return {function}
     */
    export function onData(callback: Function): Function;
    /**
     * Subscribes to global messages likely from an external `postMessage`
     * invocation.
     * @param {function} callback
     * @return {function}
     */
    export function onMessage(callback: Function): Function;
    /**
     * Calls callback when runtime is working online.
     * @param {function} callback
     * @return {function}
     */
    export function onOnline(callback: Function): Function;
    /**
     * Calls callback when runtime is not working online.
     * @param {function} callback
     * @return {function}
     */
    export function onOffline(callback: Function): Function;
    /**
     * Calls callback when runtime user preferred language has changed.
     * @param {function} callback
     * @return {function}
     */
    export function onLanguageChange(callback: Function): Function;
    /**
     * Calls callback when an application permission has changed.
     * @param {function} callback
     * @return {function}
     */
    export function onPermissionChange(callback: Function): Function;
    /**
     * Calls callback in response to a presented `Notification`.
     * @param {function} callback
     * @return {function}
     */
    export function onNotificationResponse(callback: Function): Function;
    /**
     * Calls callback when a `Notification` is presented.
     * @param {function} callback
     * @return {function}
     */
    export function onNotificationPresented(callback: Function): Function;
    /**
     * Calls callback when a `ApplicationURL` is opened.
     * @param {function(ApplicationURLEvent)} callback
     * @return {function}
     */
    export function onApplicationURL(callback: (arg0: ApplicationURLEvent) => any): Function;
    /**
     * Calls callback when a `ApplicationPause` is dispatched.
     * @param {function} callback
     * @return {function}
     */
    export function onApplicationPause(callback: Function): Function;
    /**
     * Calls callback when a `ApplicationResume` is dispatched.
     * @param {function} callback
     * @return {function}
     */
    export function onApplicationResume(callback: Function): Function;
    export const RUNTIME_INIT_EVENT_NAME: "__runtime_init__";
    export const GLOBAL_EVENTS: string[];
    /**
     * An event dispatched when the runtime has been initialized.
     */
    export class InitEvent {
        constructor();
    }
    /**
     * An event dispatched when the runtime global has been loaded.
     */
    export class LoadEvent {
        constructor();
    }
    /**
     * An event dispatched when the runtime is considered ready.
     */
    export class ReadyEvent {
        constructor();
    }
    /**
     * An event dispatched when the runtime has been initialized.
     */
    export class RuntimeInitEvent {
        constructor();
    }
    /**
     * An interface for registering callbacks for various hooks in
     * the runtime.
     */
    export class Hooks extends EventTarget {
        /**
         * @ignore
         */
        static GLOBAL_EVENTS: string[];
        /**
         * @ignore
         */
        static InitEvent: typeof InitEvent;
        /**
         * @ignore
         */
        static LoadEvent: typeof LoadEvent;
        /**
         * @ignore
         */
        static ReadyEvent: typeof ReadyEvent;
        /**
         * @ignore
         */
        static RuntimeInitEvent: typeof RuntimeInitEvent;
        /**
         * An array of all global events listened to in various hooks
         */
        get globalEvents(): string[];
        /**
         * Reference to global object
         * @type {object}
         */
        get global(): any;
        /**
         * Returns `document` in global.
         * @type {Document}
         */
        get document(): Document;
        /**
         * Returns `document` in global.
         * @type {Window}
         */
        get window(): Window;
        /**
         * Predicate for determining if the global document is ready.
         * @type {boolean}
         */
        get isDocumentReady(): boolean;
        /**
         * Predicate for determining if the global object is ready.
         * @type {boolean}
         */
        get isGlobalReady(): boolean;
        /**
         * Predicate for determining if the runtime is ready.
         * @type {boolean}
         */
        get isRuntimeReady(): boolean;
        /**
         * Predicate for determining if everything is ready.
         * @type {boolean}
         */
        get isReady(): boolean;
        /**
         * Predicate for determining if the runtime is working online.
         * @type {boolean}
         */
        get isOnline(): boolean;
        /**
         * Predicate for determining if the runtime is in a Worker context.
         * @type {boolean}
         */
        get isWorkerContext(): boolean;
        /**
         * Predicate for determining if the runtime is in a Window context.
         * @type {boolean}
         */
        get isWindowContext(): boolean;
        /**
         * Wait for a hook event to occur.
         * @template {Event | T extends Event}
         * @param {string|function} nameOrFunction
         * @param {WaitOptions=} [options]
         * @return {Promise<T>}
         */
        wait(nameOrFunction: string | Function, options?: WaitOptions | undefined): Promise<T>;
        /**
         * Wait for the global Window, Document, and Runtime to be ready.
         * The callback function is called exactly once.
         * @param {function} callback
         * @return {function}
         */
        onReady(callback: Function): Function;
        /**
         * Wait for the global Window and Document to be ready. The callback
         * function is called exactly once.
         * @param {function} callback
         * @return {function}
         */
        onLoad(callback: Function): Function;
        /**
         * Wait for the runtime to be ready. The callback
         * function is called exactly once.
         * @param {function} callback
         * @return {function}
         */
        onInit(callback: Function): Function;
        /**
         * Calls callback when a global exception occurs.
         * 'error', 'messageerror', and 'unhandledrejection' events are handled here.
         * @param {function} callback
         * @return {function}
         */
        onError(callback: Function): Function;
        /**
         * Subscribes to the global data pipe calling callback when
         * new data is emitted on the global Window.
         * @param {function} callback
         * @return {function}
         */
        onData(callback: Function): Function;
        /**
         * Subscribes to global messages likely from an external `postMessage`
         * invocation.
         * @param {function} callback
         * @return {function}
         */
        onMessage(callback: Function): Function;
        /**
         * Calls callback when runtime is working online.
         * @param {function} callback
         * @return {function}
         */
        onOnline(callback: Function): Function;
        /**
         * Calls callback when runtime is not working online.
         * @param {function} callback
         * @return {function}
         */
        onOffline(callback: Function): Function;
        /**
         * Calls callback when runtime user preferred language has changed.
         * @param {function} callback
         * @return {function}
         */
        onLanguageChange(callback: Function): Function;
        /**
         * Calls callback when an application permission has changed.
         * @param {function} callback
         * @return {function}
         */
        onPermissionChange(callback: Function): Function;
        /**
         * Calls callback in response to a displayed `Notification`.
         * @param {function} callback
         * @return {function}
         */
        onNotificationResponse(callback: Function): Function;
        /**
         * Calls callback when a `Notification` is presented.
         * @param {function} callback
         * @return {function}
         */
        onNotificationPresented(callback: Function): Function;
        /**
         * Calls callback when a `ApplicationURL` is opened.
         * @param {function} callback
         * @return {function}
         */
        onApplicationURL(callback: Function): Function;
        /**
         * Calls callback when an `ApplicationPause` is dispatched.
         * @param {function} callback
         * @return {function}
         */
        onApplicationPause(callback: Function): Function;
        /**
         * Calls callback when an `ApplicationResume` is dispatched.
         * @param {function} callback
         * @return {function}
         */
        onApplicationResume(callback: Function): Function;
        #private;
    }
    export default hooks;
    export type WaitOptions = {
        signal?: AbortSignal;
    };
    /**
     * `Hooks` single instance.
     * @ignore
     */
    const hooks: Hooks;
}

declare module "socket:fs/watcher" {
    /**
     * A container for a file system path watcher.
     */
    export class Watcher extends EventEmitter {
        /**
         * `Watcher` class constructor.
         * @ignore
         * @param {string} path
         * @param {object=} [options]
         * @param {AbortSignal=} [options.signal}
         * @param {string|number|bigint=} [options.id]
         * @param {string=} [options.encoding = 'utf8']
         */
        constructor(path: string, options?: object | undefined);
        /**
         * The underlying `fs.Watcher` resource id.
         * @ignore
         * @type {string}
         */
        id: string;
        /**
         * The path the `fs.Watcher` is watching
         * @type {string}
         */
        path: string;
        /**
         * `true` if closed, otherwise `false.
         * @type {boolean}
         */
        closed: boolean;
        /**
         * `true` if aborted, otherwise `false`.
         * @type {boolean}
         */
        aborted: boolean;
        /**
         * The encoding of the `filename`
         * @type {'utf8'|'buffer'}
         */
        encoding: "utf8" | "buffer";
        /**
         * A `AbortController` `AbortSignal` for async aborts.
         * @type {AbortSignal?}
         */
        signal: AbortSignal | null;
        /**
         * Internal event listener cancellation.
         * @ignore
         * @type {function?}
         */
        stopListening: Function | null;
        /**
         * Internal starter for watcher.
         * @ignore
         */
        start(): Promise<void>;
        /**
         * Closes watcher and stops listening for changes.
         * @return {Promise}
         */
        close(): Promise<any>;
        /**
         * Implements the `AsyncIterator` (`Symbol.asyncIterator`) iterface.
         * @ignore
         * @return {AsyncIterator<{ eventType: string, filename: string }>}
         */
        [Symbol.asyncIterator](): AsyncIterator<{
            eventType: string;
            filename: string;
        }>;
        #private;
    }
    export default Watcher;
    import { EventEmitter } from "socket:events";
}

declare module "socket:fs/promises" {
    /**
     * Asynchronously check access a file.
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromisesaccesspath-mode}
     * @param {string|Buffer|URL} path
     * @param {number=} [mode]
     * @param {object=} [options]
     */
    export function access(path: string | Buffer | URL, mode?: number | undefined, options?: object | undefined): Promise<boolean>;
    /**
     * @see {@link https://nodejs.org/api/fs.html#fspromiseschmodpath-mode}
     * @param {string | Buffer | URL} path
     * @param {number} mode
     * @returns {Promise<void>}
     */
    export function chmod(path: string | Buffer | URL, mode: number): Promise<void>;
    /**
     * Changes ownership of file or directory at `path` with `uid` and `gid`.
     * @param {string} path
     * @param {number} uid
     * @param {number} gid
     * @return {Promise}
     */
    export function chown(path: string, uid: number, gid: number): Promise<any>;
    /**
     * Asynchronously copies `src` to `dest` calling `callback` upon success or error.
     * @param {string} src - The source file path.
     * @param {string} dest - The destination file path.
     * @param {number} flags - Modifiers for copy operation.
     * @return {Promise}
     */
    export function copyFile(src: string, dest: string, flags?: number): Promise<any>;
    /**
     * Chages ownership of link at `path` with `uid` and `gid.
     * @param {string} path
     * @param {number} uid
     * @param {number} gid
     * @return {Promise}
     */
    export function lchown(path: string, uid: number, gid: number): Promise<any>;
    /**
     * Creates a link to `dest` from `dest`.
     * @param {string} src
     * @param {string} dest
     * @return {Promise}
     */
    export function link(src: string, dest: string): Promise<any>;
    /**
     * Asynchronously creates a directory.
     *
     * @param {string} path - The path to create
     * @param {object} [options] - The optional options argument can be an integer specifying mode (permission and sticky bits), or an object with a mode property and a recursive property indicating whether parent directories should be created. Calling fs.mkdir() when path is a directory that exists results in an error only when recursive is false.
     * @param {boolean} [options.recursive=false] - Recursively create missing path segments.
     * @param {number} [options.mode=0o777] - Set the mode of directory, or missing path segments when recursive is true.
     * @return {Promise} - Upon success, fulfills with undefined if recursive is false, or the first directory path created if recursive is true.
     */
    export function mkdir(path: string, options?: {
        recursive?: boolean;
        mode?: number;
    }): Promise<any>;
    /**
     * Asynchronously open a file.
     * @see {@link https://nodejs.org/api/fs.html#fspromisesopenpath-flags-mode }
     *
     * @param {string | Buffer | URL} path
     * @param {string=} flags - default: 'r'
     * @param {number=} mode - default: 0o666
     * @return {Promise<FileHandle>}
     */
    export function open(path: string | Buffer | URL, flags?: string | undefined, mode?: number | undefined): Promise<FileHandle>;
    /**
     * @see {@link https://nodejs.org/api/fs.html#fspromisesopendirpath-options}
     * @param {string | Buffer | URL} path
     * @param {object=} [options]
     * @param {string=} [options.encoding = 'utf8']
     * @param {number=} [options.bufferSize = 32]
     * @return {Promise<Dir>}
     */
    export function opendir(path: string | Buffer | URL, options?: object | undefined): Promise<Dir>;
    /**
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromisesreaddirpath-options}
     * @param {string|Buffer|URL} path
     * @param {object=} [options]
     * @param {string=} [options.encoding = 'utf8']
     * @param {boolean=} [options.withFileTypes = false]
     * @return {Promise<(string|Dirent)[]>}
     */
    export function readdir(path: string | Buffer | URL, options?: object | undefined): Promise<(string | Dirent)[]>;
    /**
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromisesreadfilepath-options}
     * @param {string} path
     * @param {object=} [options]
     * @param {(string|null)=} [options.encoding = null]
     * @param {string=} [options.flag = 'r']
     * @param {AbortSignal|undefined} [options.signal]
     * @return {Promise<Buffer | string>}
     */
    export function readFile(path: string, options?: object | undefined): Promise<Buffer | string>;
    /**
     * Reads link at `path`
     * @param {string} path
     * @return {Promise<string>}
     */
    export function readlink(path: string): Promise<string>;
    /**
     * Computes real path for `path`
     * @param {string} path
     * @return {Promise<string>}
     */
    export function realpath(path: string): Promise<string>;
    /**
     * Renames file or directory at `src` to `dest`.
     * @param {string} src
     * @param {string} dest
     * @return {Promise}
     */
    export function rename(src: string, dest: string): Promise<any>;
    /**
     * Removes directory at `path`.
     * @param {string} path
     * @return {Promise}
     */
    export function rmdir(path: string): Promise<any>;
    /**
     * Get the stats of a file
     * @see {@link https://nodejs.org/api/fs.html#fspromisesstatpath-options}
     * @param {string | Buffer | URL} path
     * @param {object=} [options]
     * @param {boolean=} [options.bigint = false]
     * @return {Promise<Stats>}
     */
    export function stat(path: string | Buffer | URL, options?: object | undefined): Promise<Stats>;
    /**
     * Get the stats of a symbolic link.
     * @see {@link https://nodejs.org/api/fs.html#fspromiseslstatpath-options}
     * @param {string | Buffer | URL} path
     * @param {object=} [options]
     * @param {boolean=} [options.bigint = false]
     * @return {Promise<Stats>}
     */
    export function lstat(path: string | Buffer | URL, options?: object | undefined): Promise<Stats>;
    /**
     * Creates a symlink of `src` at `dest`.
     * @param {string} src
     * @param {string} dest
     * @return {Promise}
     */
    export function symlink(src: string, dest: string, type?: any): Promise<any>;
    /**
     * Unlinks (removes) file at `path`.
     * @param {string} path
     * @return {Promise}
     */
    export function unlink(path: string): Promise<any>;
    /**
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromiseswritefilefile-data-options}
     * @param {string|Buffer|URL|FileHandle} path - filename or FileHandle
     * @param {string|Buffer|Array|DataView|TypedArray} data
     * @param {object=} [options]
     * @param {(string|null)=} [options.encoding = 'utf8']
     * @param {number=} [options.mode = 0o666]
     * @param {string=} [options.flag = 'w']
     * @param {AbortSignal|undefined} [options.signal]
     * @return {Promise<void>}
     */
    export function writeFile(path: string | Buffer | URL | FileHandle, data: string | Buffer | any[] | DataView | TypedArray, options?: object | undefined): Promise<void>;
    /**
     * Watch for changes at `path` calling `callback`
     * @param {string}
     * @param {function|object=} [options]
     * @param {string=} [options.encoding = 'utf8']
     * @param {AbortSignal=} [options.signal]
     * @return {Watcher}
     */
    export function watch(path: any, options?: (Function | object) | undefined): Watcher;
    export type Stats = any;
    export default exports;
    export type Buffer = import("socket:buffer").Buffer;
    export type TypedArray = Uint8Array | Int8Array;
    import { FileHandle } from "socket:fs/handle";
    import { Dir } from "socket:fs/dir";
    import { Dirent } from "socket:fs/dir";
    import { Stats } from "socket:fs/stats";
    import { Watcher } from "socket:fs/watcher";
    import bookmarks from "socket:fs/bookmarks";
    import * as constants from "socket:fs/constants";
    import { DirectoryHandle } from "socket:fs/handle";
    import fds from "socket:fs/fds";
    import { ReadStream } from "socket:fs/stream";
    import { WriteStream } from "socket:fs/stream";
    import * as exports from "socket:fs/promises";
    
    export { bookmarks, constants, Dir, DirectoryHandle, Dirent, fds, FileHandle, ReadStream, Watcher, WriteStream };
}

declare module "socket:fs/index" {
    /**
     * Asynchronously check access a file for a given mode calling `callback`
     * upon success or error.
     * @see {@link https://nodejs.org/api/fs.html#fsopenpath-flags-mode-callback}
     * @param {string | Buffer | URL} path
     * @param {number?|function(Error|null):any?} [mode = F_OK(0)]
     * @param {function(Error|null):any?} [callback]
     */
    export function access(path: string | Buffer | URL, mode: any, callback?: (arg0: Error | null) => any | null): void;
    /**
     * Synchronously check access a file for a given mode calling `callback`
     * upon success or error.
     * @see {@link https://nodejs.org/api/fs.html#fsopenpath-flags-mode-callback}
     * @param {string | Buffer | URL} path
     * @param {string?} [mode = F_OK(0)]
     */
    export function accessSync(path: string | Buffer | URL, mode?: string | null): boolean;
    /**
     * Checks if a path exists
     * @param {string | Buffer | URL} path
     * @param {function(Boolean)?} [callback]
     */
    export function exists(path: string | Buffer | URL, callback?: ((arg0: boolean) => any) | null): void;
    /**
     * Checks if a path exists
     * @param {string | Buffer | URL} path
     * @param {function(Boolean)?} [callback]
     */
    export function existsSync(path: string | Buffer | URL): boolean;
    /**
     * Asynchronously changes the permissions of a file.
     * No arguments other than a possible exception are given to the completion callback
     *
     * @see {@link https://nodejs.org/api/fs.html#fschmodpath-mode-callback}
     *
     * @param {string | Buffer | URL} path
     * @param {number} mode
     * @param {function(Error?)} callback
     */
    export function chmod(path: string | Buffer | URL, mode: number, callback: (arg0: Error | null) => any): TypeError;
    /**
     * Synchronously changes the permissions of a file.
     *
     * @see {@link https://nodejs.org/api/fs.html#fschmodpath-mode-callback}
     * @param {string | Buffer | URL} path
     * @param {number} mode
     */
    export function chmodSync(path: string | Buffer | URL, mode: number): void;
    /**
     * Changes ownership of file or directory at `path` with `uid` and `gid`.
     * @param {string} path
     * @param {number} uid
     * @param {number} gid
     * @param {function} callback
     */
    export function chown(path: string, uid: number, gid: number, callback: Function): TypeError;
    /**
     * Changes ownership of file or directory at `path` with `uid` and `gid`.
     * @param {string} path
     * @param {number} uid
     * @param {number} gid
     */
    export function chownSync(path: string, uid: number, gid: number): void;
    /**
     * Asynchronously close a file descriptor calling `callback` upon success or error.
     * @see {@link https://nodejs.org/api/fs.html#fsclosefd-callback}
     * @param {number} fd
     * @param {function(Error?)?} [callback]
     */
    export function close(fd: number, callback?: ((arg0: Error | null) => any) | null): void;
    /**
     * Synchronously close a file descriptor.
     * @param {number} fd  - fd
     */
    export function closeSync(fd: number): void;
    /**
     * Asynchronously copies `src` to `dest` calling `callback` upon success or error.
     * @param {string} src - The source file path.
     * @param {string} dest - The destination file path.
     * @param {number} flags - Modifiers for copy operation.
     * @param {function(Error=)=} [callback] - The function to call after completion.
     * @see {@link https://nodejs.org/api/fs.html#fscopyfilesrc-dest-mode-callback}
     */
    export function copyFile(src: string, dest: string, flags?: number, callback?: ((arg0: Error | undefined) => any) | undefined): void;
    /**
     * Synchronously copies `src` to `dest` calling `callback` upon success or error.
     * @param {string} src - The source file path.
     * @param {string} dest - The destination file path.
     * @param {number} flags - Modifiers for copy operation.
     * @see {@link https://nodejs.org/api/fs.html#fscopyfilesrc-dest-mode-callback}
     */
    export function copyFileSync(src: string, dest: string, flags?: number): void;
    /**
     * @see {@link https://nodejs.org/api/fs.html#fscreatewritestreampath-options}
     * @param {string | Buffer | URL} path
     * @param {object?} [options]
     * @returns {ReadStream}
     */
    export function createReadStream(path: string | Buffer | URL, options?: object | null): ReadStream;
    /**
     * @see {@link https://nodejs.org/api/fs.html#fscreatewritestreampath-options}
     * @param {string | Buffer | URL} path
     * @param {object?} [options]
     * @returns {WriteStream}
     */
    export function createWriteStream(path: string | Buffer | URL, options?: object | null): WriteStream;
    /**
     * Invokes the callback with the <fs.Stats> for the file descriptor. See
     * the POSIX fstat(2) documentation for more detail.
     *
     * @see {@link https://nodejs.org/api/fs.html#fsfstatfd-options-callback}
     *
     * @param {number} fd - A file descriptor.
     * @param {object?|function?} [options] - An options object.
     * @param {function?} callback - The function to call after completion.
     */
    export function fstat(fd: number, options: any, callback: Function | null): void;
    /**
     * Request that all data for the open file descriptor is flushed
     * to the storage device.
     * @param {number} fd - A file descriptor.
     * @param {function} callback - The function to call after completion.
     */
    export function fsync(fd: number, callback: Function): void;
    /**
     * Truncates the file up to `offset` bytes.
     * @param {number} fd - A file descriptor.
     * @param {number=|function} [offset = 0]
     * @param {function?} callback - The function to call after completion.
     */
    export function ftruncate(fd: number, offset: any, callback: Function | null): void;
    /**
     * Chages ownership of link at `path` with `uid` and `gid.
     * @param {string} path
     * @param {number} uid
     * @param {number} gid
     * @param {function} callback
     */
    export function lchown(path: string, uid: number, gid: number, callback: Function): TypeError;
    /**
     * Creates a link to `dest` from `src`.
     * @param {string} src
     * @param {string} dest
     * @param {function}
     */
    export function link(src: string, dest: string, callback: any): void;
    /**
     * @ignore
     */
    export function mkdir(path: any, options: any, callback: any): void;
    /**
     * @ignore
     * @param {string|URL} path
     * @param {object=} [options]
     */
    export function mkdirSync(path: string | URL, options?: object | undefined): void;
    /**
     * Asynchronously open a file calling `callback` upon success or error.
     * @see {@link https://nodejs.org/api/fs.html#fsopenpath-flags-mode-callback}
     * @param {string | Buffer | URL} path
     * @param {string=} [flags = 'r']
     * @param {number=} [mode = 0o666]
     * @param {(object|function(Error|null, number|undefined):any)=} [options]
     * @param {(function(Error|null, number|undefined):any)|null} [callback]
     */
    export function open(path: string | Buffer | URL, flags?: string | undefined, mode?: number | undefined, options?: (object | ((arg0: Error | null, arg1: number | undefined) => any)) | undefined, callback?: ((arg0: Error | null, arg1: number | undefined) => any) | null): void;
    /**
     * Synchronously open a file.
     * @param {string|Buffer|URL} path
     * @param {string=} [flags = 'r']
     * @param {string=} [mode = 0o666]
     * @param {object=} [options]
     */
    export function openSync(path: string | Buffer | URL, flags?: string | undefined, mode?: string | undefined, options?: object | undefined): any;
    /**
     * Asynchronously open a directory calling `callback` upon success or error.
     * @see {@link https://nodejs.org/api/fs.html#fsreaddirpath-options-callback}
     * @param {string | Buffer | URL} path
     * @param {(object|function(Error|Null, Dir|undefined):any)=} [options]
     * @param {string=} [options.encoding = 'utf8']
     * @param {boolean=} [options.withFileTypes = false]
     * @param {function(Error|null, Dir|undefined):any)} callback
     */
    export function opendir(path: string | Buffer | URL, options?: (object | ((arg0: Error | null, arg1: Dir | undefined) => any)) | undefined, callback: any): void;
    /**
     * Synchronously open a directory.
     * @see {@link https://nodejs.org/api/fs.html#fsreaddirpath-options-callback}
     * @param {string|Buffer|URL} path
     * @param {objec} [options]
     * @param {string=} [options.encoding = 'utf8']
     * @param {boolean=} [options.withFileTypes = false]
     * @return {Dir}
     */
    export function opendirSync(path: string | Buffer | URL, options?: objec): Dir;
    /**
     * Asynchronously read from an open file descriptor.
     * @see {@link https://nodejs.org/api/fs.html#fsreadfd-buffer-offset-length-position-callback}
     * @param {number} fd
     * @param {object|Buffer|Uint8Array} buffer - The buffer that the data will be written to.
     * @param {number} offset - The position in buffer to write the data to.
     * @param {number} length - The number of bytes to read.
     * @param {number|BigInt|null} position - Specifies where to begin reading from in the file. If position is null or -1 , data will be read from the current file position, and the file position will be updated. If position is an integer, the file position will be unchanged.
     * @param {function(Error|null, number|undefined, Buffer|undefined):any} callback
     */
    export function read(fd: number, buffer: object | Buffer | Uint8Array, offset: number, length: number, position: number | BigInt | null, options: any, callback: (arg0: Error | null, arg1: number | undefined, arg2: Buffer | undefined) => any): void;
    /**
     * Asynchronously write to an open file descriptor.
     * @see {@link https://nodejs.org/api/fs.html#fswritefd-buffer-offset-length-position-callback}
     * @param {number} fd
     * @param {object|Buffer|Uint8Array} buffer - The buffer that the data will be written to.
     * @param {number} offset - The position in buffer to write the data to.
     * @param {number} length - The number of bytes to read.
     * @param {number|BigInt|null} position - Specifies where to begin reading from in the file. If position is null or -1 , data will be read from the current file position, and the file position will be updated. If position is an integer, the file position will be unchanged.
     * @param {function(Error|null, number|undefined, Buffer|undefined):any} callback
     */
    export function write(fd: number, buffer: object | Buffer | Uint8Array, offset: number, length: number, position: number | BigInt | null, options: any, callback: (arg0: Error | null, arg1: number | undefined, arg2: Buffer | undefined) => any): void;
    /**
     * Asynchronously read all entries in a directory.
     * @see {@link https://nodejs.org/api/fs.html#fsreaddirpath-options-callback}
     * @param {string|Buffer|URL} path
     * @param {object|function(Error|null, (Dirent|string)[]|undefined):any} [options]
     * @param {string=} [options.encoding = 'utf8']
     * @param {boolean=} [options.withFileTypes = false]
     * @param {function(Error|null, (Dirent|string)[]):any} callback
     */
    export function readdir(path: string | Buffer | URL, options?: object | ((arg0: Error | null, arg1: (Dirent | string)[] | undefined) => any), callback: (arg0: Error | null, arg1: (Dirent | string)[]) => any): void;
    /**
     * Synchronously read all entries in a directory.
     * @see {@link https://nodejs.org/api/fs.html#fsreaddirpath-options-callback}
     * @param {string|Buffer | URL } path
     * @param {object=} [options]
     * @param {string=} [options.encoding ? 'utf8']
     * @param {boolean=} [options.withFileTypes ? false]
     * @return {(Dirent|string)[]}
     */
    export function readdirSync(path: string | Buffer | URL, options?: object | undefined): (Dirent | string)[];
    /**
     * @param {string|Buffer|URL|number} path
     * @param {object|function(Error|null, Buffer|string|undefined):any} options
     * @param {string=} [options.encoding = 'utf8']
     * @param {string=} [options.flag = 'r']
     * @param {AbortSignal|undefined} [options.signal]
     * @param {function(Error|null, Buffer|string|undefined):any} callback
     */
    export function readFile(path: string | Buffer | URL | number, options: object | ((arg0: Error | null, arg1: Buffer | string | undefined) => any), callback: (arg0: Error | null, arg1: Buffer | string | undefined) => any): void;
    /**
     * @param {string|Buffer|URL|number} path
     * @param {{ encoding?: string = 'utf8', flags?: string = 'r'}} [options]
     * @param {object|function(Error|null, Buffer|undefined):any} [options]
     * @param {AbortSignal|undefined} [options.signal]
     * @return {string|Buffer}
     */
    export function readFileSync(path: string | Buffer | URL | number, options?: {
        encoding?: string;
        flags?: string;
    }): string | Buffer;
    /**
     * Reads link at `path`
     * @param {string} path
     * @param {function(Error|null, string|undefined):any} callback
     */
    export function readlink(path: string, callback: (arg0: Error | null, arg1: string | undefined) => any): void;
    /**
     * Computes real path for `path`
     * @param {string} path
     * @param {function(Error|null, string|undefined):any} callback
     */
    export function realpath(path: string, callback: (arg0: Error | null, arg1: string | undefined) => any): void;
    /**
     * Computes real path for `path`
     * @param {string} path
     * @return {string}
     */
    export function realpathSync(path: string): string;
    /**
     * Renames file or directory at `src` to `dest`.
     * @param {string} src
     * @param {string} dest
     * @param {function(Error|null):any} callback
     */
    export function rename(src: string, dest: string, callback: (arg0: Error | null) => any): void;
    /**
     * Renames file or directory at `src` to `dest`, synchronously.
     * @param {string} src
     * @param {string} dest
     */
    export function renameSync(src: string, dest: string): void;
    /**
     * Removes directory at `path`.
     * @param {string} path
     * @param {function(Error|null):any} callback
     */
    export function rmdir(path: string, callback: (arg0: Error | null) => any): void;
    /**
     * Removes directory at `path`, synchronously.
     * @param {string} path
     */
    export function rmdirSync(path: string): void;
    /**
     * Synchronously get the stats of a file
     * @param {string} path - filename or file descriptor
     * @param {object=} [options]
     * @param {string=} [options.encoding ? 'utf8']
     * @param {string=} [options.flag ? 'r']
     */
    export function statSync(path: string, options?: object | undefined): promises.Stats;
    /**
     * Get the stats of a file
     * @param {string|Buffer|URL|number} path - filename or file descriptor
     * @param {(object|function(Error|null, Stats|undefined):any)=} [options]
     * @param {string=} [options.encoding ? 'utf8']
     * @param {string=} [options.flag ? 'r']
     * @param {AbortSignal|undefined} [options.signal]
     * @param {function(Error|null, Stats|undefined):any} callback
     */
    export function stat(path: string | Buffer | URL | number, options?: (object | ((arg0: Error | null, arg1: Stats | undefined) => any)) | undefined, callback: (arg0: Error | null, arg1: Stats | undefined) => any): void;
    /**
     * Get the stats of a symbolic link
     * @param {string|Buffer|URL|number} path - filename or file descriptor
     * @param {(object|function(Error|null, Stats|undefined):any)=} [options]
     * @param {string=} [options.encoding ? 'utf8']
     * @param {string=} [options.flag ? 'r']
     * @param {AbortSignal|undefined} [options.signal]
     * @param {function(Error|null, Stats|undefined):any} callback
     */
    export function lstat(path: string | Buffer | URL | number, options?: (object | ((arg0: Error | null, arg1: Stats | undefined) => any)) | undefined, callback: (arg0: Error | null, arg1: Stats | undefined) => any): void;
    /**
     * Creates a symlink of `src` at `dest`.
     * @param {string} src
     * @param {string} dest
     * @param {function(Error|null):any} callback
     */
    export function symlink(src: string, dest: string, type: any, callback: (arg0: Error | null) => any): void;
    /**
     * Unlinks (removes) file at `path`.
     * @param {string} path
     * @param {function(Error|null):any} callback
     */
    export function unlink(path: string, callback: (arg0: Error | null) => any): void;
    /**
     * Unlinks (removes) file at `path`, synchronously.
     * @param {string} path
     */
    export function unlinkSync(path: string): void;
    /**
     * @see {@link https://nodejs.org/api/fs.html#fswritefilefile-data-options-callback}
     * @param {string|Buffer|URL|number} path - filename or file descriptor
     * @param {string|Buffer|TypedArray|DataView|object} data
     * @param {(object|function(Error|null):any)=} [options]
     * @param {string=} [options.encoding ? 'utf8']
     * @param {string=} [options.mode ? 0o666]
     * @param {string=} [options.flag ? 'w']
     * @param {AbortSignal|undefined} [options.signal]
     * @param {function(Error|null):any} callback
     */
    export function writeFile(path: string | Buffer | URL | number, data: string | Buffer | TypedArray | DataView | object, options?: (object | ((arg0: Error | null) => any)) | undefined, callback: (arg0: Error | null) => any): void;
    /**
     * Writes data to a file synchronously.
     * @param {string|Buffer|URL|number} path - filename or file descriptor
     * @param {string|Buffer|TypedArray|DataView|object} data
     * @param {object=} [options]
     * @param {string=} [options.encoding ? 'utf8']
     * @param {string=} [options.mode ? 0o666]
     * @param {string=} [options.flag ? 'w']
     * @param {AbortSignal|undefined} [options.signal]
     * @see {@link https://nodejs.org/api/fs.html#fswritefilesyncfile-data-options}
     */
    export function writeFileSync(path: string | Buffer | URL | number, data: string | Buffer | TypedArray | DataView | object, options?: object | undefined): void;
    /**
     * Watch for changes at `path` calling `callback`
     * @param {string}
     * @param {function|object=} [options]
     * @param {string=} [options.encoding = 'utf8']
     * @param {function=} [callback]
     * @return {Watcher}
     */
    export function watch(path: any, options?: (Function | object) | undefined, callback?: Function | undefined): Watcher;
    export default exports;
    export type Buffer = import("socket:buffer").Buffer;
    export type TypedArray = Uint8Array | Int8Array;
    import { Buffer } from "socket:buffer";
    import { ReadStream } from "socket:fs/stream";
    import { WriteStream } from "socket:fs/stream";
    import { Dir } from "socket:fs/dir";
    import { Dirent } from "socket:fs/dir";
    import * as promises from "socket:fs/promises";
    import { Stats } from "socket:fs/stats";
    import { Watcher } from "socket:fs/watcher";
    import bookmarks from "socket:fs/bookmarks";
    import * as constants from "socket:fs/constants";
    import { DirectoryHandle } from "socket:fs/handle";
    import fds from "socket:fs/fds";
    import { FileHandle } from "socket:fs/handle";
    import * as exports from "socket:fs/index";
    
    export { bookmarks, constants, Dir, DirectoryHandle, Dirent, fds, FileHandle, promises, ReadStream, Stats, Watcher, WriteStream };
}

declare module "socket:fs" {
    export * from "socket:fs/index";
    export default exports;
    import * as exports from "socket:fs/index";
}

declare module "socket:external/libsodium/index" {
    const _default: any;
    export default _default;
}

declare module "socket:crypto/sodium" {
    export {};
}

declare module "socket:crypto" {
    /**
     * Generate cryptographically strong random values into the `buffer`
     * @param {TypedArray} buffer
     * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Crypto/getRandomValues}
     * @return {TypedArray}
     */
    export function getRandomValues(buffer: TypedArray, ...args: any[]): TypedArray;
    /**
     * Generate a random 64-bit number.
     * @returns {BigInt} - A random 64-bit number.
     */
    export function rand64(): BigInt;
    /**
     * Generate `size` random bytes.
     * @param {number} size - The number of bytes to generate. The size must not be larger than 2**31 - 1.
     * @returns {Buffer} - A promise that resolves with an instance of socket.Buffer with random bytes.
     */
    export function randomBytes(size: number): Buffer;
    /**
     * @param {string} algorithm - `SHA-1` | `SHA-256` | `SHA-384` | `SHA-512`
     * @param {Buffer | TypedArray | DataView} message - An instance of socket.Buffer, TypedArray or Dataview.
     * @returns {Promise<Buffer>} - A promise that resolves with an instance of socket.Buffer with the hash.
     */
    export function createDigest(algorithm: string, buf: any): Promise<Buffer>;
    /**
     * A murmur3 hash implementation based on https://github.com/jwerle/murmurhash.c
     * that works on strings and `ArrayBuffer` views (typed arrays)
     * @param {string|Uint8Array|ArrayBuffer} value
     * @param {number=} [seed = 0]
     * @return {number}
     */
    export function murmur3(value: string | Uint8Array | ArrayBuffer, seed?: number | undefined): number;
    /**
     * @typedef {Uint8Array|Int8Array} TypedArray
     */
    /**
     * WebCrypto API
     * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Crypto}
     */
    export let webcrypto: any;
    /**
     * A promise that resolves when all internals to be loaded/ready.
     * @type {Promise}
     */
    export const ready: Promise<any>;
    /**
     * Maximum total size of random bytes per page
     */
    export const RANDOM_BYTES_QUOTA: number;
    /**
     * Maximum total size for random bytes.
     */
    export const MAX_RANDOM_BYTES: 281474976710655;
    /**
     * Maximum total amount of allocated per page of bytes (max/quota)
     */
    export const MAX_RANDOM_BYTES_PAGES: number;
    export default exports;
    export type TypedArray = Uint8Array | Int8Array;
    import { Buffer } from "socket:buffer";
    export namespace sodium {
        let ready: Promise<any>;
    }
    import * as exports from "socket:crypto";
    
}

declare module "socket:ai" {
    /**
     * A class to interact with large language models (using llama.cpp)
     */
    export class LLM extends EventEmitter {
        /**
         * Constructs an LLM instance. Each parameter is designed to configure and control
         * the behavior of the underlying large language model provided by llama.cpp.
         * @param {Object} options - Configuration options for the LLM instance.
         * @param {string} options.path - The file path to the model in .gguf format. This model file contains
         *                                the weights and configuration necessary for initializing the language model.
         * @param {string} options.prompt - The initial input text to the model, setting the context or query
         *                                  for generating responses. The model uses this as a starting point for text generation.
         * @param {string} [options.id] - An optional unique identifier for this specific instance of the model,
         *                                useful for tracking or referencing the model in multi-model setups.
         * @param {number} [options.n_ctx=1024] - Specifies the maximum number of tokens that the model can consider
         *                                        for a single query. This is crucial for managing memory and computational
         *                                        efficiency. Exceeding the model's configuration may lead to errors or truncated outputs.
         * @param {number} [options.n_threads=8] - The number of threads allocated for the model's computation,
         *                                         affecting performance and speed of response generation.
         * @param {number} [options.temp=1.1] - Sampling temperature controls the randomness of predictions.
         *                                      Higher values increase diversity, potentially at the cost of coherence.
         * @param {number} [options.max_tokens=512] - The upper limit on the number of tokens that the model can generate
         *                                            in response to a single prompt. This prevents runaway generations.
         * @param {number} [options.n_gpu_layers=32] - The number of GPU layers dedicated to the model processing.
         *                                             More layers can increase accuracy and complexity of the outputs.
         * @param {number} [options.n_keep=0] - Determines how many of the top generated responses are retained after
         *                                      the initial generation phase. Useful for models that generate multiple outputs.
         * @param {number} [options.n_batch=0] - The size of processing batches. Larger batch sizes can reduce
         *                                       the time per token generation by parallelizing computations.
         * @param {number} [options.n_predict=0] - Specifies how many forward predictions the model should make
         *                                         from the current state. This can pre-generate responses or calculate probabilities.
         * @param {number} [options.grp_attn_n=0] - Group attention parameter 'N' modifies how attention mechanisms
         *                                          within the model are grouped and interact, affecting the model’s focus and accuracy.
         * @param {number} [options.grp_attn_w=0] - Group attention parameter 'W' adjusts the width of each attention group,
         *                                          influencing the breadth of context considered by each attention group.
         * @param {number} [options.seed=0] - A seed for the random number generator used in the model. Setting this ensures
         *                                    consistent results in model outputs, important for reproducibility in experiments.
         * @param {number} [options.top_k=0] - Limits the model's output choices to the top 'k' most probable next words,
         *                                     reducing the risk of less likely, potentially nonsensical outputs.
         * @param {number} [options.tok_p=0.0] - Top-p (nucleus) sampling threshold, filtering the token selection pool
         *                                      to only those whose cumulative probability exceeds this value, enhancing output relevance.
         * @param {number} [options.min_p=0.0] - Sets a minimum probability filter for token generation, ensuring
         *                                      that generated tokens have at least this likelihood of being relevant or coherent.
         * @param {number} [options.tfs_z=0.0] - Temperature factor scale for zero-shot learning scenarios, adjusting how
         *                                      the model weights novel or unseen prompts during generation.
         * @throws {Error} Throws an error if the model path is not provided, as the model cannot initialize without it.
         */
        constructor(options?: {
            path: string;
            prompt: string;
            id?: string;
            n_ctx?: number;
            n_threads?: number;
            temp?: number;
            max_tokens?: number;
            n_gpu_layers?: number;
            n_keep?: number;
            n_batch?: number;
            n_predict?: number;
            grp_attn_n?: number;
            grp_attn_w?: number;
            seed?: number;
            top_k?: number;
            tok_p?: number;
            min_p?: number;
            tfs_z?: number;
        });
        path: string;
        prompt: string;
        id: string | BigInt;
        /**
         * Tell the LLM to stop after the next token.
         * @returns {Promise<void>} A promise that resolves when the LLM stops.
         */
        stop(): Promise<void>;
        /**
         * Send a message to the chat.
         * @param {string} message - The message to send to the chat.
         * @returns {Promise<any>} A promise that resolves with the response from the chat.
         */
        chat(message: string): Promise<any>;
    }
    export default exports;
    import { EventEmitter } from "socket:events";
    import * as exports from "socket:ai";
    
}

declare module "socket:window/constants" {
    export const WINDOW_ERROR: -1;
    export const WINDOW_NONE: 0;
    export const WINDOW_CREATING: 10;
    export const WINDOW_CREATED: 11;
    export const WINDOW_HIDING: 20;
    export const WINDOW_HIDDEN: 21;
    export const WINDOW_SHOWING: 30;
    export const WINDOW_SHOWN: 31;
    export const WINDOW_CLOSING: 40;
    export const WINDOW_CLOSED: 41;
    export const WINDOW_EXITING: 50;
    export const WINDOW_EXITED: 51;
    export const WINDOW_KILLING: 60;
    export const WINDOW_KILLED: 61;
    export default exports;
    import * as exports from "socket:window/constants";
    
}

declare module "socket:application/client" {
    /**
     * @typedef {{
     *  id?: string | null,
     *  type?: 'window' | 'worker',
     *  parent?: object | null,
     *  top?: object | null,
     *  frameType?: 'top-level' | 'nested' | 'none'
     * }} ClientState
     */
    export class Client {
        /**
         * `Client` class constructor
         * @private
         * @param {ClientState} state
         */
        private constructor();
        /**
         * The unique ID of the client.
         * @type {string|null}
         */
        get id(): string;
        /**
         * The frame type of the client.
         * @type {'top-level'|'nested'|'none'}
         */
        get frameType(): "none" | "top-level" | "nested";
        /**
         * The type of the client.
         * @type {'window'|'worker'}
         */
        get type(): "window" | "worker";
        /**
         * The parent client of the client.
         * @type {Client|null}
         */
        get parent(): Client;
        /**
         * The top client of the client.
         * @type {Client|null}
         */
        get top(): Client;
        /**
         * A readonly `URL` of the current location of this client.
         * @type {URL}
         */
        get location(): URL;
        /**
         * Converts this `Client` instance to JSON.
         * @return {object}
         */
        toJSON(): object;
        #private;
    }
    const _default: any;
    export default _default;
    export type ClientState = {
        id?: string | null;
        type?: "window" | "worker";
        parent?: object | null;
        top?: object | null;
        frameType?: "top-level" | "nested" | "none";
    };
}

declare module "socket:window/hotkey" {
    /**
     * Normalizes an expression string.
     * @param {string} expression
     * @return {string}
     */
    export function normalizeExpression(expression: string): string;
    /**
     * Bind a global hotkey expression.
     * @param {string} expression
     * @param {{ passive?: boolean }} [options]
     * @return {Promise<Binding>}
     */
    export function bind(expression: string, options?: {
        passive?: boolean;
    }): Promise<Binding>;
    /**
     * Bind a global hotkey expression.
     * @param {string} expression
     * @param {object=} [options]
     * @return {Promise<Binding>}
     */
    export function unbind(id: any, options?: object | undefined): Promise<Binding>;
    /**
     * Get all known globally register hotkey bindings.
     * @param {object=} [options]
     * @return {Promise<Binding[]>}
     */
    export function getBindings(options?: object | undefined): Promise<Binding[]>;
    /**
     * Get all known possible keyboard modifier and key mappings for
     * expression bindings.
     * @param {object=} [options]
     * @return {Promise<{ keys: object, modifiers: object }>}
     */
    export function getMappings(options?: object | undefined): Promise<{
        keys: object;
        modifiers: object;
    }>;
    /**
     * Adds an event listener to the global active bindings. This function is just
     * proxy to `bindings.addEventListener`.
     * @param {string} type
     * @param {function(Event)} listener
     * @param {(boolean|object)=} [optionsOrUseCapture]
     */
    export function addEventListener(type: string, listener: (arg0: Event) => any, optionsOrUseCapture?: (boolean | object) | undefined): void;
    /**
     * Removes  an event listener to the global active bindings. This function is
     * just a proxy to `bindings.removeEventListener`
     * @param {string} type
     * @param {function(Event)} listener
     * @param {(boolean|object)=} [optionsOrUseCapture]
     */
    export function removeEventListener(type: string, listener: (arg0: Event) => any, optionsOrUseCapture?: (boolean | object) | undefined): void;
    /**
     * A high level bindings container map that dispatches events.
     */
    export class Bindings extends EventTarget {
        /**
         * `Bindings` class constructor.
         * @ignore
         * @param {EventTarget} [sourceEventTarget]
         */
        constructor(sourceEventTarget?: EventTarget);
        /**
         * Global `HotKeyEvent` event listener for `Binding` instance event dispatch.
         * @ignore
         * @param {import('../internal/events.js').HotKeyEvent} event
         */
        onHotKey(event: import("socket:internal/events").HotKeyEvent): boolean;
        /**
         * The number of `Binding` instances in the mapping.
         * @type {number}
         */
        get size(): number;
        /**
         * Setter for the level 1 'error'` event listener.
         * @ignore
         * @type {function(ErrorEvent)?}
         */
        set onerror(onerror: (arg0: ErrorEvent) => any);
        /**
         * Level 1 'error'` event listener.
         * @type {function(ErrorEvent)?}
         */
        get onerror(): (arg0: ErrorEvent) => any;
        /**
         * Setter for the level 1 'hotkey'` event listener.
         * @ignore
         * @type {function(HotKeyEvent)?}
         */
        set onhotkey(onhotkey: (arg0: hotkeyEvent) => any);
        /**
         * Level 1 'hotkey'` event listener.
         * @type {function(hotkeyEvent)?}
         */
        get onhotkey(): (arg0: hotkeyEvent) => any;
        /**
         * Initializes bindings from global context.
         * @ignore
         * @return {Promise}
         */
        init(): Promise<any>;
        /**
         * Get a binding by `id`
         * @param {number} id
         * @return {Binding}
         */
        get(id: number): Binding;
        /**
         * Set a `binding` a by `id`.
         * @param {number} id
         * @param {Binding} binding
         */
        set(id: number, binding: Binding): void;
        /**
         * Delete a binding by `id`
         * @param {number} id
         * @return {boolean}
         */
        delete(id: number): boolean;
        /**
         * Returns `true` if a binding exists in the mapping, otherwise `false`.
         * @return {boolean}
         */
        has(id: any): boolean;
        /**
         * Known `Binding` values in the mapping.
         * @return {{ next: function(): { value: Binding|undefined, done: boolean } }}
         */
        values(): {
            next: () => {
                value: Binding | undefined;
                done: boolean;
            };
        };
        /**
         * Known `Binding` keys in the mapping.
         * @return {{ next: function(): { value: number|undefined, done: boolean } }}
         */
        keys(): {
            next: () => {
                value: number | undefined;
                done: boolean;
            };
        };
        /**
         * Known `Binding` ids in the mapping.
         * @return {{ next: function(): { value: number|undefined, done: boolean } }}
         */
        ids(): {
            next: () => {
                value: number | undefined;
                done: boolean;
            };
        };
        /**
         * Known `Binding` ids and values in the mapping.
         * @return {{ next: function(): { value: [number, Binding]|undefined, done: boolean } }}
         */
        entries(): {
            next: () => {
                value: [number, Binding] | undefined;
                done: boolean;
            };
        };
        /**
         * Bind a global hotkey expression.
         * @param {string} expression
         * @return {Promise<Binding>}
         */
        bind(expression: string): Promise<Binding>;
        /**
         * Bind a global hotkey expression.
         * @param {string} expression
         * @return {Promise<Binding>}
         */
        unbind(expression: string): Promise<Binding>;
        /**
         * Returns an array of all active bindings for the application.
         * @return {Promise<Binding[]>}
         */
        active(): Promise<Binding[]>;
        /**
         * Resets all active bindings in the application.
         * @param {boolean=} [currentContextOnly]
         * @return {Promise}
         */
        reset(currentContextOnly?: boolean | undefined): Promise<any>;
        /**
         * Implements the `Iterator` protocol for each currently registered
         * active binding in this window context. The `AsyncIterator` protocol
         * will probe for all gloally active bindings.
         * @return {Iterator<Binding>}
         */
        [Symbol.iterator](): Iterator<Binding>;
        /**
         * Implements the `AsyncIterator` protocol for each globally active
         * binding registered to the application. This differs from the `Iterator`
         * protocol as this will probe for _all_ active bindings in the entire
         * application context.
         * @return {AsyncGenerator<Binding>}
         */
        [Symbol.asyncIterator](): AsyncGenerator<Binding>;
        #private;
    }
    /**
     * An `EventTarget` container for a hotkey binding.
     */
    export class Binding extends EventTarget {
        /**
         * `Binding` class constructor.
         * @ignore
         * @param {object} data
         */
        constructor(data: object);
        /**
         * `true` if the binding is valid, otherwise `false`.
         * @type {boolean}
         */
        get isValid(): boolean;
        /**
         * `true` if the binding is considered active, otherwise `false`.
         * @type {boolean}
         */
        get isActive(): boolean;
        /**
         * The global unique ID for this binding.
         * @type {number?}
         */
        get id(): number;
        /**
         * The computed hash for this binding expression.
         * @type {number?}
         */
        get hash(): number;
        /**
         * The normalized expression as a sequence of tokens.
         * @type {string[]}
         */
        get sequence(): string[];
        /**
         * The original expression of the binding.
         * @type {string?}
         */
        get expression(): string;
        /**
         * Setter for the level 1 'hotkey'` event listener.
         * @ignore
         * @type {function(HotKeyEvent)?}
         */
        set onhotkey(onhotkey: (arg0: hotkeyEvent) => any);
        /**
         * Level 1 'hotkey'` event listener.
         * @type {function(hotkeyEvent)?}
         */
        get onhotkey(): (arg0: hotkeyEvent) => any;
        /**
         * Binds this hotkey expression.
         * @return {Promise<Binding>}
         */
        bind(): Promise<Binding>;
        /**
         * Unbinds this hotkey expression.
         * @return {Promise}
         */
        unbind(): Promise<any>;
        /**
         * Implements the `AsyncIterator` protocol for async 'hotkey' events
         * on this binding instance.
         * @return {AsyncGenerator}
         */
        [Symbol.asyncIterator](): AsyncGenerator;
        #private;
    }
    /**
     * A container for all the bindings currently bound
     * by this window context.
     * @type {Bindings}
     */
    export const bindings: Bindings;
    export default bindings;
    import { HotKeyEvent } from "socket:internal/events";
}

declare module "socket:window" {
    /**
     * @param {string} url
     * @return {string}
     * @ignore
     */
    export function formatURL(url: string): string;
    /**
     * @class ApplicationWindow
     * Represents a window in the application
     */
    export class ApplicationWindow {
        static constants: typeof statuses;
        static hotkey: import("socket:window/hotkey").Bindings;
        constructor({ index, ...options }: {
            [x: string]: any;
            index: any;
        });
        /**
         * The unique ID of this window.
         * @type {string}
         */
        get id(): string;
        /**
         * Get the index of the window
         * @return {number} - the index of the window
         */
        get index(): number;
        /**
         * @type {import('./window/hotkey.js').default}
         */
        get hotkey(): import("socket:window/hotkey").Bindings;
        /**
         * The broadcast channel for this window.
         * @type {BroadcastChannel}
         */
        get channel(): BroadcastChannel;
        /**
         * Get the size of the window
         * @return {{ width: number, height: number }} - the size of the window
         */
        getSize(): {
            width: number;
            height: number;
        };
        /**
         * Get the position of the window
         * @return {{ x: number, y: number }} - the position of the window
         */
        getPosition(): {
            x: number;
            y: number;
        };
        /**
         * Get the title of the window
         * @return {string} - the title of the window
         */
        getTitle(): string;
        /**
         * Get the status of the window
         * @return {string} - the status of the window
         */
        getStatus(): string;
        /**
         * Close the window
         * @return {Promise<object>} - the options of the window
         */
        close(): Promise<object>;
        /**
         * Shows the window
         * @return {Promise<ipc.Result>}
         */
        show(): Promise<ipc.Result>;
        /**
         * Hides the window
         * @return {Promise<ipc.Result>}
         */
        hide(): Promise<ipc.Result>;
        /**
         * Maximize the window
         * @return {Promise<ipc.Result>}
         */
        maximize(): Promise<ipc.Result>;
        /**
         * Minimize the window
         * @return {Promise<ipc.Result>}
         */
        minimize(): Promise<ipc.Result>;
        /**
         * Restore the window
         * @return {Promise<ipc.Result>}
         */
        restore(): Promise<ipc.Result>;
        /**
         * Sets the title of the window
         * @param {string} title - the title of the window
         * @return {Promise<ipc.Result>}
         */
        setTitle(title: string): Promise<ipc.Result>;
        /**
         * Sets the size of the window
         * @param {object} opts - an options object
         * @param {(number|string)=} opts.width - the width of the window
         * @param {(number|string)=} opts.height - the height of the window
         * @return {Promise<ipc.Result>}
         * @throws {Error} - if the width or height is invalid
         */
        setSize(opts: {
            width?: (number | string) | undefined;
            height?: (number | string) | undefined;
        }): Promise<ipc.Result>;
        /**
         * Sets the position of the window
         * @param {object} opts - an options object
         * @param {(number|string)=} opts.x - the x position of the window
         * @param {(number|string)=} opts.y - the y position of the window
         * @return {Promise<object>}
         * @throws {Error} - if the x or y is invalid
         */
        setPosition(opts: {
            x?: (number | string) | undefined;
            y?: (number | string) | undefined;
        }): Promise<object>;
        /**
         * Navigate the window to a given path
         * @param {object} path - file path
         * @return {Promise<ipc.Result>}
         */
        navigate(path: object): Promise<ipc.Result>;
        /**
         * Opens the Web Inspector for the window
         * @return {Promise<object>}
         */
        showInspector(): Promise<object>;
        /**
         * Sets the background color of the window
         * @param {object} opts - an options object
         * @param {number} opts.red - the red value
         * @param {number} opts.green - the green value
         * @param {number} opts.blue - the blue value
         * @param {number} opts.alpha - the alpha value
         * @return {Promise<object>}
         */
        setBackgroundColor(opts: {
            red: number;
            green: number;
            blue: number;
            alpha: number;
        }): Promise<object>;
        /**
         * Gets the background color of the window
         * @return {Promise<object>}
         */
        getBackgroundColor(): Promise<object>;
        /**
         * Opens a native context menu.
         * @param {object} options - an options object
         * @return {Promise<object>}
         */
        setContextMenu(options: object): Promise<object>;
        /**
         * Shows a native open file dialog.
         * @param {object} options - an options object
         * @return {Promise<string[]>} - an array of file paths
         */
        showOpenFilePicker(options: object): Promise<string[]>;
        /**
         * Shows a native save file dialog.
         * @param {object} options - an options object
         * @return {Promise<string[]>} - an array of file paths
         */
        showSaveFilePicker(options: object): Promise<string[]>;
        /**
         * Shows a native directory dialog.
         * @param {object} options - an options object
         * @return {Promise<string[]>} - an array of file paths
         */
        showDirectoryFilePicker(options: object): Promise<string[]>;
        /**
         * This is a high-level API that you should use instead of `ipc.request` when
         * you want to send a message to another window or to the backend.
         *
         * @param {object} options - an options object
         * @param {number=} options.window - the window to send the message to
         * @param {boolean=} [options.backend = false] - whether to send the message to the backend
         * @param {string} options.event - the event to send
         * @param {(string|object)=} options.value - the value to send
         * @returns
         */
        send(options: {
            window?: number | undefined;
            backend?: boolean | undefined;
            event: string;
            value?: (string | object) | undefined;
        }): Promise<any>;
        /**
         * Post a message to a window
         * TODO(@jwerle): research using `BroadcastChannel` instead
         * @param {object} message
         * @return {Promise}
         */
        postMessage(message: object): Promise<any>;
        /**
         * Opens an URL in the default application associated with the URL protocol,
         * such as 'https:' for the default web browser.
         * @param {string} value
         * @returns {Promise<{ url: string }>}
         */
        openExternal(value: string): Promise<{
            url: string;
        }>;
        /**
         * Opens a file in the default file explorer.
         * @param {string} value
         * @returns {Promise}
         */
        revealFile(value: string): Promise<any>;
        /**
         * Adds a listener to the window.
         * @param {string} event - the event to listen to
         * @param {function(*): void} cb - the callback to call
         * @returns {void}
         */
        addListener(event: string, cb: (arg0: any) => void): void;
        /**
         * Adds a listener to the window. An alias for `addListener`.
         * @param {string} event - the event to listen to
         * @param {function(*): void} cb - the callback to call
         * @returns {void}
         * @see addListener
         */
        on(event: string, cb: (arg0: any) => void): void;
        /**
         * Adds a listener to the window. The listener is removed after the first call.
         * @param {string} event - the event to listen to
         * @param {function(*): void} cb - the callback to call
         * @returns {void}
         */
        once(event: string, cb: (arg0: any) => void): void;
        /**
         * Removes a listener from the window.
         * @param {string} event - the event to remove the listener from
         * @param {function(*): void} cb - the callback to remove
         * @returns {void}
         */
        removeListener(event: string, cb: (arg0: any) => void): void;
        /**
         * Removes all listeners from the window.
         * @param {string} event - the event to remove the listeners from
         * @returns {void}
         */
        removeAllListeners(event: string): void;
        /**
         * Removes a listener from the window. An alias for `removeListener`.
         * @param {string} event - the event to remove the listener from
         * @param {function(*): void} cb - the callback to remove
         * @returns {void}
         * @see removeListener
         */
        off(event: string, cb: (arg0: any) => void): void;
        #private;
    }
    export default ApplicationWindow;
    /**
     * @ignore
     */
    export const constants: typeof statuses;
    import ipc from "socket:ipc";
    import * as statuses from "socket:window/constants";
    import client from "socket:application/client";
    import hotkey from "socket:window/hotkey";
    export { client, hotkey };
}

declare module "socket:application" {
    /**
     * Add an application event `type` callback `listener` with `options`.
     * @param {string} type
     * @param {function(Event|MessageEvent|CustomEvent|ApplicationURLEvent): boolean} listener
     * @param {{ once?: boolean }|boolean=} [options]
     */
    export function addEventListener(type: string, listener: (arg0: Event | MessageEvent | CustomEvent | ApplicationURLEvent) => boolean, options?: ({
        once?: boolean;
    } | boolean) | undefined): void;
    /**
     * Remove an application event `type` callback `listener` with `options`.
     * @param {string} type
     * @param {function(Event|MessageEvent|CustomEvent|ApplicationURLEvent): boolean} listener
     */
    export function removeEventListener(type: string, listener: (arg0: Event | MessageEvent | CustomEvent | ApplicationURLEvent) => boolean): void;
    /**
     * Returns the current window index
     * @return {number}
     */
    export function getCurrentWindowIndex(): number;
    /**
     * Creates a new window and returns an instance of ApplicationWindow.
     * @param {object} opts - an options object
     * @param {string=} opts.aspectRatio - a string (split on ':') provides two float values which set the window's aspect ratio.
     * @param {boolean=} opts.closable - deterime if the window can be closed.
     * @param {boolean=} opts.minimizable - deterime if the window can be minimized.
     * @param {boolean=} opts.maximizable - deterime if the window can be maximized.
     * @param {number} [opts.margin] - a margin around the webview. (Private)
     * @param {number} [opts.radius] - a radius on the webview. (Private)
     * @param {number} opts.index - the index of the window.
     * @param {string} opts.path - the path to the HTML file to load into the window.
     * @param {string=} opts.title - the title of the window.
     * @param {string=} opts.titlebarStyle - determines the style of the titlebar (MacOS only).
     * @param {string=} opts.windowControlOffsets - a string (split on 'x') provides the x and y position of the traffic lights (MacOS only).
     * @param {string=} opts.backgroundColorDark - determines the background color of the window in dark mode.
     * @param {string=} opts.backgroundColorLight - determines the background color of the window in light mode.
     * @param {(number|string)=} opts.width - the width of the window. If undefined, the window will have the main window width.
     * @param {(number|string)=} opts.height - the height of the window. If undefined, the window will have the main window height.
     * @param {(number|string)=} [opts.minWidth = 0] - the minimum width of the window
     * @param {(number|string)=} [opts.minHeight = 0] - the minimum height of the window
     * @param {(number|string)=} [opts.maxWidth = '100%'] - the maximum width of the window
     * @param {(number|string)=} [opts.maxHeight = '100%'] - the maximum height of the window
     * @param {boolean=} [opts.resizable=true] - whether the window is resizable
     * @param {boolean=} [opts.frameless=false] - whether the window is frameless
     * @param {boolean=} [opts.utility=false] - whether the window is utility (macOS only)
     * @param {boolean=} [opts.shouldExitApplicationOnClose=false] - whether the window can exit the app
     * @param {boolean=} [opts.headless=false] - whether the window will be headless or not (no frame)
     * @param {string=} [opts.userScript=null] - A user script that will be injected into the window (desktop only)
     * @param {string[]=} [opts.protocolHandlers] - An array of protocol handler schemes to register with the new window (requires service worker)
     * @return {Promise<ApplicationWindow>}
     */
    export function createWindow(opts: {
        aspectRatio?: string | undefined;
        closable?: boolean | undefined;
        minimizable?: boolean | undefined;
        maximizable?: boolean | undefined;
        margin?: number;
        radius?: number;
        index: number;
        path: string;
        title?: string | undefined;
        titlebarStyle?: string | undefined;
        windowControlOffsets?: string | undefined;
        backgroundColorDark?: string | undefined;
        backgroundColorLight?: string | undefined;
        width?: (number | string) | undefined;
        height?: (number | string) | undefined;
        minWidth?: (number | string) | undefined;
        minHeight?: (number | string) | undefined;
        maxWidth?: (number | string) | undefined;
        maxHeight?: (number | string) | undefined;
        resizable?: boolean | undefined;
        frameless?: boolean | undefined;
        utility?: boolean | undefined;
        shouldExitApplicationOnClose?: boolean | undefined;
        headless?: boolean | undefined;
        userScript?: string | undefined;
        protocolHandlers?: string[] | undefined;
    }): Promise<ApplicationWindow>;
    /**
     * Returns the current screen size.
     * @returns {Promise<{ width: number, height: number }>}
     */
    export function getScreenSize(): Promise<{
        width: number;
        height: number;
    }>;
    /**
     * Returns the ApplicationWindow instances for the given indices or all windows if no indices are provided.
     * @param {number[]} [indices] - the indices of the windows
     * @throws {Error} - if indices is not an array of integer numbers
     * @return {Promise<ApplicationWindowList>}
     */
    export function getWindows(indices?: number[], options?: any): Promise<ApplicationWindowList>;
    /**
     * Returns the ApplicationWindow instance for the given index
     * @param {number} index - the index of the window
     * @throws {Error} - if index is not a valid integer number
     * @returns {Promise<ApplicationWindow>} - the ApplicationWindow instance or null if the window does not exist
     */
    export function getWindow(index: number, options: any): Promise<ApplicationWindow>;
    /**
     * Returns the ApplicationWindow instance for the current window.
     * @return {Promise<ApplicationWindow>}
     */
    export function getCurrentWindow(): Promise<ApplicationWindow>;
    /**
     * Quits the backend process and then quits the render process, the exit code used is the final exit code to the OS.
     * @param {number} [code = 0] - an exit code
     * @return {Promise<ipc.Result>}
     */
    export function exit(code?: number): Promise<ipc.Result>;
    /**
     * Set the native menu for the app.
     *
     * @param {object} options - an options object
     * @param {string} options.value - the menu layout
     * @param {number} options.index - the window to target (if applicable)
     * @return {Promise<ipc.Result>}
     *
     * Socket Runtime provides a minimalist DSL that makes it easy to create
     * cross platform native system and context menus.
     *
     * Menus are created at run time. They can be created from either the Main or
     * Render process. The can be recreated instantly by calling the `setSystemMenu` method.
     *
     * The method takes a string. Here's an example of a menu. The semi colon is
     * significant indicates the end of the menu. Use an underscore when there is no
     * accelerator key. Modifiers are optional. And well known OS menu options like
     * the edit menu will automatically get accelerators you dont need to specify them.
     *
     *
     * ```js
     * socket.application.setSystemMenu({ index: 0, value: `
     *   App:
     *     Foo: f;
     *
     *   Edit:
     *     Cut: x
     *     Copy: c
     *     Paste: v
     *     Delete: _
     *     Select All: a;
     *
     *   Other:
     *     Apple: _
     *     Another Test: T
     *     !Im Disabled: I
     *     Some Thing: S + Meta
     *     ---
     *     Bazz: s + Meta, Control, Alt;
     * `)
     * ```
     *
     * Separators
     *
     * To create a separator, use three dashes `---`.
     *
     *
     * Accelerator Modifiers
     *
     * Accelerator modifiers are used as visual indicators but don't have a
     * material impact as the actual key binding is done in the event listener.
     *
     * A capital letter implies that the accelerator is modified by the `Shift` key.
     *
     * Additional accelerators are `Meta`, `Control`, `Option`, each separated
     * by commas. If one is not applicable for a platform, it will just be ignored.
     *
     * On MacOS `Meta` is the same as `Command`.
     *
     *
     * Disabled Items
     *
     * If you want to disable a menu item just prefix the item with the `!` character.
     * This will cause the item to appear disabled when the system menu renders.
     *
     *
     * Submenus
     *
     * We feel like nested menus are an anti-pattern. We don't use them. If you have a
     * strong argument for them and a very simple pull request that makes them work we
     * may consider them.
     *
     *
     * Event Handling
     *
     * When a menu item is activated, it raises the `menuItemSelected` event in
     * the front end code, you can then communicate with your backend code if you
     * want from there.
     *
     * For example, if the `Apple` item is selected from the `Other` menu...
     *
     * ```js
     * window.addEventListener('menuItemSelected', event => {
     *   assert(event.detail.parent === 'Other')
     *   assert(event.detail.title === 'Apple')
     * })
     * ```
     *
     */
    export function setSystemMenu(o: any): Promise<ipc.Result>;
    /**
     * An alias to setSystemMenu for creating a tary menu
     */
    export function setTrayMenu(o: any): Promise<ipc.Result>;
    /**
     * Set the enabled state of the system menu.
     * @param {object} value - an options object
     * @return {Promise<ipc.Result>}
     */
    export function setSystemMenuItemEnabled(value: object): Promise<ipc.Result>;
    /**
     * Predicate function to determine if application is in a "paused" state.
     * @return {boolean}
     */
    export function isPaused(): boolean;
    export const MAX_WINDOWS: 32;
    export class ApplicationWindowList {
        static from(...args: any[]): exports.ApplicationWindowList;
        constructor(items: any);
        get length(): number;
        get size(): number;
        forEach(callback: any, thisArg: any): void;
        item(index: any): any;
        entries(): any[][];
        keys(): any[];
        values(): any[];
        add(window: any): this;
        remove(windowOrIndex: any): boolean;
        contains(windowOrIndex: any): boolean;
        clear(): this;
        get [Symbol.iterator](): () => IterableIterator<any>;
        #private;
    }
    /**
     * Socket Runtime version.
     * @type {object} - an object containing the version information
     */
    export const runtimeVersion: object;
    /**
     * Runtime debug flag.
     * @type {boolean}
     */
    export const debug: boolean;
    /**
     * Application configuration.
     * @type {Record<string, string|number|boolean|(string|number|boolean)[]>}
     */
    export const config: Record<string, string | number | boolean | (string | number | boolean)[]>;
    export namespace backend {
        /**
         * @param {object} opts - an options object
         * @param {boolean} [opts.force = false] - whether to force the existing process to close
         * @return {Promise<ipc.Result>}
         */
        function open(opts?: {
            force?: boolean;
        }): Promise<ipc.Result>;
        /**
         * @return {Promise<ipc.Result>}
         */
        function close(): Promise<ipc.Result>;
    }
    export default exports;
    import { ApplicationURLEvent } from "socket:internal/events";
    import ApplicationWindow from "socket:window";
    import ipc from "socket:ipc";
    import client from "socket:application/client";
    import menu from "socket:application/menu";
    import * as exports from "socket:application";
    
    export { client, menu };
}

declare module "socket:test/fast-deep-equal" {
    export default function equal(a: any, b: any): boolean;
}

declare module "socket:assert" {
    export function assert(value: any, message?: any): void;
    export function ok(value: any, message?: any): void;
    export function equal(actual: any, expected: any, message?: any): void;
    export function notEqual(actual: any, expected: any, message?: any): void;
    export function strictEqual(actual: any, expected: any, message?: any): void;
    export function notStrictEqual(actual: any, expected: any, message?: any): void;
    export function deepEqual(actual: any, expected: any, message?: any): void;
    export function notDeepEqual(actual: any, expected: any, message?: any): void;
    export class AssertionError extends Error {
        constructor(options: any);
        actual: any;
        expected: any;
        operator: any;
    }
    const _default: typeof assert & {
        AssertionError: typeof AssertionError;
        ok: typeof ok;
        equal: typeof equal;
        notEqual: typeof notEqual;
        strictEqual: typeof strictEqual;
        notStrictEqual: typeof notStrictEqual;
        deepEqual: typeof deepEqual;
        notDeepEqual: typeof notDeepEqual;
    };
    export default _default;
}

declare module "socket:async_hooks" {
    export default exports;
    import { AsyncLocalStorage } from "socket:async/storage";
    import { AsyncResource } from "socket:async/resource";
    import { executionAsyncResource } from "socket:async/hooks";
    import { executionAsyncId } from "socket:async/hooks";
    import { triggerAsyncId } from "socket:async/hooks";
    import { createHook } from "socket:async/hooks";
    import * as exports from "socket:async_hooks";
    
    export { AsyncLocalStorage, AsyncResource, executionAsyncResource, executionAsyncId, triggerAsyncId, createHook };
}

declare module "socket:bluetooth" {
    export default exports;
    /**
     * Create an instance of a Bluetooth service.
     */
    export class Bluetooth extends EventEmitter {
        static isInitalized: boolean;
        /**
         * constructor is an example property that is set to `true`
         * Creates a new service with key-value pairs
         * @param {string} serviceId - Given a default value to determine the type
         */
        constructor(serviceId?: string);
        serviceId: string;
        /**
         * Start the Bluetooth service.
         * @return {Promise<ipc.Result>}
         *
         */
        start(): Promise<ipc.Result>;
        /**
         * Start scanning for published values that correspond to a well-known UUID.
         * Once subscribed to a UUID, events that correspond to that UUID will be
         * emitted. To receive these events you can add an event listener, for example...
         *
         * ```js
         * const ble = new Bluetooth(id)
         * ble.subscribe(uuid)
         * ble.on(uuid, (data, details) => {
         *   // ...do something interesting
         * })
         * ```
         *
         * @param {string} [id = ''] - A well-known UUID
         * @return {Promise<ipc.Result>}
         */
        subscribe(id?: string): Promise<ipc.Result>;
        /**
         * Start advertising a new value for a well-known UUID
         * @param {string} [id=''] - A well-known UUID
         * @param {string} [value='']
         * @return {Promise<void>}
         */
        publish(id?: string, value?: string): Promise<void>;
    }
    import * as exports from "socket:bluetooth";
    import { EventEmitter } from "socket:events";
    import ipc from "socket:ipc";
    
}

declare module "socket:bootstrap" {
    /**
     * @param {string} dest - file path
     * @param {string} hash - hash string
     * @param {string} hashAlgorithm - hash algorithm
     * @returns {Promise<boolean>}
     */
    export function checkHash(dest: string, hash: string, hashAlgorithm: string): Promise<boolean>;
    export function bootstrap(options: any): Bootstrap;
    namespace _default {
        export { bootstrap };
        export { checkHash };
    }
    export default _default;
    class Bootstrap extends EventEmitter {
        constructor(options: any);
        options: any;
        run(): Promise<void>;
        /**
         * @param {object} options
         * @param {Uint8Array} options.fileBuffer
         * @param {string} options.dest
         * @returns {Promise<void>}
         */
        write({ fileBuffer, dest }: {
            fileBuffer: Uint8Array;
            dest: string;
        }): Promise<void>;
        /**
         * @param {string} url - url to download
         * @returns {Promise<Uint8Array>}
         * @throws {Error} - if status code is not 200
         */
        download(url: string): Promise<Uint8Array>;
        cleanup(): void;
    }
    import { EventEmitter } from "socket:events";
}

declare module "socket:shared-worker/index" {
    export function init(sharedWorker: any, options: any): Promise<void>;
    /**
     * Gets the SharedWorker context window.
     * This function will create it if it does not already exist.
     * @return {Promise<import('./window.js').ApplicationWindow}
     */
    export function getContextWindow(): Promise<any>;
    export const SHARED_WORKER_WINDOW_INDEX: 46;
    export const SHARED_WORKER_WINDOW_TITLE: "socket:shared-worker";
    export const SHARED_WORKER_WINDOW_PATH: "/socket/shared-worker/index.html";
    export const channel: BroadcastChannel;
    export const workers: Map<any, any>;
    export class SharedWorkerMessagePort extends ipc.IPCMessagePort {
    }
    export class SharedWorker extends EventTarget {
        /**
         * `SharedWorker` class constructor.
         * @param {string|URL|Blob} aURL
         * @param {string|object=} [nameOrOptions]
         */
        constructor(aURL: string | URL | Blob, nameOrOptions?: (string | object) | undefined);
        set onerror(onerror: any);
        get onerror(): any;
        get ready(): any;
        get port(): any;
        get id(): any;
        #private;
    }
    export default SharedWorker;
    import ipc from "socket:ipc";
}

declare module "socket:internal/promise" {
    export const NativePromise: PromiseConstructor;
    export namespace NativePromisePrototype {
        export let then: <TResult1 = any, TResult2 = never>(onfulfilled?: (value: any) => TResult1 | PromiseLike<TResult1>, onrejected?: (reason: any) => TResult2 | PromiseLike<TResult2>) => globalThis.Promise<TResult1 | TResult2>;
        let _catch: <TResult = never>(onrejected?: (reason: any) => TResult | PromiseLike<TResult>) => globalThis.Promise<any | TResult>;
        export { _catch as catch };
        let _finally: (onfinally?: () => void) => globalThis.Promise<any>;
        export { _finally as finally };
    }
    export const NativePromiseAll: any;
    export const NativePromiseAny: any;
    /**
     * @typedef {function(any): void} ResolveFunction
     */
    /**
     * @typedef {function(Error|string|null): void} RejectFunction
     */
    /**
     * @typedef {function(ResolveFunction, RejectFunction): void} ResolverFunction
     */
    /**
     * @typedef {{
     *   promise: Promise,
     *   resolve: ResolveFunction,
     *   reject: RejectFunction
     * }} PromiseResolvers
     */
    export class Promise extends globalThis.Promise<any> {
        /**
         * Creates a new `Promise` with resolver functions.
         * @see {https://github.com/tc39/proposal-promise-with-resolvers}
         * @return {PromiseResolvers}
         */
        static withResolvers(): PromiseResolvers;
        /**
         * `Promise` class constructor.
         * @ignore
         * @param {ResolverFunction} resolver
         */
        constructor(resolver: ResolverFunction);
        [resourceSymbol]: {
            "__#15@#type": any;
            "__#15@#destroyed": boolean;
            "__#15@#asyncId": number;
            "__#15@#triggerAsyncId": any;
            "__#15@#requireManualDestroy": boolean;
            readonly type: string;
            readonly destroyed: boolean;
            asyncId(): number;
            triggerAsyncId(): number;
            emitDestroy(): CoreAsyncResource;
            bind(fn: Function, thisArg?: object | undefined): Function;
            runInAsyncScope(fn: Function, thisArg?: object | undefined, ...args?: any[]): any;
        };
    }
    export namespace Promise {
        function all(iterable: any): any;
        function any(iterable: any): any;
    }
    export default Promise;
    export type ResolveFunction = (arg0: any) => void;
    export type RejectFunction = (arg0: Error | string | null) => void;
    export type ResolverFunction = (arg0: ResolveFunction, arg1: RejectFunction) => void;
    export type PromiseResolvers = {
        promise: Promise;
        resolve: ResolveFunction;
        reject: RejectFunction;
    };
    const resourceSymbol: unique symbol;
    import * as asyncHooks from "socket:internal/async/hooks";
}

declare module "socket:internal/globals" {
    /**
     * Gets a runtime global value by name.
     * @ignore
     * @param {string} name
     * @return {any|null}
     */
    export function get(name: string): any | null;
    /**
     * Symbolic global registry
     * @ignore
     */
    export class GlobalsRegistry {
        get global(): any;
        symbol(name: any): symbol;
        register(name: any, value: any): any;
        get(name: any): any;
    }
    export default registry;
    const registry: any;
}

declare module "socket:console" {
    export function patchGlobalConsole(globalConsole: any, options?: {}): any;
    export const globalConsole: globalThis.Console;
    export class Console {
        /**
         * @ignore
         */
        constructor(options: any);
        /**
         * @type {import('dom').Console}
         */
        console: any;
        /**
         * @type {Map}
         */
        timers: Map<any, any>;
        /**
         * @type {Map}
         */
        counters: Map<any, any>;
        /**
         * @type {function?}
         */
        postMessage: Function | null;
        write(destination: any, ...args: any[]): any;
        assert(assertion: any, ...args: any[]): void;
        clear(): void;
        count(label?: string): void;
        countReset(label?: string): void;
        debug(...args: any[]): void;
        dir(...args: any[]): void;
        dirxml(...args: any[]): void;
        error(...args: any[]): void;
        info(...args: any[]): void;
        log(...args: any[]): void;
        table(...args: any[]): any;
        time(label?: string): void;
        timeEnd(label?: string): void;
        timeLog(label?: string): void;
        trace(...objects: any[]): void;
        warn(...args: any[]): void;
    }
    const _default: Console & {
        Console: typeof Console;
        globalConsole: globalThis.Console;
    };
    export default _default;
}

declare module "socket:vm" {
    /**
     * @ignore
     * @param {object[]} transfer
     * @param {object} object
     * @param {object=} [options]
     * @return {object[]}
     */
    export function findMessageTransfers(transfers: any, object: object, options?: object | undefined): object[];
    /**
     * @ignore
     * @param {object} context
     */
    export function applyInputContextReferences(context: object): void;
    /**
     * @ignore
     * @param {object} context
     */
    export function applyOutputContextReferences(context: object): void;
    /**
     * @ignore
     * @param {object} context
     */
    export function filterNonTransferableValues(context: object): void;
    /**
     * @ignore
     * @param {object=} [currentContext]
     * @param {object=} [updatedContext]
     * @param {object=} [contextReference]
     * @return {{ deletions: string[], merges: string[] }}
     */
    export function applyContextDifferences(currentContext?: object | undefined, updatedContext?: object | undefined, contextReference?: object | undefined, preserveScriptArgs?: boolean): {
        deletions: string[];
        merges: string[];
    };
    /**
     * Wrap a JavaScript function source.
     * @ignore
     * @param {string} source
     * @param {object=} [options]
     */
    export function wrapFunctionSource(source: string, options?: object | undefined): string;
    /**
     * Gets the VM context window.
     * This function will create it if it does not already exist.
     * @return {Promise<import('./window.js').ApplicationWindow}
     */
    export function getContextWindow(): Promise<import("socket:window").ApplicationWindow>;
    /**
     * Gets the `SharedWorker` that for the VM context.
     * @return {Promise<SharedWorker>}
     */
    export function getContextWorker(): Promise<SharedWorker>;
    /**
     * Terminates the VM script context window.
     * @ignore
     */
    export function terminateContextWindow(): Promise<void>;
    /**
     * Terminates the VM script context worker.
     * @ignore
     */
    export function terminateContextWorker(): Promise<void>;
    /**
     * Creates a prototype object of known global reserved intrinsics.
     * @ignore
     */
    export function createIntrinsics(options: any): any;
    /**
     * Returns `true` if value is an intrinsic, otherwise `false`.
     * @param {any} value
     * @return {boolean}
     */
    export function isIntrinsic(value: any): boolean;
    /**
     * Get the intrinsic type of a given `value`.
     * @param {any}
     * @return {function|object|null|undefined}
     */
    export function getIntrinsicType(value: any): Function | object | null | undefined;
    /**
     * Get the intrinsic type string of a given `value`.
     * @param {any}
     * @return {string|null}
     */
    export function getIntrinsicTypeString(value: any): string | null;
    /**
     * Creates a global proxy object for context execution.
     * @ignore
     * @param {object} context
     * @param {object=} [options]
     * @return {Proxy}
     */
    export function createGlobalObject(context: object, options?: object | undefined): ProxyConstructor;
    /**
     * @ignore
     * @param {string} source
     * @return {boolean}
     */
    export function detectFunctionSourceType(source: string): boolean;
    /**
     * Compiles `source`  with `options` into a function.
     * @ignore
     * @param {string} source
     * @param {object=} [options]
     * @return {function}
     */
    export function compileFunction(source: string, options?: object | undefined): Function;
    /**
     * Run `source` JavaScript in given context. The script context execution
     * context is preserved until the `context` object that points to it is
     * garbage collected or there are no longer any references to it and its
     * associated `Script` instance.
     * @param {string|object|function} source
     * @param {object=} [context]
     * @param {ScriptOptions=} [options]
     * @return {Promise<any>}
     */
    export function runInContext(source: string | object | Function, context?: object | undefined, options?: ScriptOptions | undefined): Promise<any>;
    /**
     * Run `source` JavaScript in new context. The script context is destroyed after
     * execution. This is typically a "one off" isolated run.
     * @param {string} source
     * @param {object=} [context]
     * @param {ScriptOptions=} [options]
     * @return {Promise<any>}
     */
    export function runInNewContext(source: string, context?: object | undefined, options?: ScriptOptions | undefined): Promise<any>;
    /**
     * Run `source` JavaScript in this current context (`globalThis`).
     * @param {string} source
     * @param {ScriptOptions=} [options]
     * @return {Promise<any>}
     */
    export function runInThisContext(source: string, options?: ScriptOptions | undefined): Promise<any>;
    /**
     * @ignore
     * @param {Reference} reference
     */
    export function putReference(reference: Reference): void;
    /**
     * Create a `Reference` for a `value` in a script `context`.
     * @param {any} value
     * @param {object} context
     * @param {object=} [options]
     * @return {Reference}
     */
    export function createReference(value: any, context: object, options?: object | undefined): Reference;
    /**
     * Get a script context by ID or values
     * @param {string|object|function} id
     * @return {Reference?}
     */
    export function getReference(id: string | object | Function): Reference | null;
    /**
     * Remove a script context reference by ID.
     * @param {string} id
     */
    export function removeReference(id: string): void;
    /**
     * Get all transferable values in the `object` hierarchy.
     * @param {object} object
     * @return {object[]}
     */
    export function getTransferables(object: object): object[];
    /**
     * @ignore
     * @param {object} object
     * @return {object}
     */
    export function createContext(object: object): object;
    /**
     * Returns `true` if `object` is a "context" object.
     * @param {object}
     * @return {boolean}
     */
    export function isContext(object: any): boolean;
    /**
     * Shared broadcast for virtual machaines
     * @type {BroadcastChannel}
     */
    export const channel: BroadcastChannel;
    /**
     * A container for a context worker message channel that looks like a "worker".
     * @ignore
     */
    export class ContextWorkerInterface extends EventTarget {
        get channel(): any;
        get port(): any;
        destroy(): void;
        #private;
    }
    /**
     * A container proxy for a context worker message channel that
     * looks like a "worker".
     * @ignore
     */
    export class ContextWorkerInterfaceProxy extends EventTarget {
        constructor(globals: any);
        get port(): any;
        #private;
    }
    /**
     * Global reserved values that a script context may not modify.
     * @type {string[]}
     */
    export const RESERVED_GLOBAL_INTRINSICS: string[];
    /**
     * A unique reference to a value owner by a "context object" and a
     * `Script` instance.
     */
    export class Reference {
        /**
         * Predicate function to determine if a `value` is an internal or external
         * script reference value.
         * @param {amy} value
         * @return {boolean}
         */
        static isReference(value: amy): boolean;
        /**
         * `Reference` class constructor.
         * @param {string} id
         * @param {any} value
         * @param {object=} [context]
         * @param {object=} [options]
         */
        constructor(id: string, value: any, context?: object | undefined, options?: object | undefined);
        /**
         * The unique id of the reference
         * @type {string}
         */
        get id(): string;
        /**
         * The underling primitive type of the reference value.
         * @ignore
         * @type {'undefined'|'object'|'number'|'boolean'|'function'|'symbol'}
         */
        get type(): "number" | "boolean" | "symbol" | "undefined" | "object" | "function";
        /**
         * The underlying value of the reference.
         * @type {any?}
         */
        get value(): any;
        /**
         * The name of the type.
         * @type {string?}
         */
        get name(): string;
        /**
         * The `Script` this value belongs to, if available.
         * @type {Script?}
         */
        get script(): Script;
        /**
         * The "context object" this reference value belongs to.
         * @type {object?}
         */
        get context(): any;
        /**
         * A boolean value to indicate if the underlying reference value is an
         * intrinsic value.
         * @type {boolean}
         */
        get isIntrinsic(): boolean;
        /**
         * A boolean value to indicate if the underlying reference value is an
         * external reference value.
         * @type {boolean}
         */
        get isExternal(): boolean;
        /**
         * The intrinsic type this reference may be an instance of or directly refer to.
         * @type {function|object}
         */
        get intrinsicType(): any;
        /**
         * Releases strongly held value and weak references
         * to the "context object".
         */
        release(): void;
        /**
         * Converts this `Reference` to a JSON object.
         * @param {boolean=} [includeValue = false]
         */
        toJSON(includeValue?: boolean | undefined): {
            __vmScriptReference__: boolean;
            id: string;
            type: "number" | "boolean" | "symbol" | "undefined" | "object" | "function";
            name: string;
            isIntrinsic: boolean;
            intrinsicType: string;
        };
        #private;
    }
    /**
     * @typedef {{
     *  filename?: string,
     *  context?: object
     * }} ScriptOptions
     */
    /**
     * A `Script` is a container for raw JavaScript to be executed in
     * a completely isolated virtual machine context, optionally with
     * user supplied context. Context objects references are not actually
     * shared, but instead provided to the script execution context using the
     * structured cloning algorithm used by the Message Channel API. Context
     * differences are computed and applied after execution so the user supplied
     * context object realizes context changes after script execution. All script
     * sources run in an "async" context so a "top level await" should work.
     */
    export class Script extends EventTarget {
        /**
         * `Script` class constructor
         * @param {string} source
         * @param {ScriptOptions} [options]
         */
        constructor(source: string, options?: ScriptOptions);
        /**
         * The script identifier.
         */
        get id(): any;
        /**
         * The source for this script.
         * @type {string}
         */
        get source(): string;
        /**
         * The filename for this script.
         * @type {string}
         */
        get filename(): string;
        /**
         * A promise that resolves when the script is ready.
         * @type {Promise<Boolean>}
         */
        get ready(): Promise<boolean>;
        /**
         * The default script context object
         * @type {object}
         */
        get context(): any;
        /**
         * Destroy the script execution context.
         * @return {Promise}
         */
        destroy(): Promise<any>;
        /**
         * Run `source` JavaScript in given context. The script context execution
         * context is preserved until the `context` object that points to it is
         * garbage collected or there are no longer any references to it and its
         * associated `Script` instance.
         * @param {ScriptOptions=} [options]
         * @param {object=} [context]
         * @return {Promise<any>}
         */
        runInContext(context?: object | undefined, options?: ScriptOptions | undefined): Promise<any>;
        /**
         * Run `source` JavaScript in new context. The script context is destroyed after
         * execution. This is typically a "one off" isolated run.
         * @param {ScriptOptions=} [options]
         * @param {object=} [context]
         * @return {Promise<any>}
         */
        runInNewContext(context?: object | undefined, options?: ScriptOptions | undefined): Promise<any>;
        /**
         * Run `source` JavaScript in this current context (`globalThis`).
         * @param {ScriptOptions=} [options]
         * @return {Promise<any>}
         */
        runInThisContext(options?: ScriptOptions | undefined): Promise<any>;
        #private;
    }
    namespace _default {
        export { createGlobalObject };
        export { compileFunction };
        export { createReference };
        export { getContextWindow };
        export { getContextWorker };
        export { getReference };
        export { getTransferables };
        export { putReference };
        export { Reference };
        export { removeReference };
        export { runInContext };
        export { runInNewContext };
        export { runInThisContext };
        export { Script };
        export { createContext };
        export { isContext };
        export { channel };
    }
    export default _default;
    export type ScriptOptions = {
        filename?: string;
        context?: object;
    };
    import { SharedWorker } from "socket:shared-worker/index";
}

declare module "socket:worker_threads/init" {
    export const SHARE_ENV: unique symbol;
    export const isMainThread: boolean;
    export namespace state {
        export { isMainThread };
        export let parentPort: any;
        export let mainPort: any;
        export let workerData: any;
        export let url: any;
        export let env: {};
        export let id: number;
    }
    namespace _default {
        export { state };
    }
    export default _default;
}

declare module "socket:worker_threads" {
    /**
     * Set shared worker environment data.
     * @param {string} key
     * @param {any} value
     */
    export function setEnvironmentData(key: string, value: any): void;
    /**
     * Get shared worker environment data.
     * @param {string} key
     * @return {any}
     */
    export function getEnvironmentData(key: string): any;
    /**
    
     * A pool of known worker threads.
     * @type {<Map<string, Worker>}
     */
    export const workers: <Map>() => <string, Worker>() => any;
    /**
     * `true` if this is the "main" thread, otherwise `false`
     * The "main" thread is the top level webview window.
     * @type {boolean}
     */
    export const isMainThread: boolean;
    /**
     * The main thread `MessagePort` which is `null` when the
     * current context is not the "main thread".
     * @type {MessagePort?}
     */
    export const mainPort: MessagePort | null;
    /**
     * A worker thread `BroadcastChannel` class.
     */
    export class BroadcastChannel extends globalThis.BroadcastChannel {
    }
    /**
     * A worker thread `MessageChannel` class.
     */
    export class MessageChannel extends globalThis.MessageChannel {
    }
    /**
     * A worker thread `MessagePort` class.
     */
    export class MessagePort extends globalThis.MessagePort {
    }
    /**
     * The current unique thread ID.
     * @type {number}
     */
    export const threadId: number;
    /**
     * The parent `MessagePort` instance
     * @type {MessagePort?}
     */
    export const parentPort: MessagePort | null;
    /**
     * Transferred "worker data" when creating a new `Worker` instance.
     * @type {any?}
     */
    export const workerData: any | null;
    export class Pipe extends AsyncResource {
        /**
         * `Pipe` class constructor.
         * @param {Childworker} worker
         * @ignore
         */
        constructor(worker: Childworker);
        /**
         * `true` if the pipe is still reading, otherwise `false`.
         * @type {boolean}
         */
        get reading(): boolean;
        /**
         * Destroys the pipe
         */
        destroy(): void;
        #private;
    }
    /**
     * @typedef {{
     *   env?: object,
     *   stdin?: boolean = false,
     *   stdout?: boolean = false,
     *   stderr?: boolean = false,
     *   workerData?: any,
     *   transferList?: any[],
     *   eval?: boolean = false
     * }} WorkerOptions
    
    /**
     * A worker thread that can communicate directly with a parent thread,
     * share environment data, and process streamed data.
     */
    export class Worker extends EventEmitter {
        /**
         * `Worker` class constructor.
         * @param {string} filename
         * @param {WorkerOptions=} [options]
         */
        constructor(filename: string, options?: WorkerOptions | undefined);
        /**
         * Handles incoming worker messages.
         * @ignore
         * @param {MessageEvent} event
         */
        onWorkerMessage(event: MessageEvent): boolean;
        /**
         * Handles process environment change events
         * @ignore
         * @param {import('./process.js').ProcessEnvironmentEvent} event
         */
        onProcessEnvironmentEvent(event: import("socket:process").ProcessEnvironmentEvent): void;
        /**
         * The unique ID for this `Worker` thread instace.
         * @type {number}
         */
        get id(): number;
        get threadId(): number;
        /**
         * A `Writable` standard input stream if `{ stdin: true }` was set when
         * creating this `Worker` instance.
         * @type {import('./stream.js').Writable?}
         */
        get stdin(): Writable;
        /**
         * A `Readable` standard output stream if `{ stdout: true }` was set when
         * creating this `Worker` instance.
         * @type {import('./stream.js').Readable?}
         */
        get stdout(): Readable;
        /**
         * A `Readable` standard error stream if `{ stderr: true }` was set when
         * creating this `Worker` instance.
         * @type {import('./stream.js').Readable?}
         */
        get stderr(): Readable;
        /**
         * Terminates the `Worker` instance
         */
        terminate(): void;
        postMessage(...args: any[]): void;
        #private;
    }
    namespace _default {
        export { Worker };
        export { isMainThread };
        export { parentPort };
        export { setEnvironmentData };
        export { getEnvironmentData };
        export { workerData };
        export { threadId };
        export { SHARE_ENV };
    }
    export default _default;
    /**
     * /**
     * A worker thread that can communicate directly with a parent thread,
     * share environment data, and process streamed data.
     */
    export type WorkerOptions = {
        env?: object;
        stdin?: boolean;
        stdout?: boolean;
        stderr?: boolean;
        workerData?: any;
        transferList?: any[];
        eval?: boolean;
    };
    import { AsyncResource } from "socket:async/resource";
    import { EventEmitter } from "socket:events";
    import { Writable } from "socket:stream";
    import { Readable } from "socket:stream";
    import { SHARE_ENV } from "socket:worker_threads/init";
    import init from "socket:worker_threads/init";
    export { SHARE_ENV, init };
}

declare module "socket:child_process" {
    /**
     * Spawns a child process exeucting `command` with `args`
     * @param {string} command
     * @param {string[]|object=} [args]
     * @param {object=} [options
     * @return {ChildProcess}
     */
    export function spawn(command: string, args?: (string[] | object) | undefined, options?: object | undefined): ChildProcess;
    export function exec(command: any, options: any, callback: any): ChildProcess & {
        then(resolve: any, reject: any): Promise<any>;
        catch(reject: any): Promise<any>;
        finally(next: any): Promise<any>;
    };
    export function execSync(command: any, options: any): any;
    export class Pipe extends AsyncResource {
        /**
         * `Pipe` class constructor.
         * @param {ChildProcess} process
         * @ignore
         */
        constructor(process: ChildProcess);
        /**
         * `true` if the pipe is still reading, otherwise `false`.
         * @type {boolean}
         */
        get reading(): boolean;
        /**
         * @type {import('./process')}
         */
        get process(): typeof import("socket:process");
        /**
         * Destroys the pipe
         */
        destroy(): void;
        #private;
    }
    export class ChildProcess extends EventEmitter {
        /**
         * `ChildProcess` class constructor.
         * @param {{
         *   env?: object,
         *   stdin?: boolean,
         *   stdout?: boolean,
         *   stderr?: boolean,
         *   signal?: AbortSigal,
         * }=} [options]
         */
        constructor(options?: {
            env?: object;
            stdin?: boolean;
            stdout?: boolean;
            stderr?: boolean;
            signal?: AbortSigal;
        } | undefined);
        /**
         * @ignore
         * @type {Pipe}
         */
        get pipe(): Pipe;
        /**
         * `true` if the child process was killed with kill()`,
         * otherwise `false`.
         * @type {boolean}
         */
        get killed(): boolean;
        /**
         * The process identifier for the child process. This value is
         * `> 0` if the process was spawned successfully, otherwise `0`.
         * @type {number}
         */
        get pid(): number;
        /**
         * The executable file name of the child process that is launched. This
         * value is `null` until the child process has successfully been spawned.
         * @type {string?}
         */
        get spawnfile(): string;
        /**
         * The full list of command-line arguments the child process was spawned with.
         * This value is an empty array until the child process has successfully been
         * spawned.
         * @type {string[]}
         */
        get spawnargs(): string[];
        /**
         * Always `false` as the IPC messaging is not supported.
         * @type {boolean}
         */
        get connected(): boolean;
        /**
         * The child process exit code. This value is `null` if the child process
         * is still running, otherwise it is a positive integer.
         * @type {number?}
         */
        get exitCode(): number;
        /**
         * If available, the underlying `stdin` writable stream for
         * the child process.
         * @type {import('./stream').Writable?}
         */
        get stdin(): import("socket:stream").Writable;
        /**
         * If available, the underlying `stdout` readable stream for
         * the child process.
         * @type {import('./stream').Readable?}
         */
        get stdout(): import("socket:stream").Readable;
        /**
         * If available, the underlying `stderr` readable stream for
         * the child process.
         * @type {import('./stream').Readable?}
         */
        get stderr(): import("socket:stream").Readable;
        /**
         * The underlying worker thread.
         * @ignore
         * @type {import('./worker_threads').Worker}
         */
        get worker(): Worker;
        /**
         * This function does nothing, but is present for nodejs compat.
         */
        disconnect(): boolean;
        /**
         * This function does nothing, but is present for nodejs compat.
         * @return {boolean}
         */
        send(): boolean;
        /**
         * This function does nothing, but is present for nodejs compat.
         */
        ref(): boolean;
        /**
         * This function does nothing, but is present for nodejs compat.
         */
        unref(): boolean;
        /**
         * Kills the child process. This function throws an error if the child
         * process has not been spawned or is already killed.
         * @param {number|string} signal
         */
        kill(...args: any[]): this;
        /**
         * Spawns the child process. This function will thrown an error if the process
         * is already spawned.
         * @param {string} command
         * @param {string[]=} [args]
         * @return {ChildProcess}
         */
        spawn(...args?: string[] | undefined): ChildProcess;
        /**
         * `EventTarget` based `addEventListener` method.
         * @param {string} event
         * @param {function(Event)} callback
         * @param {{ once?: false }} [options]
         */
        addEventListener(event: string, callback: (arg0: Event) => any, options?: {
            once?: false;
        }): void;
        /**
         * `EventTarget` based `removeEventListener` method.
         * @param {string} event
         * @param {function(Event)} callback
         * @param {{ once?: false }} [options]
         */
        removeEventListener(event: string, callback: (arg0: Event) => any): void;
        #private;
    }
    export function execFile(command: any, options: any, callback: any): ChildProcess & {
        then(resolve: any, reject: any): Promise<any>;
        catch(reject: any): Promise<any>;
        finally(next: any): Promise<any>;
    };
    namespace _default {
        export { ChildProcess };
        export { spawn };
        export { execFile };
        export { exec };
    }
    export default _default;
    import { AsyncResource } from "socket:async/resource";
    import { EventEmitter } from "socket:events";
    import { Worker } from "socket:worker_threads";
}

declare module "socket:constants" {
    export * from "socket:fs/constants";
    export * from "socket:window/constants";
    export const E2BIG: any;
    export const EACCES: any;
    export const EADDRINUSE: any;
    export const EADDRNOTAVAIL: any;
    export const EAFNOSUPPORT: any;
    export const EAGAIN: any;
    export const EALREADY: any;
    export const EBADF: any;
    export const EBADMSG: any;
    export const EBUSY: any;
    export const ECANCELED: any;
    export const ECHILD: any;
    export const ECONNABORTED: any;
    export const ECONNREFUSED: any;
    export const ECONNRESET: any;
    export const EDEADLK: any;
    export const EDESTADDRREQ: any;
    export const EDOM: any;
    export const EDQUOT: any;
    export const EEXIST: any;
    export const EFAULT: any;
    export const EFBIG: any;
    export const EHOSTUNREACH: any;
    export const EIDRM: any;
    export const EILSEQ: any;
    export const EINPROGRESS: any;
    export const EINTR: any;
    export const EINVAL: any;
    export const EIO: any;
    export const EISCONN: any;
    export const EISDIR: any;
    export const ELOOP: any;
    export const EMFILE: any;
    export const EMLINK: any;
    export const EMSGSIZE: any;
    export const EMULTIHOP: any;
    export const ENAMETOOLONG: any;
    export const ENETDOWN: any;
    export const ENETRESET: any;
    export const ENETUNREACH: any;
    export const ENFILE: any;
    export const ENOBUFS: any;
    export const ENODATA: any;
    export const ENODEV: any;
    export const ENOENT: any;
    export const ENOEXEC: any;
    export const ENOLCK: any;
    export const ENOLINK: any;
    export const ENOMEM: any;
    export const ENOMSG: any;
    export const ENOPROTOOPT: any;
    export const ENOSPC: any;
    export const ENOSR: any;
    export const ENOSTR: any;
    export const ENOSYS: any;
    export const ENOTCONN: any;
    export const ENOTDIR: any;
    export const ENOTEMPTY: any;
    export const ENOTSOCK: any;
    export const ENOTSUP: any;
    export const ENOTTY: any;
    export const ENXIO: any;
    export const EOPNOTSUPP: any;
    export const EOVERFLOW: any;
    export const EPERM: any;
    export const EPIPE: any;
    export const EPROTO: any;
    export const EPROTONOSUPPORT: any;
    export const EPROTOTYPE: any;
    export const ERANGE: any;
    export const EROFS: any;
    export const ESPIPE: any;
    export const ESRCH: any;
    export const ESTALE: any;
    export const ETIME: any;
    export const ETIMEDOUT: any;
    export const ETXTBSY: any;
    export const EWOULDBLOCK: any;
    export const EXDEV: any;
    export const SIGHUP: any;
    export const SIGINT: any;
    export const SIGQUIT: any;
    export const SIGILL: any;
    export const SIGTRAP: any;
    export const SIGABRT: any;
    export const SIGIOT: any;
    export const SIGBUS: any;
    export const SIGFPE: any;
    export const SIGKILL: any;
    export const SIGUSR1: any;
    export const SIGSEGV: any;
    export const SIGUSR2: any;
    export const SIGPIPE: any;
    export const SIGALRM: any;
    export const SIGTERM: any;
    export const SIGCHLD: any;
    export const SIGCONT: any;
    export const SIGSTOP: any;
    export const SIGTSTP: any;
    export const SIGTTIN: any;
    export const SIGTTOU: any;
    export const SIGURG: any;
    export const SIGXCPU: any;
    export const SIGXFSZ: any;
    export const SIGVTALRM: any;
    export const SIGPROF: any;
    export const SIGWINCH: any;
    export const SIGIO: any;
    export const SIGINFO: any;
    export const SIGSYS: any;
    const _default: any;
    export default _default;
}

declare module "socket:timers/platform" {
    export namespace platform {
        let setTimeout: any;
        let setInterval: any;
        let setImmediate: any;
        let clearTimeout: any;
        let clearInterval: any;
        let clearImmediate: any;
        let postTask: any;
    }
    export default platform;
}

declare module "socket:timers/timer" {
    export class Timer extends AsyncResource {
        static from(...args: any[]): Timer;
        constructor(type: any, create: any, destroy: any);
        get id(): number;
        init(...args: any[]): this;
        close(): boolean;
        [Symbol.toPrimitive](): number;
        #private;
    }
    export class Timeout extends Timer {
        constructor();
    }
    export class Interval extends Timer {
        constructor();
    }
    export class Immediate extends Timer {
        constructor();
    }
    namespace _default {
        export { Timer };
        export { Immediate };
        export { Timeout };
        export { Interval };
    }
    export default _default;
    import { AsyncResource } from "socket:async/resource";
}

declare module "socket:timers/promises" {
    export function setTimeout(delay?: number, value?: any, options?: any): Promise<any>;
    export function setInterval(delay?: number, value?: any, options?: any): AsyncGenerator<any, void, unknown>;
    export function setImmediate(value?: any, options?: any): Promise<any>;
    namespace _default {
        export { setImmediate };
        export { setInterval };
        export { setTimeout };
    }
    export default _default;
}

declare module "socket:timers/scheduler" {
    export function wait(delay: any, options?: any): Promise<any>;
    export function postTask(callback: any, options?: any): Promise<any>;
    namespace _default {
        export { postTask };
        export { setImmediate as yield };
        export { wait };
    }
    export default _default;
    import { setImmediate } from "socket:timers/promises";
}

declare module "socket:timers/index" {
    export function setTimeout(callback: any, delay: any, ...args: any[]): import("socket:timers/timer").Timer;
    export function clearTimeout(timeout: any): void;
    export function setInterval(callback: any, delay: any, ...args: any[]): import("socket:timers/timer").Timer;
    export function clearInterval(interval: any): void;
    export function setImmediate(callback: any, ...args: any[]): import("socket:timers/timer").Timer;
    export function clearImmediate(immediate: any): void;
    /**
     * Pause async execution for `timeout` milliseconds.
     * @param {number} timeout
     * @return {Promise}
     */
    export function sleep(timeout: number): Promise<any>;
    export namespace sleep {
        /**
         * Pause sync execution for `timeout` milliseconds.
         * @param {number} timeout
         */
        function sync(timeout: number): void;
    }
    export { platform };
    namespace _default {
        export { platform };
        export { promises };
        export { scheduler };
        export { setTimeout };
        export { clearTimeout };
        export { setInterval };
        export { clearInterval };
        export { setImmediate };
        export { clearImmediate };
    }
    export default _default;
    import platform from "socket:timers/platform";
    import promises from "socket:timers/promises";
    import scheduler from "socket:timers/scheduler";
}

declare module "socket:timers" {
    export * from "socket:timers/index";
    export default exports;
    import * as exports from "socket:timers/index";
}

declare module "socket:internal/conduit" {
    /**
     * @typedef {{ options: object, payload: Uint8Array }} ReceiveMessage
     * @typedef {function(Error?, ReceiveCallback | undefined)} ReceiveCallback
     * @typedef {{ isActive: boolean, handles: { ids: string[], count: number }}} ConduitDiagnostics
     * @typedef {{ isActive: boolean, port: number }} ConduitStatus
     * @typedef {{
     *   id?: string|BigInt|number,
     *   sharedKey?: string
     *}} ConduitOptions
     */
    export const DEFALUT_MAX_RECONNECT_RETRIES: 32;
    export const DEFAULT_MAX_RECONNECT_TIMEOUT: 256;
    /**
     * A pool of known `Conduit` instances.
     * @type {Set<Conduit>}
     */
    export const pool: Set<Conduit>;
    /**
     * A container for managing a WebSocket connection to the internal runtime
     * Conduit WebSocket server.
     */
    export class Conduit extends EventTarget {
        static set port(port: number);
        /**
         * The global `Conduit` port
         * @type {number}
         */
        static get port(): number;
        /**
         * Returns diagnostics information about the conduit server
         * @return {Promise<ConduitDiagnostics>}
         */
        static diagnostics(): Promise<ConduitDiagnostics>;
        /**
         * Returns the current Conduit server status
         * @return {Promise<ConduitStatus>}
         */
        static status(): Promise<ConduitStatus>;
        /**
         * Waits for conduit to be active
         * @param {{ maxQueriesForStatus?: number }=} [options]
         * @return {Promise}
         */
        static waitForActiveState(options?: {
            maxQueriesForStatus?: number;
        } | undefined): Promise<any>;
        /**
         * Creates an instance of Conduit.
         *
         * @param {ConduitOptions} options
         */
        constructor(options: ConduitOptions);
        /**
         * @type {boolean}
         */
        shouldReconnect: boolean;
        /**
         * @type {boolean}
         */
        isConnecting: boolean;
        /**
         * @type {boolean}
         */
        isActive: boolean;
        /**
         * @type {WebSocket?}
         */
        socket: WebSocket | null;
        /**
         * @type {number}
         */
        port: number;
        /**
         * @type {number?}
         */
        id: number | null;
        /**
         * @type {string}
         */
        sharedKey: string;
        /**
         * The URL string for the WebSocket server.
         * @type {string}
         */
        get url(): string;
        set onmessage(onmessage: (arg0: MessageEvent) => any);
        /**
         * @type {function(MessageEvent)}
         */
        get onmessage(): (arg0: MessageEvent) => any;
        set onerror(onerror: (arg0: ErrorEvent) => any);
        /**
         * @type {function(ErrorEvent)}
         */
        get onerror(): (arg0: ErrorEvent) => any;
        set onclose(onclose: (arg0: CloseEvent) => any);
        /**
         * @type {function(CloseEvent)}
         */
        get onclose(): (arg0: CloseEvent) => any;
        set onopen(onopen: (arg0: Event) => any);
        /**
         * @type {function(Event)}
         */
        get onopen(): (arg0: Event) => any;
        /**
         * Connects the underlying conduit `WebSocket`.
         * @param {function(Error?)=} [callback]
         * @return {Promise<Conduit>}
         */
        connect(callback?: ((arg0: Error | null) => any) | undefined): Promise<Conduit>;
        /**
         * Reconnects a `Conduit` socket.
         * @param {{retries?: number, timeout?: number}} [options]
         * @return {Promise<Conduit>}
         */
        reconnect(options?: {
            retries?: number;
            timeout?: number;
        }): Promise<Conduit>;
        /**
         * Encodes a single header into a Uint8Array.
         *
         * @private
         * @param {string} key - The header key.
         * @param {string} value - The header value.
         * @returns {Uint8Array} The encoded header.
         */
        private encodeOption;
        /**
         * Encodes options and payload into a single Uint8Array message.
         *
         * @private
         * @param {object} options - The options to encode.
         * @param {Uint8Array} payload - The payload to encode.
         * @returns {Uint8Array} The encoded message.
         */
        private encodeMessage;
        /**
         * Decodes a Uint8Array message into options and payload.
         * @param {Uint8Array} data - The data to decode.
         * @return {ReceiveMessage} The decoded message containing options and payload.
         * @throws Will throw an error if the data is invalid.
         */
        decodeMessage(data: Uint8Array): ReceiveMessage;
        /**
         * Registers a callback to handle incoming messages.
         * The callback will receive an error object and an object containing
         * decoded options and payload.
         * @param {ReceiveCallback} callback - The callback function to handle incoming messages.
         */
        receive(callback: ReceiveCallback): void;
        /**
         * Sends a message with the specified options and payload over the
         * WebSocket connection.
         * @param {object} options - The options to send.
         * @param {Uint8Array=} [payload] - The payload to send.
         * @return {boolean}
         */
        send(options: object, payload?: Uint8Array | undefined): boolean;
        /**
         * Closes the WebSocket connection, preventing reconnects.
         */
        close(): void;
        #private;
    }
    export type ReceiveMessage = {
        options: object;
        payload: Uint8Array;
    };
    export type ReceiveCallback = (arg0: Error | null, arg1: ReceiveCallback | undefined) => any;
    export type ConduitDiagnostics = {
        isActive: boolean;
        handles: {
            ids: string[];
            count: number;
        };
    };
    export type ConduitStatus = {
        isActive: boolean;
        port: number;
    };
    export type ConduitOptions = {
        id?: string | BigInt | number;
        sharedKey?: string;
    };
}

declare module "socket:ip" {
    /**
     * Normalizes input as an IPv4 address string
     * @param {string|object|string[]|Uint8Array} input
     * @return {string}
     */
    export function normalizeIPv4(input: string | object | string[] | Uint8Array): string;
    /**
     * Determines if an input `string` is in IP address version 4 format.
     * @param {string|object|string[]|Uint8Array} input
     * @return {boolean}
     */
    export function isIPv4(input: string | object | string[] | Uint8Array): boolean;
    namespace _default {
        export { normalizeIPv4 };
        export { isIPv4 };
    }
    export default _default;
}

declare module "socket:dns/promises" {
    /**
     * @async
     * @see {@link https://nodejs.org/api/dns.html#dnspromiseslookuphostname-options}
     * @param {string} hostname - The host name to resolve.
     * @param {Object=} opts - An options object.
     * @param {(number|string)=} [opts.family=0] - The record family. Must be 4, 6, or 0. For backward compatibility reasons,'IPv4' and 'IPv6' are interpreted as 4 and 6 respectively. The value 0 indicates that IPv4 and IPv6 addresses are both returned. Default: 0.
     * @returns {Promise}
     */
    export function lookup(hostname: string, opts?: any | undefined): Promise<any>;
    export default exports;
    import * as exports from "socket:dns/promises";
    
}

declare module "socket:dns/index" {
    /**
     * Resolves a host name (e.g. `example.org`) into the first found A (IPv4) or
     * AAAA (IPv6) record. All option properties are optional. If options is an
     * integer, then it must be 4 or 6 – if options is 0 or not provided, then IPv4
     * and IPv6 addresses are both returned if found.
     *
     * From the node.js website...
     *
     * > With the all option set to true, the arguments for callback change to (err,
     * addresses), with addresses being an array of objects with the properties
     * address and family.
     *
     * > On error, err is an Error object, where err.code is the error code. Keep in
     * mind that err.code will be set to 'ENOTFOUND' not only when the host name does
     * not exist but also when the lookup fails in other ways such as no available
     * file descriptors. dns.lookup() does not necessarily have anything to do with
     * the DNS protocol. The implementation uses an operating system facility that
     * can associate names with addresses and vice versa. This implementation can
     * have subtle but important consequences on the behavior of any Node.js program.
     * Please take some time to consult the Implementation considerations section
     * before using dns.lookup().
     *
     * @see {@link https://nodejs.org/api/dns.html#dns_dns_lookup_hostname_options_callback}
     * @param {string} hostname - The host name to resolve.
     * @param {(object|intenumberger)=} [options] - An options object or record family.
     * @param {(number|string)=} [options.family=0] - The record family. Must be 4, 6, or 0. For backward compatibility reasons,'IPv4' and 'IPv6' are interpreted as 4 and 6 respectively. The value 0 indicates that IPv4 and IPv6 addresses are both returned. Default: 0.
     * @param {function} cb - The function to call after the method is complete.
     * @returns {void}
     */
    export function lookup(hostname: string, options?: (object | intenumberger) | undefined, cb: Function): void;
    export { promises };
    export default exports;
    import * as promises from "socket:dns/promises";
    import * as exports from "socket:dns/index";
    
}

declare module "socket:dns" {
    export * from "socket:dns/index";
    export default exports;
    import * as exports from "socket:dns/index";
}

declare module "socket:dgram" {
    export function createSocket(options: string | any, callback?: Function | undefined): Socket;
    /**
     * New instances of dgram.Socket are created using dgram.createSocket().
     * The new keyword is not to be used to create dgram.Socket instances.
     */
    export class Socket extends EventEmitter {
        constructor(options: any, callback: any);
        id: any;
        knownIdWasGivenInSocketConstruction: boolean;
        type: any;
        signal: any;
        state: {
            recvBufferSize: any;
            sendBufferSize: any;
            bindState: number;
            connectState: number;
            reuseAddr: boolean;
            ipv6Only: boolean;
        };
        /**
         * Listen for datagram messages on a named port and optional address
         * If the address is not specified, the operating system will attempt to
         * listen on all addresses. Once the binding is complete, a 'listening'
         * event is emitted and the optional callback function is called.
         *
         * If binding fails, an 'error' event is emitted.
         *
         * @param {number} port - The port to listen for messages on
         * @param {string} address - The address to bind to (0.0.0.0)
         * @param {function} callback - With no parameters. Called when binding is complete.
         * @see {@link https://nodejs.org/api/dgram.html#socketbindport-address-callback}
         */
        bind(arg1: any, arg2: any, arg3: any): this;
        dataListener: ({ detail }: {
            detail: any;
        }) => any;
        conduit: Conduit;
        /**
         * Associates the dgram.Socket to a remote address and port. Every message sent
         * by this handle is automatically sent to that destination. Also, the socket
         * will only receive messages from that remote peer. Trying to call connect()
         * on an already connected socket will result in an ERR_SOCKET_DGRAM_IS_CONNECTED
         * exception. If the address is not provided, '0.0.0.0' (for udp4 sockets) or '::1'
         * (for udp6 sockets) will be used by default. Once the connection is complete,
         * a 'connect' event is emitted and the optional callback function is called.
         * In case of failure, the callback is called or, failing this, an 'error' event
         * is emitted.
         *
         * @param {number} port - Port the client should connect to.
         * @param {string=} host - Host the client should connect to.
         * @param {function=} connectListener - Common parameter of socket.connect() methods. Will be added as a listener for the 'connect' event once.
         * @see {@link https://nodejs.org/api/dgram.html#socketconnectport-address-callback}
         */
        connect(arg1: any, arg2: any, arg3: any): void;
        /**
         * A synchronous function that disassociates a connected dgram.Socket from
         * its remote address. Trying to call disconnect() on an unbound or already
         * disconnected socket will result in an ERR_SOCKET_DGRAM_NOT_CONNECTED exception.
         *
         * @see {@link https://nodejs.org/api/dgram.html#socketdisconnect}
         */
        disconnect(): void;
        /**
         * Broadcasts a datagram on the socket. For connectionless sockets, the
         * destination port and address must be specified. Connected sockets, on the
         * other hand, will use their associated remote endpoint, so the port and
         * address arguments must not be set.
         *
         * > The msg argument contains the message to be sent. Depending on its type,
         * different behavior can apply. If msg is a Buffer, any TypedArray, or a
         * DataView, the offset and length specify the offset within the Buffer where
         * the message begins and the number of bytes in the message, respectively.
         * If msg is a String, then it is automatically converted to a Buffer with
         * 'utf8' encoding. With messages that contain multi-byte characters, offset,
         * and length will be calculated with respect to byte length and not the
         * character position. If msg is an array, offset and length must not be
         * specified.
         *
         * > The address argument is a string. If the value of the address is a hostname,
         * DNS will be used to resolve the address of the host. If the address is not
         * provided or otherwise nullish, '0.0.0.0' (for udp4 sockets) or '::1'
         * (for udp6 sockets) will be used by default.
         *
         * > If the socket has not been previously bound with a call to bind, the socket
         * is assigned a random port number and is bound to the "all interfaces"
         * address ('0.0.0.0' for udp4 sockets, '::1' for udp6 sockets.)
         *
         * > An optional callback function may be specified as a way of reporting DNS
         * errors or for determining when it is safe to reuse the buf object. DNS
         * lookups delay the time to send for at least one tick of the Node.js event
         * loop.
         *
         * > The only way to know for sure that the datagram has been sent is by using a
         * callback. If an error occurs and a callback is given, the error will be
         * passed as the first argument to the callback. If a callback is not given,
         * the error is emitted as an 'error' event on the socket object.
         *
         * > Offset and length are optional but both must be set if either is used.
         * They are supported only when the first argument is a Buffer, a TypedArray,
         * or a DataView.
         *
         * @param {Buffer | TypedArray | DataView | string | Array} msg - Message to be sent.
         * @param {integer=} offset - Offset in the buffer where the message starts.
         * @param {integer=} length - Number of bytes in the message.
         * @param {integer=} port - Destination port.
         * @param {string=} address - Destination host name or IP address.
         * @param {Function=} callback - Called when the message has been sent.
         * @see {@link https://nodejs.org/api/dgram.html#socketsendmsg-offset-length-port-address-callback}
         */
        send(buffer: any, ...args: any[]): Promise<any>;
        /**
         * Close the underlying socket and stop listening for data on it. If a
         * callback is provided, it is added as a listener for the 'close' event.
         *
         * @param {function=} callback - Called when the connection is completed or on error.
         *
         * @see {@link https://nodejs.org/api/dgram.html#socketclosecallback}
         */
        close(cb: any): this;
        /**
         *
         * Returns an object containing the address information for a socket. For
         * UDP sockets, this object will contain address, family, and port properties.
         *
         * This method throws EBADF if called on an unbound socket.
         * @returns {Object} socketInfo - Information about the local socket
         * @returns {string} socketInfo.address - The IP address of the socket
         * @returns {string} socketInfo.port - The port of the socket
         * @returns {string} socketInfo.family - The IP family of the socket
         *
         * @see {@link https://nodejs.org/api/dgram.html#socketaddress}
         */
        address(): any;
        /**
         * Returns an object containing the address, family, and port of the remote
         * endpoint. This method throws an ERR_SOCKET_DGRAM_NOT_CONNECTED exception
         * if the socket is not connected.
         *
         * @returns {Object} socketInfo - Information about the remote socket
         * @returns {string} socketInfo.address - The IP address of the socket
         * @returns {string} socketInfo.port - The port of the socket
         * @returns {string} socketInfo.family - The IP family of the socket
         * @see {@link https://nodejs.org/api/dgram.html#socketremoteaddress}
         */
        remoteAddress(): any;
        /**
         * Sets the SO_RCVBUF socket option. Sets the maximum socket receive buffer in
         * bytes.
         *
         * @param {number} size - The size of the new receive buffer
         * @see {@link https://nodejs.org/api/dgram.html#socketsetrecvbuffersizesize}
         */
        setRecvBufferSize(size: number): Promise<void>;
        /**
         * Sets the SO_SNDBUF socket option. Sets the maximum socket send buffer in
         * bytes.
         *
         * @param {number} size - The size of the new send buffer
         * @see {@link https://nodejs.org/api/dgram.html#socketsetsendbuffersizesize}
         */
        setSendBufferSize(size: number): Promise<void>;
        /**
         * @see {@link https://nodejs.org/api/dgram.html#socketgetrecvbuffersize}
         */
        getRecvBufferSize(): any;
        /**
         * @returns {number} the SO_SNDBUF socket send buffer size in bytes.
         * @see {@link https://nodejs.org/api/dgram.html#socketgetsendbuffersize}
         */
        getSendBufferSize(): number;
        setBroadcast(): void;
        setTTL(): void;
        setMulticastTTL(): void;
        setMulticastLoopback(): void;
        setMulticastMembership(): void;
        setMulticastInterface(): void;
        addMembership(): void;
        dropMembership(): void;
        addSourceSpecificMembership(): void;
        dropSourceSpecificMembership(): void;
        ref(): this;
        unref(): this;
        #private;
    }
    /**
     * Generic error class for an error occurring on a `Socket` instance.
     * @ignore
     */
    export class SocketError extends InternalError {
        /**
         * @type {string}
         */
        get code(): string;
    }
    /**
     * Thrown when a socket is already bound.
     */
    export class ERR_SOCKET_ALREADY_BOUND extends exports.SocketError {
        get message(): string;
    }
    /**
     * @ignore
     */
    export class ERR_SOCKET_BAD_BUFFER_SIZE extends exports.SocketError {
    }
    /**
     * @ignore
     */
    export class ERR_SOCKET_BUFFER_SIZE extends exports.SocketError {
    }
    /**
     * Thrown when the socket is already connected.
     */
    export class ERR_SOCKET_DGRAM_IS_CONNECTED extends exports.SocketError {
        get message(): string;
    }
    /**
     * Thrown when the socket is not connected.
     */
    export class ERR_SOCKET_DGRAM_NOT_CONNECTED extends exports.SocketError {
        syscall: string;
        get message(): string;
    }
    /**
     * Thrown when the socket is not running (not bound or connected).
     */
    export class ERR_SOCKET_DGRAM_NOT_RUNNING extends exports.SocketError {
        get message(): string;
    }
    /**
     * Thrown when a bad socket type is used in an argument.
     */
    export class ERR_SOCKET_BAD_TYPE extends TypeError {
        code: string;
        get message(): string;
    }
    /**
     * Thrown when a bad port is given.
     */
    export class ERR_SOCKET_BAD_PORT extends RangeError {
        code: string;
    }
    export default exports;
    export type SocketOptions = any;
    import { EventEmitter } from "socket:events";
    import { Conduit } from "socket:internal/conduit";
    import { InternalError } from "socket:errors";
    import * as exports from "socket:dgram";
    
}

declare module "socket:fs/web" {
    /**
     * Creates a new `File` instance from `filename`.
     * @param {string} filename
     * @param {{ fd: fs.FileHandle, highWaterMark?: number }=} [options]
     * @return {File}
     */
    export function createFile(filename: string, options?: {
        fd: fs.FileHandle;
        highWaterMark?: number;
    } | undefined): File;
    /**
     * Creates a `FileSystemWritableFileStream` instance backed
     * by `socket:fs:` module from a given `FileSystemFileHandle` instance.
     * @param {string|File} file
     * @return {Promise<FileSystemFileHandle>}
     */
    export function createFileSystemWritableFileStream(handle: any, options: any): Promise<FileSystemFileHandle>;
    /**
     * Creates a `FileSystemFileHandle` instance backed by `socket:fs:` module from
     * a given `File` instance or filename string.
     * @param {string|File} file
     * @param {object} [options]
     * @return {Promise<FileSystemFileHandle>}
     */
    export function createFileSystemFileHandle(file: string | File, options?: object): Promise<FileSystemFileHandle>;
    /**
     * Creates a `FileSystemDirectoryHandle` instance backed by `socket:fs:` module
     * from a given directory name string.
     * @param {string} dirname
     * @return {Promise<FileSystemFileHandle>}
     */
    export function createFileSystemDirectoryHandle(dirname: string, options?: any): Promise<FileSystemFileHandle>;
    export const kFileSystemHandleFullName: unique symbol;
    export const kFileDescriptor: unique symbol;
    export const kFileFullName: unique symbol;
    export const File: {
        new (fileBits: BlobPart[], fileName: string, options?: FilePropertyBag): File;
        prototype: File;
    } | {
        new (): {
            readonly lastModifiedDate: Date;
            readonly lastModified: number;
            readonly name: any;
            readonly size: number;
            readonly type: string;
            slice(): void;
            arrayBuffer(): Promise<void>;
            bytes(): Promise<void>;
            text(): Promise<void>;
            stream(): void;
        };
    };
    export const FileSystemHandle: {
        new (): {
            readonly name: any;
            readonly kind: any;
        };
    };
    export const FileSystemFileHandle: {
        new (): FileSystemFileHandle;
        prototype: FileSystemFileHandle;
    } | {
        new (): {
            getFile(): Promise<void>;
            createWritable(options?: any): Promise<void>;
            createSyncAccessHandle(): Promise<void>;
            readonly name: any;
            readonly kind: any;
        };
    };
    export const FileSystemDirectoryHandle: {
        new (): FileSystemDirectoryHandle;
        prototype: FileSystemDirectoryHandle;
    } | {
        new (): {
            entries(): AsyncGenerator<never, void, unknown>;
            values(): AsyncGenerator<never, void, unknown>;
            keys(): AsyncGenerator<never, void, unknown>;
            resolve(possibleDescendant: any): Promise<void>;
            removeEntry(name: any, options?: any): Promise<void>;
            getDirectoryHandle(name: any, options?: any): Promise<void>;
            getFileHandle(name: any, options?: any): Promise<void>;
            readonly name: any;
            readonly kind: any;
        };
    };
    export const FileSystemWritableFileStream: {
        new (underlyingSink?: UnderlyingSink<any>, strategy?: QueuingStrategy<any>): {
            seek(position: any): Promise<void>;
            truncate(size: any): Promise<void>;
            write(data: any): Promise<void>;
            readonly locked: boolean;
            abort(reason?: any): Promise<void>;
            close(): Promise<void>;
            getWriter(): WritableStreamDefaultWriter<any>;
        };
    };
    namespace _default {
        export { createFileSystemWritableFileStream };
        export { createFileSystemDirectoryHandle };
        export { createFileSystemFileHandle };
        export { createFile };
    }
    export default _default;
    import fs from "socket:fs/promises";
}

declare module "socket:extension" {
    /**
     * Load an extension by name.
     * @template {Record<string, any> T}
     * @param {string} name
     * @param {ExtensionLoadOptions} [options]
     * @return {Promise<Extension<T>>}
     */
    export function load<T extends Record<string, any>>(name: string, options?: ExtensionLoadOptions): Promise<Extension<T>>;
    /**
     * Provides current stats about the loaded extensions.
     * @return {Promise<ExtensionStats>}
     */
    export function stats(): Promise<ExtensionStats>;
    /**
     * @typedef {{
     *   allow: string[] | string,
     *   imports?: object,
     *   type?: 'shared' | 'wasm32',
     *   path?: string,
     *   stats?: object,
     *   instance?: WebAssembly.Instance,
     *   adapter?: WebAssemblyExtensionAdapter
     * }} ExtensionLoadOptions
     */
    /**
     * @typedef {{ abi: number, version: string, description: string }} ExtensionInfo
     */
    /**
     * @typedef {{ abi: number, loaded: number }} ExtensionStats
     */
    /**
     * A interface for a native extension.
     * @template {Record<string, any> T}
     */
    export class Extension<T extends Record<string, any>> extends EventTarget {
        /**
         * Load an extension by name.
         * @template {Record<string, any> T}
         * @param {string} name
         * @param {ExtensionLoadOptions} [options]
         * @return {Promise<Extension<T>>}
         */
        static load<T_1 extends Record<string, any>>(name: string, options?: ExtensionLoadOptions): Promise<Extension<T_1>>;
        /**
         * Query type of extension by name.
         * @param {string} name
         * @return {Promise<'shared'|'wasm32'|'unknown'|null>}
         */
        static type(name: string): Promise<"shared" | "wasm32" | "unknown" | null>;
        /**
         * Provides current stats about the loaded extensions or one by name.
         * @param {?string} name
         * @return {Promise<ExtensionStats|null>}
         */
        static stats(name: string | null): Promise<ExtensionStats | null>;
        /**
         * `Extension` class constructor.
         * @param {string} name
         * @param {ExtensionInfo} info
         * @param {ExtensionLoadOptions} [options]
         */
        constructor(name: string, info: ExtensionInfo, options?: ExtensionLoadOptions);
        /**
         * The name of the extension
         * @type {string?}
         */
        name: string | null;
        /**
         * The version of the extension
         * @type {string?}
         */
        version: string | null;
        /**
         * The description of the extension
         * @type {string?}
         */
        description: string | null;
        /**
         * The abi of the extension
         * @type {number}
         */
        abi: number;
        /**
         * @type {object}
         */
        options: object;
        /**
         * @type {T}
         */
        binding: T;
        /**
         * Not `null` if extension is of type 'wasm32'
         * @type {?WebAssemblyExtensionAdapter}
         */
        adapter: WebAssemblyExtensionAdapter | null;
        /**
         * `true` if the extension was loaded, otherwise `false`
         * @type {boolean}
         */
        get loaded(): boolean;
        /**
         * The extension type: 'shared' or  'wasm32'
         * @type {'shared'|'wasm32'}
         */
        get type(): "shared" | "wasm32";
        /**
         * Unloads the loaded extension.
         * @throws Error
         */
        unload(): Promise<boolean>;
        instance: any;
        [$type]: "shared" | "wasm32";
        [$loaded]: boolean;
    }
    namespace _default {
        export { load };
        export { stats };
    }
    export default _default;
    export type Pointer = number;
    export type ExtensionLoadOptions = {
        allow: string[] | string;
        imports?: object;
        type?: "shared" | "wasm32";
        path?: string;
        stats?: object;
        instance?: WebAssembly.Instance;
        adapter?: WebAssemblyExtensionAdapter;
    };
    export type ExtensionInfo = {
        abi: number;
        version: string;
        description: string;
    };
    export type ExtensionStats = {
        abi: number;
        loaded: number;
    };
    /**
     * An adapter for reading and writing various values from a WebAssembly instance's
     * memory buffer.
     * @ignore
     */
    class WebAssemblyExtensionAdapter {
        constructor({ instance, module, table, memory, policies }: {
            instance: any;
            module: any;
            table: any;
            memory: any;
            policies: any;
        });
        view: any;
        heap: any;
        table: any;
        stack: any;
        buffer: any;
        module: any;
        memory: any;
        context: any;
        policies: any[];
        externalReferences: Map<any, any>;
        instance: any;
        exitStatus: any;
        textDecoder: TextDecoder;
        textEncoder: TextEncoder;
        errorMessagePointers: {};
        indirectFunctionTable: any;
        get globalBaseOffset(): any;
        destroy(): void;
        init(): boolean;
        get(pointer: any, size?: number): any;
        set(pointer: any, value: any): void;
        createExternalReferenceValue(value: any): any;
        getExternalReferenceValue(pointer: any): any;
        setExternalReferenceValue(pointer: any, value: any): Map<any, any>;
        removeExternalReferenceValue(pointer: any): void;
        getExternalReferencePointer(value: any): any;
        getFloat32(pointer: any): any;
        setFloat32(pointer: any, value: any): boolean;
        getFloat64(pointer: any): any;
        setFloat64(pointer: any, value: any): boolean;
        getInt8(pointer: any): any;
        setInt8(pointer: any, value: any): boolean;
        getInt16(pointer: any): any;
        setInt16(pointer: any, value: any): boolean;
        getInt32(pointer: any): any;
        setInt32(pointer: any, value: any): boolean;
        getUint8(pointer: any): any;
        setUint8(pointer: any, value: any): boolean;
        getUint16(pointer: any): any;
        setUint16(pointer: any, value: any): boolean;
        getUint32(pointer: any): any;
        setUint32(pointer: any, value: any): boolean;
        getString(pointer: any, buffer: any, size: any): string;
        setString(pointer: any, string: any, buffer?: any): boolean;
    }
    const $type: unique symbol;
    /**
     * @typedef {number} Pointer
     */
    const $loaded: unique symbol;
}

declare module "socket:internal/database" {
    /**
     * A typed container for optional options given to the `Database`
     * class constructor.
     *
     * @typedef {{
     *   version?: string | undefined
     * }} DatabaseOptions
     */
    /**
     * A typed container for various optional options made to a `get()` function
     * on a `Database` instance.
     *
     * @typedef {{
     *   store?: string | undefined,
     *   stores?: string[] | undefined,
     *   count?: number | undefined
     * }} DatabaseGetOptions
     */
    /**
     * A typed container for various optional options made to a `put()` function
     * on a `Database` instance.
     *
     * @typedef {{
     *   store?: string | undefined,
     *   stores?: string[] | undefined,
     *   durability?: 'strict' | 'relaxed' | undefined
     * }} DatabasePutOptions
     */
    /**
     * A typed container for various optional options made to a `delete()` function
     * on a `Database` instance.
     *
     * @typedef {{
     *   store?: string | undefined,
     *   stores?: string[] | undefined
     * }} DatabaseDeleteOptions
     */
    /**
     * A typed container for optional options given to the `Database`
     * class constructor.
     *
     * @typedef {{
     *   offset?: number | undefined,
     *   backlog?: number | undefined
     * }} DatabaseRequestQueueWaitOptions
     */
    /**
     * A typed container for various optional options made to a `entries()` function
     * on a `Database` instance.
     *
     * @typedef {{
     *   store?: string | undefined,
     *   stores?: string[] | undefined
     * }} DatabaseEntriesOptions
     */
    /**
     * A `DatabaseRequestQueueRequestConflict` callback function type.
     * @typedef {function(Event, DatabaseRequestQueueRequestConflict): any} DatabaseRequestQueueConflictResolutionCallback
     */
    /**
     * Waits for an event of `eventType` to be dispatched on a given `EventTarget`.
     * @param {EventTarget} target
     * @param {string} eventType
     * @return {Promise<Event>}
     */
    export function waitFor(target: EventTarget, eventType: string): Promise<Event>;
    /**
     * Creates an opens a named `Database` instance.
     * @param {string} name
     * @param {?DatabaseOptions | undefined} [options]
     * @return {Promise<Database>}
     */
    export function open(name: string, options?: (DatabaseOptions | undefined) | null): Promise<Database>;
    /**
     * Complete deletes a named `Database` instance.
     * @param {string} name
     * @param {?DatabaseOptions|undefined} [options]
     */
    export function drop(name: string, options?: (DatabaseOptions | undefined) | null): Promise<void>;
    /**
     * A mapping of named `Database` instances that are currently opened
     * @type {Map<string, WeakRef<Database>>}
     */
    export const opened: Map<string, WeakRef<Database>>;
    /**
     * A container for conflict resolution for a `DatabaseRequestQueue` instance
     * `IDBRequest` instance.
     */
    export class DatabaseRequestQueueRequestConflict {
        /**
         * `DatabaseRequestQueueRequestConflict` class constructor
         * @param {function(any): void)} resolve
         * @param {function(Error): void)} reject
         * @param {function(): void)} cleanup
         */
        constructor(resolve: any, reject: any, cleanup: any);
        /**
         * Called when a conflict is resolved.
         * @param {any} argument
         */
        resolve(argument?: any): void;
        /**
         * Called when a conflict is rejected
         * @param {Error} error
         */
        reject(error: Error): void;
        #private;
    }
    /**
     * An event dispatched on a `DatabaseRequestQueue`
     */
    export class DatabaseRequestQueueEvent extends Event {
        /**
         * `DatabaseRequestQueueEvent` class constructor.
         * @param {string} type
         * @param {IDBRequest|IDBTransaction} request
         */
        constructor(type: string, request: IDBRequest | IDBTransaction);
        /**
         * A reference to the underlying request for this event.
         * @type {IDBRequest|IDBTransaction}
         */
        get request(): IDBRequest<any> | IDBTransaction;
        #private;
    }
    /**
     * An event dispatched on a `Database`
     */
    export class DatabaseEvent extends Event {
        /**
         * `DatabaseEvent` class constructor.
         * @param {string} type
         * @param {Database} database
         */
        constructor(type: string, database: Database);
        /**
         * A reference to the underlying database for this event.
         * @type {Database}
         */
        get database(): Database;
        #private;
    }
    /**
     * An error event dispatched on a `DatabaseRequestQueue`
     */
    export class DatabaseRequestQueueErrorEvent extends ErrorEvent {
        /**
         * `DatabaseRequestQueueErrorEvent` class constructor.
         * @param {string} type
         * @param {IDBRequest|IDBTransaction} request
         * @param {{ error: Error, cause?: Error }} options
         */
        constructor(type: string, request: IDBRequest | IDBTransaction, options: {
            error: Error;
            cause?: Error;
        });
        /**
         * A reference to the underlying request for this error event.
         * @type {IDBRequest|IDBTransaction}
         */
        get request(): IDBRequest<any> | IDBTransaction;
        #private;
    }
    /**
     * A container for various `IDBRequest` and `IDBTransaction` instances
     * occurring during the life cycles of a `Database` instance.
     */
    export class DatabaseRequestQueue extends EventTarget {
        /**
         * Computed queue length
         * @type {number}
         */
        get length(): number;
        /**
         * Pushes an `IDBRequest` or `IDBTransaction onto the queue and returns a
         * `Promise` that resolves upon a 'success' or 'complete' event and rejects
         * upon an error' event.
         * @param {IDBRequest|IDBTransaction}
         * @param {?DatabaseRequestQueueConflictResolutionCallback} [conflictResolutionCallback]
         * @return {Promise}
         */
        push(request: any, conflictResolutionCallback?: DatabaseRequestQueueConflictResolutionCallback | null): Promise<any>;
        /**
         * Waits for all pending requests to complete. This function will throw when
         * an `IDBRequest` or `IDBTransaction` instance emits an 'error' event.
         * Callers of this function can optionally specify a maximum backlog to wait
         * for instead of waiting for all requests to finish.
         * @param {?DatabaseRequestQueueWaitOptions | undefined} [options]
         */
        wait(options?: (DatabaseRequestQueueWaitOptions | undefined) | null): Promise<any[]>;
        #private;
    }
    /**
     * An interface for reading from named databases backed by IndexedDB.
     */
    export class Database extends EventTarget {
        /**
         * `Database` class constructor.
         * @param {string} name
         * @param {?DatabaseOptions | undefined} [options]
         */
        constructor(name: string, options?: (DatabaseOptions | undefined) | null);
        /**
         * `true` if the `Database` is currently opening, otherwise `false`.
         * A `Database` instance should not attempt to be opened if this property value
         * is `true`.
         * @type {boolean}
         */
        get opening(): boolean;
        /**
         * `true` if the `Database` instance was successfully opened such that the
         * internal `IDBDatabase` storage instance was created and can be referenced
         * on the `Database` instance, otherwise `false`.
         * @type {boolean}
         */
        get opened(): boolean;
        /**
         * `true` if the `Database` instance was closed or has not been opened such
         * that the internal `IDBDatabase` storage instance was not created or cannot
         * be referenced on the `Database` instance, otherwise `false`.
         * @type {boolean}
         */
        get closed(): boolean;
        /**
         * `true` if the `Database` is currently closing, otherwise `false`.
         * A `Database` instance should not attempt to be closed if this property value
         * is `true`.
         * @type {boolean}
         */
        get closing(): boolean;
        /**
         * The name of the `IDBDatabase` database. This value cannot be `null`.
         * @type {string}
         */
        get name(): string;
        /**
         * The version of the `IDBDatabase` database. This value may be `null`.
         * @type {?string}
         */
        get version(): string;
        /**
         * A reference to the `IDBDatabase`, if the `Database` instance was opened.
         * This value may ba `null`.
         * @type {?IDBDatabase}
         */
        get storage(): IDBDatabase;
        /**
         * Opens the `IDBDatabase` database optionally at a specific "version" if
         * one was given upon construction of the `Database` instance. This function
         * is not idempotent and will throw if the underlying `IDBDatabase` instance
         * was created successfully or is in the process of opening.
         * @return {Promise}
         */
        open(): Promise<any>;
        /**
         * Closes the `IDBDatabase` database storage, if opened. This function is not
         * idempotent and will throw if the underlying `IDBDatabase` instance is
         * already closed (not opened) or currently closing.
         * @return {Promise}
         */
        close(): Promise<any>;
        /**
         * Deletes entire `Database` instance and closes after successfully
         * delete storage.
         */
        drop(): Promise<void>;
        /**
         * Gets a "readonly" value by `key` in the `Database` object storage.
         * @param {string} key
         * @param {?DatabaseGetOptions|undefined} [options]
         * @return {Promise<object|object[]|null>}
         */
        get(key: string, options?: (DatabaseGetOptions | undefined) | null): Promise<object | object[] | null>;
        /**
         * Put a `value` at `key`, updating if it already exists, otherwise
         * "inserting" it into the `Database` instance.
         * @param {string} key
         * @param {any} value
         * @param {?DatabasePutOptions|undefined} [options]
         * @return {Promise}
         */
        put(key: string, value: any, options?: (DatabasePutOptions | undefined) | null): Promise<any>;
        /**
         * Inserts a new `value` at `key`. This function throws if a value at `key`
         * already exists.
         * @param {string} key
         * @param {any} value
         * @param {?DatabasePutOptions|undefined} [options]
         * @return {Promise}
         */
        insert(key: string, value: any, options?: (DatabasePutOptions | undefined) | null): Promise<any>;
        /**
         * Update a `value` at `key`, updating if it already exists, otherwise
         * "inserting" it into the `Database` instance.
         * @param {string} key
         * @param {any} value
         * @param {?DatabasePutOptions|undefined} [options]
         * @return {Promise}
         */
        update(key: string, value: any, options?: (DatabasePutOptions | undefined) | null): Promise<any>;
        /**
         * Delete a value at `key`.
         * @param {string} key
         * @param {?DatabaseDeleteOptions|undefined} [options]
         * @return {Promise}
         */
        delete(key: string, options?: (DatabaseDeleteOptions | undefined) | null): Promise<any>;
        /**
         * Gets a "readonly" value by `key` in the `Database` object storage.
         * @param {string} key
         * @param {?DatabaseEntriesOptions|undefined} [options]
         * @return {Promise<object|object[]|null>}
         */
        entries(options?: (DatabaseEntriesOptions | undefined) | null): Promise<object | object[] | null>;
        #private;
    }
    namespace _default {
        export { Database };
        export { open };
        export { drop };
    }
    export default _default;
    /**
     * A typed container for optional options given to the `Database`
     * class constructor.
     */
    export type DatabaseOptions = {
        version?: string | undefined;
    };
    /**
     * A typed container for various optional options made to a `get()` function
     * on a `Database` instance.
     */
    export type DatabaseGetOptions = {
        store?: string | undefined;
        stores?: string[] | undefined;
        count?: number | undefined;
    };
    /**
     * A typed container for various optional options made to a `put()` function
     * on a `Database` instance.
     */
    export type DatabasePutOptions = {
        store?: string | undefined;
        stores?: string[] | undefined;
        durability?: "strict" | "relaxed" | undefined;
    };
    /**
     * A typed container for various optional options made to a `delete()` function
     * on a `Database` instance.
     */
    export type DatabaseDeleteOptions = {
        store?: string | undefined;
        stores?: string[] | undefined;
    };
    /**
     * A typed container for optional options given to the `Database`
     * class constructor.
     */
    export type DatabaseRequestQueueWaitOptions = {
        offset?: number | undefined;
        backlog?: number | undefined;
    };
    /**
     * A typed container for various optional options made to a `entries()` function
     * on a `Database` instance.
     */
    export type DatabaseEntriesOptions = {
        store?: string | undefined;
        stores?: string[] | undefined;
    };
    /**
     * A `DatabaseRequestQueueRequestConflict` callback function type.
     */
    export type DatabaseRequestQueueConflictResolutionCallback = (arg0: Event, arg1: DatabaseRequestQueueRequestConflict) => any;
}

declare module "socket:service-worker/env" {
    /**
     * Opens an environment for a particular scope.
     * @param {EnvironmentOptions} options
     * @return {Promise<Environment>}
     */
    export function open(options: EnvironmentOptions): Promise<Environment>;
    /**
     * Closes an active `Environment` instance, dropping the global
     * instance reference.
     * @return {Promise<boolean>}
     */
    export function close(): Promise<boolean>;
    /**
     * Resets an active `Environment` instance
     * @return {Promise<boolean>}
     */
    export function reset(): Promise<boolean>;
    /**
     * @typedef {{
     *   scope: string
     * }} EnvironmentOptions
     */
    /**
     * An event dispatched when a environment value is updated (set, delete)
     */
    export class EnvironmentEvent extends Event {
        /**
         * `EnvironmentEvent` class constructor.
         * @param {'set'|'delete'} type
         * @param {object=} [entry]
         */
        constructor(type: "set" | "delete", entry?: object | undefined);
        entry: any;
    }
    /**
     * An environment context object with persistence and durability
     * for service worker environments.
     */
    export class Environment extends EventTarget {
        /**
         * Maximum entries that will be restored from storage into the environment
         * context object.
         * @type {number}
         */
        static MAX_CONTEXT_ENTRIES: number;
        /**
         * Opens an environment for a particular scope.
         * @param {EnvironmentOptions} options
         * @return {Environment}
         */
        static open(options: EnvironmentOptions): Environment;
        /**
         * The current `Environment` instance
         * @type {Environment?}
         */
        static instance: Environment | null;
        /**
         * `Environment` class constructor
         * @ignore
         * @param {EnvironmentOptions} options
         */
        constructor(options: EnvironmentOptions);
        /**
         * A reference to the currently opened environment database.
         * @type {import('../internal/database.js').Database}
         */
        get database(): import("socket:internal/database").Database;
        /**
         * A proxied object for reading and writing environment state.
         * Values written to this object must be cloneable with respect to the
         * structured clone algorithm.
         * @see {https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm}
         * @type {Proxy<object>}
         */
        get context(): ProxyConstructor;
        /**
         * The environment type
         * @type {string}
         */
        get type(): string;
        /**
         * The current environment name. This value is also used as the
         * internal database name.
         * @type {string}
         */
        get name(): string;
        /**
         * Resets the current environment to an empty state.
         */
        reset(): Promise<void>;
        /**
         * Opens the environment.
         * @ignore
         */
        open(): Promise<void>;
        /**
         * Closes the environment database, purging existing state.
         * @ignore
         */
        close(): Promise<void>;
        #private;
    }
    namespace _default {
        export { Environment };
        export { close };
        export { reset };
        export { open };
    }
    export default _default;
    export type EnvironmentOptions = {
        scope: string;
    };
}

declare module "socket:service-worker/debug" {
    export function debug(...args: any[]): void;
    export default debug;
}

declare module "socket:service-worker/state" {
    export const channel: BroadcastChannel;
    export const state: any;
    export default state;
}

declare module "socket:service-worker/clients" {
    export class Client {
        constructor(options: any);
        get id(): any;
        get url(): any;
        get type(): any;
        get frameType(): any;
        postMessage(message: any, optionsOrTransferables?: any): void;
        #private;
    }
    export class WindowClient extends Client {
        get focused(): boolean;
        get ancestorOrigins(): any[];
        get visibilityState(): string;
        focus(): Promise<this>;
        navigate(url: any): Promise<this>;
        #private;
    }
    export class Clients {
        get(id: any): Promise<Client>;
        matchAll(options?: any): Promise<any>;
        openWindow(url: any, options?: any): Promise<WindowClient>;
        claim(): Promise<void>;
    }
    const _default: Clients;
    export default _default;
}

declare module "socket:service-worker/context" {
    /**
     * A context given to `ExtendableEvent` interfaces and provided to
     * simplified service worker modules
     */
    export class Context {
        /**
         * `Context` class constructor.
         * @param {import('./events.js').ExtendableEvent} event
         */
        constructor(event: import("socket:service-worker/events").ExtendableEvent);
        /**
         * Context data. This may be a custom protocol handler scheme data
         * by default, if available.
         * @type {any?}
         */
        data: any | null;
        /**
         * The `ExtendableEvent` for this `Context` instance.
         * @type {ExtendableEvent}
         */
        get event(): ExtendableEvent;
        /**
         * An environment context object.
         * @type {object?}
         */
        get env(): any;
        /**
         * Resets the current environment context.
         * @return {Promise<boolean>}
         */
        resetEnvironment(): Promise<boolean>;
        /**
         * Unused, but exists for cloudflare compat.
         * @ignore
         */
        passThroughOnException(): void;
        /**
         * Tells the event dispatcher that work is ongoing.
         * It can also be used to detect whether that work was successful.
         * @param {Promise} promise
         */
        waitUntil(promise: Promise<any>): Promise<any>;
        /**
         * TODO
         */
        handled(): Promise<any>;
        /**
         * Gets the client for this event context.
         * @return {Promise<import('./clients.js').Client>}
         */
        client(): Promise<import("socket:service-worker/clients").Client>;
        #private;
    }
    namespace _default {
        export { Context };
    }
    export default _default;
}

declare module "socket:service-worker/events" {
    export const textEncoder: TextEncoderStream;
    export const FETCH_EVENT_TIMEOUT: number;
    export const FETCH_EVENT_MAX_RESPONSE_REDIRECTS: number;
    /**
     * The `ExtendableEvent` interface extends the lifetime of the "install" and
     * "activate" events dispatched on the global scope as part of the service
     * worker lifecycle.
     */
    export class ExtendableEvent extends Event {
        /**
         * `ExtendableEvent` class constructor.
         * @ignore
         */
        constructor(...args: any[]);
        /**
         * A context for this `ExtendableEvent` instance.
         * @type {import('./context.js').Context}
         */
        get context(): Context;
        /**
         * A promise that can be awaited which waits for this `ExtendableEvent`
         * instance no longer has pending promises.
         * @type {Promise}
         */
        get awaiting(): Promise<any>;
        /**
         * The number of pending promises
         * @type {number}
         */
        get pendingPromises(): number;
        /**
         * `true` if the `ExtendableEvent` instance is considered "active",
         * otherwise `false`.
         * @type {boolean}
         */
        get isActive(): boolean;
        /**
         * Tells the event dispatcher that work is ongoing.
         * It can also be used to detect whether that work was successful.
         * @param {Promise} promise
         */
        waitUntil(promise: Promise<any>): void;
        /**
         * Returns a promise that this `ExtendableEvent` instance is waiting for.
         * @return {Promise}
         */
        waitsFor(): Promise<any>;
        #private;
    }
    /**
     * This is the event type for "fetch" events dispatched on the service worker
     * global scope. It contains information about the fetch, including the
     * request and how the receiver will treat the response.
     */
    export class FetchEvent extends ExtendableEvent {
        static defaultHeaders: Headers;
        /**
         * `FetchEvent` class constructor.
         * @ignore
         * @param {string=} [type = 'fetch']
         * @param {object=} [options]
         */
        constructor(type?: string | undefined, options?: object | undefined);
        /**
         * The handled property of the `FetchEvent` interface returns a promise
         * indicating if the event has been handled by the fetch algorithm or not.
         * This property allows executing code after the browser has consumed a
         * response, and is usually used together with the `waitUntil()` method.
         * @type {Promise}
         */
        get handled(): Promise<any>;
        /**
         * The request read-only property of the `FetchEvent` interface returns the
         * `Request` that triggered the event handler.
         * @type {Request}
         */
        get request(): Request;
        /**
         * The `clientId` read-only property of the `FetchEvent` interface returns
         * the id of the Client that the current service worker is controlling.
         * @type {string}
         */
        get clientId(): string;
        /**
         * @ignore
         * @type {string}
         */
        get resultingClientId(): string;
        /**
         * @ignore
         * @type {string}
         */
        get replacesClientId(): string;
        /**
         * @ignore
         * @type {boolean}
         */
        get isReload(): boolean;
        /**
         * @ignore
         * @type {Promise}
         */
        get preloadResponse(): Promise<any>;
        /**
         * The `respondWith()` method of `FetchEvent` prevents the webview's
         * default fetch handling, and allows you to provide a promise for a
         * `Response` yourself.
         * @param {Response|Promise<Response>} response
         */
        respondWith(response: Response | Promise<Response>): void;
        #private;
    }
    export class ExtendableMessageEvent extends ExtendableEvent {
        /**
         * `ExtendableMessageEvent` class constructor.
         * @param {string=} [type = 'message']
         * @param {object=} [options]
         */
        constructor(type?: string | undefined, options?: object | undefined);
        /**
         * @type {any}
         */
        get data(): any;
        /**
         * @type {MessagePort[]}
         */
        get ports(): MessagePort[];
        /**
         * @type {import('./clients.js').Client?}
         */
        get source(): import("socket:service-worker/clients").Client;
        /**
         * @type {string?}
         */
        get origin(): string;
        /**
         * @type {string}
         */
        get lastEventId(): string;
        #private;
    }
    export class NotificationEvent extends ExtendableEvent {
        constructor(type: any, options: any);
        get action(): string;
        get notification(): any;
        #private;
    }
    namespace _default {
        export { ExtendableMessageEvent };
        export { ExtendableEvent };
        export { FetchEvent };
    }
    export default _default;
    import { Context } from "socket:service-worker/context";
}

declare module "socket:http/adapters" {
    /**
     * @typedef {{
     *   Connection: typeof import('../http.js').Connection,
     *   globalAgent: import('../http.js').Agent,
     *   IncomingMessage: typeof import('../http.js').IncomingMessage,
     *   ServerResponse: typeof import('../http.js').ServerResponse,
     *   STATUS_CODES: object,
     *   METHODS: string[]
     * }} HTTPModuleInterface
     */
    /**
     * An abstract base clase for a HTTP server adapter.
     */
    export class ServerAdapter extends EventTarget {
        /**
         * `ServerAdapter` class constructor.
         * @ignore
         * @param {import('../http.js').Server} server
         * @param {HTTPModuleInterface} httpInterface
         */
        constructor(server: import("socket:http").Server, httpInterface: HTTPModuleInterface);
        /**
         * A readonly reference to the underlying HTTP(S) server
         * for this adapter.
         * @type {import('../http.js').Server}
         */
        get server(): import("socket:http").Server;
        /**
         * A readonly reference to the underlying HTTP(S) module interface
         * for creating various HTTP module class objects.
         * @type {HTTPModuleInterface}
         */
        get httpInterface(): HTTPModuleInterface;
        /**
         * A readonly reference to the `AsyncContext.Variable` associated with this
         * `ServerAdapter` instance.
         */
        get context(): import("socket:async/context").Variable<any>;
        /**
         * Called when the adapter should destroy itself.
         * @abstract
         */
        destroy(): Promise<void>;
        #private;
    }
    /**
     * A HTTP adapter for running a HTTP server in a service worker that uses the
     * "fetch" event for the request and response lifecycle.
     */
    export class ServiceWorkerServerAdapter extends ServerAdapter {
        /**
         * Handles the 'install' service worker event.
         * @ignore
         * @param {import('../service-worker/events.js').ExtendableEvent} event
         */
        onInstall(event: import("socket:service-worker/events").ExtendableEvent): Promise<void>;
        /**
         * Handles the 'activate' service worker event.
         * @ignore
         * @param {import('../service-worker/events.js').ExtendableEvent} event
         */
        onActivate(event: import("socket:service-worker/events").ExtendableEvent): Promise<void>;
        /**
         * Handles the 'fetch' service worker event.
         * @ignore
         * @param {import('../service-worker/events.js').FetchEvent}
         */
        onFetch(event: any): Promise<void>;
    }
    namespace _default {
        export { ServerAdapter };
        export { ServiceWorkerServerAdapter };
    }
    export default _default;
    export type HTTPModuleInterface = {
        Connection: typeof import("socket:http").Connection;
        globalAgent: import("socket:http").Agent;
        IncomingMessage: typeof import("socket:http").IncomingMessage;
        ServerResponse: typeof import("socket:http").ServerResponse;
        STATUS_CODES: object;
        METHODS: string[];
    };
}

declare module "socket:http" {
    /**
     * Makes a HTTP or `socket:` GET request. A simplified alias to `request()`.
     * @param {string|object} optionsOrURL
     * @param {(object|function)=} [options]
     * @param {function=} [callback]
     * @return {ClientRequest}
     */
    export function get(optionsOrURL: string | object, options?: (object | Function) | undefined, callback?: Function | undefined): ClientRequest;
    /**
     * Creates a HTTP server that can listen for incoming requests.
     * Requests that are dispatched to this server depend on the context
     * in which it is created, such as a service worker which will use a
     * "fetch event" adapter.
     * @param {object|function=} [options]
     * @param {function=} [callback]
     * @return {Server}
     */
    export function createServer(options?: (object | Function) | undefined, callback?: Function | undefined): Server;
    /**
     * All known possible HTTP methods.
     * @type {string[]}
     */
    export const METHODS: string[];
    /**
     * A mapping of status codes to status texts
     * @type {object}
     */
    export const STATUS_CODES: object;
    export const CONTINUE: 100;
    export const SWITCHING_PROTOCOLS: 101;
    export const PROCESSING: 102;
    export const EARLY_HINTS: 103;
    export const OK: 200;
    export const CREATED: 201;
    export const ACCEPTED: 202;
    export const NONAUTHORITATIVE_INFORMATION: 203;
    export const NO_CONTENT: 204;
    export const RESET_CONTENT: 205;
    export const PARTIAL_CONTENT: 206;
    export const MULTISTATUS: 207;
    export const ALREADY_REPORTED: 208;
    export const IM_USED: 226;
    export const MULTIPLE_CHOICES: 300;
    export const MOVED_PERMANENTLY: 301;
    export const FOUND: 302;
    export const SEE_OTHER: 303;
    export const NOT_MODIFIED: 304;
    export const USE_PROXY: 305;
    export const TEMPORARY_REDIRECT: 307;
    export const PERMANENT_REDIRECT: 308;
    export const BAD_REQUEST: 400;
    export const UNAUTHORIZED: 401;
    export const PAYMENT_REQUIRED: 402;
    export const FORBIDDEN: 403;
    export const NOT_FOUND: 404;
    export const METHOD_NOT_ALLOWED: 405;
    export const NOT_ACCEPTABLE: 406;
    export const PROXY_AUTHENTICATION_REQUIRED: 407;
    export const REQUEST_TIMEOUT: 408;
    export const CONFLICT: 409;
    export const GONE: 410;
    export const LENGTH_REQUIRED: 411;
    export const PRECONDITION_FAILED: 412;
    export const PAYLOAD_TOO_LARGE: 413;
    export const URI_TOO_LONG: 414;
    export const UNSUPPORTED_MEDIA_TYPE: 415;
    export const RANGE_NOT_SATISFIABLE: 416;
    export const EXPECTATION_FAILED: 417;
    export const IM_A_TEAPOT: 418;
    export const MISDIRECTED_REQUEST: 421;
    export const UNPROCESSABLE_ENTITY: 422;
    export const LOCKED: 423;
    export const FAILED_DEPENDENCY: 424;
    export const TOO_EARLY: 425;
    export const UPGRADE_REQUIRED: 426;
    export const PRECONDITION_REQUIRED: 428;
    export const TOO_MANY_REQUESTS: 429;
    export const REQUEST_HEADER_FIELDS_TOO_LARGE: 431;
    export const UNAVAILABLE_FOR_LEGAL_REASONS: 451;
    export const INTERNAL_SERVER_ERROR: 500;
    export const NOT_IMPLEMENTED: 501;
    export const BAD_GATEWAY: 502;
    export const SERVICE_UNAVAILABLE: 503;
    export const GATEWAY_TIMEOUT: 504;
    export const HTTP_VERSION_NOT_SUPPORTED: 505;
    export const VARIANT_ALSO_NEGOTIATES: 506;
    export const INSUFFICIENT_STORAGE: 507;
    export const LOOP_DETECTED: 508;
    export const BANDWIDTH_LIMIT_EXCEEDED: 509;
    export const NOT_EXTENDED: 510;
    export const NETWORK_AUTHENTICATION_REQUIRED: 511;
    /**
     * The parent class of `ClientRequest` and `ServerResponse`.
     * It is an abstract outgoing message from the perspective of the
     * participants of an HTTP transaction.
     * @see {@link https://nodejs.org/api/http.html#class-httpoutgoingmessage}
     */
    export class OutgoingMessage extends Writable {
        /**
         * `OutgoingMessage` class constructor.
         * @ignore
         */
        constructor();
        /**
         * `true` if the headers were sent
         * @type {boolean}
         */
        headersSent: boolean;
        /**
         * Internal buffers
         * @ignore
         * @type {Buffer[]}
         */
        get buffers(): Buffer[];
        /**
         * An object of the outgoing message headers.
         * This is equivalent to `getHeaders()`
         * @type {object}
         */
        get headers(): any;
        /**
         * @ignore
         */
        get socket(): this;
        /**
         * `true` if the write state is "ended"
         * @type {boolean}
         */
        get writableEnded(): boolean;
        /**
         * `true` if the write state is "finished"
         * @type {boolean}
         */
        get writableFinished(): boolean;
        /**
         * The number of buffered bytes.
         * @type {number}
         */
        get writableLength(): number;
        /**
         * @ignore
         * @type {boolean}
         */
        get writableObjectMode(): boolean;
        /**
         * @ignore
         */
        get writableCorked(): number;
        /**
         * The `highWaterMark` of the writable stream.
         * @type {number}
         */
        get writableHighWaterMark(): number;
        /**
         * @ignore
         * @return {OutgoingMessage}
         */
        addTrailers(headers: any): OutgoingMessage;
        /**
         * @ignore
         * @return {OutgoingMessage}
         */
        cork(): OutgoingMessage;
        /**
         * @ignore
         * @return {OutgoingMessage}
         */
        uncork(): OutgoingMessage;
        /**
         * Destroys the message.
         * Once a socket is associated with the message and is connected,
         * that socket will be destroyed as well.
         * @param {Error?} [err]
         * @return {OutgoingMessage}
         */
        destroy(err?: Error | null): OutgoingMessage;
        /**
         * Finishes the outgoing message.
         * @param {(Buffer|Uint8Array|string|function)=} [chunk]
         * @param {(string|function)=} [encoding]
         * @param {function=} [callback]
         * @return {OutgoingMessage}
         */
        end(chunk?: (Buffer | Uint8Array | string | Function) | undefined, encoding?: (string | Function) | undefined, callback?: Function | undefined): OutgoingMessage;
        /**
         * Append a single header value for the header object.
         * @param {string} name
         * @param {string|string[]} value
         * @return {OutgoingMessage}
         */
        appendHeader(name: string, value: string | string[]): OutgoingMessage;
        /**
         * Append a single header value for the header object.
         * @param {string} name
         * @param {string} value
         * @return {OutgoingMessage}
         */
        setHeader(name: string, value: string): OutgoingMessage;
        /**
         * Flushes the message headers.
         */
        flushHeaders(): void;
        /**
         * Gets the value of the HTTP header with the given name.
         * If that header is not set, the returned value will be `undefined`.
         * @param {string}
         * @return {string|undefined}
         */
        getHeader(name: any): string | undefined;
        /**
         * Returns an array containing the unique names of the current outgoing
         * headers. All names are lowercase.
         * @return {string[]}
         */
        getHeaderNames(): string[];
        /**
         * @ignore
         */
        getRawHeaderNames(): string[];
        /**
         * Returns a copy of the HTTP headers as an object.
         * @return {object}
         */
        getHeaders(): object;
        /**
         * Returns true if the header identified by name is currently set in the
         * outgoing headers. The header name is case-insensitive.
         * @param {string} name
         * @return {boolean}
         */
        hasHeader(name: string): boolean;
        /**
         * Removes a header that is queued for implicit sending.
         * @param {string} name
         */
        removeHeader(name: string): void;
        /**
         * Sets the outgoing message timeout with an optional callback.
         * @param {number} timeout
         * @param {function=} [callback]
         * @return {OutgoingMessage}
         */
        setTimeout(timeout: number, callback?: Function | undefined): OutgoingMessage;
        /**
         * @ignore
         */
        _implicitHeader(): void;
        #private;
    }
    /**
     * An `IncomingMessage` object is created by `Server` or `ClientRequest` and
     * passed as the first argument to the 'request' and 'response' event
     * respectively.
     * It may be used to access response status, headers, and data.
     * @see {@link https://nodejs.org/api/http.html#class-httpincomingmessage}
     */
    export class IncomingMessage extends Readable {
        /**
         * `IncomingMessage` class constructor.
         * @ignore
         * @param {object} options
         */
        constructor(options: object);
        set url(url: string);
        /**
         * The URL for this incoming message. This value is not absolute with
         * respect to the protocol and hostname. It includes the path and search
         * query component parameters.
         * @type {string}
         */
        get url(): string;
        /**
         * @type {Server}
         */
        get server(): exports.Server;
        /**
         * @type {AsyncContext.Variable}
         */
        get context(): typeof import("socket:async/context").Variable;
        /**
         * This property will be `true` if a complete HTTP message has been received
         * and successfully parsed.
         * @type {boolean}
         */
        get complete(): boolean;
        /**
         * An object of the incoming message headers.
         * @type {object}
         */
        get headers(): any;
        /**
         * Similar to `message.headers`, but there is no join logic and the values
         * are always arrays of strings, even for headers received just once.
         * @type {object}
         */
        get headersDistinct(): any;
        /**
         * The HTTP major version of this request.
         * @type {number}
         */
        get httpVersionMajor(): number;
        /**
         * The HTTP minor version of this request.
         * @type {number}
         */
        get httpVersionMinor(): number;
        /**
         * The HTTP version string.
         * A concatenation of `httpVersionMajor` and `httpVersionMinor`.
         * @type {string}
         */
        get httpVersion(): string;
        /**
         * The HTTP request method.
         * @type {string}
         */
        get method(): string;
        /**
         * The raw request/response headers list potentially  as they were received.
         * @type {string[]}
         */
        get rawHeaders(): string[];
        /**
         * @ignore
         */
        get rawTrailers(): any[];
        /**
         * @ignore
         */
        get socket(): this;
        /**
         * The HTTP request status code.
         * Only valid for response obtained from `ClientRequest`.
         * @type {number}
         */
        get statusCode(): number;
        /**
         * The HTTP response status message (reason phrase).
         * Such as "OK" or "Internal Server Error."
         * Only valid for response obtained from `ClientRequest`.
         * @type {string?}
         */
        get statusMessage(): string;
        /**
         * An alias for `statusCode`
         * @type {number}
         */
        get status(): number;
        /**
         * An alias for `statusMessage`
         * @type {string?}
         */
        get statusText(): string;
        /**
         * @ignore
         */
        get trailers(): {};
        /**
         * @ignore
         */
        get trailersDistinct(): {};
        /**
         * Gets the value of the HTTP header with the given name.
         * If that header is not set, the returned value will be `undefined`.
         * @param {string}
         * @return {string|undefined}
         */
        getHeader(name: any): string | undefined;
        /**
         * Returns an array containing the unique names of the current outgoing
         * headers. All names are lowercase.
         * @return {string[]}
         */
        getHeaderNames(): string[];
        /**
         * @ignore
         */
        getRawHeaderNames(): string[];
        /**
         * Returns a copy of the HTTP headers as an object.
         * @return {object}
         */
        getHeaders(): object;
        /**
         * Returns true if the header identified by name is currently set in the
         * outgoing headers. The header name is case-insensitive.
         * @param {string} name
         * @return {boolean}
         */
        hasHeader(name: string): boolean;
        /**
         * Sets the incoming message timeout with an optional callback.
         * @param {number} timeout
         * @param {function=} [callback]
         * @return {IncomingMessage}
         */
        setTimeout(timeout: number, callback?: Function | undefined): IncomingMessage;
        #private;
    }
    /**
     * An object that is created internally and returned from `request()`.
     * @see {@link https://nodejs.org/api/http.html#class-httpclientrequest}
     */
    export class ClientRequest extends exports.OutgoingMessage {
        /**
         * `ClientRequest` class constructor.
         * @ignore
         * @param {object} options
         */
        constructor(options: object);
        /**
         * The HTTP request method.
         * @type {string}
         */
        get method(): string;
        /**
         * The request protocol
         * @type {string?}
         */
        get protocol(): string;
        /**
         * The request path.
         * @type {string}
         */
        get path(): string;
        /**
         * The request host name (including port).
         * @type {string?}
         */
        get host(): string;
        /**
         * The URL for this outgoing message. This value is not absolute with
         * respect to the protocol and hostname. It includes the path and search
         * query component parameters.
         * @type {string}
         */
        get url(): string;
        /**
         * @ignore
         * @type {boolean}
         */
        get finished(): boolean;
        /**
         * @ignore
         * @type {boolean}
         */
        get reusedSocket(): boolean;
        /**
         * @ignore
         * @param {boolean=} [value]
         * @return {ClientRequest}
         */
        setNoDelay(value?: boolean | undefined): ClientRequest;
        /**
         * @ignore
         * @param {boolean=} [enable]
         * @param {number=} [initialDelay]
         * @return {ClientRequest}
         */
        setSocketKeepAlive(enable?: boolean | undefined, initialDelay?: number | undefined): ClientRequest;
        #private;
    }
    /**
     * An object that is created internally by a `Server` instance, not by the user.
     * It is passed as the second parameter to the 'request' event.
     * @see {@link https://nodejs.org/api/http.html#class-httpserverresponse}
     */
    export class ServerResponse extends exports.OutgoingMessage {
        /**
         * `ServerResponse` class constructor.
         * @param {object} options
         */
        constructor(options: object);
        /**
         * @type {Server}
         */
        get server(): exports.Server;
        /**
         * A reference to the original HTTP request object.
         * @type {IncomingMessage}
         */
        get request(): exports.IncomingMessage;
        /**
         * A reference to the original HTTP request object.
         * @type {IncomingMessage}
         */
        get req(): exports.IncomingMessage;
        set statusCode(statusCode: number);
        /**
         * The HTTP request status code.
         * Only valid for response obtained from `ClientRequest`.
         * @type {number}
         */
        get statusCode(): number;
        set statusMessage(statusMessage: string);
        /**
         * The HTTP response status message (reason phrase).
         * Such as "OK" or "Internal Server Error."
         * Only valid for response obtained from `ClientRequest`.
         * @type {string?}
         */
        get statusMessage(): string;
        set status(status: number);
        /**
         * An alias for `statusCode`
         * @type {number}
         */
        get status(): number;
        set statusText(statusText: string);
        /**
         * An alias for `statusMessage`
         * @type {string?}
         */
        get statusText(): string;
        set sendDate(value: boolean);
        /**
         * If `true`, the "Date" header will be automatically generated and sent in
         * the response if it is not already present in the headers.
         * Defaults to `true`.
         * @type {boolean}
         */
        get sendDate(): boolean;
        /**
         * @ignore
         */
        writeContinue(): this;
        /**
         * @ignore
         */
        writeEarlyHints(): this;
        /**
         * @ignore
         */
        writeProcessing(): this;
        /**
         * Writes the response header to the request.
         * The `statusCode` is a 3-digit HTTP status code, like 200 or 404.
         * The last argument, `headers`, are the response headers.
         * Optionally one can give a human-readable `statusMessage`
         * as the second argument.
         * @param {number|string} statusCode
         * @param {string|object|string[]} [statusMessage]
         * @param {object|string[]} [headers]
         * @return {ClientRequest}
         */
        writeHead(statusCode: number | string, statusMessage?: string | object | string[], headers?: object | string[]): ClientRequest;
        #private;
    }
    /**
     * An options object container for an `Agent` instance.
     */
    export class AgentOptions {
        /**
         * `AgentOptions` class constructor.
         * @ignore
         * @param {{
         *   keepAlive?: boolean,
         *   timeout?: number
         * }} [options]
         */
        constructor(options?: {
            keepAlive?: boolean;
            timeout?: number;
        });
        keepAlive: boolean;
        timeout: number;
    }
    /**
     * An Agent is responsible for managing connection persistence
     * and reuse for HTTP clients.
     * @see {@link https://nodejs.org/api/http.html#class-httpagent}
     */
    export class Agent extends EventEmitter {
        /**
         * `Agent` class constructor.
         * @param {AgentOptions=} [options]
         */
        constructor(options?: AgentOptions | undefined);
        defaultProtocol: string;
        options: any;
        requests: Set<any>;
        sockets: {};
        maxFreeSockets: number;
        maxTotalSockets: number;
        maxSockets: number;
        /**
         * @ignore
         */
        get freeSockets(): {};
        /**
         * @ignore
         * @param {object} options
         */
        getName(options: object): string;
        /**
         * Produces a socket/stream to be used for HTTP requests.
         * @param {object} options
         * @param {function(Duplex)=} [callback]
         * @return {Duplex}
         */
        createConnection(options: object, callback?: ((arg0: Duplex) => any) | undefined): Duplex;
        /**
         * @ignore
         */
        keepSocketAlive(): void;
        /**
         * @ignore
         */
        reuseSocket(): void;
        /**
         * @ignore
         */
        destroy(): void;
    }
    /**
     * The global and default HTTP agent.
     * @type {Agent}
     */
    export const globalAgent: Agent;
    /**
     * A duplex stream between a HTTP request `IncomingMessage` and the
     * response `ServerResponse`
     */
    export class Connection extends Duplex {
        /**
         * `Connection` class constructor.
         * @ignore
         * @param {Server} server
         * @param {IncomingMessage} incomingMessage
         * @param {ServerResponse} serverResponse
         */
        constructor(server: Server, incomingMessage: IncomingMessage, serverResponse: ServerResponse);
        server: any;
        active: boolean;
        request: any;
        response: any;
        /**
         * Closes the connection, destroying the underlying duplex, request, and
         * response streams.
         * @return {Connection}
         */
        close(): Connection;
    }
    /**
     * A nodejs compat HTTP server typically intended for running in a "worker"
     * environment.
     * @see {@link https://nodejs.org/api/http.html#class-httpserver}
     */
    export class Server extends EventEmitter {
        requestTimeout: number;
        timeout: number;
        maxRequestsPerSocket: number;
        keepAliveTimeout: number;
        headersTimeout: number;
        /**
         * @ignore
         * @type {AsyncResource}
         */
        get resource(): AsyncResource;
        /**
         * The adapter interface for this `Server` instance.
         * @ignore
         */
        get adapterInterace(): {
            Connection: typeof exports.Connection;
            globalAgent: exports.Agent;
            IncomingMessage: typeof exports.IncomingMessage;
            METHODS: string[];
            ServerResponse: typeof exports.ServerResponse;
            STATUS_CODES: any;
        };
        /**
         * `true` if the server is closed, otherwise `false`.
         * @type {boolean}
         */
        get closed(): boolean;
        /**
         * The host to listen to. This value can be `null`.
         * Defaults to `location.hostname`. This value
         * is used to filter requests by hostname.
         * @type {string?}
         */
        get host(): string;
        /**
         * The `port` to listen on. This value can be `0`, which is the default.
         * This value is used to filter requests by port, if given. A port value
         * of `0` does not filter on any port.
         * @type {number}
         */
        get port(): number;
        /**
         * A readonly array of all active or inactive (idle) connections.
         * @type {Connection[]}
         */
        get connections(): exports.Connection[];
        /**
         * `true` if the server is listening for requests.
         * @type {boolean}
         */
        get listening(): boolean;
        set maxConnections(value: number);
        /**
         * The number of concurrent max connections this server should handle.
         * Default: Infinity
         * @type {number}
         */
        get maxConnections(): number;
        /**
         * Gets the HTTP server address and port that it this server is
         * listening (emulated) on in the runtime with respect to the
         * adapter internal being used by the server.
         * @return {{ family: string, address: string, port: number}}
         */
        address(): {
            family: string;
            address: string;
            port: number;
        };
        /**
         * Closes the server.
         * @param {function=} [close]
         */
        close(callback?: any): void;
        /**
         * Closes all connections.
         */
        closeAllConnections(): void;
        /**
         * Closes all idle connections.
         */
        closeIdleConnections(): void;
        /**
         * @ignore
         */
        setTimeout(timeout?: number, callback?: any): this;
        /**
         * @param {number|object=} [port]
         * @param {string=} [host]
         * @param {function|null} [unused]
         * @param {function=} [callback
         * @return Server
         */
        listen(port?: (number | object) | undefined, host?: string | undefined, unused?: Function | null, callback?: Function | undefined): this;
        #private;
    }
    export default exports;
    import { Writable } from "socket:stream";
    import { Buffer } from "socket:buffer";
    import { Readable } from "socket:stream";
    import { EventEmitter } from "socket:events";
    import { Duplex } from "socket:stream";
    import { AsyncResource } from "socket:async/resource";
    import * as exports from "socket:http";
    
}

declare module "socket:https" {
    /**
     * Makes a HTTPS request, optionally a `socket://` for relative paths when
     * `socket:` is the origin protocol.
     * @param {string|object} optionsOrURL
     * @param {(object|function)=} [options]
     * @param {function=} [callback]
     * @return {ClientRequest}
     */
    export function request(optionsOrURL: string | object, options?: (object | Function) | undefined, callback?: Function | undefined): ClientRequest;
    /**
     * Makes a HTTPS or `socket:` GET request. A simplified alias to `request()`.
     * @param {string|object} optionsOrURL
     * @param {(object|function)=} [options]
     * @param {function=} [callback]
     * @return {ClientRequest}
     */
    export function get(optionsOrURL: string | object, options?: (object | Function) | undefined, callback?: Function | undefined): ClientRequest;
    /**
     * Creates a HTTPS server that can listen for incoming requests.
     * Requests that are dispatched to this server depend on the context
     * in which it is created, such as a service worker which will use a
     * "fetch event" adapter.
     * @param {object|function=} [options]
     * @param {function=} [callback]
     * @return {Server}
     */
    export function createServer(...args: any[]): Server;
    export const CONTINUE: 100;
    export const SWITCHING_PROTOCOLS: 101;
    export const PROCESSING: 102;
    export const EARLY_HINTS: 103;
    export const OK: 200;
    export const CREATED: 201;
    export const ACCEPTED: 202;
    export const NONAUTHORITATIVE_INFORMATION: 203;
    export const NO_CONTENT: 204;
    export const RESET_CONTENT: 205;
    export const PARTIAL_CONTENT: 206;
    export const MULTISTATUS: 207;
    export const ALREADY_REPORTED: 208;
    export const IM_USED: 226;
    export const MULTIPLE_CHOICES: 300;
    export const MOVED_PERMANENTLY: 301;
    export const FOUND: 302;
    export const SEE_OTHER: 303;
    export const NOT_MODIFIED: 304;
    export const USE_PROXY: 305;
    export const TEMPORARY_REDIRECT: 307;
    export const PERMANENT_REDIRECT: 308;
    export const BAD_REQUEST: 400;
    export const UNAUTHORIZED: 401;
    export const PAYMENT_REQUIRED: 402;
    export const FORBIDDEN: 403;
    export const NOT_FOUND: 404;
    export const METHOD_NOT_ALLOWED: 405;
    export const NOT_ACCEPTABLE: 406;
    export const PROXY_AUTHENTICATION_REQUIRED: 407;
    export const REQUEST_TIMEOUT: 408;
    export const CONFLICT: 409;
    export const GONE: 410;
    export const LENGTH_REQUIRED: 411;
    export const PRECONDITION_FAILED: 412;
    export const PAYLOAD_TOO_LARGE: 413;
    export const URI_TOO_LONG: 414;
    export const UNSUPPORTED_MEDIA_TYPE: 415;
    export const RANGE_NOT_SATISFIABLE: 416;
    export const EXPECTATION_FAILED: 417;
    export const IM_A_TEAPOT: 418;
    export const MISDIRECTED_REQUEST: 421;
    export const UNPROCESSABLE_ENTITY: 422;
    export const LOCKED: 423;
    export const FAILED_DEPENDENCY: 424;
    export const TOO_EARLY: 425;
    export const UPGRADE_REQUIRED: 426;
    export const PRECONDITION_REQUIRED: 428;
    export const TOO_MANY_REQUESTS: 429;
    export const REQUEST_HEADER_FIELDS_TOO_LARGE: 431;
    export const UNAVAILABLE_FOR_LEGAL_REASONS: 451;
    export const INTERNAL_SERVER_ERROR: 500;
    export const NOT_IMPLEMENTED: 501;
    export const BAD_GATEWAY: 502;
    export const SERVICE_UNAVAILABLE: 503;
    export const GATEWAY_TIMEOUT: 504;
    export const HTTP_VERSION_NOT_SUPPORTED: 505;
    export const VARIANT_ALSO_NEGOTIATES: 506;
    export const INSUFFICIENT_STORAGE: 507;
    export const LOOP_DETECTED: 508;
    export const BANDWIDTH_LIMIT_EXCEEDED: 509;
    export const NOT_EXTENDED: 510;
    export const NETWORK_AUTHENTICATION_REQUIRED: 511;
    /**
     * All known possible HTTP methods.
     * @type {string[]}
     */
    export const METHODS: string[];
    /**
     * A mapping of status codes to status texts
     * @type {object}
     */
    export const STATUS_CODES: object;
    /**
     * An options object container for an `Agent` instance.
     */
    export class AgentOptions extends http.AgentOptions {
    }
    /**
     * An Agent is responsible for managing connection persistence
     * and reuse for HTTPS clients.
     * @see {@link https://nodejs.org/api/https.html#class-httpsagent}
     */
    export class Agent extends http.Agent {
    }
    /**
     * An object that is created internally and returned from `request()`.
     * @see {@link https://nodejs.org/api/http.html#class-httpclientrequest}
     */
    export class ClientRequest extends http.ClientRequest {
    }
    /**
     * The parent class of `ClientRequest` and `ServerResponse`.
     * It is an abstract outgoing message from the perspective of the
     * participants of an HTTP transaction.
     * @see {@link https://nodejs.org/api/http.html#class-httpoutgoingmessage}
     */
    export class OutgoingMessage extends http.OutgoingMessage {
    }
    /**
     * An `IncomingMessage` object is created by `Server` or `ClientRequest` and
     * passed as the first argument to the 'request' and 'response' event
     * respectively.
     * It may be used to access response status, headers, and data.
     * @see {@link https://nodejs.org/api/http.html#class-httpincomingmessage}
     */
    export class IncomingMessage extends http.IncomingMessage {
    }
    /**
     * An object that is created internally by a `Server` instance, not by the user.
     * It is passed as the second parameter to the 'request' event.
     * @see {@link https://nodejs.org/api/http.html#class-httpserverresponse}
     */
    export class ServerResponse extends http.ServerResponse {
    }
    /**
     * A duplex stream between a HTTP request `IncomingMessage` and the
     * response `ServerResponse`
     */
    export class Connection extends http.Connection {
    }
    /**
     * A nodejs compat HTTP server typically intended for running in a "worker"
     * environment.
     * @see {@link https://nodejs.org/api/http.html#class-httpserver}
     */
    export class Server extends http.Server {
    }
    /**
     * The global and default HTTPS agent.
     * @type {Agent}
     */
    export const globalAgent: Agent;
    export default exports;
    import http from "socket:http";
    import * as exports from "socket:http";
}

declare module "socket:enumeration" {
    /**
     * @module enumeration
     * This module provides a data structure for enumerated unique values.
     */
    /**
     * A container for enumerated values.
     */
    export class Enumeration extends Set<any> {
        /**
         * Creates an `Enumeration` instance from arguments.
         * @param {...any} values
         * @return {Enumeration}
         */
        static from(...values: any[]): Enumeration;
        /**
         * `Enumeration` class constructor.
         * @param {any[]} values
         * @param {object=} [options = {}]
         * @param {number=} [options.start = 0]
         */
        constructor(values: any[], options?: object | undefined);
        /**
         * @type {number}
         */
        get length(): number;
        /**
         * Returns `true` if enumeration contains `value`. An alias
         * for `Set.prototype.has`.
         * @return {boolean}
         */
        contains(value: any): boolean;
        /**
         * @ignore
         */
        add(): void;
        /**
         * @ignore
         */
        delete(): void;
        /**
         * JSON represenation of a `Enumeration` instance.
         * @ignore
         * @return {string[]}
         */
        toJSON(): string[];
        /**
         * Internal inspect function.
         * @ignore
         * @return {LanguageQueryResult}
         */
        inspect(): LanguageQueryResult;
    }
    export default Enumeration;
}

declare module "socket:language" {
    /**
     * Look up a language name or code by query.
     * @param {string} query
     * @param {object=} [options]
     * @param {boolean=} [options.strict = false]
     * @return {?LanguageQueryResult[]}
     */
    export function lookup(query: string, options?: object | undefined, ...args: any[]): LanguageQueryResult[] | null;
    /**
     * Describe a language by tag
     * @param {string} query
     * @param {object=} [options]
     * @param {boolean=} [options.strict = true]
     * @return {?LanguageDescription[]}
     */
    export function describe(query: string, options?: object | undefined): LanguageDescription[] | null;
    /**
     * A list of ISO 639-1 language names.
     * @type {string[]}
     */
    export const names: string[];
    /**
     * A list of ISO 639-1 language codes.
     * @type {string[]}
     */
    export const codes: string[];
    /**
     * A list of RFC 5646 language tag identifiers.
     * @see {@link http://tools.ietf.org/html/rfc5646}
     */
    export const tags: Enumeration;
    /**
     * A list of RFC 5646 language tag titles corresponding
     * to language tags.
     * @see {@link http://tools.ietf.org/html/rfc5646}
     */
    export const descriptions: Enumeration;
    /**
     * A container for a language query response containing an ISO language
     * name and code.
     * @see {@link https://www.sitepoint.com/iso-2-letter-language-codes}
     */
    export class LanguageQueryResult {
        /**
         * `LanguageQueryResult` class constructor.
         * @param {string} code
         * @param {string} name
         * @param {string[]} [tags]
         */
        constructor(code: string, name: string, tags?: string[]);
        /**
         * The language code corresponding to the query.
         * @type {string}
         */
        get code(): string;
        /**
         * The language name corresponding to the query.
         * @type {string}
         */
        get name(): string;
        /**
         * The language tags corresponding to the query.
         * @type {string[]}
         */
        get tags(): string[];
        /**
         * JSON represenation of a `LanguageQueryResult` instance.
         * @return {{
         *   code: string,
         *   name: string,
         *   tags: string[]
         * }}
         */
        toJSON(): {
            code: string;
            name: string;
            tags: string[];
        };
        /**
         * Internal inspect function.
         * @ignore
         * @return {LanguageQueryResult}
         */
        inspect(): LanguageQueryResult;
        #private;
    }
    /**
     * A container for a language code, tag, and description.
     */
    export class LanguageDescription {
        /**
         * `LanguageDescription` class constructor.
         * @param {string} code
         * @param {string} tag
         * @param {string} description
         */
        constructor(code: string, tag: string, description: string);
        /**
         * The language code corresponding to the language
         * @type {string}
         */
        get code(): string;
        /**
         * The language tag corresponding to the language.
         * @type {string}
         */
        get tag(): string;
        /**
         * The language description corresponding to the language.
         * @type {string}
         */
        get description(): string;
        /**
         * JSON represenation of a `LanguageDescription` instance.
         * @return {{
         *   code: string,
         *   tag: string,
         *   description: string
         * }}
         */
        toJSON(): {
            code: string;
            tag: string;
            description: string;
        };
        /**
         * Internal inspect function.
         * @ignore
         * @return {LanguageDescription}
         */
        inspect(): LanguageDescription;
        #private;
    }
    namespace _default {
        export { codes };
        export { describe };
        export { lookup };
        export { names };
        export { tags };
    }
    export default _default;
    import Enumeration from "socket:enumeration";
}

declare module "socket:latica/packets" {
    /**
     * The magic bytes prefixing every packet. They are the
     * 2nd, 3rd, 5th, and 7th, prime numbers.
     * @type {number[]}
     */
    export const MAGIC_BYTES_PREFIX: number[];
    /**
     * The version of the protocol.
     */
    export const VERSION: 6;
    /**
     * The size in bytes of the prefix magic bytes.
     */
    export const MAGIC_BYTES: 4;
    /**
     * The maximum size of the user message.
     */
    export const MESSAGE_BYTES: 1024;
    /**
     * The cache TTL in milliseconds.
     */
    export const CACHE_TTL: number;
    export namespace PACKET_SPEC {
        namespace type {
            let bytes: number;
            let encoding: string;
        }
        namespace version {
            let bytes_1: number;
            export { bytes_1 as bytes };
            let encoding_1: string;
            export { encoding_1 as encoding };
            export { VERSION as default };
        }
        namespace clock {
            let bytes_2: number;
            export { bytes_2 as bytes };
            let encoding_2: string;
            export { encoding_2 as encoding };
            let _default: number;
            export { _default as default };
        }
        namespace hops {
            let bytes_3: number;
            export { bytes_3 as bytes };
            let encoding_3: string;
            export { encoding_3 as encoding };
            let _default_1: number;
            export { _default_1 as default };
        }
        namespace index {
            let bytes_4: number;
            export { bytes_4 as bytes };
            let encoding_4: string;
            export { encoding_4 as encoding };
            let _default_2: number;
            export { _default_2 as default };
            export let signed: boolean;
        }
        namespace ttl {
            let bytes_5: number;
            export { bytes_5 as bytes };
            let encoding_5: string;
            export { encoding_5 as encoding };
            export { CACHE_TTL as default };
        }
        namespace clusterId {
            let bytes_6: number;
            export { bytes_6 as bytes };
            let encoding_6: string;
            export { encoding_6 as encoding };
            let _default_3: number[];
            export { _default_3 as default };
        }
        namespace subclusterId {
            let bytes_7: number;
            export { bytes_7 as bytes };
            let encoding_7: string;
            export { encoding_7 as encoding };
            let _default_4: number[];
            export { _default_4 as default };
        }
        namespace previousId {
            let bytes_8: number;
            export { bytes_8 as bytes };
            let encoding_8: string;
            export { encoding_8 as encoding };
            let _default_5: number[];
            export { _default_5 as default };
        }
        namespace packetId {
            let bytes_9: number;
            export { bytes_9 as bytes };
            let encoding_9: string;
            export { encoding_9 as encoding };
            let _default_6: number[];
            export { _default_6 as default };
        }
        namespace nextId {
            let bytes_10: number;
            export { bytes_10 as bytes };
            let encoding_10: string;
            export { encoding_10 as encoding };
            let _default_7: number[];
            export { _default_7 as default };
        }
        namespace usr1 {
            let bytes_11: number;
            export { bytes_11 as bytes };
            let _default_8: number[];
            export { _default_8 as default };
        }
        namespace usr2 {
            let bytes_12: number;
            export { bytes_12 as bytes };
            let _default_9: number[];
            export { _default_9 as default };
        }
        namespace usr3 {
            let bytes_13: number;
            export { bytes_13 as bytes };
            let _default_10: number[];
            export { _default_10 as default };
        }
        namespace usr4 {
            let bytes_14: number;
            export { bytes_14 as bytes };
            let _default_11: number[];
            export { _default_11 as default };
        }
        namespace message {
            let bytes_15: number;
            export { bytes_15 as bytes };
            let _default_12: number[];
            export { _default_12 as default };
        }
        namespace sig {
            let bytes_16: number;
            export { bytes_16 as bytes };
            let _default_13: number[];
            export { _default_13 as default };
        }
    }
    /**
     * The size in bytes of the total packet frame and message.
     */
    export const PACKET_BYTES: number;
    /**
     * The maximum distance that a packet can be replicated.
     */
    export const MAX_HOPS: 16;
    export function validateMessage(o: object, constraints: {
        [key: string]: constraint;
    }): void;
    /**
     * Computes a SHA-256 hash of input returning a hex encoded string.
     * @type {function(string|Buffer|Uint8Array): Promise<string>}
     */
    export const sha256: (arg0: string | Buffer | Uint8Array) => Promise<string>;
    export function decode(buf: Buffer): Packet;
    export function getTypeFromBytes(buf: any): any;
    export class Packet {
        static ttl: number;
        static maxLength: number;
        /**
         * Returns an empty `Packet` instance.
         * @return {Packet}
         */
        static empty(): Packet;
        /**
         * @param {Packet|object} packet
         * @return {Packet}
         */
        static from(packet: Packet | object): Packet;
        /**
         * Determines if input is a packet.
         * @param {Buffer|Uint8Array|number[]|object|Packet} packet
         * @return {boolean}
         */
        static isPacket(packet: Buffer | Uint8Array | number[] | object | Packet): boolean;
        /**
        */
        static encode(p: any): Promise<Uint8Array>;
        static decode(buf: any): Packet;
        /**
         * `Packet` class constructor.
         * @param {Packet|object?} options
         */
        constructor(options?: Packet | (object | null));
        /**
         * @param {Packet} packet
         * @return {Packet}
         */
        copy(): Packet;
        timestamp: any;
        isComposed: any;
        isReconciled: any;
        meta: any;
    }
    export class PacketPing extends Packet {
        static type: number;
    }
    export class PacketPong extends Packet {
        static type: number;
    }
    export class PacketIntro extends Packet {
        static type: number;
    }
    export class PacketJoin extends Packet {
        static type: number;
    }
    export class PacketPublish extends Packet {
        static type: number;
    }
    export class PacketStream extends Packet {
        static type: number;
    }
    export class PacketSync extends Packet {
        static type: number;
    }
    export class PacketQuery extends Packet {
        static type: number;
    }
    export default Packet;
    export type constraint = {
        type: string;
        required?: boolean;
        /**
         * optional validator fn returning boolean
         */
        assert?: Function;
    };
    import { Buffer } from "socket:buffer";
}

declare module "socket:latica/encryption" {
    /**
     * Class for handling encryption and key management.
     */
    export class Encryption {
        /**
         * Creates a shared key based on the provided seed or generates a random one.
         * @param {Uint8Array|string} seed - Seed for key generation.
         * @returns {Promise<Uint8Array>} - Shared key.
         */
        static createSharedKey(seed: Uint8Array | string): Promise<Uint8Array>;
        /**
         * Creates a key pair for signing and verification.
         * @param {Uint8Array|string} seed - Seed for key generation.
         * @returns {Promise<{ publicKey: Uint8Array, privateKey: Uint8Array }>} - Key pair.
         */
        static createKeyPair(seed: Uint8Array | string): Promise<{
            publicKey: Uint8Array;
            privateKey: Uint8Array;
        }>;
        /**
         * Creates an ID using SHA-256 hash.
         * @param {string} str - String to hash.
         * @returns {Promise<Uint8Array>} - SHA-256 hash.
         */
        static createId(str: string): Promise<Uint8Array>;
        /**
         * Creates a cluster ID using SHA-256 hash with specified output size.
         * @param {string} str - String to hash.
         * @returns {Promise<Uint8Array>} - SHA-256 hash with specified output size.
         */
        static createClusterId(str: string): Promise<Uint8Array>;
        /**
         * Signs a message using the given secret key.
         * @param {Buffer} b - The message to sign.
         * @param {Uint8Array} sk - The secret key to use.
         * @returns {Uint8Array} - Signature.
         */
        static sign(b: Buffer, sk: Uint8Array): Uint8Array;
        /**
         * Verifies the signature of a message using the given public key.
         * @param {Buffer} b - The message to verify.
         * @param {Uint8Array} sig - The signature to check.
         * @param {Uint8Array} pk - The public key to use.
         * @returns {number} - Returns non-zero if the buffer could not be verified.
         */
        static verify(b: Buffer, sig: Uint8Array, pk: Uint8Array): number;
        /**
         * Mapping of public keys to key objects.
         * @type {Object.<string, { publicKey: Uint8Array, privateKey: Uint8Array, ts: number }>}
         */
        keys: {
            [x: string]: {
                publicKey: Uint8Array;
                privateKey: Uint8Array;
                ts: number;
            };
        };
        /**
         * Adds a key pair to the keys mapping.
         * @param {Uint8Array|string} publicKey - Public key.
         * @param {Uint8Array} privateKey - Private key.
         */
        add(publicKey: Uint8Array | string, privateKey: Uint8Array): void;
        /**
         * Removes a key from the keys mapping.
         * @param {Uint8Array|string} publicKey - Public key.
         */
        remove(publicKey: Uint8Array | string): void;
        /**
         * Checks if a key is in the keys mapping.
         * @param {Uint8Array|string} to - Public key or Uint8Array.
         * @returns {boolean} - True if the key is present, false otherwise.
         */
        has(to: Uint8Array | string): boolean;
        /**
         * Opens a sealed message using the specified key.
         * @param {Buffer} message - The sealed message.
         * @param {Object|string} v - Key object or public key.
         * @returns {Buffer} - Decrypted message.
         * @throws {Error} - Throws ENOKEY if the key is not found.
         */
        openUnsigned(message: Buffer, v: any | string): Buffer;
        sealUnsigned(message: any, v: any): any;
        /**
         * Decrypts a sealed and signed message for a specific receiver.
         * @param {Buffer} message - The sealed message.
         * @param {Object|string} v - Key object or public key.
         * @returns {Buffer} - Decrypted message.
         * @throws {Error} - Throws ENOKEY if the key is not found, EMALFORMED if the message is malformed, ENOTVERIFIED if the message cannot be verified.
         */
        open(message: Buffer, v: any | string): Buffer;
        /**
         * Seals and signs a message for a specific receiver using their public key.
         *
         * `Seal(message, receiver)` performs an _encrypt-sign-encrypt_ (ESE) on
         * a plaintext `message` for a `receiver` identity. This prevents repudiation
         * attacks and doesn't rely on packet chain guarantees.
         *
         * let ct = Seal(sender | pt, receiver)
         * let sig = Sign(ct, sk)
         * let out = Seal(sig | ct)
         *
         * In an setup between Alice & Bob, this means:
         * - Only Bob sees the plaintext
         * - Alice wrote the plaintext and the ciphertext
         * - Only Bob can see that Alice wrote the plaintext and ciphertext
         * - Bob cannot forward the message without invalidating Alice's signature.
         * - The outer encryption serves to prevent an attacker from replacing Alice's
         *   signature. As with _sign-encrypt-sign (SES), ESE is a variant of
         *   including the recipient's name inside the plaintext, which is then signed
         *   and encrypted Alice signs her plaintext along with her ciphertext, so as
         *   to protect herself from a laintext-substitution attack. At the same time,
         *   Alice's signed plaintext gives Bob non-repudiation.
         *
         * @see https://theworld.com/~dtd/sign_encrypt/sign_encrypt7.html
         *
         * @param {Buffer} message - The message to seal.
         * @param {Object|string} v - Key object or public key.
         * @returns {Buffer} - Sealed message.
         * @throws {Error} - Throws ENOKEY if the key is not found.
         */
        seal(message: Buffer, v: any | string): Buffer;
    }
    import Buffer from "socket:buffer";
}

declare module "socket:latica/cache" {
    /**
     * @typedef {Packet} CacheEntry
     * @typedef {function(CacheEntry, CacheEntry): number} CacheEntrySiblingResolver
     */
    /**
     * Default cache sibling resolver that computes a delta between
     * two entries clocks.
     * @param {CacheEntry} a
     * @param {CacheEntry} b
     * @return {number}
     */
    export function defaultSiblingResolver(a: CacheEntry, b: CacheEntry): number;
    /**
     * Default max size of a `Cache` instance.
     */
    export const DEFAULT_MAX_SIZE: number;
    /**
     * Internal mapping of packet IDs to packet data used by `Cache`.
     */
    export class CacheData extends Map<any, any> {
        constructor();
        constructor(entries?: readonly (readonly [any, any])[]);
        constructor();
        constructor(iterable?: Iterable<readonly [any, any]>);
    }
    /**
     * A class for storing a cache of packets by ID. This class includes a scheme
     * for reconciling disjointed packet caches in a large distributed system. The
     * following are key design characteristics.
     *
     * Space Efficiency: This scheme can be space-efficient because it summarizes
     * the cache's contents in a compact binary format. By sharing these summaries,
     * two computers can quickly determine whether their caches have common data or
     * differences.
     *
     * Bandwidth Efficiency: Sharing summaries instead of the full data can save
     * bandwidth. If the differences between the caches are small, sharing summaries
     * allows for more efficient data synchronization.
     *
     * Time Efficiency: The time efficiency of this scheme depends on the size of
     * the cache and the differences between the two caches. Generating summaries
     * and comparing them can be faster than transferring and comparing the entire
     * dataset, especially for large caches.
     *
     * Complexity: The scheme introduces some complexity due to the need to encode
     * and decode summaries. In some cases, the overhead introduced by this
     * complexity might outweigh the benefits, especially if the caches are
     * relatively small. In this case, you should be using a query.
     *
     * Data Synchronization Needs: The efficiency also depends on the data
     * synchronization needs. If the data needs to be synchronized in real-time,
     * this scheme might not be suitable. It's more appropriate for cases where
     * periodic or batch synchronization is acceptable.
     *
     * Scalability: The scheme's efficiency can vary depending on the scalability
     * of the system. As the number of cache entries or computers involved
     * increases, the complexity of generating and comparing summaries will stay
     * bound to a maximum of 16Mb.
     *
     */
    export class Cache {
        static HASH_SIZE_BYTES: number;
        static HASH_EMPTY: string;
        /**
         * The encodeSummary method provides a compact binary encoding of the output
         * of summary()
         *
         * @param {Object} summary - the output of calling summary()
         * @return {Buffer}
        **/
        static encodeSummary(summary: any): Buffer;
        /**
         * The decodeSummary method decodes the output of encodeSummary()
         *
         * @param {Buffer} bin - the output of calling encodeSummary()
         * @return {Object} summary
        **/
        static decodeSummary(bin: Buffer): any;
        /**
         * Test a summary hash format is valid
         *
         * @param {string} hash
         * @returns boolean
         */
        static isValidSummaryHashFormat(hash: string): boolean;
        /**
         * `Cache` class constructor.
         * @param {CacheData?} [data]
         */
        constructor(data?: CacheData | null, siblingResolver?: typeof defaultSiblingResolver);
        data: CacheData;
        maxSize: number;
        siblingResolver: typeof defaultSiblingResolver;
        /**
         * Readonly count of the number of cache entries.
         * @type {number}
         */
        get size(): number;
        /**
         * Readonly size of the cache in bytes.
         * @type {number}
         */
        get bytes(): number;
        /**
         * Inserts a `CacheEntry` value `v` into the cache at key `k`.
         * @param {string} k
         * @param {CacheEntry} v
         * @return {boolean}
         */
        insert(k: string, v: CacheEntry): boolean;
        /**
         * Gets a `CacheEntry` value at key `k`.
         * @param {string} k
         * @return {CacheEntry?}
         */
        get(k: string): CacheEntry | null;
        /**
         * @param {string} k
         * @return {boolean}
         */
        delete(k: string): boolean;
        /**
         * Predicate to determine if cache contains an entry at key `k`.
         * @param {string} k
         * @return {boolean}
         */
        has(k: string): boolean;
        /**
         * Composes an indexed packet into a new `Packet`
         * @param {Packet} packet
         */
        compose(packet: Packet, source?: CacheData): Promise<Packet>;
        sha1(value: any, toHex: any): Promise<any>;
        /**
         *
         * The summarize method returns a terse yet comparable summary of the cache
         * contents.
         *
         * Think of the cache as a trie of hex characters, the summary returns a
         * checksum for the current level of the trie and for its 16 children.
         *
         * This is similar to a merkel tree as equal subtrees can easily be detected
         * without the need for further recursion. When the subtree checksums are
         * inequivalent then further negotiation at lower levels may be required, this
         * process continues until the two trees become synchonized.
         *
         * When the prefix is empty, the summary will return an array of 16 checksums
         * these checksums provide a way of comparing that subtree with other peers.
         *
         * When a variable-length hexidecimal prefix is provided, then only cache
         * member hashes sharing this prefix will be considered.
         *
         * For each hex character provided in the prefix, the trie will decend by one
         * level, each level divides the 2^128 address space by 16. For exmaple...
         *
         * ```
         * Level  0   1   2
         * ----------------
         * 2b00
         * aa0e  ━┓  ━┓
         * aa1b   ┃   ┃
         * aae3   ┃   ┃  ━┓
         * aaea   ┃   ┃   ┃
         * aaeb   ┃  ━┛  ━┛
         * ab00   ┃  ━┓
         * ab1e   ┃   ┃
         * ab2a   ┃   ┃
         * abef   ┃   ┃
         * abf0  ━┛  ━┛
         * bff9
         * ```
         *
         * @param {string} prefix - a string of lowercased hexidecimal characters
         * @return {Object}
         *
         */
        summarize(prefix?: string, predicate?: (o: any) => boolean): any;
    }
    export default Cache;
    export type CacheEntry = Packet;
    export type CacheEntrySiblingResolver = (arg0: CacheEntry, arg1: CacheEntry) => number;
    import { Packet } from "socket:latica/packets";
    import { Buffer } from "socket:buffer";
}

declare module "socket:latica/nat" {
    /**
     * The NAT type is encoded using 5 bits:
     *
     * 0b00001 : the lsb indicates if endpoint dependence information is included
     * 0b00010 : the second bit indicates the endpoint dependence value
     *
     * 0b00100 : the third bit indicates if firewall information is included
     * 0b01000 : the fourth bit describes which requests can pass the firewall, only known IPs (0) or any IP (1)
     * 0b10000 : the fifth bit describes which requests can pass the firewall, only known ports (0) or any port (1)
     */
    /**
     * Every remote will see the same IP:PORT mapping for this peer.
     *
     *                        :3333 ┌──────┐
     *   :1111                ┌───▶ │  R1  │
     * ┌──────┐    ┌───────┐  │     └──────┘
     * │  P1  ├───▶│  NAT  ├──┤
     * └──────┘    └───────┘  │     ┌──────┐
     *                        └───▶ │  R2  │
     *                        :3333 └──────┘
     */
    export const MAPPING_ENDPOINT_INDEPENDENT: 3;
    /**
     * Every remote will see a different IP:PORT mapping for this peer.
     *
     *                        :4444 ┌──────┐
     *   :1111                ┌───▶ │  R1  │
     * ┌──────┐    ┌───────┐  │     └──────┘
     * │  P1  ├───▶│  NAT  ├──┤
     * └──────┘    └───────┘  │     ┌──────┐
     *                        └───▶ │  R2  │
     *                        :5555 └──────┘
     */
    export const MAPPING_ENDPOINT_DEPENDENT: 1;
    /**
     * The firewall allows the port mapping to be accessed by:
     * - Any IP:PORT combination (FIREWALL_ALLOW_ANY)
     * - Any PORT on a previously connected IP (FIREWALL_ALLOW_KNOWN_IP)
     * - Only from previously connected IP:PORT combinations (FIREWALL_ALLOW_KNOWN_IP_AND_PORT)
     */
    export const FIREWALL_ALLOW_ANY: 28;
    export const FIREWALL_ALLOW_KNOWN_IP: 12;
    export const FIREWALL_ALLOW_KNOWN_IP_AND_PORT: 4;
    /**
     * The initial state of the nat is unknown and its value is 0
     */
    export const UNKNOWN: 0;
    /**
     * Full-cone NAT, also known as one-to-one NAT
     *
     * Any external host can send packets to iAddr:iPort by sending packets to eAddr:ePort.
     *
     * @summary its a packet party at this mapping and everyone's invited
     */
    export const UNRESTRICTED: number;
    /**
     * (Address)-restricted-cone NAT
     *
     * An external host (hAddr:any) can send packets to iAddr:iPort by sending packets to eAddr:ePort only
     * if iAddr:iPort has previously sent a packet to hAddr:any. "Any" means the port number doesn't matter.
     *
     * @summary The NAT will drop your packets unless a peer within its network has previously messaged you from *any* port.
     */
    export const ADDR_RESTRICTED: number;
    /**
     * Port-restricted cone NAT
     *
     * An external host (hAddr:hPort) can send packets to iAddr:iPort by sending
     * packets to eAddr:ePort only if iAddr:iPort has previously sent a packet to
     * hAddr:hPort.
     *
     * @summary The NAT will drop your packets unless a peer within its network
     * has previously messaged you from this *specific* port.
     */
    export const PORT_RESTRICTED: number;
    /**
     * Symmetric NAT
     *
     * Only an external host that receives a packet from an internal host can send
     * a packet back.
     *
     * @summary The NAT will only accept replies to a correspondence initialized
     * by itself, the mapping it created is only valid for you.
     */
    export const ENDPOINT_RESTRICTED: number;
    export function isEndpointDependenceDefined(nat: any): boolean;
    export function isFirewallDefined(nat: any): boolean;
    export function isValid(nat: any): boolean;
    export function toString(n: any): "UNRESTRICTED" | "ADDR_RESTRICTED" | "PORT_RESTRICTED" | "ENDPOINT_RESTRICTED" | "UNKNOWN";
    export function toStringStrategy(n: any): "STRATEGY_DEFER" | "STRATEGY_DIRECT_CONNECT" | "STRATEGY_TRAVERSAL_OPEN" | "STRATEGY_TRAVERSAL_CONNECT" | "STRATEGY_PROXY" | "STRATEGY_UNKNOWN";
    export const STRATEGY_DEFER: 0;
    export const STRATEGY_DIRECT_CONNECT: 1;
    export const STRATEGY_TRAVERSAL_OPEN: 2;
    export const STRATEGY_TRAVERSAL_CONNECT: 3;
    export const STRATEGY_PROXY: 4;
    export function connectionStrategy(a: any, b: any): 0 | 1 | 2 | 3 | 4;
}

declare module "socket:latica/index" {
    /**
     * Computes rate limit predicate value for a port and address pair for a given
     * threshold updating an input rates map. This method is accessed concurrently,
     * the rates object makes operations atomic to avoid race conditions.
     *
     * @param {Map} rates
     * @param {number} type
     * @param {number} port
     * @param {string} address
     * @return {boolean}
     */
    export function rateLimit(rates: Map<any, any>, type: number, port: number, address: string, subclusterIdQuota: any): boolean;
    /**
     * Retry delay in milliseconds for ping.
     * @type {number}
     */
    export const PING_RETRY: number;
    /**
     * Probe wait timeout in milliseconds.
     * @type {number}
     */
    export const PROBE_WAIT: number;
    /**
     * Default keep alive timeout.
     * @type {number}
     */
    export const DEFAULT_KEEP_ALIVE: number;
    /**
     * Default rate limit threshold in milliseconds.
     * @type {number}
     */
    export const DEFAULT_RATE_LIMIT_THRESHOLD: number;
    export function getRandomPort(ports: object, p: number | null): number;
    /**
     * A `RemotePeer` represents an initial, discovered, or connected remote peer.
     * Typically, you will not need to create instances of this class directly.
     */
    export class RemotePeer {
        /**
         * `RemotePeer` class constructor.
         * @param {{
         *   peerId?: string,
         *   address?: string,
         *   port?: number,
         *   natType?: number,
         *   clusters: object,
         *   reflectionId?: string,
         *   distance?: number,
         *   publicKey?: string,
         *   privateKey?: string,
         *   clock?: number,
         *   lastUpdate?: number,
         *   lastRequest?: number
         * }} o
         */
        constructor(o: {
            peerId?: string;
            address?: string;
            port?: number;
            natType?: number;
            clusters: object;
            reflectionId?: string;
            distance?: number;
            publicKey?: string;
            privateKey?: string;
            clock?: number;
            lastUpdate?: number;
            lastRequest?: number;
        }, peer: any);
        peerId: any;
        address: any;
        port: number;
        natType: any;
        clusters: {};
        pingId: any;
        distance: number;
        connected: boolean;
        opening: number;
        probed: number;
        proxy: any;
        clock: number;
        uptime: number;
        lastUpdate: number;
        lastRequest: number;
        localPeer: any;
        write(sharedKey: any, args: any): Promise<any[]>;
    }
    /**
     * `Peer` class factory.
     * @param {{ createSocket: function('udp4', null, object?): object }} options
     */
    export class Peer {
        /**
         * Test a peerID is valid
         *
         * @param {string} pid
         * @returns boolean
         */
        static isValidPeerId(pid: string): boolean;
        /**
         * Test a reflectionID is valid
         *
         * @param {string} rid
         * @returns boolean
         */
        static isValidReflectionId(rid: string): boolean;
        /**
         * Test a pingID is valid
         *
         * @param {string} pid
         * @returns boolean
         */
        static isValidPingId(pid: string): boolean;
        /**
         * Returns the online status of the browser, else true.
         *
         * note: globalThis.navigator was added to node in v22.
         *
         * @returns boolean
         */
        static onLine(): boolean;
        /**
         * `Peer` class constructor.
         * @param {object=} opts - Options
         * @param {Buffer} opts.peerId - A 32 byte buffer (ie, `Encryption.createId()`).
         * @param {Buffer} opts.clusterId - A 32 byte buffer (ie, `Encryption.createClusterId()`).
         * @param {number=} opts.port - A port number.
         * @param {number=} opts.probeInternalPort - An internal port number (semi-private for testing).
         * @param {number=} opts.probeExternalPort - An external port number (semi-private for testing).
         * @param {number=} opts.natType - A nat type.
         * @param {string=} opts.address - An ipv4 address.
         * @param {number=} opts.keepalive - The interval of the main loop.
         * @param {function=} opts.siblingResolver - A function that can be used to determine canonical data in case two packets have concurrent clock values.
         * @param {object} dgram - A nodejs compatible implementation of the dgram module (sans multicast).
         */
        constructor(persistedState: {}, dgram: object);
        port: any;
        address: any;
        natType: number;
        nextNatType: number;
        clusters: {};
        syncs: {};
        reflectionId: any;
        reflectionTimeout: any;
        reflectionStage: number;
        reflectionRetry: number;
        reflectionFirstResponder: any;
        peerId: string;
        isListening: boolean;
        ctime: number;
        lastUpdate: number;
        lastSync: number;
        closing: boolean;
        clock: number;
        unpublished: {};
        cache: any;
        uptime: number;
        maxHops: number;
        bdpCache: number[];
        dgram: any;
        onListening: any;
        onDelete: any;
        sendQueue: any[];
        firewall: any;
        rates: Map<any, any>;
        streamBuffer: Map<any, any>;
        gate: Map<any, any>;
        returnRoutes: Map<any, any>;
        metrics: {
            i: {
                0: number;
                1: number;
                2: number;
                3: number;
                4: number;
                5: number;
                6: number;
                7: number;
                8: number;
                DROPPED: number;
            };
            o: {
                0: number;
                1: number;
                2: number;
                3: number;
                4: number;
                5: number;
                6: number;
                7: number;
                8: number;
            };
        };
        peers: any;
        encryption: Encryption;
        config: any;
        _onError: (err: any) => any;
        socket: any;
        probeSocket: any;
        /**
         * An implementation for clearning an interval that can be overridden by the test suite
         * @param Number the number that identifies the timer
         * @return {undefined}
         * @ignore
         */
        _clearInterval(tid: any): undefined;
        /**
         * An implementation for clearning a timeout that can be overridden by the test suite
         * @param Number the number that identifies the timer
         * @return {undefined}
         * @ignore
         */
        _clearTimeout(tid: any): undefined;
        /**
         * An implementation of an internal timer that can be overridden by the test suite
         * @return {Number}
         * @ignore
         */
        _setInterval(fn: any, t: any): number;
        /**
         * An implementation of an timeout timer that can be overridden by the test suite
         * @return {Number}
         * @ignore
         */
        _setTimeout(fn: any, t: any): number;
        _onDebug(...args: any[]): void;
        /**
         * A method that encapsulates the listing procedure
         * @return {undefined}
         * @ignore
         */
        _listen(): undefined;
        init(cb: any): Promise<any>;
        onReady: any;
        mainLoopTimer: number;
        /**
         * Continuously evaluate the state of the peer and its network
         * @return {undefined}
         * @ignore
         */
        _mainLoop(ts: any): undefined;
        /**
         * Enqueue packets to be sent to the network
         * @param {Buffer} data - An encoded packet
         * @param {number} port - The desination port of the remote host
         * @param {string} address - The destination address of the remote host
         * @param {Socket=this.socket} socket - The socket to send on
         * @return {undefined}
         * @ignore
         */
        send(data: Buffer, port: number, address: string, socket?: any): undefined;
        /**
         * @private
         */
        private stream;
        /**
         * @private
         */
        private _scheduleSend;
        sendTimeout: number;
        /**
         * @private
         */
        private _dequeue;
        /**
         * Send any unpublished packets
         * @return {undefined}
         * @ignore
         */
        sendUnpublished(): undefined;
        /**
         * Get the serializable state of the peer (can be passed to the constructor or create method)
         * @return {undefined}
         */
        getState(): undefined;
        getInfo(): Promise<{
            address: any;
            port: any;
            clock: number;
            uptime: number;
            natType: number;
            natName: string;
            peerId: string;
        }>;
        cacheInsert(packet: any): Promise<void>;
        addIndexedPeer(info: any): Promise<void>;
        reconnect(): Promise<void>;
        disconnect(): Promise<void>;
        probeReflectionTimeout: any;
        sealUnsigned(...args: any[]): Promise<any>;
        openUnsigned(...args: any[]): Promise<Buffer>;
        seal(...args: any[]): Promise<Buffer>;
        open(...args: any[]): Promise<Buffer>;
        addEncryptionKey(...args: any[]): Promise<void>;
        /**
         * Get a selection of known peers
         * @return {Array<RemotePeer>}
         * @ignore
         */
        getPeers(packet: any, peers: any, ignorelist: any, filter?: (o: any) => any): Array<RemotePeer>;
        /**
         * Send an eventually consistent packet to a selection of peers (fanout)
         * @return {undefined}
         * @ignore
         */
        mcast(packet: any, ignorelist?: any[]): undefined;
        /**
         * The process of determining this peer's NAT behavior (firewall and dependentness)
         * @return {undefined}
         * @ignore
         */
        requestReflection(): undefined;
        /**
         * Ping another peer
         * @return {PacketPing}
         * @ignore
         */
        ping(peer: any, withRetry: any, props: any, socket: any): PacketPing;
        /**
         * Get a peer
         * @return {RemotePeer}
         * @ignore
         */
        getPeer(id: any): RemotePeer;
        /**
         * This should be called at least once when an app starts to multicast
         * this peer, and starts querying the network to discover peers.
         * @param {object} keys - Created by `Encryption.createKeyPair()`.
         * @param {object=} args - Options
         * @param {number=MAX_BANDWIDTH} args.rateLimit - How many requests per second to allow for this subclusterId.
         * @return {RemotePeer}
         */
        join(sharedKey: any, args?: object | undefined): RemotePeer;
        /**
         * @param {Packet} T - The constructor to be used to create packets.
         * @param {Any} message - The message to be split and packaged.
         * @return {Array<Packet<T>>}
         * @ignore
         */
        _message2packets(T: Packet, message: Any, args: any): Array<Packet<Packet>>;
        /**
         * Sends a packet into the network that will be replicated and buffered.
         * Each peer that receives it will buffer it until TTL and then replicate
         * it provided it has has not exceeded their maximum number of allowed hops.
         *
         * @param {object} keys - the public and private key pair created by `Encryption.createKeyPair()`.
         * @param {object} args - The arguments to be applied.
         * @param {Buffer} args.message - The message to be encrypted by keys and sent.
         * @param {Packet<T>=} args.packet - The previous packet in the packet chain.
         * @param {Buffer} args.usr1 - 32 bytes of arbitrary clusterId in the protocol framing.
         * @param {Buffer} args.usr2 - 32 bytes of arbitrary clusterId in the protocol framing.
         * @return {Array<PacketPublish>}
         */
        publish(sharedKey: any, args: {
            message: Buffer;
            packet?: Packet<T> | undefined;
            usr1: Buffer;
            usr2: Buffer;
        }): Array<PacketPublish>;
        /**
         * @return {undefined}
         */
        sync(peer: any, ptime?: number): undefined;
        close(): void;
        /**
         * Deploy a query into the network
         * @return {undefined}
         *
         */
        query(query: any): undefined;
        /**
         *
         * This is a default implementation for deciding what to summarize
         * from the cache when receiving a request to sync. that can be overridden
         *
         */
        cachePredicate(ts: any): (packet: any) => boolean;
        /**
         * A connection was made, add the peer to the local list of known
         * peers and call the onConnection if it is defined by the user.
         *
         * @return {undefined}
         * @ignore
         */
        _onConnection(packet: any, peerId: any, port: any, address: any, proxy: any, socket: any): undefined;
        /**
         * Received a Sync Packet
         * @return {undefined}
         * @ignore
         */
        _onSync(packet: any, port: any, address: any): undefined;
        /**
         * Received a Query Packet
         *
         * a -> b -> c -> (d) -> c -> b -> a
         *
         * @return {undefined}
         * @example
         *
         * ```js
         * peer.onQuery = (packet) => {
         *   //
         *   // read a database or something
         *   //
         *   return {
         *     message: Buffer.from('hello'),
         *     publicKey: '',
         *     privateKey: ''
         *   }
         * }
         * ```
         */
        _onQuery(packet: any, port: any, address: any): undefined;
        /**
         * Received a Ping Packet
         * @return {undefined}
         * @ignore
         */
        _onPing(packet: any, port: any, address: any): undefined;
        /**
         * Received a Pong Packet
         * @return {undefined}
         * @ignore
         */
        _onPong(packet: any, port: any, address: any): undefined;
        reflectionFirstReponderTimeout: number;
        /**
         * Received an Intro Packet
         * @return {undefined}
         * @ignore
         */
        _onIntro(packet: any, port: any, address: any, _: any, opts?: {
            attempts: number;
        }): undefined;
        socketPool: any[];
        /**
         * Received an Join Packet
         * @return {undefined}
         * @ignore
         */
        _onJoin(packet: any, port: any, address: any, data: any): undefined;
        /**
         * Received an Publish Packet
         * @return {undefined}
         * @ignore
         */
        _onPublish(packet: any, port: any, address: any, data: any): undefined;
        /**
         * Received an Stream Packet
         * @return {undefined}
         * @ignore
         */
        _onStream(packet: any, port: any, address: any, data: any): undefined;
        /**
         * Received any packet on the probe port to determine the firewall:
         * are you port restricted, host restricted, or unrestricted.
         * @return {undefined}
         * @ignore
         */
        _onProbeMessage(data: any, { port, address }: {
            port: any;
            address: any;
        }): undefined;
        /**
         * When a packet is received it is decoded, the packet contains the type
         * of the message. Based on the message type it is routed to a function.
         * like WebSockets, don't answer queries unless we know its another SRP peer.
         *
         * @param {Buffer|Uint8Array} data
         * @param {{ port: number, address: string }} info
         */
        _onMessage(data: Buffer | Uint8Array, { port, address }: {
            port: number;
            address: string;
        }): Promise<undefined>;
    }
    export default Peer;
    import { Packet } from "socket:latica/packets";
    import { sha256 } from "socket:latica/packets";
    import { Cache } from "socket:latica/cache";
    import { Encryption } from "socket:latica/encryption";
    import * as NAT from "socket:latica/nat";
    import { Buffer } from "socket:buffer";
    import { PacketPing } from "socket:latica/packets";
    import { PacketPublish } from "socket:latica/packets";
    export { Packet, sha256, Cache, Encryption, NAT };
}

declare module "socket:latica/proxy" {
    export default PeerWorkerProxy;
    /**
     * `Proxy` class factory, returns a Proxy class that is a proxy to the Peer.
     * @param {{ createSocket: function('udp4', null, object?): object }} options
     */
    export class PeerWorkerProxy {
        constructor(options: any, port: any, fn: any);
        init(): Promise<any>;
        reconnect(): Promise<any>;
        disconnect(): Promise<any>;
        getInfo(): Promise<any>;
        getMetrics(): Promise<any>;
        getState(): Promise<any>;
        open(...args: any[]): Promise<any>;
        seal(...args: any[]): Promise<any>;
        sealUnsigned(...args: any[]): Promise<any>;
        openUnsigned(...args: any[]): Promise<any>;
        addEncryptionKey(...args: any[]): Promise<any>;
        send(...args: any[]): Promise<any>;
        sendUnpublished(...args: any[]): Promise<any>;
        cacheInsert(...args: any[]): Promise<any>;
        mcast(...args: any[]): Promise<any>;
        requestReflection(...args: any[]): Promise<any>;
        stream(...args: any[]): Promise<any>;
        join(...args: any[]): Promise<any>;
        publish(...args: any[]): Promise<any>;
        sync(...args: any[]): Promise<any>;
        close(...args: any[]): Promise<any>;
        query(...args: any[]): Promise<any>;
        compileCachePredicate(src: any): Promise<any>;
        callWorkerThread(prop: any, data: any): any;
        callMainThread(prop: any, args: any): void;
        resolveMainThread(seq: any, result: any): any;
        #private;
    }
}

declare module "socket:latica/api" {
    export default api;
    /**
     * Initializes and returns the network bus.
     *
     * @async
     * @function
     * @param {object} options - Configuration options for the network bus.
     * @param {object} events - A nodejs compatibe implementation of the events module.
     * @param {object} dgram - A nodejs compatible implementation of the dgram module.
     * @returns {Promise<events.EventEmitter>} - A promise that resolves to the initialized network bus.
     */
    export function api(options: object, events: object, dgram: object): Promise<events.EventEmitter>;
}

declare module "socket:network" {
    export default network;
    export function network(options: any): Promise<events.EventEmitter>;
    import { Cache } from "socket:latica/index";
    import { sha256 } from "socket:latica/index";
    import { Encryption } from "socket:latica/index";
    import { Packet } from "socket:latica/index";
    import { NAT } from "socket:latica/index";
    export { Cache, sha256, Encryption, Packet, NAT };
}

declare module "socket:service-worker" {
    /**
     * A reference to the opened environment. This value is an instance of an
     * `Environment` if the scope is a ServiceWorker scope.
     * @type {Environment|null}
     */
    export const env: Environment | null;
    namespace _default {
        export { ExtendableEvent };
        export { FetchEvent };
        export { Environment };
        export { Context };
        export { env };
    }
    export default _default;
    import { Environment } from "socket:service-worker/env";
    import { ExtendableEvent } from "socket:service-worker/events";
    import { FetchEvent } from "socket:service-worker/events";
    import { Context } from "socket:service-worker/context";
    export { ExtendableEvent, FetchEvent, Environment, Context };
}

declare module "socket:string_decoder" {
    export function StringDecoder(encoding: any): void;
    export class StringDecoder {
        constructor(encoding: any);
        encoding: any;
        text: typeof utf16Text | typeof base64Text;
        end: typeof utf16End | typeof base64End | typeof simpleEnd;
        fillLast: typeof utf8FillLast;
        write: typeof simpleWrite;
        lastNeed: number;
        lastTotal: number;
        lastChar: Uint8Array;
    }
    export default StringDecoder;
    function utf16Text(buf: any, i: any): any;
    class utf16Text {
        constructor(buf: any, i: any);
        lastNeed: number;
        lastTotal: number;
    }
    function base64Text(buf: any, i: any): any;
    class base64Text {
        constructor(buf: any, i: any);
        lastNeed: number;
        lastTotal: number;
    }
    function utf16End(buf: any): any;
    function base64End(buf: any): any;
    function simpleEnd(buf: any): any;
    function utf8FillLast(buf: any): any;
    function simpleWrite(buf: any): any;
}

declare module "socket:test/context" {
    export default function _default(GLOBAL_TEST_RUNNER: any): void;
}

declare module "socket:test/dom-helpers" {
    /**
     * Converts querySelector string to an HTMLElement or validates an existing HTMLElement.
     *
     * @export
     * @param {string|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
     * @returns {Element} The HTMLElement, Element, or Window that corresponds to the selector.
     * @throws {Error} Throws an error if the `selector` is not a string that resolves to an HTMLElement or not an instance of HTMLElement, Element, or Window.
     *
     */
    export function toElement(selector: string | Element): Element;
    /**
     * Waits for an element to appear in the DOM and resolves the promise when it does.
     *
     * @export
     * @param {Object} args - Configuration arguments.
     * @param {string} [args.selector] - The CSS selector to look for.
     * @param {boolean} [args.visible=true] - Whether the element should be visible.
     * @param {number} [args.timeout=defaultTimeout] - Time in milliseconds to wait before rejecting the promise.
     * @param {() => HTMLElement | Element | null | undefined} [lambda] - An optional function that returns the element. Used if the `selector` is not provided.
     * @returns {Promise<Element|HTMLElement|void>} - A promise that resolves to the found element.
     *
     * @throws {Error} - Throws an error if neither `lambda` nor `selector` is provided.
     * @throws {Error} - Throws an error if the element is not found within the timeout.
     *
     * @example
     * ```js
     * waitFor({ selector: '#my-element', visible: true, timeout: 5000 })
     *   .then(el => console.log('Element found:', el))
     *   .catch(err => console.log('Element not found:', err));
     * ```
     */
    export function waitFor(args: {
        selector?: string;
        visible?: boolean;
        timeout?: number;
    }, lambda?: () => HTMLElement | Element | null | undefined): Promise<Element | HTMLElement | void>;
    /**
     * Waits for an element's text content to match a given string or regular expression.
     *
     * @export
     * @param {Object} args - Configuration arguments.
     * @param {Element} args.element - The root element from which to begin searching.
     * @param {string} [args.text] - The text to search for within elements.
     * @param {RegExp} [args.regex] - A regular expression to match against element text content.
     * @param {boolean} [args.multipleTags=false] - Whether to look for text across multiple sibling elements.
     * @param {number} [args.timeout=defaultTimeout] - Time in milliseconds to wait before rejecting the promise.
     * @returns {Promise<Element|HTMLElement|void>} - A promise that resolves to the found element or null.
     *
     * @example
     * ```js
     * waitForText({ element: document.body, text: 'Hello', timeout: 5000 })
     *   .then(el => console.log('Element found:', el))
     *   .catch(err => console.log('Element not found:', err));
     * ```
     */
    export function waitForText(args: {
        element: Element;
        text?: string;
        regex?: RegExp;
        multipleTags?: boolean;
        timeout?: number;
    }): Promise<Element | HTMLElement | void>;
    /**
     * @export
     * @param {Object} args - Arguments
     * @param {string | Event} args.event - The event to dispatch.
     * @param {HTMLElement | Element | window} [args.element=window] - The element to dispatch the event on.
     * @returns {void}
     *
     * @throws {Error} Throws an error if the `event` is not a string that can be converted to a CustomEvent or not an instance of Event.
     */
    export function event(args: {
        event: string | Event;
        element?: HTMLElement | Element | (Window & typeof globalThis);
    }): void;
    /**
     * @export
     * Copy pasted from https://raw.githubusercontent.com/testing-library/jest-dom/master/src/to-be-visible.js
     * @param {Element | HTMLElement} element
     * @param {Element | HTMLElement} [previousElement]
     * @returns {boolean}
     */
    export function isElementVisible(element: Element | HTMLElement, previousElement?: Element | HTMLElement): boolean;
}

declare module "socket:test/index" {
    /**
     * @returns {number} - The default timeout for tests in milliseconds.
     */
    export function getDefaultTestRunnerTimeout(): number;
    /**
     * @param {string} name
     * @param {TestFn} [fn]
     * @returns {void}
     */
    export function only(name: string, fn?: TestFn): void;
    /**
     * @param {string} _name
     * @param {TestFn} [_fn]
     * @returns {void}
     */
    export function skip(_name: string, _fn?: TestFn): void;
    /**
     * @param {boolean} strict
     * @returns {void}
     */
    export function setStrict(strict: boolean): void;
    /**
     * @typedef {{
     *    (name: string, fn?: TestFn): void
     *    only(name: string, fn?: TestFn): void
     *    skip(name: string, fn?: TestFn): void
     * }} testWithProperties
     * @ignore
     */
    /**
     * @type {testWithProperties}
     * @param {string} name
     * @param {TestFn} [fn]
     * @returns {void}
     */
    export function test(name: string, fn?: TestFn): void;
    export namespace test {
        export { only };
        export { skip };
        export function linux(name: any, fn: any): void;
        export function windows(name: any, fn: any): void;
        export function win32(name: any, fn: any): void;
        export function unix(name: any, fn: any): void;
        export function macosx(name: any, fn: any): void;
        export function macos(name: any, fn: any): void;
        export function mac(name: any, fn: any): void;
        export function darwin(name: any, fn: any): void;
        export function iphone(name: any, fn: any): void;
        export namespace iphone {
            function simulator(name: any, fn: any): void;
        }
        export function ios(name: any, fn: any): void;
        export namespace ios {
            function simulator(name: any, fn: any): void;
        }
        export function android(name: any, fn: any): void;
        export namespace android {
            function emulator(name: any, fn: any): void;
        }
        export function desktop(name: any, fn: any): void;
        export function mobile(name: any, fn: any): void;
    }
    /**
     * @typedef {(t: Test) => (void | Promise<void>)} TestFn
     */
    /**
     * @class
     */
    export class Test {
        /**
         * @constructor
         * @param {string} name
         * @param {TestFn} fn
         * @param {TestRunner} runner
         */
        constructor(name: string, fn: TestFn, runner: TestRunner);
        /**
         * @type {string}
         * @ignore
         */
        name: string;
        /**
         * @type {null|number}
         * @ignore
         */
        _planned: null | number;
        /**
         * @type {null|number}
         * @ignore
         */
        _actual: null | number;
        /**
         * @type {TestFn}
         * @ignore
         */
        fn: TestFn;
        /**
         * @type {TestRunner}
         * @ignore
         */
        runner: TestRunner;
        /**
         * @type{{ pass: number, fail: number }}
         * @ignore
         */
        _result: {
            pass: number;
            fail: number;
        };
        /**
         * @type {boolean}
         * @ignore
         */
        done: boolean;
        /**
         * @type {boolean}
         * @ignore
         */
        strict: boolean;
        /**
         * @param {string} msg
         * @returns {void}
         */
        comment(msg: string): void;
        /**
         * Plan the number of assertions.
         *
         * @param {number} n
         * @returns {void}
         */
        plan(n: number): void;
        /**
         * @template T
         * @param {T} actual
         * @param {T} expected
         * @param {string} [msg]
         * @returns {void}
         */
        deepEqual<T>(actual: T, expected: T, msg?: string): void;
        /**
         * @template T
         * @param {T} actual
         * @param {T} expected
         * @param {string} [msg]
         * @returns {void}
         */
        notDeepEqual<T>(actual: T, expected: T, msg?: string): void;
        /**
         * @template T
         * @param {T} actual
         * @param {T} expected
         * @param {string} [msg]
         * @returns {void}
         */
        equal<T>(actual: T, expected: T, msg?: string): void;
        /**
         * @param {unknown} actual
         * @param {unknown} expected
         * @param {string} [msg]
         * @returns {void}
         */
        notEqual(actual: unknown, expected: unknown, msg?: string): void;
        /**
         * @param {string} [msg]
         * @returns {void}
         */
        fail(msg?: string): void;
        /**
         * @param {unknown} actual
         * @param {string} [msg]
         * @returns {void}
         */
        ok(actual: unknown, msg?: string): void;
        /**
         * @param {string} [msg]
         * @returns {void}
         */
        pass(msg?: string): void;
        /**
         * @param {Error | null | undefined} err
         * @param {string} [msg]
         * @returns {void}
         */
        ifError(err: Error | null | undefined, msg?: string): void;
        /**
         * @param {Function} fn
         * @param {RegExp | any} [expected]
         * @param {string} [message]
         * @returns {void}
         */
        throws(fn: Function, expected?: RegExp | any, message?: string): void;
        /**
         * Sleep for ms with an optional msg
         *
         * @param {number} ms
         * @param {string} [msg]
         * @returns {Promise<void>}
         *
         * @example
         * ```js
         * await t.sleep(100)
         * ```
         */
        sleep(ms: number, msg?: string): Promise<void>;
        /**
         * Request animation frame with an optional msg. Falls back to a 0ms setTimeout when
         * tests are run headlessly.
         *
         * @param {string} [msg]
         * @returns {Promise<void>}
         *
         * @example
         * ```js
         * await t.requestAnimationFrame()
         * ```
         */
        requestAnimationFrame(msg?: string): Promise<void>;
        /**
         * Dispatch the `click`` method on an element specified by selector.
         *
         * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
         * @param {string} [msg]
         * @returns {Promise<void>}
         *
         * @example
         * ```js
         * await t.click('.class button', 'Click a button')
         * ```
         */
        click(selector: string | HTMLElement | Element, msg?: string): Promise<void>;
        /**
         * Dispatch the click window.MouseEvent on an element specified by selector.
         *
         * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
         * @param {string} [msg]
         * @returns {Promise<void>}
         *
         * @example
         * ```js
         * await t.eventClick('.class button', 'Click a button with an event')
         * ```
         */
        eventClick(selector: string | HTMLElement | Element, msg?: string): Promise<void>;
        /**
         *  Dispatch an event on the target.
         *
         * @param {string | Event} event - The event name or Event instance to dispatch.
         * @param {string|HTMLElement|Element} target - A CSS selector string, or an instance of HTMLElement, or Element to dispatch the event on.
         * @param {string} [msg]
         * @returns {Promise<void>}
         *
         * @example
         * ```js
         * await t.dispatchEvent('my-event', '#my-div', 'Fire the my-event event')
         * ```
         */
        dispatchEvent(event: string | Event, target: string | HTMLElement | Element, msg?: string): Promise<void>;
        /**
         *  Call the focus method on element specified by selector.
         *
         * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
         * @param {string} [msg]
         * @returns {Promise<void>}
         *
         * @example
         * ```js
         * await t.focus('#my-div')
         * ```
         */
        focus(selector: string | HTMLElement | Element, msg?: string): Promise<void>;
        /**
         *  Call the blur method on element specified by selector.
         *
         * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
         * @param {string} [msg]
         * @returns {Promise<void>}
         *
         * @example
         * ```js
         * await t.blur('#my-div')
         * ```
         */
        blur(selector: string | HTMLElement | Element, msg?: string): Promise<void>;
        /**
         * Consecutively set the str value of the element specified by selector to simulate typing.
         *
         * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
         * @param {string} str - The string to type into the :focus element.
         * @param {string} [msg]
         * @returns {Promise<void>}
         *
         * @example
         * ```js
         * await t.typeValue('#my-div', 'Hello World', 'Type "Hello World" into #my-div')
         * ```
         */
        type(selector: string | HTMLElement | Element, str: string, msg?: string): Promise<void>;
        /**
         * appendChild an element el to a parent selector element.
         *
         * @param {string|HTMLElement|Element} parentSelector - A CSS selector string, or an instance of HTMLElement, or Element to appendChild on.
         * @param {HTMLElement|Element} el - A element to append to the parent element.
         * @param {string} [msg]
         * @returns {Promise<void>}
         *
         * @example
         * ```js
         * const myElement = createElement('div')
         * await t.appendChild('#parent-selector', myElement, 'Append myElement into #parent-selector')
         * ```
         */
        appendChild(parentSelector: string | HTMLElement | Element, el: HTMLElement | Element, msg?: string): Promise<void>;
        /**
         * Remove an element from the DOM.
         *
         * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element to remove from the DOM.
         * @param {string} [msg]
         * @returns {Promise<void>}
         *
         * @example
         * ```js
         * await t.removeElement('#dom-selector', 'Remove #dom-selector')
         * ```
         */
        removeElement(selector: string | HTMLElement | Element, msg?: string): Promise<void>;
        /**
         * Test if an element is visible
         *
         * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element to test visibility on.
         * @param {string} [msg]
         * @returns {Promise<void>}
         *
         * @example
         * ```js
         * await t.elementVisible('#dom-selector','Element is visible')
         * ```
         */
        elementVisible(selector: string | HTMLElement | Element, msg?: string): Promise<void>;
        /**
         * Test if an element is invisible
         *
         * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element to test visibility on.
         * @param {string} [msg]
         * @returns {Promise<void>}
         *
         * @example
         * ```js
         * await t.elementInvisible('#dom-selector','Element is invisible')
         * ```
         */
        elementInvisible(selector: string | HTMLElement | Element, msg?: string): Promise<void>;
        /**
         * Test if an element is invisible
         *
         * @param {string|(() => HTMLElement|Element|null|undefined)} querySelectorOrFn - A query string or a function that returns an element.
         * @param {Object} [opts]
         * @param {boolean} [opts.visible] - The element needs to be visible.
         * @param {number} [opts.timeout] - The maximum amount of time to wait.
         * @param {string} [msg]
         * @returns {Promise<HTMLElement|Element|void>}
         *
         * @example
         * ```js
         * await t.waitFor('#dom-selector', { visible: true },'#dom-selector is on the page and visible')
         * ```
         */
        waitFor(querySelectorOrFn: string | (() => HTMLElement | Element | null | undefined), opts?: {
            visible?: boolean;
            timeout?: number;
        }, msg?: string): Promise<HTMLElement | Element | void>;
        /**
         * @typedef {Object} WaitForTextOpts
         * @property {string} [text] - The text to wait for
         * @property {number} [timeout]
         * @property {Boolean} [multipleTags]
         * @property {RegExp} [regex] The regex to wait for
         */
        /**
         * Test if an element is invisible
         *
         * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
         * @param {WaitForTextOpts | string | RegExp} [opts]
         * @param {string} [msg]
         * @returns {Promise<HTMLElement|Element|void>}
         *
         * @example
         * ```js
         * await t.waitForText('#dom-selector', 'Text to wait for')
         * ```
         *
         * @example
         * ```js
         * await t.waitForText('#dom-selector', /hello/i)
         * ```
         *
         * @example
         * ```js
         * await t.waitForText('#dom-selector', {
         *   text: 'Text to wait for',
         *   multipleTags: true
         * })
         * ```
         */
        waitForText(selector: string | HTMLElement | Element, opts?: {
            /**
             * - The text to wait for
             */
            text?: string;
            timeout?: number;
            multipleTags?: boolean;
            /**
             * The regex to wait for
             */
            regex?: RegExp;
        } | string | RegExp, msg?: string): Promise<HTMLElement | Element | void>;
        /**
         * Run a querySelector as an assert and also get the results
         *
         * @param {string} selector - A CSS selector string, or an instance of HTMLElement, or Element to select.
         * @param {string} [msg]
         * @returns {HTMLElement | Element}
         *
         * @example
         * ```js
         * const element = await t.querySelector('#dom-selector')
         * ```
         */
        querySelector(selector: string, msg?: string): HTMLElement | Element;
        /**
         * Run a querySelectorAll as an assert and also get the results
         *
         * @param {string} selector - A CSS selector string, or an instance of HTMLElement, or Element to select.
         * @param {string} [msg]
         @returns {Array<HTMLElement | Element>}
         *
         * @example
         * ```js
         * const elements = await t.querySelectorAll('#dom-selector', '')
         * ```
         */
        querySelectorAll(selector: string, msg?: string): Array<HTMLElement | Element>;
        /**
         * Retrieves the computed styles for a given element.
         *
         * @param {string|Element} selector - The CSS selector or the Element object for which to get the computed styles.
         * @param {string} [msg] - An optional message to display when the operation is successful. Default message will be generated based on the type of selector.
         * @returns {CSSStyleDeclaration} - The computed styles of the element.
         * @throws {Error} - Throws an error if the element has no `ownerDocument` or if `ownerDocument.defaultView` is not available.
         *
         * @example
         * ```js
         * // Using CSS selector
         * const style = getComputedStyle('.my-element', 'Custom success message');
         * ```
         *
         * @example
         * ```js
         * // Using Element object
         * const el = document.querySelector('.my-element');
         * const style = getComputedStyle(el);
         * ```
         */
        getComputedStyle(selector: string | Element, msg?: string): CSSStyleDeclaration;
        /**
         * @param {boolean} pass
         * @param {unknown} actual
         * @param {unknown} expected
         * @param {string} description
         * @param {string} operator
         * @returns {void}
         * @ignore
         */
        _assert(pass: boolean, actual: unknown, expected: unknown, description: string, operator: string): void;
        /**
         * @returns {Promise<{
         *   pass: number,
         *   fail: number
         * }>}
         */
        run(): Promise<{
            pass: number;
            fail: number;
        }>;
    }
    /**
     * @class
     */
    export class TestRunner {
        /**
         * @constructor
         * @param {(lines: string) => void} [report]
         */
        constructor(report?: (lines: string) => void);
        /**
         * @type {(lines: string) => void}
         * @ignore
         */
        report: (lines: string) => void;
        /**
         * @type {Test[]}
         * @ignore
         */
        tests: Test[];
        /**
         * @type {Test[]}
         * @ignore
         */
        onlyTests: Test[];
        /**
         * @type {boolean}
         * @ignore
         */
        scheduled: boolean;
        /**
         * @type {number}
         * @ignore
         */
        _id: number;
        /**
         * @type {boolean}
         * @ignore
         */
        completed: boolean;
        /**
         * @type {boolean}
         * @ignore
         */
        rethrowExceptions: boolean;
        /**
         * @type {boolean}
         * @ignore
         */
        strict: boolean;
        /**
         * @type {function | void}
         * @ignore
         */
        _onFinishCallback: Function | void;
        /**
         * @returns {string}
         */
        nextId(): string;
        /**
         * @type {number}
         */
        get length(): number;
        /**
         * @param {string} name
         * @param {TestFn} fn
         * @param {boolean} only
         * @returns {void}
         */
        add(name: string, fn: TestFn, only: boolean): void;
        /**
         * @returns {Promise<void>}
         */
        run(): Promise<void>;
        /**
         * @param {(result: { total: number, success: number, fail: number }) => void} callback
         * @returns {void}
         */
        onFinish(callback: (result: {
            total: number;
            success: number;
            fail: number;
        }) => void): void;
    }
    /**
     * @ignore
     */
    export const GLOBAL_TEST_RUNNER: TestRunner;
    export default test;
    export type testWithProperties = {
        (name: string, fn?: TestFn): void;
        only(name: string, fn?: TestFn): void;
        skip(name: string, fn?: TestFn): void;
    };
    export type TestFn = (t: Test) => (void | Promise<void>);
}

declare module "socket:test" {
    export * from "socket:test/index";
    export default test;
    import test from "socket:test/index";
}

declare module "socket:commonjs/builtins" {
    /**
     * Defines a builtin module by name making a shallow copy of the
     * module exports.
     * @param {string}
     * @param {object} exports
     */
    export function defineBuiltin(name: any, exports: object, copy?: boolean): void;
    /**
     * Predicate to determine if a given module name is a builtin module.
     * @param {string} name
     * @param {{ builtins?: object }}
     * @return {boolean}
     */
    export function isBuiltin(name: string, options?: any): boolean;
    /**
     * Gets a builtin module by name.
     * @param {string} name
     * @param {{ builtins?: object }} [options]
     * @return {any}
     */
    export function getBuiltin(name: string, options?: {
        builtins?: object;
    }): any;
    /**
     * A mapping of builtin modules
     * @type {object}
     */
    export const builtins: object;
    /**
     * Known runtime specific builtin modules.
     * @type {Set<string>}
     */
    export const runtimeModules: Set<string>;
    export default builtins;
}

declare module "socket:commonjs/cache" {
    /**
     * @typedef {{
     *   types?: object,
     *   loader?: import('./loader.js').Loader
     * }} CacheOptions
     */
    export const CACHE_CHANNEL_MESSAGE_ID: "id";
    export const CACHE_CHANNEL_MESSAGE_REPLICATE: "replicate";
    /**
     * @typedef {{
     *   name: string
     * }} StorageOptions
     */
    /**
     * An storage context object with persistence and durability
     * for service worker storages.
     */
    export class Storage extends EventTarget {
        /**
         * Maximum entries that will be restored from storage into the context object.
         * @type {number}
         */
        static MAX_CONTEXT_ENTRIES: number;
        /**
         * A mapping of known `Storage` instances.
         * @type {Map<string, Storage>}
         */
        static instances: Map<string, Storage>;
        /**
         * Opens an storage for a particular name.
         * @param {StorageOptions} options
         * @return {Promise<Storage>}
         */
        static open(options: StorageOptions): Promise<Storage>;
        /**
         * `Storage` class constructor
         * @ignore
         * @param {StorageOptions} options
         */
        constructor(options: StorageOptions);
        /**
         * A reference to the currently opened storage database.
         * @type {import('../internal/database.js').Database}
         */
        get database(): import("socket:internal/database").Database;
        /**
         * `true` if the storage is opened, otherwise `false`.
         * @type {boolean}
         */
        get opened(): boolean;
        /**
         * `true` if the storage is opening, otherwise `false`.
         * @type {boolean}
         */
        get opening(): boolean;
        /**
         * A proxied object for reading and writing storage state.
         * Values written to this object must be cloneable with respect to the
         * structured clone algorithm.
         * @see {https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm}
         * @type {Proxy<object>}
         */
        get context(): ProxyConstructor;
        /**
         * The current storage name. This value is also used as the
         * internal database name.
         * @type {string}
         */
        get name(): string;
        /**
         * A promise that resolves when the storage is opened.
         * @type {Promise?}
         */
        get ready(): Promise<any>;
        /**
         * @ignore
         * @param {Promise} promise
         */
        forwardRequest(promise: Promise<any>): Promise<any>;
        /**
         * Resets the current storage to an empty state.
         */
        reset(): Promise<void>;
        /**
         * Synchronizes database entries into the storage context.
         */
        sync(options?: any): Promise<void>;
        /**
         * Opens the storage.
         * @ignore
         */
        open(options?: any): Promise<any>;
        /**
         * Closes the storage database, purging existing state.
         * @ignore
         */
        close(): Promise<void>;
        #private;
    }
    /**
     * A container for `Snapshot` data storage.
     */
    export class SnapshotData {
        /**
         * `SnapshotData` class constructor.
         * @param {object=} [data]
         */
        constructor(data?: object | undefined);
        toJSON: () => this;
        [Symbol.toStringTag]: string;
    }
    /**
     * A container for storing a snapshot of the cache data.
     */
    export class Snapshot {
        /**
         * @type {typeof SnapshotData}
         */
        static Data: typeof SnapshotData;
        /**
         * A reference to the snapshot data.
         * @type {Snapshot.Data}
         */
        get data(): typeof SnapshotData;
        /**
         * @ignore
         * @return {object}
         */
        toJSON(): object;
        #private;
    }
    /**
     * An interface for managing and performing operations on a collection
     * of `Cache` objects.
     */
    export class CacheCollection {
        /**
         * `CacheCollection` class constructor.
         * @ignore
         * @param {Cache[]|Record<string, Cache>=} [collection]
         */
        constructor(collection?: (Cache[] | Record<string, Cache>) | undefined);
        /**
         * Adds a `Cache` instance to the collection.
         * @param {string|Cache} name
         * @param {Cache=} [cache]
         * @param {boolean}
         */
        add(name: string | Cache, cache?: Cache | undefined): any;
        /**
         * Calls a method on each `Cache` object in the collection.
         * @param {string} method
         * @param {...any} args
         * @return {Promise<Record<string,any>>}
         */
        call(method: string, ...args: any[]): Promise<Record<string, any>>;
        restore(): Promise<Record<string, any>>;
        reset(): Promise<Record<string, any>>;
        snapshot(): Promise<Record<string, any>>;
        get(key: any): Promise<Record<string, any>>;
        delete(key: any): Promise<Record<string, any>>;
        keys(key: any): Promise<Record<string, any>>;
        values(key: any): Promise<Record<string, any>>;
        clear(key: any): Promise<Record<string, any>>;
    }
    /**
     * A container for a shared cache that lives for the life time of
     * application execution. Updates to this storage are replicated to other
     * instances in the application context, including windows and workers.
     */
    export class Cache {
        /**
         * A globally shared type mapping for the cache to use when
         * derserializing a value.
         * @type {Map<string, function>}
         */
        static types: Map<string, Function>;
        /**
         * A globally shared cache store keyed by cache name. This is useful so
         * when multiple instances of a `Cache` are created, they can share the
         * same data store, reducing duplications.
         * @type {Record<string, Map<string, object>}
         */
        static shared: Record<string, Map<string, object>>;
        /**
         * A mapping of opened `Storage` instances.
         * @type {Map<string, Storage>}
         */
        static storages: Map<string, Storage>;
        /**
         * The `Cache.Snapshot` class.
         * @type {typeof Snapshot}
         */
        static Snapshot: typeof Snapshot;
        /**
         * The `Cache.Storage` class
         * @type {typeof Storage}
         */
        static Storage: typeof Storage;
        /**
         * Creates a snapshot of the current cache which can be serialized and
         * stored in persistent storage.
         * @return {Snapshot}
         */
        static snapshot(): Snapshot;
        /**
         * Restore caches from persistent storage.
         * @param {string[]} names
         * @return {Promise}
         */
        static restore(names: string[]): Promise<any>;
        /**
         * `Cache` class constructor.
         * @param {string} name
         * @param {CacheOptions=} [options]
         */
        constructor(name: string, options?: CacheOptions | undefined);
        /**
         * The unique ID for this cache.
         * @type {string}
         */
        get id(): string;
        /**
         * The loader associated with this cache.
         * @type {import('./loader.js').Loader}
         */
        get loader(): import("socket:commonjs/loader").Loader;
        /**
         * A reference to the persisted storage.
         * @type {Storage}
         */
        get storage(): Storage;
        /**
         * The cache name
         * @type {string}
         */
        get name(): string;
        /**
         * The underlying cache data map.
         * @type {Map}
         */
        get data(): Map<any, any>;
        /**
         * The broadcast channel associated with this cach.
         * @type {BroadcastChannel}
         */
        get channel(): BroadcastChannel;
        /**
         * The size of the cache.
         * @type {number}
         */
        get size(): number;
        /**
         * @type {Map}
         */
        get types(): Map<any, any>;
        /**
         * Resets the cache map and persisted storage.
         */
        reset(): Promise<void>;
        /**
         * Restores cache data from storage.
         */
        restore(): Promise<void>;
        /**
         * Creates a snapshot of the current cache which can be serialized and
         * stored in persistent storage.
         * @return {Snapshot.Data}
         */
        snapshot(): typeof SnapshotData;
        /**
         * Get a value at `key`.
         * @param {string} key
         * @return {object|undefined}
         */
        get(key: string): object | undefined;
        /**
         * Set `value` at `key`.
         * @param {string} key
         * @param {object} value
         * @return {Cache}
         */
        set(key: string, value: object): Cache;
        /**
         * Returns `true` if `key` is in cache, otherwise `false`.
         * @param {string}
         * @return {boolean}
         */
        has(key: any): boolean;
        /**
         * Delete a value at `key`.
         * This does not replicate to shared caches.
         * @param {string} key
         * @return {boolean}
         */
        delete(key: string): boolean;
        /**
         * Returns an iterator for all cache keys.
         * @return {object}
         */
        keys(): object;
        /**
         * Returns an iterator for all cache values.
         * @return {object}
         */
        values(): object;
        /**
         * Returns an iterator for all cache entries.
         * @return {object}
         */
        entries(): object;
        /**
         * Clears all entries in the cache.
         * This does not replicate to shared caches.
         * @return {undefined}
         */
        clear(): undefined;
        /**
         * Enumerates entries in map calling `callback(value, key
         * @param {function(object, string, Cache): any} callback
         */
        forEach(callback: (arg0: object, arg1: string, arg2: Cache) => any): void;
        /**
         * Broadcasts a replication to other shared caches.
         */
        replicate(): this;
        /**
         * Destroys the cache. This function stops the broadcast channel and removes
         * and listeners
         */
        destroy(): void;
        /**
         * @ignore
         */
        [Symbol.iterator](): any;
        #private;
    }
    export default Cache;
    export type CacheOptions = {
        types?: object;
        loader?: import("socket:commonjs/loader").Loader;
    };
    export type StorageOptions = {
        name: string;
    };
}

declare module "socket:commonjs/loader" {
    /**
     * @typedef {{
     *   extensions?: string[] | Set<string>
     *   origin?: URL | string,
     *   statuses?: Cache
     *   cache?: { response?: Cache, status?: Cache },
     *   headers?: Headers | Map | object | string[][]
     * }} LoaderOptions
     */
    /**
     * @typedef {{
     *   loader?: Loader,
     *   origin?: URL | string
     * }} RequestOptions
     */
    /**
     * @typedef {{
     *   headers?: Headers | object | array[],
     *   status?: number
     * }} RequestStatusOptions
     */
    /**
     * @typedef {{
     *   headers?: Headers | object
     * }} RequestLoadOptions
     */
    /**
     * @typedef {{
     *   request?: Request,
     *   headers?: Headers,
     *   status?: number,
     *   buffer?: ArrayBuffer,
     *   text?: string
     * }} ResponseOptions
     */
    /**
     * A container for the status of a CommonJS resource. A `RequestStatus` object
     * represents meta data for a `Request` that comes from a preflight
     * HTTP HEAD request.
     */
    export class RequestStatus {
        /**
         * Creates a `RequestStatus` from JSON input.
         * @param {object} json
         * @return {RequestStatus}
         */
        static from(json: object, options: any): RequestStatus;
        /**
         * `RequestStatus` class constructor.
         * @param {Request} request
         * @param {RequestStatusOptions} [options]
         */
        constructor(request: Request, options?: RequestStatusOptions);
        set request(request: Request);
        /**
         * The `Request` object associated with this `RequestStatus` object.
         * @type {Request}
         */
        get request(): Request;
        /**
         * The unique ID of this `RequestStatus`, which is the absolute URL as a string.
         * @type {string}
         */
        get id(): string;
        /**
         * The origin for this `RequestStatus` object.
         * @type {string}
         */
        get origin(): string;
        /**
         * A HTTP status code for this `RequestStatus` object.
         * @type {number|undefined}
         */
        get status(): number;
        /**
         * An alias for `status`.
         * @type {number|undefined}
         */
        get value(): number;
        /**
         * @ignore
         */
        get valueOf(): number;
        /**
         * The HTTP headers for this `RequestStatus` object.
         * @type {Headers}
         */
        get headers(): Headers;
        /**
         * The resource location for this `RequestStatus` object. This value is
         * determined from the 'Content-Location' header, if available, otherwise
         * it is derived from the request URL pathname (including the query string).
         * @type {string}
         */
        get location(): string;
        /**
         * `true` if the response status is considered OK, otherwise `false`.
         * @type {boolean}
         */
        get ok(): boolean;
        /**
         * Loads the internal state for this `RequestStatus` object.
         * @param {RequestLoadOptions|boolean} [options]
         * @return {RequestStatus}
         */
        load(options?: RequestLoadOptions | boolean): RequestStatus;
        /**
         * Converts this `RequestStatus` to JSON.
         * @ignore
         * @return {{
         *   id: string,
         *   origin: string | null,
         *   status: number,
         *   headers: Array<string[]>
         *   request: object | null | undefined
         * }}
         */
        toJSON(includeRequest?: boolean): {
            id: string;
            origin: string | null;
            status: number;
            headers: Array<string[]>;
            request: object | null | undefined;
        };
        #private;
    }
    /**
     * A container for a synchronous CommonJS request to local resource or
     * over the network.
     */
    export class Request {
        /**
         * Creates a `Request` instance from JSON input
         * @param {object} json
         * @param {RequestOptions=} [options]
         * @return {Request}
         */
        static from(json: object, options?: RequestOptions | undefined): Request;
        /**
         * `Request` class constructor.
         * @param {URL|string} url
         * @param {URL|string=} [origin]
         * @param {RequestOptions=} [options]
         */
        constructor(url: URL | string, origin?: (URL | string) | undefined, options?: RequestOptions | undefined);
        /**
         * The unique ID of this `Request`, which is the absolute URL as a string.
         * @type {string}
         */
        get id(): string;
        /**
         * The absolute `URL` of this `Request` object.
         * @type {URL}
         */
        get url(): URL;
        /**
         * The origin for this `Request`.
         * @type {string}
         */
        get origin(): string;
        /**
         * The `Loader` for this `Request` object.
         * @type {Loader?}
         */
        get loader(): Loader;
        /**
         * The `RequestStatus` for this `Request`
         * @type {RequestStatus}
         */
        get status(): RequestStatus;
        /**
         * Loads the CommonJS source file, optionally checking the `Loader` cache
         * first, unless ignored when `options.cache` is `false`.
         * @param {RequestLoadOptions=} [options]
         * @return {Response}
         */
        load(options?: RequestLoadOptions | undefined): Response;
        /**
         * Converts this `Request` to JSON.
         * @ignore
         * @return {{
         *   url: string,
         *   status: object | undefined
         * }}
         */
        toJSON(includeStatus?: boolean): {
            url: string;
            status: object | undefined;
        };
        #private;
    }
    /**
     * A container for a synchronous CommonJS request response for a local resource
     * or over the network.
     */
    export class Response {
        /**
         * Creates a `Response` from JSON input
         * @param {obejct} json
         * @param {ResponseOptions=} [options]
         * @return {Response}
         */
        static from(json: obejct, options?: ResponseOptions | undefined): Response;
        /**
         * `Response` class constructor.
         * @param {Request|ResponseOptions} request
         * @param {ResponseOptions=} [options]
         */
        constructor(request: Request | ResponseOptions, options?: ResponseOptions | undefined);
        /**
         * The unique ID of this `Response`, which is the absolute
         * URL of the request as a string.
         * @type {string}
         */
        get id(): string;
        /**
         * The `Request` object associated with this `Response` object.
         * @type {Request}
         */
        get request(): Request;
        /**
         * The response headers from the associated request.
         * @type {Headers}
         */
        get headers(): Headers;
        /**
         * The `Loader` associated with this `Response` object.
         * @type {Loader?}
         */
        get loader(): Loader;
        /**
         * The `Response` status code from the associated `Request` object.
         * @type {number}
         */
        get status(): number;
        /**
         * The `Response` string from the associated `Request`
         * @type {string}
         */
        get text(): string;
        /**
         * The `Response` array buffer from the associated `Request`
         * @type {ArrayBuffer?}
         */
        get buffer(): ArrayBuffer;
        /**
         * `true` if the response is considered OK, otherwise `false`.
         * @type {boolean}
         */
        get ok(): boolean;
        /**
         * Converts this `Response` to JSON.
         * @ignore
         * @return {{
         *   id: string,
         *   text: string,
         *   status: number,
         *   buffer: number[] | null,
         *   headers: Array<string[]>
         * }}
         */
        toJSON(): {
            id: string;
            text: string;
            status: number;
            buffer: number[] | null;
            headers: Array<string[]>;
        };
        #private;
    }
    /**
     * A container for loading CommonJS module sources
     */
    export class Loader {
        /**
         * A request class used by `Loader` objects.
         * @type {typeof Request}
         */
        static Request: typeof Request;
        /**
         * A response class used by `Loader` objects.
         * @type {typeof Request}
         */
        static Response: typeof Request;
        /**
         * Resolves a given module URL to an absolute URL with an optional `origin`.
         * @param {URL|string} url
         * @param {URL|string} [origin]
         * @return {string}
         */
        static resolve(url: URL | string, origin?: URL | string): string;
        /**
         * Default extensions for a loader.
         * @type {Set<string>}
         */
        static defaultExtensions: Set<string>;
        /**
         * `Loader` class constructor.
         * @param {string|URL|LoaderOptions} origin
         * @param {LoaderOptions=} [options]
         */
        constructor(origin: string | URL | LoaderOptions, options?: LoaderOptions | undefined);
        /**
         * The internal caches for this `Loader` object.
         * @type {{ response: Cache, status: Cache }}
         */
        get cache(): {
            response: Cache;
            status: Cache;
        };
        /**
         * Headers used in too loader requests.
         * @type {Headers}
         */
        get headers(): Headers;
        /**
         * A set of supported `Loader` extensions.
         * @type {Set<string>}
         */
        get extensions(): Set<string>;
        set origin(origin: string);
        /**
         * The origin of this `Loader` object.
         * @type {string}
         */
        get origin(): string;
        /**
         * Loads a CommonJS module source file at `url` with an optional `origin`, which
         * defaults to the application origin.
         * @param {URL|string} url
         * @param {URL|string|object} [origin]
         * @param {RequestOptions=} [options]
         * @return {Response}
         */
        load(url: URL | string, origin?: URL | string | object, options?: RequestOptions | undefined): Response;
        /**
         * Queries the status of a CommonJS module source file at `url` with an
         * optional `origin`, which defaults to the application origin.
         * @param {URL|string} url
         * @param {URL|string|object} [origin]
         * @param {RequestOptions=} [options]
         * @return {RequestStatus}
         */
        status(url: URL | string, origin?: URL | string | object, options?: RequestOptions | undefined): RequestStatus;
        /**
         * Resolves a given module URL to an absolute URL based on the loader origin.
         * @param {URL|string} url
         * @param {URL|string} [origin]
         * @return {string}
         */
        resolve(url: URL | string, origin?: URL | string): string;
        #private;
    }
    export default Loader;
    export type LoaderOptions = {
        extensions?: string[] | Set<string>;
        origin?: URL | string;
        statuses?: Cache;
        cache?: {
            response?: Cache;
            status?: Cache;
        };
        headers?: Headers | Map<any, any> | object | string[][];
    };
    export type RequestOptions = {
        loader?: Loader;
        origin?: URL | string;
    };
    export type RequestStatusOptions = {
        headers?: Headers | object | any[][];
        status?: number;
    };
    export type RequestLoadOptions = {
        headers?: Headers | object;
    };
    export type ResponseOptions = {
        request?: Request;
        headers?: Headers;
        status?: number;
        buffer?: ArrayBuffer;
        text?: string;
    };
    import { Headers } from "socket:ipc";
    import URL from "socket:url";
    import { Cache } from "socket:commonjs/cache";
}

declare module "socket:commonjs/package" {
    /**
     * @ignore
     * @param {string} source
     * @return {boolean}
     */
    export function detectESMSource(source: string): boolean;
    /**
     * @typedef {{
     *   manifest?: string,
     *   index?: string,
     *   description?: string,
     *   version?: string,
     *   license?: string,
     *   exports?: object,
     *   type?: 'commonjs' | 'module',
     *   info?: object,
     *   origin?: string,
     *   dependencies?: Dependencies | object | Map
     * }} PackageOptions
     */
    /**
     * @typedef {import('./loader.js').RequestOptions & {
     *   type?: 'commonjs' | 'module'
     *   prefix?: string
     * }} PackageLoadOptions
     */
    /**
     * {import('./loader.js').RequestOptions & {
     *   load?: boolean,
     *   type?: 'commonjs' | 'module',
     *   browser?: boolean,
     *   children?: string[]
     *   extensions?: string[] | Set<string>
     * }} PackageResolveOptions
     */
    /**
     * @typedef {{
     *   organization: string | null,
     *   name: string,
     *   version: string | null,
     *   pathname: string,
     *   url: URL,
     *   isRelative: boolean,
     *   hasManifest: boolean
     * }} ParsedPackageName
     */
    /**
     * @typedef {{
     *   require?: string | string[],
     *   import?: string | string[],
     *   default?: string | string[],
     *   default?: string | string[],
     *   worker?: string | string[],
     *   browser?: string | string[]
     * }} PackageExports
    
    /**
     * The default package index file such as 'index.js'
     * @type {string}
     */
    export const DEFAULT_PACKAGE_INDEX: string;
    /**
     * The default package manifest file name such as 'package.json'
     * @type {string}
     */
    export const DEFAULT_PACKAGE_MANIFEST_FILE_NAME: string;
    /**
     * The default package path prefix such as 'node_modules/'
     * @type {string}
     */
    export const DEFAULT_PACKAGE_PREFIX: string;
    /**
     * The default package version, when one is not provided
     * @type {string}
     */
    export const DEFAULT_PACKAGE_VERSION: string;
    /**
     * The default license for a package'
     * @type {string}
     */
    export const DEFAULT_LICENSE: string;
    /**
     * A container for a package name that includes a package organization identifier,
     * its fully qualified name, or for relative package names, its pathname
     */
    export class Name {
        /**
         * Parses a package name input resolving the actual module name, including an
         * organization name given. If a path includes a manifest file
         * ('package.json'), then the directory containing that file is considered a
         * valid package and it will be included in the returned value. If a relative
         * path is given, then the path is returned if it is a valid pathname. This
         * function returns `null` for bad input.
         * @param {string|URL} input
         * @param {{ origin?: string | URL, manifest?: string }=} [options]
         * @return {ParsedPackageName?}
         */
        static parse(input: string | URL, options?: {
            origin?: string | URL;
            manifest?: string;
        } | undefined): ParsedPackageName | null;
        /**
         * Returns `true` if the given `input` can be parsed by `Name.parse` or given
         * as input to the `Name` class constructor.
         * @param {string|URL} input
         * @param {{ origin?: string | URL, manifest?: string }=} [options]
         * @return {boolean}
         */
        static canParse(input: string | URL, options?: {
            origin?: string | URL;
            manifest?: string;
        } | undefined): boolean;
        /**
         * Creates a new `Name` from input.
         * @param {string|URL} input
         * @param {{ origin?: string | URL, manifest?: string }=} [options]
         * @return {Name}
         */
        static from(input: string | URL, options?: {
            origin?: string | URL;
            manifest?: string;
        } | undefined): Name;
        /**
         * `Name` class constructor.
         * @param {string|URL|NameOptions|Name} name
         * @param {{ origin?: string | URL, manifest?: string }=} [options]
         * @throws TypeError
         */
        constructor(name: string | URL | NameOptions | Name, options?: {
            origin?: string | URL;
            manifest?: string;
        } | undefined);
        /**
         * The id of this package name.
         * @type {string}
         */
        get id(): string;
        /**
         * The actual package name.
         * @type {string}
         */
        get name(): string;
        /**
         * An alias for 'name'.
         * @type {string}
         */
        get value(): string;
        /**
         * The origin of the package, if available.
         * This value may be `null`.
         * @type {string?}
         */
        get origin(): string;
        /**
         * The package version if available.
         * This value may be `null`.
         * @type {string?}
         */
        get version(): string;
        /**
         * The actual package pathname, if given in name string.
         * This value is always a string defaulting to '.' if no path
         * was given in name string.
         * @type {string}
         */
        get pathname(): string;
        /**
         * The organization name.
         * This value may be `null`.
         * @type {string?}
         */
        get organization(): string;
        /**
         * `true` if the package name was relative, otherwise `false`.
         * @type {boolean}
         */
        get isRelative(): boolean;
        /**
         * Converts this package name to a string.
         * @ignore
         * @return {string}
         */
        toString(): string;
        /**
         * Converts this `Name` instance to JSON.
         * @ignore
         * @return {object}
         */
        toJSON(): object;
        #private;
    }
    /**
     * A container for package dependencies that map a package name to a `Package` instance.
     */
    export class Dependencies {
        constructor(parent: any, options?: any);
        get map(): Map<any, any>;
        get origin(): any;
        add(name: any, info?: any): void;
        get(name: any, options?: any): any;
        entries(): IterableIterator<[any, any]>;
        keys(): IterableIterator<any>;
        values(): IterableIterator<any>;
        load(options?: any): void;
        [Symbol.iterator](): IterableIterator<[any, any]>;
        #private;
    }
    /**
     * A container for CommonJS module metadata, often in a `package.json` file.
     */
    export class Package {
        /**
         * A high level class for a package name.
         * @type {typeof Name}
         */
        static Name: typeof Name;
        /**
         * A high level container for package dependencies.
         * @type {typeof Dependencies}
         */
        static Dependencies: typeof Dependencies;
        /**
         * Creates and loads a package
         * @param {string|URL|NameOptions|Name} name
         * @param {PackageOptions & PackageLoadOptions=} [options]
         * @return {Package}
         */
        static load(name: string | URL | NameOptions | Name, options?: (PackageOptions & PackageLoadOptions) | undefined): Package;
        /**
         * `Package` class constructor.
         * @param {string|URL|NameOptions|Name} name
         * @param {PackageOptions=} [options]
         */
        constructor(name: string | URL | NameOptions | Name, options?: PackageOptions | undefined);
        /**
         * The unique ID of this `Package`, which is the absolute
         * URL of the directory that contains its manifest file.
         * @type {string}
         */
        get id(): string;
        /**
         * The absolute URL to the package manifest file
         * @type {string}
         */
        get url(): string;
        /**
         * A reference to the package subpath imports and browser mappings.
         * These values are typically used with its corresponding `Module`
         * instance require resolvers.
         * @type {object}
         */
        get imports(): any;
        /**
         * A loader for this package, if available. This value may be `null`.
         * @type {Loader}
         */
        get loader(): Loader;
        /**
         * `true` if the package was actually "loaded", otherwise `false`.
         * @type {boolean}
         */
        get loaded(): boolean;
        /**
         * The name of the package.
         * @type {string}
         */
        get name(): string;
        /**
         * The description of the package.
         * @type {string}
         */
        get description(): string;
        /**
         * The organization of the package. This value may be `null`.
         * @type {string?}
         */
        get organization(): string;
        /**
         * The license of the package.
         * @type {string}
         */
        get license(): string;
        /**
         * The version of the package.
         * @type {string}
         */
        get version(): string;
        /**
         * The origin for this package.
         * @type {string}
         */
        get origin(): string;
        /**
         * The exports mappings for the package
         * @type {object}
         */
        get exports(): any;
        /**
         * The package type.
         * @type {'commonjs'|'module'}
         */
        get type(): "module" | "commonjs";
        /**
         * The raw package metadata object.
         * @type {object?}
         */
        get info(): any;
        /**
         * @type {Dependencies}
         */
        get dependencies(): Dependencies;
        /**
         * An alias for `entry`
         * @type {string?}
         */
        get main(): string;
        /**
         * The entry to the package
         * @type {string?}
         */
        get entry(): string;
        /**
         * Load the package information at an optional `origin` with
         * optional request `options`.
         * @param {PackageLoadOptions=} [options]
         * @throws SyntaxError
         * @return {boolean}
         */
        load(origin?: any, options?: PackageLoadOptions | undefined): boolean;
        /**
         * Resolve a file's `pathname` within the package.
         * @param {string|URL} pathname
         * @param {PackageResolveOptions=} [options]
         * @return {string}
         */
        resolve(pathname: string | URL, options?: PackageResolveOptions | undefined): string;
        #private;
    }
    export default Package;
    export type PackageOptions = {
        manifest?: string;
        index?: string;
        description?: string;
        version?: string;
        license?: string;
        exports?: object;
        type?: "commonjs" | "module";
        info?: object;
        origin?: string;
        dependencies?: Dependencies | object | Map<any, any>;
    };
    export type PackageLoadOptions = import("socket:commonjs/loader").RequestOptions & {
        type?: "commonjs" | "module";
        prefix?: string;
    };
    export type ParsedPackageName = {
        organization: string | null;
        name: string;
        version: string | null;
        pathname: string;
        url: URL;
        isRelative: boolean;
        hasManifest: boolean;
    };
    /**
     * /**
     * The default package index file such as 'index.js'
     */
    export type PackageExports = {
        require?: string | string[];
        import?: string | string[];
        default?: string | string[];
        default?: string | string[];
        worker?: string | string[];
        browser?: string | string[];
    };
    import URL from "socket:url";
    import { Loader } from "socket:commonjs/loader";
}

declare module "socket:commonjs/module" {
    /**
     * CommonJS module scope with module scoped globals.
     * @ignore
     * @param {object} exports
     * @param {function(string): any} require
     * @param {Module} module
     * @param {string} __filename
     * @param {string} __dirname
     * @param {typeof process} process
     * @param {object} global
     */
    export function CommonJSModuleScope(exports: object, require: (arg0: string) => any, module: Module, __filename: string, __dirname: string, process: any, global: object): void;
    /**
     * Creates a `require` function from a given module URL.
     * @param {string|URL} url
     * @param {ModuleOptions=} [options]
     * @return {RequireFunction}
     */
    export function createRequire(url: string | URL, options?: ModuleOptions | undefined): RequireFunction;
    /**
     * @typedef {function(string, Module, function(string): any): any} ModuleResolver
     */
    /**
     * @typedef {import('./require.js').RequireFunction} RequireFunction
     */
    /**
     * @typedef {import('./package.js').PackageOptions} PackageOptions
     */
    /**
     * @typedef {{
     *   prefix?: string,
     *   request?: import('./loader.js').RequestOptions,
     *   builtins?: object
     * } CreateRequireOptions
     */
    /**
     * @typedef {{
     *   resolvers?: ModuleResolver[],
     *   importmap?: ImportMap,
     *   loader?: Loader | object,
     *   loaders?: object,
     *   package?: Package | PackageOptions
     *   parent?: Module,
     *   state?: State
     * }} ModuleOptions
     */
    /**
     * @typedef {{
     *   extensions?: object
     * }} ModuleLoadOptions
     */
    export const builtinModules: any;
    /**
     * CommonJS module scope source wrapper.
     * @type {string}
     */
    export const COMMONJS_WRAPPER: string;
    /**
     * A container for imports.
     * @see {@link https://developer.mozilla.org/en-US/docs/Web/HTML/Element/script/type/importmap}
     */
    export class ImportMap {
        set imports(imports: any);
        /**
         * The imports object for the importmap.
         * @type {object}
         */
        get imports(): any;
        /**
         * Extends the current imports object.
         * @param {object} imports
         * @return {ImportMap}
         */
        extend(importmap: any): ImportMap;
        #private;
    }
    /**
     * A container for `Module` instance state.
     */
    export class State {
        /**
         * `State` class constructor.
         * @ignore
         * @param {object|State=} [state]
         */
        constructor(state?: (object | State) | undefined);
        loading: boolean;
        loaded: boolean;
        error: any;
    }
    /**
     * The module scope for a loaded module.
     * This is a special object that is seal, frozen, and only exposes an
     * accessor the 'exports' field.
     * @ignore
     */
    export class ModuleScope {
        /**
         * `ModuleScope` class constructor.
         * @param {Module} module
         */
        constructor(module: Module);
        get id(): any;
        get filename(): any;
        get loaded(): any;
        get children(): any;
        set exports(exports: any);
        get exports(): any;
        toJSON(): {
            id: any;
            filename: any;
            children: any;
            exports: any;
        };
        #private;
    }
    /**
     * An abstract base class for loading a module.
     */
    export class ModuleLoader {
        /**
         * Creates a `ModuleLoader` instance from the `module` currently being loaded.
         * @param {Module} module
         * @param {ModuleLoadOptions=} [options]
         * @return {ModuleLoader}
         */
        static from(module: Module, options?: ModuleLoadOptions | undefined): ModuleLoader;
        /**
         * Creates a new `ModuleLoader` instance from the `module` currently
         * being loaded with the `source` string to parse and load with optional
         * `ModuleLoadOptions` options.
         * @param {Module} module
         * @param {ModuleLoadOptions=} [options]
         * @return {boolean}
         */
        static load(module: Module, options?: ModuleLoadOptions | undefined): boolean;
        /**
         * @param {Module} module
         * @param {ModuleLoadOptions=} [options]
         * @return {boolean}
         */
        load(module: Module, options?: ModuleLoadOptions | undefined): boolean;
    }
    /**
     * A JavaScript module loader
     */
    export class JavaScriptModuleLoader extends ModuleLoader {
    }
    /**
     * A JSON module loader.
     */
    export class JSONModuleLoader extends ModuleLoader {
    }
    /**
     * A WASM module loader
    
     */
    export class WASMModuleLoader extends ModuleLoader {
    }
    /**
     * A container for a loaded CommonJS module. All errors bubble
     * to the "main" module and global object (if possible).
     */
    export class Module extends EventTarget {
        /**
         * A reference to the currently scoped module.
         * @type {Module?}
         */
        static current: Module | null;
        /**
         * A reference to the previously scoped module.
         * @type {Module?}
         */
        static previous: Module | null;
        /**
         * A cache of loaded modules
         * @type {Map<string, Module>}
         */
        static cache: Map<string, Module>;
        /**
         * An array of globally available module loader resolvers.
         * @type {ModuleResolver[]}
         */
        static resolvers: ModuleResolver[];
        /**
         * Globally available 'importmap' for all loaded modules.
         * @type {ImportMap}
         * @see {@link https://developer.mozilla.org/en-US/docs/Web/HTML/Element/script/type/importmap}
         */
        static importmap: ImportMap;
        /**
         * A limited set of builtins exposed to CommonJS modules.
         * @type {object}
         */
        static builtins: object;
        /**
         * A limited set of builtins exposed to CommonJS modules.
         * @type {object}
         */
        static builtinModules: object;
        /**
         * CommonJS module scope source wrapper components.
         * @type {string[]}
         */
        static wrapper: string[];
        /**
         * An array of global require paths, relative to the origin.
         * @type {string[]}
         */
        static globalPaths: string[];
        /**
         * Globabl module loaders
         * @type {object}
         */
        static loaders: object;
        /**
         * The main entry module, lazily created.
         * @type {Module}
         */
        static get main(): Module;
        /**
         * Wraps source in a CommonJS module scope.
         * @param {string} source
         */
        static wrap(source: string): string;
        /**
         * Compiles given JavaScript module source.
         * @param {string} source
         * @param {{ url?: URL | string }=} [options]
         * @return {function(
         *   object,
         *   function(string): any,
         *   Module,
         *   string,
         *   string,
         *   typeof process,
         *   object
         * ): any}
         */
        static compile(source: string, options?: {
            url?: URL | string;
        } | undefined): (arg0: object, arg1: (arg0: string) => any, arg2: Module, arg3: string, arg4: string, arg5: typeof process, arg6: object) => any;
        /**
         * Creates a `Module` from source URL and optionally a parent module.
         * @param {string|URL|Module} url
         * @param {ModuleOptions=} [options]
         */
        static from(url: string | URL | Module, options?: ModuleOptions | undefined): any;
        /**
         * Creates a `require` function from a given module URL.
         * @param {string|URL} url
         * @param {ModuleOptions=} [options]
         */
        static createRequire(url: string | URL, options?: ModuleOptions | undefined): any;
        /**
         * `Module` class constructor.
         * @param {string|URL} url
         * @param {ModuleOptions=} [options]
         */
        constructor(url: string | URL, options?: ModuleOptions | undefined);
        /**
         * A unique ID for this module.
         * @type {string}
         */
        get id(): string;
        /**
         * A reference to the "main" module.
         * @type {Module}
         */
        get main(): Module;
        /**
         * Child modules of this module.
         * @type {Module[]}
         */
        get children(): Module[];
        /**
         * A reference to the module cache. Possibly shared with all
         * children modules.
         * @type {object}
         */
        get cache(): any;
        /**
         * A reference to the module package.
         * @type {Package}
         */
        get package(): Package;
        /**
         * The `ImportMap` for this module.
         * @type {ImportMap}
         * @see {@link https://developer.mozilla.org/en-US/docs/Web/HTML/Element/script/type/importmap}
         */
        get importmap(): ImportMap;
        /**
         * The module level resolvers.
         * @type {ModuleResolver[]}
         */
        get resolvers(): ModuleResolver[];
        /**
         * `true` if the module is currently loading, otherwise `false`.
         * @type {boolean}
         */
        get loading(): boolean;
        /**
         * `true` if the module is currently loaded, otherwise `false`.
         * @type {boolean}
         */
        get loaded(): boolean;
        /**
         * An error associated with the module if it failed to load.
         * @type {Error?}
         */
        get error(): Error;
        /**
         * The exports of the module
         * @type {object}
         */
        get exports(): any;
        /**
         * The scope of the module given to parsed modules.
         * @type {ModuleScope}
         */
        get scope(): ModuleScope;
        /**
         * The origin of the loaded module.
         * @type {string}
         */
        get origin(): string;
        /**
         * The parent module for this module.
         * @type {Module?}
         */
        get parent(): Module;
        /**
         * The `Loader` for this module.
         * @type {Loader}
         */
        get loader(): Loader;
        /**
         * The filename of the module.
         * @type {string}
         */
        get filename(): string;
        /**
         * Known source loaders for this module keyed by file extension.
         * @type {object}
         */
        get loaders(): any;
        /**
         * Factory for creating a `require()` function based on a module context.
         * @param {CreateRequireOptions=} [options]
         * @return {RequireFunction}
         */
        createRequire(options?: CreateRequireOptions | undefined): RequireFunction;
        /**
         * Creates a `Module` from source the URL with this module as
         * the parent.
         * @param {string|URL|Module} url
         * @param {ModuleOptions=} [options]
         */
        createModule(url: string | URL | Module, options?: ModuleOptions | undefined): any;
        /**
         * Requires a module at for a given `input` which can be a relative file,
         * named module, or an absolute URL within the context of this odule.
         * @param {string|URL} input
         * @param {RequireOptions=} [options]
         * @throws ModuleNotFoundError
         * @throws ReferenceError
         * @throws SyntaxError
         * @throws TypeError
         * @return {any}
         */
        require(url: any, options?: RequireOptions | undefined): any;
        /**
         * Loads the module
         * @param {ModuleLoadOptions=} [options]
         * @return {boolean}
         */
        load(options?: ModuleLoadOptions | undefined): boolean;
        resolve(input: any): string;
        /**
         * @ignore
         */
        [Symbol.toStringTag](): string;
        #private;
    }
    export namespace Module {
        export { Module };
    }
    export default Module;
    export type ModuleResolver = (arg0: string, arg1: Module, arg2: (arg0: string) => any) => any;
    export type RequireFunction = import("socket:commonjs/require").RequireFunction;
    export type PackageOptions = import("socket:commonjs/package").PackageOptions;
    export type CreateRequireOptions = {
        prefix?: string;
        request?: import("socket:commonjs/loader").RequestOptions;
        builtins?: object;
    };
    export type ModuleOptions = {
        resolvers?: ModuleResolver[];
        importmap?: ImportMap;
        loader?: Loader | object;
        loaders?: object;
        package?: Package | PackageOptions;
        parent?: Module;
        state?: State;
    };
    export type ModuleLoadOptions = {
        extensions?: object;
    };
    import { Package } from "socket:commonjs/package";
    import { Loader } from "socket:commonjs/loader";
    import process from "socket:process";
}

declare module "socket:commonjs/require" {
    /**
     * Factory for creating a `require()` function based on a module context.
     * @param {CreateRequireOptions} options
     * @return {RequireFunction}
     */
    export function createRequire(options: CreateRequireOptions): RequireFunction;
    /**
     * @typedef {function(string, import('./module.js').Module, function(string): any): any} RequireResolver
     */
    /**
     * @typedef {{
     *   module: import('./module.js').Module,
     *   prefix?: string,
     *   request?: import('./loader.js').RequestOptions,
     *   builtins?: object,
     *   resolvers?: RequireFunction[]
     * }} CreateRequireOptions
     */
    /**
     * @typedef {function(string): any} RequireFunction
     */
    /**
     * @typedef {import('./package.js').PackageOptions} PackageOptions
     */
    /**
     * @typedef {import('./package.js').PackageResolveOptions} PackageResolveOptions
     */
    /**
     * @typedef {
     *   PackageResolveOptions &
     *   PackageOptions &
     *   { origins?: string[] | URL[] }
     * } ResolveOptions
     */
    /**
     * @typedef {ResolveOptions & {
     *   resolvers?: RequireResolver[],
     *   importmap?: import('./module.js').ImportMap,
     *   cache?: boolean
     * }} RequireOptions
     */
    /**
     * An array of global require paths, relative to the origin.
     * @type {string[]}
     */
    export const globalPaths: string[];
    /**
     * An object attached to a `require()` function that contains metadata
     * about the current module context.
     */
    export class Meta {
        /**
         * `Meta` class constructor.
         * @param {import('./module.js').Module} module
         */
        constructor(module: import("socket:commonjs/module").Module);
        /**
         * The referrer (parent) of this module.
         * @type {string}
         */
        get referrer(): string;
        /**
         * The referrer (parent) of this module.
         * @type {string}
         */
        get url(): string;
        #private;
    }
    export default createRequire;
    export type RequireResolver = (arg0: string, arg1: import("socket:commonjs/module").Module, arg2: (arg0: string) => any) => any;
    export type CreateRequireOptions = {
        module: import("socket:commonjs/module").Module;
        prefix?: string;
        request?: import("socket:commonjs/loader").RequestOptions;
        builtins?: object;
        resolvers?: RequireFunction[];
    };
    export type RequireFunction = (arg0: string) => any;
    export type PackageOptions = import("socket:commonjs/package").PackageOptions;
    export type PackageResolveOptions = import("socket:commonjs/package").PackageResolveOptions;
    export type RequireOptions = ResolveOptions & {
        resolvers?: RequireResolver[];
        importmap?: import("socket:commonjs/module").ImportMap;
        cache?: boolean;
    };
}

declare module "socket:commonjs" {
    export default exports;
    import * as exports from "socket:commonjs";
    import builtins from "socket:commonjs/builtins";
    import Cache from "socket:commonjs/cache";
    import createRequire from "socket:commonjs/require";
    import Loader from "socket:commonjs/loader";
    import Module from "socket:commonjs/module";
    import Package from "socket:commonjs/package";
    
    export { builtins, Cache, createRequire, Loader, Module, Package };
}

declare module "socket:fetch/fetch" {
    export function Headers(headers: any): void;
    export class Headers {
        constructor(headers: any);
        map: {};
        append(name: any, value: any): void;
        delete(name: any): void;
        get(name: any): any;
        has(name: any): boolean;
        set(name: any, value: any): void;
        forEach(callback: any, thisArg: any): void;
        keys(): {
            next: () => {
                done: boolean;
                value: any;
            };
        };
        values(): {
            next: () => {
                done: boolean;
                value: any;
            };
        };
        entries(): {
            next: () => {
                done: boolean;
                value: any;
            };
        };
    }
    export function Request(input: any, options: any): void;
    export class Request {
        constructor(input: any, options: any);
        url: string;
        credentials: any;
        headers: Headers;
        method: any;
        mode: any;
        signal: any;
        referrer: any;
        clone(): Request;
    }
    export function Response(bodyInit: any, options: any): void;
    export class Response {
        constructor(bodyInit: any, options: any);
        type: string;
        status: any;
        ok: boolean;
        statusText: string;
        headers: Headers;
        url: any;
        clone(): Response;
    }
    export namespace Response {
        function error(): Response;
        function redirect(url: any, status: any): Response;
    }
    export function fetch(input: any, init: any): Promise<any>;
    export class DOMException {
        private constructor();
    }
    namespace _default {
        export { fetch };
        export { Headers };
        export { Request };
        export { Response };
    }
    export default _default;
}

declare module "socket:fetch/index" {
    export default fetch;
    import { fetch } from "socket:fetch/fetch";
    import { Headers } from "socket:fetch/fetch";
    import { Request } from "socket:fetch/fetch";
    import { Response } from "socket:fetch/fetch";
    export { fetch, Headers, Request, Response };
}

declare module "socket:fetch" {
    export * from "socket:fetch/index";
    export default fetch;
    import fetch from "socket:fetch/index";
}

declare module "socket:i18n" {
    /**
     * Get messages for `locale` pattern. This function could return many results
     * for various locales given a `locale` pattern. such as `fr`, which could
     * return results for `fr`, `fr-FR`, `fr-BE`, etc.
     * @ignore
     * @param {string} locale
     * @return {object[]}
     */
    export function getMessagesForLocale(locale: string): object[];
    /**
     * Returns user preferred ISO 639 language codes or RFC 5646 language tags.
     * @return {string[]}
     */
    export function getAcceptLanguages(): string[];
    /**
     * Returns the current user ISO 639 language code or RFC 5646 language tag.
     * @return {?string}
     */
    export function getUILanguage(): string | null;
    /**
     * Gets a localized message string for the specified message name.
     * @param {string} messageName
     * @param {object|string[]=} [substitutions = []]
     * @param {object=} [options]
     * @param {string=} [options.locale = null]
     * @see {@link https://developer.chrome.com/docs/extensions/reference/i18n/#type-LanguageCode}
     * @see {@link https://www.ibm.com/docs/en/rbd/9.5.1?topic=syslib-getmessage}
     * @return {?string}
     */
    export function getMessage(messageName: string, substitutions?: (object | string[]) | undefined, options?: object | undefined): string | null;
    /**
     * Gets a localized message description string for the specified message name.
     * @param {string} messageName
     * @param {object=} [options]
     * @param {string=} [options.locale = null]
     * @return {?string}
     */
    export function getMessageDescription(messageName: string, options?: object | undefined): string | null;
    /**
     * A cache of loaded locale messages.
     * @type {Map}
     */
    export const cache: Map<any, any>;
    /**
     * Default location of i18n locale messages
     * @type {string}
     */
    export const DEFAULT_LOCALES_LOCATION: string;
    /**
     * An enumeration of supported ISO 639 language codes or RFC 5646 language tags.
     * @type {Enumeration}
     * @see {@link https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/API/i18n/LanguageCode}
     * @see {@link https://developer.chrome.com/docs/extensions/reference/i18n/#type-LanguageCode}
     */
    export const LanguageCode: Enumeration;
    namespace _default {
        export { LanguageCode };
        export { getAcceptLanguages };
        export { getMessage };
        export { getUILanguage };
    }
    export default _default;
    import Enumeration from "socket:enumeration";
}

declare module "socket:node/index" {
    export default network;
    export function network(options: any): Promise<events.EventEmitter>;
    import { Cache } from "socket:latica/index";
    import { sha256 } from "socket:latica/index";
    import { Encryption } from "socket:latica/index";
    import { Packet } from "socket:latica/index";
    import { NAT } from "socket:latica/index";
    export { Cache, sha256, Encryption, Packet, NAT };
}

declare module "socket:index" {
    import { network } from "socket:node/index";
    import { Cache } from "socket:node/index";
    import { sha256 } from "socket:node/index";
    import { Encryption } from "socket:node/index";
    import { Packet } from "socket:node/index";
    import { NAT } from "socket:node/index";
    export { network, Cache, sha256, Encryption, Packet, NAT };
}

declare module "socket:latica" {
    export * from "socket:latica/index";
    export default def;
    import def from "socket:latica/index";
}

declare module "socket:module" {
    export const builtinModules: any;
    export default Module;
    export type ModuleOptions = import("socket:commonjs/module").ModuleOptions;
    export type ModuleResolver = import("socket:commonjs/module").ModuleResolver;
    export type ModuleLoadOptions = import("socket:commonjs/module").ModuleLoadOptions;
    export type RequireFunction = import("socket:commonjs/module").RequireFunction;
    export type CreateRequireOptions = import("socket:commonjs/module").CreateRequireOptions;
    import { createRequire } from "socket:commonjs/module";
    import { Module } from "socket:commonjs/module";
    import builtins from "socket:commonjs/builtins";
    import { isBuiltin } from "socket:commonjs/builtins";
    export { createRequire, Module, builtins, isBuiltin };
}

declare module "socket:node-esm-loader" {
    export function resolve(specifier: any, ctx: any, next: any): Promise<any>;
    export default resolve;
}

declare module "socket:internal/permissions" {
    /**
     * Query for a permission status.
     * @param {PermissionDescriptor} descriptor
     * @param {object=} [options]
     * @param {?AbortSignal} [options.signal = null]
     * @return {Promise<PermissionStatus>}
     */
    export function query(descriptor: PermissionDescriptor, options?: object | undefined, ...args: any[]): Promise<PermissionStatus>;
    /**
     * Request a permission to be granted.
     * @param {PermissionDescriptor} descriptor
     * @param {object=} [options]
     * @param {?AbortSignal} [options.signal = null]
     * @return {Promise<PermissionStatus>}
     */
    export function request(descriptor: PermissionDescriptor, options?: object | undefined, ...args: any[]): Promise<PermissionStatus>;
    /**
     * An enumeration of the permission types.
     * - 'geolocation'
     * - 'notifications'
     * - 'push'
     * - 'persistent-storage'
     * - 'midi'
     * - 'storage-access'
     * @type {Enumeration}
     * @ignore
     */
    export const types: Enumeration;
    const _default: any;
    export default _default;
    export type PermissionDescriptor = {
        name: string;
    };
    /**
     * A container that provides the state of an object and an event handler
     * for monitoring changes permission changes.
     * @ignore
     */
    class PermissionStatus extends EventTarget {
        /**
         * `PermissionStatus` class constructor.
         * @param {string} name
         * @param {string} initialState
         * @param {object=} [options]
         * @param {?AbortSignal} [options.signal = null]
         */
        constructor(name: string, initialState: string, options?: object | undefined);
        /**
         * The name of this permission this status is for.
         * @type {string}
         */
        get name(): string;
        /**
         * The current state of the permission status.
         * @type {string}
         */
        get state(): string;
        set onchange(onchange: (arg0: Event) => any);
        /**
         * Level 0 event target 'change' event listener accessor
         * @type {function(Event)}
         */
        get onchange(): (arg0: Event) => any;
        /**
         * Non-standard method for unsubscribing to status state updates.
         * @ignore
         */
        unsubscribe(): void;
        /**
         * String tag for `PermissionStatus`.
         * @ignore
         */
        get [Symbol.toStringTag](): string;
        #private;
    }
    import Enumeration from "socket:enumeration";
}

declare module "socket:notification" {
    /**
     * Show a notification. Creates a `Notification` instance and displays
     * it to the user.
     * @param {string} title
     * @param {NotificationOptions=} [options]
     * @param {function(Event)=} [onclick]
     * @param {function(Event)=} [onclose]
     * @return {Promise}
     */
    export function showNotification(title: string, options?: NotificationOptions | undefined, onclick?: ((arg0: Event) => any) | undefined, onshow?: any): Promise<any>;
    /**
     * The global event dispatched when a `Notification` is presented to
     * the user.
     * @ignore
     * @type {string}
     */
    export const NOTIFICATION_PRESENTED_EVENT: string;
    /**
     * The global event dispatched when a `Notification` has a response
     * from the user.
     * @ignore
     * @type {string}
     */
    export const NOTIFICATION_RESPONSE_EVENT: string;
    /**
     * An enumeratino of notification test directions:
     * - 'auto'  Automatically determined by the operating system
     * - 'ltr'   Left-to-right text direction
     * - 'rtl'   Right-to-left text direction
     * @type {Enumeration}
     * @ignore
     */
    export const NotificationDirection: Enumeration;
    /**
     * An enumeration of permission types granted by the user for the current
     * origin to display notifications to the end user.
     * - 'granted'  The user has explicitly granted permission for the current
     *              origin to display system notifications.
     * - 'denied'   The user has explicitly denied permission for the current
     *              origin to display system notifications.
     * - 'default'  The user decision is unknown; in this case the application
     *              will act as if permission was denied.
     * @type {Enumeration}
     * @ignore
     */
    export const NotificationPermission: Enumeration;
    /**
     * A validated notification action object container.
     * You should never need to construct this.
     * @ignore
     */
    export class NotificationAction {
        /**
         * `NotificationAction` class constructor.
         * @ignore
         * @param {object} options
         * @param {string} options.action
         * @param {string} options.title
         * @param {string|URL=} [options.icon = '']
         */
        constructor(options: {
            action: string;
            title: string;
            icon?: (string | URL) | undefined;
        });
        /**
         * A string identifying a user action to be displayed on the notification.
         * @type {string}
         */
        get action(): string;
        /**
         * A string containing action text to be shown to the user.
         * @type {string}
         */
        get title(): string;
        /**
         * A string containing the URL of an icon to display with the action.
         * @type {string}
         */
        get icon(): string;
        #private;
    }
    /**
     * A validated notification options object container.
     * You should never need to construct this.
     * @ignore
     */
    export class NotificationOptions {
        /**
         * `NotificationOptions` class constructor.
         * @ignore
         * @param {object} [options = {}]
         * @param {string=} [options.dir = 'auto']
         * @param {NotificationAction[]=} [options.actions = []]
         * @param {string|URL=} [options.badge = '']
         * @param {string=} [options.body = '']
         * @param {?any=} [options.data = null]
         * @param {string|URL=} [options.icon = '']
         * @param {string|URL=} [options.image = '']
         * @param {string=} [options.lang = '']
         * @param {string=} [options.tag = '']
         * @param {boolean=} [options.boolean = '']
         * @param {boolean=} [options.requireInteraction = false]
         * @param {boolean=} [options.silent = false]
         * @param {number[]=} [options.vibrate = []]
         */
        constructor(options?: {
            dir?: string | undefined;
            actions?: NotificationAction[] | undefined;
            badge?: (string | URL) | undefined;
            body?: string | undefined;
            data?: (any | null) | undefined;
            icon?: (string | URL) | undefined;
            image?: (string | URL) | undefined;
            lang?: string | undefined;
            tag?: string | undefined;
            boolean?: boolean | undefined;
            requireInteraction?: boolean | undefined;
            silent?: boolean | undefined;
            vibrate?: number[] | undefined;
        }, allowServiceWorkerGlobalScope?: boolean);
        /**
         * An array of actions to display in the notification.
         * @type {NotificationAction[]}
         */
        get actions(): NotificationAction[];
        /**
         * A string containing the URL of the image used to represent
         * the notification when there isn't enough space to display the
         * notification itself.
         * @type {string}
         */
        get badge(): string;
        /**
         * A string representing the body text of the notification,
         * which is displayed below the title.
         * @type {string}
         */
        get body(): string;
        /**
         * Arbitrary data that you want associated with the notification.
         * This can be of any data type.
         * @type {?any}
         */
        get data(): any;
        /**
         * The direction in which to display the notification.
         * It defaults to 'auto', which just adopts the environments
         * language setting behavior, but you can override that behavior
         * by setting values of 'ltr' and 'rtl'.
         * @type {'auto'|'ltr'|'rtl'}
         */
        get dir(): "auto" | "ltr" | "rtl";
        /**
          A string containing the URL of an icon to be displayed in the notification.
         * @type {string}
         */
        get icon(): string;
        /**
         * The URL of an image to be displayed as part of the notification, as
         * specified in the constructor's options parameter.
         * @type {string}
         */
        get image(): string;
        /**
         * The notification's language, as specified using a string representing a
         * language tag according to RFC 5646.
         * @type {string}
         */
        get lang(): string;
        /**
         * A boolean value specifying whether the user should be notified after a
         * new notification replaces an old one. The default is `false`, which means
         * they won't be notified. If `true`, then tag also must be set.
         * @type {boolean}
         */
        get renotify(): boolean;
        /**
         * Indicates that a notification should remain active until the user clicks
         * or dismisses it, rather than closing automatically.
         * The default value is `false`.
         * @type {boolean}
         */
        get requireInteraction(): boolean;
        /**
         * A boolean value specifying whether the notification is silent (no sounds
         * or vibrations issued), regardless of the device settings.
         * The default is `false`, which means it won't be silent. If `true`, then
         * vibrate must not be present.
         * @type {boolean}
         */
        get silent(): boolean;
        /**
         * A string representing an identifying tag for the notification.
         * The default is the empty string.
         * @type {string}
         */
        get tag(): string;
        /**
         * A vibration pattern for the device's vibration hardware to emit with
         * the notification. If specified, silent must not be `true`.
         * @type {number[]}
         * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Vibration_API#vibration_patterns}
         */
        get vibrate(): number[];
        /**
         * @ignore
         * @return {object}
         */
        toJSON(): object;
        #private;
    }
    /**
     * The Notification interface is used to configure and display
     * desktop and mobile notifications to the user.
     */
    export class Notification extends EventTarget {
        /**
         * A read-only property that indicates the current permission granted
         * by the user to display notifications.
         * @type {'prompt'|'granted'|'denied'}
         */
        static get permission(): "denied" | "granted" | "prompt";
        /**
         * The maximum number of actions supported by the device.
         * @type {number}
         */
        static get maxActions(): number;
        /**
         * Requests permission from the user to display notifications.
         * @param {object=} [options]
         * @param {boolean=} [options.alert = true] - (macOS/iOS only)
         * @param {boolean=} [options.sound = false] - (macOS/iOS only)
         * @param {boolean=} [options.badge = false] - (macOS/iOS only)
         * @param {boolean=} [options.force = false]
         * @return {Promise<'granted'|'default'|'denied'>}
         */
        static requestPermission(options?: object | undefined): Promise<"granted" | "default" | "denied">;
        /**
         * `Notification` class constructor.
         * @param {string} title
         * @param {NotificationOptions=} [options]
         */
        constructor(title: string, options?: NotificationOptions | undefined, existingState?: any, ...args: any[]);
        /**
         * @ignore
         */
        get options(): any;
        /**
         * A unique identifier for this notification.
         * @type {string}
         */
        get id(): string;
        /**
         * `true` if the notification was closed, otherwise `false`.
         * @type {boolea}
         */
        get closed(): boolea;
        set onclick(onclick: Function);
        /**
         * The click event is dispatched when the user clicks on
         * displayed notification.
         * @type {?function}
         */
        get onclick(): Function;
        set onclose(onclose: Function);
        /**
         * The close event is dispatched when the notification closes.
         * @type {?function}
         */
        get onclose(): Function;
        set onerror(onerror: Function);
        /**
         * The eror event is dispatched when the notification fails to display
         * or encounters an error.
         * @type {?function}
         */
        get onerror(): Function;
        set onshow(onshow: Function);
        /**
         * The click event is dispatched when the notification is displayed.
         * @type {?function}
         */
        get onshow(): Function;
        /**
         * An array of actions to display in the notification.
         * @type {NotificationAction[]}
         */
        get actions(): NotificationAction[];
        /**
         * A string containing the URL of the image used to represent
         * the notification when there isn't enough space to display the
         * notification itself.
         * @type {string}
         */
        get badge(): string;
        /**
         * A string representing the body text of the notification,
         * which is displayed below the title.
         * @type {string}
         */
        get body(): string;
        /**
         * Arbitrary data that you want associated with the notification.
         * This can be of any data type.
         * @type {?any}
         */
        get data(): any;
        /**
         * The direction in which to display the notification.
         * It defaults to 'auto', which just adopts the environments
         * language setting behavior, but you can override that behavior
         * by setting values of 'ltr' and 'rtl'.
         * @type {'auto'|'ltr'|'rtl'}
         */
        get dir(): "auto" | "ltr" | "rtl";
        /**
         * A string containing the URL of an icon to be displayed in the notification.
         * @type {string}
         */
        get icon(): string;
        /**
         * The URL of an image to be displayed as part of the notification, as
         * specified in the constructor's options parameter.
         * @type {string}
         */
        get image(): string;
        /**
         * The notification's language, as specified using a string representing a
         * language tag according to RFC 5646.
         * @type {string}
         */
        get lang(): string;
        /**
         * A boolean value specifying whether the user should be notified after a
         * new notification replaces an old one. The default is `false`, which means
         * they won't be notified. If `true`, then tag also must be set.
         * @type {boolean}
         */
        get renotify(): boolean;
        /**
         * Indicates that a notification should remain active until the user clicks
         * or dismisses it, rather than closing automatically.
         * The default value is `false`.
         * @type {boolean}
         */
        get requireInteraction(): boolean;
        /**
         * A boolean value specifying whether the notification is silent (no sounds
         * or vibrations issued), regardless of the device settings.
         * The default is `false`, which means it won't be silent. If `true`, then
         * vibrate must not be present.
         * @type {boolean}
         */
        get silent(): boolean;
        /**
         * A string representing an identifying tag for the notification.
         * The default is the empty string.
         * @type {string}
         */
        get tag(): string;
        /**
         * A vibration pattern for the device's vibration hardware to emit with
         * the notification. If specified, silent must not be `true`.
         * @type {number[]}
         * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Vibration_API#vibration_patterns}
         */
        get vibrate(): number[];
        /**
         * The timestamp of the notification.
         * @type {number}
         */
        get timestamp(): number;
        /**
         * The title read-only property of the `Notification` instace indicates
         * the title of the notification, as specified in the `title` parameter
         * of the `Notification` constructor.
         * @type {string}
         */
        get title(): string;
        /**
         * Closes the notification programmatically.
         */
        close(): Promise<any>;
        #private;
    }
    export default Notification;
    import { Enumeration } from "socket:enumeration";
    import URL from "socket:url";
}

declare module "socket:protocol-handlers" {
    /**
     * @typedef {{ scheme: string }} GetServiceWorkerOptions
    
    /**
     * @param {GetServiceWorkerOptions} options
     * @return {Promise<ServiceWorker|null>
     */
    export function getServiceWorker(options: GetServiceWorkerOptions): Promise<ServiceWorker | null>;
    namespace _default {
        export { getServiceWorker };
    }
    export default _default;
    /**
     * /**
     */
    export type GetServiceWorkerOptions = {
        scheme: string;
    };
}

declare module "socket:shared-worker" {
    /**
     * A reference to the opened environment. This value is an instance of an
     * `Environment` if the scope is a ServiceWorker scope.
     * @type {Environment|null}
     */
    export const env: Environment | null;
    export default SharedWorker;
    import { SharedWorker } from "socket:shared-worker/index";
    export { Environment, SharedWorker };
}

declare module "socket:signal" {
    export * from "socket:process/signal";
    export default signal;
    import signal from "socket:process/signal";
}

declare module "socket:service-worker/instance" {
    export function createServiceWorker(currentState?: any, options?: any): any;
    export const SHARED_WORKER_URL: string;
    export const ServiceWorker: {
        new (): ServiceWorker;
        prototype: ServiceWorker;
    } | {
        new (): {
            onmessage: any;
            onerror: any;
            onstatechange: any;
            readonly state: any;
            readonly scriptURL: any;
            postMessage(): void;
            addEventListener(type: string, callback: EventListenerOrEventListenerObject | null, options?: AddEventListenerOptions | boolean): void;
            dispatchEvent(event: Event): boolean;
            removeEventListener(type: string, callback: EventListenerOrEventListenerObject | null, options?: EventListenerOptions | boolean): void;
        };
    };
    export default createServiceWorker;
}

declare module "socket:worker" {
    export default Worker;
    import { SharedWorker } from "socket:shared-worker/index";
    import { ServiceWorker } from "socket:service-worker/instance";
    import { Worker } from "socket:worker_threads";
    export { SharedWorker, ServiceWorker, Worker };
}

declare module "socket:child_process/worker" {
    export {};
}

declare module "socket:internal/callsite" {
    /**
     * Creates an ordered and link array of `CallSite` instances from a
     * given `Error`.
     * @param {Error} error
     * @param {string} source
     * @return {CallSite[]}
     */
    export function createCallSites(error: Error, source: string): CallSite[];
    /**
     * @typedef {{
     *   sourceURL: string | null,
     *   symbol: string,
     *   column: number | undefined,
     *   line: number | undefined,
     *   native: boolean
     * }} ParsedStackFrame
     */
    /**
     * A container for location data related to a `StackFrame`
     */
    export class StackFrameLocation {
        /**
         * Creates a `StackFrameLocation` from JSON input.
         * @param {object=} json
         * @return {StackFrameLocation}
         */
        static from(json?: object | undefined): StackFrameLocation;
        /**
         * The line number of the location of the stack frame, if available.
         * @type {number | undefined}
         */
        lineNumber: number | undefined;
        /**
         * The column number of the location of the stack frame, if available.
         * @type {number | undefined}
         */
        columnNumber: number | undefined;
        /**
         * The source URL of the location of the stack frame, if available. This value
         * may be `null`.
         * @type {string?}
         */
        sourceURL: string | null;
        /**
         * `true` if the stack frame location is in native location, otherwise
         * this value `false` (default).
         * @type
         */
        isNative: any;
        /**
         * Converts this `StackFrameLocation` to a JSON object.
         * @ignore
         * @return {{
         *   lineNumber: number | undefined,
         *   columnNumber: number | undefined,
         *   sourceURL: string | null,
         *   isNative: boolean
         * }}
         */
        toJSON(): {
            lineNumber: number | undefined;
            columnNumber: number | undefined;
            sourceURL: string | null;
            isNative: boolean;
        };
    }
    /**
     * A stack frame container related to a `CallSite`.
     */
    export class StackFrame {
        /**
         * Parses a raw stack frame string into structured data.
         * @param {string} rawStackFrame
         * @return {ParsedStackFrame}
         */
        static parse(rawStackFrame: string): ParsedStackFrame;
        /**
         * Creates a new `StackFrame` from an `Error` and raw stack frame
         * source `rawStackFrame`.
         * @param {Error} error
         * @param {string} rawStackFrame
         * @return {StackFrame}
         */
        static from(error: Error, rawStackFrame: string): StackFrame;
        /**
         * `StackFrame` class constructor.
         * @param {Error} error
         * @param {ParsedStackFrame=} [frame]
         * @param {string=} [source]
         */
        constructor(error: Error, frame?: ParsedStackFrame | undefined, source?: string | undefined);
        /**
         * The stack frame location data.
         * @type {StackFrameLocation}
         */
        location: StackFrameLocation;
        /**
         * The `Error` associated with this `StackFrame` instance.
         * @type {Error?}
         */
        error: Error | null;
        /**
         * The name of the function where the stack frame is located.
         * @type {string?}
         */
        symbol: string | null;
        /**
         * The raw stack frame source string.
         * @type {string?}
         */
        source: string | null;
        /**
         * Converts this `StackFrameLocation` to a JSON object.
         * @ignore
         * @return {{
         *   location: {
         *     lineNumber: number | undefined,
         *     columnNumber: number | undefined,
         *     sourceURL: string | null,
         *     isNative: boolean
         *   },
         *   isNative: boolean,
         *   symbol: string | null,
         *   source: string | null,
         *   error: { message: string, name: string, stack: string } | null
         * }}
         */
        toJSON(): {
            location: {
                lineNumber: number | undefined;
                columnNumber: number | undefined;
                sourceURL: string | null;
                isNative: boolean;
            };
            isNative: boolean;
            symbol: string | null;
            source: string | null;
            error: {
                message: string;
                name: string;
                stack: string;
            } | null;
        };
    }
    /**
     * A v8 compatible interface and container for call site information.
     */
    export class CallSite {
        /**
         * An internal symbol used to refer to the index of a promise in
         * `Promise.all` or `Promise.any` function call site.
         * @ignore
         * @type {symbol}
         */
        static PromiseElementIndexSymbol: symbol;
        /**
         * An internal symbol used to indicate that a call site is in a `Promise.all`
         * function call.
         * @ignore
         * @type {symbol}
         */
        static PromiseAllSymbol: symbol;
        /**
         * An internal symbol used to indicate that a call site is in a `Promise.any`
         * function call.
         * @ignore
         * @type {symbol}
         */
        static PromiseAnySymbol: symbol;
        /**
         * An internal source symbol used to store the original `Error` stack source.
         * @ignore
         * @type {symbol}
         */
        static StackSourceSymbol: symbol;
        /**
         * `CallSite` class constructor
         * @param {Error} error
         * @param {string} rawStackFrame
         * @param {CallSite=} previous
         */
        constructor(error: Error, rawStackFrame: string, previous?: CallSite | undefined);
        /**
         * The `Error` associated with the call site.
         * @type {Error}
         */
        get error(): Error;
        /**
         * The previous `CallSite` instance, if available.
         * @type {CallSite?}
         */
        get previous(): CallSite;
        /**
         * A reference to the `StackFrame` data.
         * @type {StackFrame}
         */
        get frame(): StackFrame;
        /**
         * This function _ALWAYS__ returns `globalThis` as `this` cannot be determined.
         * @return {object}
         */
        getThis(): object;
        /**
         * This function _ALWAYS__ returns `null` as the type name of `this`
         * cannot be determined.
         * @return {null}
         */
        getTypeName(): null;
        /**
         * This function _ALWAYS__ returns `undefined` as the current function
         * reference cannot be determined.
         * @return {undefined}
         */
        getFunction(): undefined;
        /**
         * Returns the name of the function in at the call site, if available.
         * @return {string|undefined}
         */
        getFunctionName(): string | undefined;
        /**
         * An alias to `getFunctionName()
         * @return {string}
         */
        getMethodName(): string;
        /**
         * Get the filename of the call site location, if available, otherwise this
         * function returns 'unknown location'.
         * @return {string}
         */
        getFileName(): string;
        /**
         * Returns the location source URL defaulting to the global location.
         * @return {string}
         */
        getScriptNameOrSourceURL(): string;
        /**
         * Returns a hash value of the source URL return by `getScriptNameOrSourceURL()`
         * @return {string}
         */
        getScriptHash(): string;
        /**
         * Returns the line number of the call site location.
         * This value may be `undefined`.
         * @return {number|undefined}
         */
        getLineNumber(): number | undefined;
        /**
         * @ignore
         * @return {number}
         */
        getPosition(): number;
        /**
         * Attempts to get an "enclosing" line number, potentially the previous
         * line number of the call site
         * @param {number|undefined}
         */
        getEnclosingLineNumber(): any;
        /**
         * Returns the column number of the call site location.
         * This value may be `undefined`.
         * @return {number|undefined}
         */
        getColumnNumber(): number | undefined;
        /**
         * Attempts to get an "enclosing" column number, potentially the previous
         * line number of the call site
         * @param {number|undefined}
         */
        getEnclosingColumnNumber(): any;
        /**
         * Gets the origin of where `eval()` was called if this call site function
         * originated from a call to `eval()`. This function may return `undefined`.
         * @return {string|undefined}
         */
        getEvalOrigin(): string | undefined;
        /**
         * This function _ALWAYS__ returns `false` as `this` cannot be determined so
         * "top level" detection is not possible.
         * @return {boolean}
         */
        isTopLevel(): boolean;
        /**
         * Returns `true` if this call site originated from a call to `eval()`.
         * @return {boolean}
         */
        isEval(): boolean;
        /**
         * Returns `true` if the call site is in a native location, otherwise `false`.
         * @return {boolean}
         */
        isNative(): boolean;
        /**
         * This function _ALWAYS_ returns `false` as constructor detection
         * is not possible.
         * @return {boolean}
         */
        isConstructor(): boolean;
        /**
         * Returns `true` if the call site is in async context, otherwise `false`.
         * @return {boolean}
         */
        isAsync(): boolean;
        /**
         * Returns `true` if the call site is in a `Promise.all()` function call,
         * otherwise `false.
         * @return {boolean}
         */
        isPromiseAll(): boolean;
        /**
         * Gets the index of the promise element that was followed in a
         * `Promise.all()` or `Promise.any()` function call. If not available, then
         * this function returns `null`.
         * @return {number|null}
         */
        getPromiseIndex(): number | null;
        /**
         * Converts this call site to a string.
         * @return {string}
         */
        toString(): string;
        /**
         * Converts this `CallSite` to a JSON object.
         * @ignore
         * @return {{
         *   frame: {
         *     location: {
         *       lineNumber: number | undefined,
         *       columnNumber: number | undefined,
         *       sourceURL: string | null,
         *       isNative: boolean
         *     },
         *     isNative: boolean,
         *     symbol: string | null,
         *     source: string | null,
         *     error: { message: string, name: string, stack: string } | null
         *   }
         * }}
         */
        toJSON(): {
            frame: {
                location: {
                    lineNumber: number | undefined;
                    columnNumber: number | undefined;
                    sourceURL: string | null;
                    isNative: boolean;
                };
                isNative: boolean;
                symbol: string | null;
                source: string | null;
                error: {
                    message: string;
                    name: string;
                    stack: string;
                } | null;
            };
        };
        set [$previous](previous: any);
        /**
         * Private accessor to "friend class" `CallSiteList`.
         * @ignore
         */
        get [$previous](): any;
        #private;
    }
    /**
     * An array based list container for `CallSite` instances.
     */
    export class CallSiteList extends Array<any> {
        /**
         * Creates a `CallSiteList` instance from `Error` input.
         * @param {Error} error
         * @param {string} source
         * @return {CallSiteList}
         */
        static from(error: Error, source: string): CallSiteList;
        /**
         * `CallSiteList` class constructor.
         * @param {Error} error
         * @param {string[]=} [sources]
         */
        constructor(error: Error, sources?: string[] | undefined);
        /**
         * A reference to the `Error` for this `CallSiteList` instance.
         * @type {Error}
         */
        get error(): Error;
        /**
         * An array of stack frame source strings.
         * @type {string[]}
         */
        get sources(): string[];
        /**
         * The original stack string derived from the sources.
         * @type {string}
         */
        get stack(): string;
        /**
         * Adds `CallSite` instances to the top of the list, linking previous
         * instances to the next one.
         * @param {...CallSite} callsites
         * @return {number}
         */
        unshift(...callsites: CallSite[]): number;
        /**
         * A no-op function as `CallSite` instances cannot be added to the end
         * of the list.
         * @return {number}
         */
        push(): number;
        /**
         * Pops a `CallSite` off the end of the list.
         * @return {CallSite|undefined}
         */
        pop(): CallSite | undefined;
        /**
         * Converts this `CallSiteList` to a JSON object.
         * @return {{
         *   frame: {
         *     location: {
         *       lineNumber: number | undefined,
         *       columnNumber: number | undefined,
         *       sourceURL: string | null,
         *       isNative: boolean
         *     },
         *     isNative: boolean,
         *     symbol: string | null,
         *     source: string | null,
         *     error: { message: string, name: string, stack: string } | null
         *   }
         * }[]}
         */
        toJSON(): {
            frame: {
                location: {
                    lineNumber: number | undefined;
                    columnNumber: number | undefined;
                    sourceURL: string | null;
                    isNative: boolean;
                };
                isNative: boolean;
                symbol: string | null;
                source: string | null;
                error: {
                    message: string;
                    name: string;
                    stack: string;
                } | null;
            };
        }[];
        #private;
    }
    export default CallSite;
    export type ParsedStackFrame = {
        sourceURL: string | null;
        symbol: string;
        column: number | undefined;
        line: number | undefined;
        native: boolean;
    };
    const $previous: unique symbol;
}

declare module "socket:internal/error" {
    /**
     * The default `Error` class stack trace limit.
     * @type {number}
     */
    export const DEFAULT_ERROR_STACK_TRACE_LIMIT: number;
    export const DefaultPlatformError: ErrorConstructor;
    export const Error: ErrorConstructor;
    export const URIError: ErrorConstructor;
    export const EvalError: ErrorConstructor;
    export const TypeError: ErrorConstructor;
    export const RangeError: ErrorConstructor;
    export const MediaError: ErrorConstructor;
    export const SyntaxError: ErrorConstructor;
    export const ReferenceError: ErrorConstructor;
    export const AggregateError: ErrorConstructor;
    export const RTCError: ErrorConstructor;
    export const OverconstrainedError: ErrorConstructor;
    export const GeolocationPositionError: ErrorConstructor;
    export const ApplePayError: ErrorConstructor;
    namespace _default {
        export { Error };
        export { URIError };
        export { EvalError };
        export { TypeError };
        export { RangeError };
        export { MediaError };
        export { SyntaxError };
        export { ReferenceError };
        export { AggregateError };
        export { RTCError };
        export { OverconstrainedError };
        export { GeolocationPositionError };
        export { ApplePayError };
    }
    export default _default;
}

declare module "socket:internal/geolocation" {
    /**
     * Get the current position of the device.
     * @param {function(GeolocationPosition)} onSuccess
     * @param {onError(Error)} onError
     * @param {object=} options
     * @param {number=} options.timeout
     * @return {Promise}
     */
    export function getCurrentPosition(onSuccess: (arg0: GeolocationPosition) => any, onError: any, options?: object | undefined, ...args: any[]): Promise<any>;
    /**
     * Register a handler function that will be called automatically each time the
     * position of the device changes. You can also, optionally, specify an error
     * handling callback function.
     * @param {function(GeolocationPosition)} onSuccess
     * @param {function(Error)} onError
     * @param {object=} [options]
     * @param {number=} [options.timeout = null]
     * @return {number}
     */
    export function watchPosition(onSuccess: (arg0: GeolocationPosition) => any, onError: (arg0: Error) => any, options?: object | undefined, ...args: any[]): number;
    /**
     * Unregister location and error monitoring handlers previously installed
     * using `watchPosition`.
     * @param {number} id
     */
    export function clearWatch(id: number, ...args: any[]): any;
    export namespace platform {
        let getCurrentPosition: Function;
        let watchPosition: Function;
        let clearWatch: Function;
    }
    namespace _default {
        export { getCurrentPosition };
        export { watchPosition };
        export { clearWatch };
    }
    export default _default;
}

declare module "socket:internal/post-message" {
    const _default: any;
    export default _default;
}

declare module "socket:service-worker/notification" {
    export function showNotification(registration: any, title: any, options: any): Promise<void>;
    export function getNotifications(registration: any, options?: any): Promise<any>;
    namespace _default {
        export { showNotification };
        export { getNotifications };
    }
    export default _default;
}

declare module "socket:service-worker/registration" {
    export class ServiceWorkerRegistration {
        constructor(info: any, serviceWorker: any);
        get scope(): any;
        get updateViaCache(): string;
        get installing(): any;
        get waiting(): any;
        get active(): any;
        set onupdatefound(onupdatefound: any);
        get onupdatefound(): any;
        get navigationPreload(): any;
        getNotifications(): Promise<any>;
        showNotification(title: any, options: any): Promise<void>;
        unregister(): Promise<void>;
        update(): Promise<void>;
        #private;
    }
    export default ServiceWorkerRegistration;
}

declare module "socket:service-worker/container" {
    /**
     * Predicate to determine if service workers are allowed
     * @return {boolean}
     */
    export function isServiceWorkerAllowed(): boolean;
    /**
     * A `ServiceWorkerContainer` implementation that is attached to the global
     * `globalThis.navigator.serviceWorker` object.
     */
    export class ServiceWorkerContainer extends EventTarget {
        get ready(): any;
        get controller(): any;
        /**
         * A special initialization function for augmenting the global
         * `globalThis.navigator.serviceWorker` platform `ServiceWorkerContainer`
         * instance.
         *
         * All functions MUST be sure to what a lexically bound `this` becomes as the
         * target could change with respect to the `internal` `Map` instance which
         * contains private implementation properties relevant to the runtime
         * `ServiceWorkerContainer` internal state implementations.
         * @ignore
         */
        init(): Promise<any>;
        register(scriptURL: any, options?: any): Promise<globalThis.ServiceWorkerRegistration | ServiceWorkerRegistration>;
        getRegistration(clientURL: any): Promise<globalThis.ServiceWorkerRegistration | ServiceWorkerRegistration>;
        getRegistrations(): Promise<readonly globalThis.ServiceWorkerRegistration[] | ServiceWorkerRegistration[]>;
        startMessages(): void;
    }
    export default ServiceWorkerContainer;
    import { ServiceWorkerRegistration } from "socket:service-worker/registration";
}

declare module "socket:internal/service-worker" {
    export const serviceWorker: ServiceWorkerContainer;
    export default serviceWorker;
    import { ServiceWorkerContainer } from "socket:service-worker/container";
}

declare module "socket:internal/webassembly" {
    /**
     * The `instantiateStreaming()` function compiles and instantiates a WebAssembly
     * module directly from a streamed source.
     * @ignore
     * @param {Response} response
     * @param {=object} [importObject]
     * @return {Promise<WebAssembly.Instance>}
     */
    export function instantiateStreaming(response: Response, importObject?: any): Promise<WebAssembly.Instance>;
    /**
     * The `compileStreaming()` function compiles and instantiates a WebAssembly
     * module directly from a streamed source.
     * @ignore
     * @param {Response} response
     * @return {Promise<WebAssembly.Module>}
     */
    export function compileStreaming(response: Response): Promise<WebAssembly.Module>;
    namespace _default {
        export { instantiateStreaming };
    }
    export default _default;
}

declare module "socket:internal/scheduler" {
    export * from "socket:timers/scheduler";
    export default scheduler;
    import scheduler from "socket:timers/scheduler";
}

declare module "socket:internal/timers" {
    export function setTimeout(callback: any, ...args: any[]): number;
    export function clearTimeout(timeout: any): any;
    export function setInterval(callback: any, ...args: any[]): number;
    export function clearInterval(interval: any): any;
    export function setImmediate(callback: any, ...args: any[]): number;
    export function clearImmediate(immediate: any): any;
    namespace _default {
        export { setTimeout };
        export { setInterval };
        export { setImmediate };
        export { clearTimeout };
        export { clearInterval };
        export { clearImmediate };
    }
    export default _default;
}

declare module "socket:internal/pickers" {
    /**
     * @typedef {{
     *   id?: string,
     *   mode?: 'read' | 'readwrite'
     *   startIn?: FileSystemHandle | 'desktop' | 'documents' | 'downloads' | 'music' | 'pictures' | 'videos',
     * }} ShowDirectoryPickerOptions
     *
     */
    /**
     * Shows a directory picker which allows the user to select a directory.
     * @param {ShowDirectoryPickerOptions=} [options]
     * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Window/showDirectoryPicker}
     * @return {Promise<FileSystemDirectoryHandle[]>}
     */
    export function showDirectoryPicker(options?: ShowDirectoryPickerOptions | undefined): Promise<FileSystemDirectoryHandle[]>;
    /**
     * @typedef {{
     *   id?: string,
     *   excludeAcceptAllOption?: boolean,
     *   startIn?: FileSystemHandle | 'desktop' | 'documents' | 'downloads' | 'music' | 'pictures' | 'videos',
     *   types?: Array<{
     *     description?: string,
     *     [keyof object]?: string[]
     *   }>
     * }} ShowOpenFilePickerOptions
     */
    /**
     * Shows a file picker that allows a user to select a file or multiple files
     * and returns a handle for each selected file.
     * @param {ShowOpenFilePickerOptions=} [options]
     * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Window/showOpenFilePicker}
     * @return {Promise<FileSystemFileHandle[]>}
     */
    export function showOpenFilePicker(options?: ShowOpenFilePickerOptions | undefined): Promise<FileSystemFileHandle[]>;
    /**
     * @typedef {{
     *   id?: string,
     *   excludeAcceptAllOption?: boolean,
     *   suggestedName?: string,
     *   startIn?: FileSystemHandle | 'desktop' | 'documents' | 'downloads' | 'music' | 'pictures' | 'videos',
     *   types?: Array<{
     *     description?: string,
     *     [keyof object]?: string[]
     *   }>
     * }} ShowSaveFilePickerOptions
     */
    /**
     * Shows a file picker that allows a user to save a file by selecting an
     * existing file, or entering a name for a new file.
     * @param {ShowSaveFilePickerOptions=} [options]
     * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Window/showSaveFilePicker}
     * @return {Promise<FileSystemHandle>}
     */
    export function showSaveFilePicker(options?: ShowSaveFilePickerOptions | undefined): Promise<FileSystemHandle>;
    /**
     * Key-value store for general usage by the file pickers"
     * @ignore
     */
    export class Database {
        get(key: any): any;
        set(key: any, value: any): void;
    }
    /**
     * Internal database for pickers, such as mapping IDs to directory/file paths.
     * @ignore
     */
    export const db: Database;
    namespace _default {
        export { showDirectoryPicker };
        export { showOpenFilePicker };
        export { showSaveFilePicker };
    }
    export default _default;
    export type ShowDirectoryPickerOptions = {
        id?: string;
        mode?: "read" | "readwrite";
        startIn?: FileSystemHandle | "desktop" | "documents" | "downloads" | "music" | "pictures" | "videos";
    };
    /**
     * ]?: string[]
     *   }>
     * }} ShowOpenFilePickerOptions
     */
    export type object = {
        id?: string;
        excludeAcceptAllOption?: boolean;
        startIn?: FileSystemHandle | "desktop" | "documents" | "downloads" | "music" | "pictures" | "videos";
        types?: Array<{
            description?: string;
            [keyof]: any;
        }>;
    };
}

declare module "socket:internal/primitives" {
    export function init(): {
        natives: {};
        patches: {};
    };
    namespace _default {
        export { natives };
        export { patches };
    }
    export default _default;
    const natives: {};
    const patches: {};
}

declare module "socket:internal/init" {
    namespace _default {
        export { location };
    }
    export default _default;
    import location from "socket:location";
}

declare module "socket:internal/worker" {
    export function onWorkerMessage(event: any): Promise<any>;
    export function addEventListener(eventName: any, callback: any, ...args: any[]): any;
    export function removeEventListener(eventName: any, callback: any, ...args: any[]): any;
    export function dispatchEvent(event: any): any;
    export function postMessage(message: any, ...args: any[]): any;
    export function close(): any;
    export function importScripts(...scripts: any[]): void;
    export const WorkerGlobalScopePrototype: any;
    /**
     * The absolute `URL` of the internal worker initialization entry.
     * @ignore
     * @type {URL}
     */
    export const url: URL;
    /**
     * The worker entry source.
     * @ignore
     * @type {string}
     */
    export const source: string;
    /**
     * A unique identifier for this worker made available on the global scope
     * @ignore
     * @type {string}
     */
    export const RUNTIME_WORKER_ID: string;
    /**
     * Internally scoped event interface for a worker context.
     * @ignore
     * @type {object}
     */
    export const worker: object;
    /**
     * A reference to the global worker scope.
     * @type {WorkerGlobalScope}
     */
    export const self: WorkerGlobalScope;
    namespace _default {
        export { RUNTIME_WORKER_ID };
        export { removeEventListener };
        export { addEventListener };
        export { importScripts };
        export { dispatchEvent };
        export { postMessage };
        export { source };
        export { close };
        export { url };
    }
    export default _default;
}

declare module "socket:latica/worker" {
    export {};
}

declare module "socket:npm/module" {
    /**
     * @typedef {{
     * package: Package
     * origin: string,
     * type: 'commonjs' | 'module',
     * url: string
     * }} ModuleResolution
     */
    /**
     * Resolves an NPM module for a given `specifier` and an optional `origin`.
     * @param {string|URL} specifier
     * @param {string|URL=} [origin]
     * @param {{ prefix?: string, type?: 'commonjs' | 'module' }} [options]
     * @return {ModuleResolution|null}
     */
    export function resolve(specifier: string | URL, origin?: (string | URL) | undefined, options?: {
        prefix?: string;
        type?: "commonjs" | "module";
    }): ModuleResolution | null;
    namespace _default {
        export { resolve };
    }
    export default _default;
    export type ModuleResolution = {
        package: Package;
        origin: string;
        type: "commonjs" | "module";
        url: string;
    };
    import { Package } from "socket:commonjs/package";
}

declare module "socket:npm/service-worker" {
    /**
     * @ignore
     * @param {Request}
     * @param {object} env
     * @param {import('../service-worker/context.js').Context} ctx
     * @return {Promise<Response|null>}
     */
    export function onRequest(request: any, env: object, ctx: import("socket:service-worker/context").Context): Promise<Response | null>;
    /**
     * Handles incoming 'npm://<module_name>/<pathspec...>' requests.
     * @param {Request} request
     * @param {object} env
     * @param {import('../service-worker/context.js').Context} ctx
     * @return {Response?}
     */
    export default function _default(request: Request, env: object, ctx: import("socket:service-worker/context").Context): Response | null;
}

declare module "socket:service-worker/global" {
    export class ServiceWorkerGlobalScope {
        get isServiceWorkerScope(): boolean;
        get ExtendableEvent(): typeof ExtendableEvent;
        get FetchEvent(): typeof FetchEvent;
        get serviceWorker(): any;
        set registration(value: any);
        get registration(): any;
        get clients(): import("socket:service-worker/clients").Clients;
        set onactivate(listener: any);
        get onactivate(): any;
        set onmessage(listener: any);
        get onmessage(): any;
        set oninstall(listener: any);
        get oninstall(): any;
        set onfetch(listener: any);
        get onfetch(): any;
        skipWaiting(): Promise<void>;
    }
    const _default: ServiceWorkerGlobalScope;
    export default _default;
    import { ExtendableEvent } from "socket:service-worker/events";
    import { FetchEvent } from "socket:service-worker/events";
}

declare module "socket:service-worker/init" {
    export function onRegister(event: any): Promise<void>;
    export function onUnregister(event: any): Promise<void>;
    export function onSkipWaiting(event: any): Promise<void>;
    export function onActivate(event: any): Promise<void>;
    export function onFetch(event: any): Promise<any>;
    export function onNotificationShow(event: any, target: any): any;
    export function onNotificationClose(event: any, target: any): void;
    export function onGetNotifications(event: any, target: any): any;
    export const workers: Map<any, any>;
    export class ServiceWorkerInstance extends Worker {
        constructor(filename: any, options: any);
        get info(): any;
        get notifications(): any[];
        onMessage(event: any): Promise<void>;
        #private;
    }
    export class ServiceWorkerInfo {
        constructor(data: any);
        id: any;
        url: any;
        hash: any;
        scope: any;
        scriptURL: any;
        get pathname(): string;
    }
    const _default: any;
    export default _default;
}
declare function isTypedArray(object: any): boolean;
declare function isTypedArray(object: any): boolean;
declare function isArrayBuffer(object: any): object is ArrayBuffer;
declare function isArrayBuffer(object: any): object is ArrayBuffer;
declare function findMessageTransfers(transfers: any, object: any, options?: any): any;
declare function findMessageTransfers(transfers: any, object: any, options?: any): any;
declare const Uint8ArrayPrototype: Uint8Array;
declare const TypedArrayPrototype: any;
declare const TypedArray: any;
declare const ports: any[];

declare module "socket:service-worker/storage" {
    /**
     * A factory for creating storage interfaces.
     * @param {'memoryStorage'|'localStorage'|'sessionStorage'} type
     * @return {Promise<Storage>}
     */
    export function createStorageInterface(type: "memoryStorage" | "localStorage" | "sessionStorage"): Promise<Storage>;
    /**
     * @typedef {{ done: boolean, value: string | undefined }} IndexIteratorResult
     */
    /**
     * An iterator interface for an `Index` instance.
     */
    export class IndexIterator {
        /**
         * `IndexIterator` class constructor.
         * @ignore
         * @param {Index} index
         */
        constructor(index: Index);
        /**
         * `true` if the iterator is "done", otherwise `false`.
         * @type {boolean}
         */
        get done(): boolean;
        /**
         * Returns the next `IndexIteratorResult`.
         * @return {IndexIteratorResult}
         */
        next(): IndexIteratorResult;
        /**
         * Mark `IndexIterator` as "done"
         * @return {IndexIteratorResult}
         */
        return(): IndexIteratorResult;
        #private;
    }
    /**
     * A container used by the `Provider` to index keys and values
     */
    export class Index {
        /**
         * A reference to the keys in this index.
         * @type {string[]}
         */
        get keys(): string[];
        /**
         * A reference to the values in this index.
         * @type {string[]}
         */
        get values(): string[];
        /**
         * The number of entries in this index.
         * @type {number}
         */
        get length(): number;
        /**
         * Returns the key at a given `index`, if it exists otherwise `null`.
         * @param {number} index}
         * @return {string?}
         */
        key(index: number): string | null;
        /**
         * Returns the value at a given `index`, if it exists otherwise `null`.
         * @param {number} index}
         * @return {string?}
         */
        value(index: number): string | null;
        /**
         * Inserts a value in the index.
         * @param {string} key
         * @param {string} value
         */
        insert(key: string, value: string): void;
        /**
         * Computes the index of a key in this index.
         * @param {string} key
         * @return {number}
         */
        indexOf(key: string): number;
        /**
         * Clears all keys and values in the index.
         */
        clear(): void;
        /**
         * Returns an entry at `index` if it exists, otherwise `null`.
         * @param {number} index
         * @return {string[]|null}
         */
        entry(index: number): string[] | null;
        /**
         * Removes entries at a given `index`.
         * @param {number} index
         * @return {boolean}
         */
        remove(index: number): boolean;
        /**
         * Returns an array of computed entries in this index.
         * @return {IndexIterator}
         */
        entries(): IndexIterator;
        /**
         * @ignore
         * @return {IndexIterator}
         */
        [Symbol.iterator](): IndexIterator;
        #private;
    }
    /**
     * A base class for a storage provider.
     */
    export class Provider {
        /**
         * An error currently associated with the provider, likely from an
         * async operation.
         * @type {Error?}
         */
        get error(): Error;
        /**
         * A promise that resolves when the provider is ready.
         * @type {Promise}
         */
        get ready(): Promise<any>;
        /**
         * A reference the service worker storage ID, which is the service worker
         * registration ID.
         * @type {string}
         * @throws DOMException
         */
        get id(): string;
        /**
         * A reference to the provider `Index`
         * @type {Index}
         * @throws DOMException
         */
        get index(): Index;
        /**
         * The number of entries in the provider.
         * @type {number}
         * @throws DOMException
         */
        get length(): number;
        /**
         * Returns `true` if the provider has a value for a given `key`.
         * @param {string} key}
         * @return {boolean}
         * @throws DOMException
         */
        has(key: string): boolean;
        /**
         * Get a value by `key`.
         * @param {string} key
         * @return {string?}
         * @throws DOMException
         */
        get(key: string): string | null;
        /**
         * Sets a `value` by `key`
         * @param {string} key
         * @param {string} value
         * @throws DOMException
         */
        set(key: string, value: string): void;
        /**
         * Removes a value by `key`.
         * @param {string} key
         * @return {boolean}
         * @throws DOMException
         */
        remove(key: string): boolean;
        /**
         * Clear all keys and values.
         * @throws DOMException
         */
        clear(): void;
        /**
         * The keys in the provider index.
         * @return {string[]}
         * @throws DOMException
         */
        keys(): string[];
        /**
         * The values in the provider index.
         * @return {string[]}
         * @throws DOMException
         */
        values(): string[];
        /**
         * Returns the key at a given `index`
         * @param {number} index
         * @return {string|null}
         * @throws DOMException
         */
        key(index: number): string | null;
        /**
         * Loads the internal index with keys and values.
         * @return {Promise}
         */
        load(): Promise<any>;
        #private;
    }
    /**
     * An in-memory storage provider. It just used the built-in provider `Index`
     * for storing key-value entries.
     */
    export class MemoryStorageProvider extends Provider {
    }
    /**
     * A session storage provider that persists for the runtime of the
     * application and through service worker restarts.
     */
    export class SessionStorageProvider extends Provider {
        /**
         * Remove a value by `key`.
         * @param {string} key
         * @return {string?}
         * @throws DOMException
         * @throws NotFoundError
         */
        remove(key: string): string | null;
    }
    /**
     * A local storage provider that persists until the data is cleared.
     */
    export class LocalStorageProvider extends Provider {
    }
    /**
     * A generic interface for storage implementations
     */
    export class Storage {
        /**
         * A factory for creating a `Storage` instance that is backed
         * by a storage provider. Extending classes should define a `Provider`
         * class that is statically available on the extended `Storage` class.
         * @param {symbol} token
         * @return {Promise<Proxy<Storage>>}
         */
        static create(token: symbol): Promise<ProxyConstructor>;
        /**
         * `Storage` class constructor.
         * @ignore
         * @param {symbol} token
         * @param {Provider} provider
         */
        constructor(token: symbol, provider: Provider);
        /**
         * A readonly reference to the storage provider.
         * @type {Provider}
         */
        get provider(): Provider;
        /**
         * The number of entries in the storage.
         * @type {number}
         */
        get length(): number;
        /**
         * Returns `true` if the storage has a value for a given `key`.
         * @param {string} key
         * @return {boolean}
         * @throws TypeError
         */
        hasItem(key: string, ...args: any[]): boolean;
        /**
         * Clears the storage of all entries
         */
        clear(): void;
        /**
         * Returns the key at a given `index`
         * @param {number} index
         * @return {string|null}
         */
        key(index: number, ...args: any[]): string | null;
        /**
         * Get a storage value item for a given `key`.
         * @param {string} key
         * @return {string|null}
         */
        getItem(key: string, ...args: any[]): string | null;
        /**
         * Removes a storage value entry for a given `key`.
         * @param {string}
         * @return {boolean}
         */
        removeItem(key: any, ...args: any[]): boolean;
        /**
         * Sets a storage item `value` for a given `key`.
         * @param {string} key
         * @param {string} value
         */
        setItem(key: string, value: string, ...args: any[]): void;
        /**
         * @ignore
         */
        get [Symbol.toStringTag](): string;
        #private;
    }
    /**
     * An in-memory `Storage` interface.
     */
    export class MemoryStorage extends Storage {
        static Provider: typeof MemoryStorageProvider;
    }
    /**
     * A locally persisted `Storage` interface.
     */
    export class LocalStorage extends Storage {
        static Provider: typeof LocalStorageProvider;
    }
    /**
     * A session `Storage` interface.
     */
    export class SessionStorage extends Storage {
        static Provider: typeof SessionStorageProvider;
    }
    namespace _default {
        export { Storage };
        export { LocalStorage };
        export { MemoryStorage };
        export { SessionStorage };
        export { createStorageInterface };
    }
    export default _default;
    export type IndexIteratorResult = {
        done: boolean;
        value: string | undefined;
    };
}

declare module "socket:service-worker/worker" {
    export function onReady(): void;
    export function onMessage(event: any): Promise<any>;
    const _default: any;
    export default _default;
    export namespace SERVICE_WORKER_READY_TOKEN {
        let __service_worker_ready: boolean;
    }
    export namespace module {
        let exports: {};
    }
    export const events: Set<any>;
    export namespace stages {
        let register: Deferred;
        let install: Deferred;
        let activate: Deferred;
    }
    import { Deferred } from "socket:async";
}

declare module "socket:shared-worker/debug" {
    export function debug(...args: any[]): void;
    export default debug;
}

declare module "socket:shared-worker/global" {
    export class SharedWorkerGlobalScope {
        get isSharedWorkerScope(): boolean;
        set onconnect(listener: any);
        get onconnect(): any;
    }
    const _default: SharedWorkerGlobalScope;
    export default _default;
}

declare module "socket:shared-worker/init" {
    export function onInstall(event: any): Promise<void>;
    export function onUninstall(event: any): Promise<void>;
    export function onConnect(event: any): Promise<void>;
    export const workers: Map<any, any>;
    export { channel };
    export class SharedWorkerInstance extends Worker {
        constructor(filename: any, options: any);
        get info(): any;
        onMessage(event: any): Promise<void>;
        #private;
    }
    export class SharedWorkerInfo {
        constructor(data: any);
        id: any;
        port: any;
        client: any;
        scriptURL: any;
        url: any;
        hash: any;
        get pathname(): string;
    }
    const _default: any;
    export default _default;
    import { channel } from "socket:shared-worker/index";
}

declare module "socket:shared-worker/state" {
    export const state: any;
    export default state;
}

declare module "socket:shared-worker/worker" {
    export function onReady(): void;
    export function onMessage(event: any): Promise<void>;
    const _default: any;
    export default _default;
    export namespace SHARED_WORKER_READY_TOKEN {
        let __shared_worker_ready: boolean;
    }
    export namespace module {
        let exports: {};
    }
    export const connections: Set<any>;
}

declare module "socket:test/harness" {
    /**
     * @typedef {import('./index').Test} Test
     * @typedef {(t: Test) => Promise<void> | void} TestCase
     * @typedef {{
     *    bootstrap(): Promise<void>
     *    close(): Promise<void>
     * }} Harness
     */
    /**
     * @template {Harness} T
     * @typedef {{
     *    (
     *      name: string,
     *      cb?: (harness: T, test: Test) => (void | Promise<void>)
     *    ): void;
     *    (
     *      name: string,
     *      opts: object,
     *      cb: (harness: T, test: Test) => (void | Promise<void>)
     *    ): void;
     *    only(
     *      name: string,
     *      cb?: (harness: T, test: Test) => (void | Promise<void>)
     *    ): void;
     *    only(
     *      name: string,
     *      opts: object,
     *      cb: (harness: T, test: Test) => (void | Promise<void>)
     *    ): void;
     *    skip(
     *      name: string,
     *      cb?: (harness: T, test: Test) => (void | Promise<void>)
     *    ): void;
     *    skip(
     *      name: string,
     *      opts: object,
     *      cb: (harness: T, test: Test) => (void | Promise<void>)
     *    ): void;
     * }} TapeTestFn
     */
    /**
     * @template {Harness} T
     * @param {import('./index.js')} tapzero
     * @param {new (options: object) => T} harnessClass
     * @returns {TapeTestFn<T>}
     */
    export function wrapHarness<T extends Harness>(tapzero: typeof import("socket:test/index"), harnessClass: new (options: object) => T): TapeTestFn<T>;
    export default exports;
    /**
     * @template {Harness} T
     */
    export class TapeHarness<T extends Harness> {
        /**
         * @param {import('./index.js')} tapzero
         * @param {new (options: object) => T} harnessClass
         */
        constructor(tapzero: typeof import("socket:test/index"), harnessClass: new (options: object) => T);
        /** @type {import('./index.js')} */
        tapzero: typeof import("socket:test/index");
        /** @type {new (options: object) => T} */
        harnessClass: new (options: object) => T;
        /**
         * @param {string} testName
         * @param {object} [options]
         * @param {(harness: T, test: Test) => (void | Promise<void>)} [fn]
         * @returns {void}
         */
        test(testName: string, options?: object, fn?: (harness: T, test: Test) => (void | Promise<void>)): void;
        /**
         * @param {string} testName
         * @param {object} [options]
         * @param {(harness: T, test: Test) => (void | Promise<void>)} [fn]
         * @returns {void}
         */
        only(testName: string, options?: object, fn?: (harness: T, test: Test) => (void | Promise<void>)): void;
        /**
         * @param {string} testName
         * @param {object} [options]
         * @param {(harness: T, test: Test) => (void | Promise<void>)} [fn]
         * @returns {void}
         */
        skip(testName: string, options?: object, fn?: (harness: T, test: Test) => (void | Promise<void>)): void;
        /**
         * @param {(str: string, fn?: TestCase) => void} tapzeroFn
         * @param {string} testName
         * @param {object} [options]
         * @param {(harness: T, test: Test) => (void | Promise<void>)} [fn]
         * @returns {void}
         */
        _test(tapzeroFn: (str: string, fn?: TestCase) => void, testName: string, options?: object, fn?: (harness: T, test: Test) => (void | Promise<void>)): void;
        /**
         * @param {Test} assert
         * @param {object} options
         * @param {(harness: T, test: Test) => (void | Promise<void>)} fn
         * @returns {Promise<void>}
         */
        _onAssert(assert: Test, options: object, fn: (harness: T, test: Test) => (void | Promise<void>)): Promise<void>;
    }
    export type Test = import("socket:test/index").Test;
    export type TestCase = (t: Test) => Promise<void> | void;
    export type Harness = {
        bootstrap(): Promise<void>;
        close(): Promise<void>;
    };
    export type TapeTestFn<T extends Harness> = {
        (name: string, cb?: (harness: T, test: Test) => (void | Promise<void>)): void;
        (name: string, opts: object, cb: (harness: T, test: Test) => (void | Promise<void>)): void;
        only(name: string, cb?: (harness: T, test: Test) => (void | Promise<void>)): void;
        only(name: string, opts: object, cb: (harness: T, test: Test) => (void | Promise<void>)): void;
        skip(name: string, cb?: (harness: T, test: Test) => (void | Promise<void>)): void;
        skip(name: string, opts: object, cb: (harness: T, test: Test) => (void | Promise<void>)): void;
    };
    import * as exports from "socket:test/harness";
    
}

declare module "socket:vm/init" {
    export {};
}
declare function isTypedArray(object: any): boolean;
declare function isTypedArray(object: any): boolean;
declare function isArrayBuffer(object: any): object is ArrayBuffer;
declare function isArrayBuffer(object: any): object is ArrayBuffer;
declare function findMessageTransfers(transfers: any, object: any, options?: any): any;
declare function findMessageTransfers(transfers: any, object: any, options?: any): any;
declare const Uint8ArrayPrototype: Uint8Array;
declare const TypedArrayPrototype: any;
declare const TypedArray: any;
declare class Client extends EventTarget {
    constructor(id: any, port: any);
    id: any;
    port: any;
    onMessage(event: any): void;
    postMessage(...args: any[]): any;
}
declare class Realm {
    constructor(state: any, port: any);
    /**
     * The `MessagePort` for the VM realm.
     * @type {MessagePort}
     */
    port: MessagePort;
    /**
     * A reference to the top level worker statae
     * @type {State}
     */
    state: State;
    /**
     * Known content worlds that exist in a realm
     * @type {Map<String, World>}
     */
    worlds: Map<string, World>;
    get clients(): Map<any, any>;
    postMessage(...args: any[]): void;
}
declare class State {
    static init(): State;
    /**
     * All known connected `MessagePort` instances
     * @type {MessagePort[]}
     */
    ports: MessagePort[];
    /**
     * Pending events to be dispatched to realm
     * @type {MessageEvent[]}
     */
    pending: MessageEvent[];
    /**
     * The realm for all virtual machines. This is a headless webview
     */
    realm: any;
    clients: Map<any, any>;
    onConnect(event: any): void;
    init(): void;
    onPortMessage(port: any, event: any): void;
}

declare module "socket:vm/world" {
    export {};
}
type MenuItemSelection = {
    title: string
    parent: string
    state: '0'
}
declare interface Window {
    addEventListener(
        type: "menuItemSelected",
        listener: (event: CustomEvent<MenuItemSelection>) => void,
        options?: boolean | AddEventListenerOptions
    ): void;
    addEventListener(
        type: "process-error",
        listener: (event: CustomEvent<string>) => void,
        options?: boolean | AddEventListenerOptions
    ): void;
    addEventListener(
        type: "backend-exit",
        listener: (event: CustomEvent<string>) => void,
        options?: boolean | AddEventListenerOptions
    ): void;
}