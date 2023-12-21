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
         */
        constructor(message: string, requireStack: any);
        requireStack: any;
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
declare module "socket:buffer" {
    export default Buffer;
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
        export function concat(list: any, length: any): Uint8Array;
        export { byteLength };
    }
    export function SlowBuffer(length: any): Uint8Array;
    export const INSPECT_MAX_BYTES: 50;
    export const kMaxLength: 2147483647;
    function byteLength(string: any, encoding: any, ...args: any[]): any;
}
declare module "socket:url/urlpattern/urlpattern" {
    export { me as URLPattern };
    var me: {
        new (t: {}, r: any, n: any): {
            "__#2@#i": any;
            "__#2@#n": {};
            "__#2@#t": {};
            "__#2@#e": {};
            "__#2@#s": {};
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
        };
        compareComponent(t: any, r: any, n: any): number;
    };
}
declare module "socket:url/url/url" {
    const _default: any;
    export default _default;
}
declare module "socket:url/index" {
    export function resolve(from: any, to: any): any;
    export const parse: any;
    export default URL;
    export const URL: any;
    import { URLPattern } from "socket:url/urlpattern/urlpattern";
    export const URLSearchParams: any;
    export const parseURL: any;
    export { URLPattern };
}
declare module "socket:url" {
    export * from "socket:url/index";
    export default URL;
    import URL from "socket:url/index";
}
declare module "socket:util" {
    export function hasOwnProperty(object: any, property: any): any;
    export function isTypedArray(object: any): boolean;
    export function isArrayLike(object: any): boolean;
    export function isArrayBufferView(buf: any): boolean;
    export function isAsyncFunction(object: any): boolean;
    export function isArgumentsObject(object: any): boolean;
    export function isEmptyObject(object: any): boolean;
    export function isObject(object: any): boolean;
    export function isPlainObject(object: any): boolean;
    export function isArrayBuffer(object: any): boolean;
    export function isBufferLike(object: any): boolean;
    export function isFunction(value: any): boolean;
    export function isErrorLike(error: any): boolean;
    export function isClass(value: any): boolean;
    export function isPromiseLike(object: any): boolean;
    export function toString(object: any): string;
    export function toBuffer(object: any, encoding?: any): any;
    export function toProperCase(string: any): any;
    export function splitBuffer(buffer: any, highWaterMark: any): any[];
    export function InvertedPromise(): Promise<any>;
    export function clamp(value: any, min: any, max: any): number;
    export function promisify(original: any): any;
    export function inspect(value: any, options: any): any;
    export namespace inspect {
        let custom: symbol;
        let ignore: symbol;
    }
    export function format(format: any, ...args: any[]): string;
    export function parseJSON(string: any): any;
    export function parseHeaders(headers: any): string[][];
    export function noop(): void;
    export function isValidPercentageValue(input: any): boolean;
    export function compareBuffers(a: any, b: any): any;
    export class IllegalConstructor {
    }
    export default exports;
    import * as exports from "socket:util";
    
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
        write(destination: any, ...args: any[]): Promise<any>;
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
    const _default: Console;
    export default _default;
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
        prototype: CustomEvent<any>;
    } | {
        new (type: any, options: any): {
            "__#3@#detail": any;
            readonly detail: any;
        };
    };
    export const MessageEvent: {
        new <T>(type: string, eventInitDict?: MessageEventInit<T>): MessageEvent<T>;
        prototype: MessageEvent<any>;
    } | {
        new (type: any, options: any): {
            "__#4@#detail": any;
            "__#4@#data": any;
            readonly detail: any;
            readonly data: any;
        };
    };
    export const ErrorEvent: {
        new (type: string, eventInitDict?: ErrorEventInit): ErrorEvent;
        prototype: ErrorEvent;
    } | {
        new (type: any, options: any): {
            "__#5@#detail": any;
            "__#5@#error": any;
            readonly detail: any;
            readonly error: any;
        };
    };
    export default exports;
    export function EventEmitter(): void;
    export class EventEmitter {
        _events: any;
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
        eventNames(): any;
    }
    export namespace EventEmitter {
        export { EventEmitter };
        export let defaultMaxListeners: number;
        export function init(): void;
        export function listenerCount(emitter: any, type: any): any;
        export { once };
    }
    export function once(emitter: any, name: any): Promise<any>;
    import * as exports from "socket:events";
    
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
    export function host(): 'android' | 'android-emulator' | 'iphoneos' | iphone;
    /**
     * @type {string}
     * The operating system's end-of-line marker. `'\r\n'` on Windows and `'\n'` on POSIX.
     */
    export const EOL: string;
    export default exports;
    import * as exports from "socket:os";
    
}
declare module "socket:process" {
    /**
     * Adds callback to the 'nextTick' queue.
     * @param {Function} callback
     */
    export function nextTick(callback: Function): void;
    /**
     * @returns {string} The home directory of the current user.
     */
    export function homedir(): string;
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
    export default process;
    const process: any;
}
declare module "socket:location" {
    export function toString(): string;
    export const globalLocation: Location | {
        origin: string;
        host: string;
        hostname: string;
        pathname: string;
        href: string;
    };
    export const href: string;
    export const protocol: "socket:";
    export const hostname: string;
    export const host: string;
    export const search: string;
    export const hash: string;
    export const pathname: string;
    export const origin: string;
    namespace _default {
        export { origin };
        export { href };
        export { protocol };
        export { hostname };
        export { host };
        export { search };
        export { pathname };
        export { toString };
    }
    export default _default;
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
            "__#2@#i": any;
            "__#2@#n": {};
            "__#2@#t": {};
            "__#2@#e": {};
            "__#2@#s": {};
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
        export { MUSIC };
        export { HOME };
    }
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
     * @param {string} suffix
     * @return {string}
     */
    export function basename(path: PathComponent, suffix: string): string;
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
    import * as posix from "socket:path/posix";
    import { RESOURCES } from "socket:path/well-known";
    import { DOWNLOADS } from "socket:path/well-known";
    import { DOCUMENTS } from "socket:path/well-known";
    import { PICTURES } from "socket:path/well-known";
    import { DESKTOP } from "socket:path/well-known";
    import { VIDEOS } from "socket:path/well-known";
    import { MUSIC } from "socket:path/well-known";
    import * as exports from "socket:path/win32";
    
    export { posix, Path, RESOURCES, DOWNLOADS, DOCUMENTS, PICTURES, DESKTOP, VIDEOS, MUSIC };
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
     * @param {string} suffix
     * @return {string}
     */
    export function basename(path: PathComponent, suffix: string): string;
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
    import * as win32 from "socket:path/win32";
    import { RESOURCES } from "socket:path/well-known";
    import { DOWNLOADS } from "socket:path/well-known";
    import { DOCUMENTS } from "socket:path/well-known";
    import { PICTURES } from "socket:path/well-known";
    import { DESKTOP } from "socket:path/well-known";
    import { VIDEOS } from "socket:path/well-known";
    import { MUSIC } from "socket:path/well-known";
    import * as exports from "socket:path/posix";
    
    export { win32, Path, RESOURCES, DOWNLOADS, DOCUMENTS, PICTURES, DESKTOP, VIDEOS, MUSIC };
}
declare module "socket:path/index" {
    export * as _default from "socket:path/index";
    
    import * as posix from "socket:path/posix";
    import * as win32 from "socket:path/win32";
    import { Path } from "socket:path/path";
    import { RESOURCES } from "socket:path/well-known";
    import { DOWNLOADS } from "socket:path/well-known";
    import { DOCUMENTS } from "socket:path/well-known";
    import { PICTURES } from "socket:path/well-known";
    import { DESKTOP } from "socket:path/well-known";
    import { VIDEOS } from "socket:path/well-known";
    import { MUSIC } from "socket:path/well-known";
    import { HOME } from "socket:path/well-known";
    export { posix, win32, Path, RESOURCES, DOWNLOADS, DOCUMENTS, PICTURES, DESKTOP, VIDEOS, MUSIC, HOME };
}
declare module "socket:path" {
    export const sep: "/" | "\\";
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
    import { RESOURCES } from "socket:path/index";
    import { DOWNLOADS } from "socket:path/index";
    import { DOCUMENTS } from "socket:path/index";
    import { PICTURES } from "socket:path/index";
    import { DESKTOP } from "socket:path/index";
    import { VIDEOS } from "socket:path/index";
    import { MUSIC } from "socket:path/index";
    import { HOME } from "socket:path/index";
    export { Path, posix, win32, RESOURCES, DOWNLOADS, DOCUMENTS, PICTURES, DESKTOP, VIDEOS, MUSIC, HOME };
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
         * @param {string} name
         * @param {object} message
         * @return Promise<boolean>
         */
        publish(name: string, message: object): Promise<boolean>;
        /**
         * Returns a string representation of the `ChannelRegistry`.
         * @ignore
         */
        toString(): string;
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
        toString(): string;
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
declare module "socket:worker" {
    /**
     * @type {import('dom').Worker}
     */
    export const Worker: any;
    export default Worker;
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
                (method: string, url: string | URL, async: boolean, username?: string, password?: string): void;
            };
            send: (body?: Document | XMLHttpRequestBodyInit) => void;
        };
    }
    export class WorkerMetric extends Metric {
        /**
         * @type {Worker}
         */
        static GlobalWorker: Worker;
        constructor(options: any);
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
    import { Worker } from "socket:worker";
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
    
    export { channels, window };
}
declare module "socket:diagnostics" {
    export * from "socket:diagnostics/index";
    export default exports;
    import * as exports from "socket:diagnostics/index";
}
declare module "socket:gc" {
    /**
     * Track `object` ref to call `Symbol.for('gc.finalize')` method when
     * environment garbage collects object.
     * @param {object} object
     * @return {boolean}
     */
    export function ref(object: object, ...args: any[]): boolean;
    /**
     * Stop tracking `object` ref to call `Symbol.for('gc.finalize')` method when
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
    export const pool: Set<any>;
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
declare module "socket:stream" {
    export function pipelinePromise(...streams: any[]): Promise<any>;
    export function pipeline(stream: any, ...streams: any[]): any;
    export function isStream(stream: any): boolean;
    export function isStreamx(stream: any): boolean;
    export function isReadStreamx(stream: any): any;
    export default exports;
    export class FixedFIFO {
        constructor(hwm: any);
        buffer: any[];
        mask: number;
        top: number;
        btm: number;
        next: any;
        push(data: any): boolean;
        shift(): any;
        isEmpty(): boolean;
    }
    export class FastFIFO {
        constructor(hwm: any);
        hwm: any;
        head: exports.FixedFIFO;
        tail: exports.FixedFIFO;
        push(val: any): void;
        shift(): any;
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
        queue: exports.FastFIFO;
        highWaterMark: number;
        buffered: number;
        error: any;
        pipeline: any;
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
        queue: exports.FastFIFO;
        highWaterMark: number;
        buffered: number;
        error: any;
        pipeline: exports.Pipeline;
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
        on(name: any, fn: any): any;
    }
    export class Readable extends exports.Stream {
        static _fromAsyncIterator(ite: any, opts: any): exports.Readable;
        static from(data: any, opts: any): any;
        static isBackpressured(rs: any): boolean;
        static isPaused(rs: any): boolean;
        _readableState: exports.ReadableState;
        _read(cb: any): void;
        pipe(dest: any, cb: any): any;
        read(): any;
        push(data: any): boolean;
        unshift(data: any): void;
        resume(): this;
        pause(): this;
    }
    export class Writable extends exports.Stream {
        static isBackpressured(ws: any): boolean;
        _writableState: exports.WritableState;
        _writev(batch: any, cb: any): void;
        _write(data: any, cb: any): void;
        _final(cb: any): void;
        write(data: any): boolean;
        end(data: any): this;
    }
    export class Duplex extends exports.Readable {
        _writableState: exports.WritableState;
        _writev(batch: any, cb: any): void;
        _write(data: any, cb: any): void;
        _final(cb: any): void;
        write(data: any): boolean;
        end(data: any): this;
    }
    export class Transform extends exports.Duplex {
        _transformState: exports.TransformState;
        _transform(data: any, cb: any): void;
        _flush(cb: any): void;
    }
    export class PassThrough extends exports.Transform {
    }
    import * as exports from "socket:stream";
    import { EventEmitter } from "socket:events";
    
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
declare module "socket:fs/constants" {
    /**
     * This flag can be used with uv_fs_copyfile() to return an error if the
     * destination already exists.
     */
    export const COPYFILE_EXCL: 1;
    /**
     * This flag can be used with uv_fs_copyfile() to attempt to create a reflink.
     * If copy-on-write is not supported, a fallback copy mechanism is used.
     */
    export const COPYFILE_FICLONE: 2;
    /**
     * This flag can be used with uv_fs_copyfile() to attempt to create a reflink.
     * If copy-on-write is not supported, an error is returned.
     */
    export const COPYFILE_FICLONE_FORCE: 4;
    export const UV_DIRENT_UNKNOWN: any;
    export const UV_DIRENT_FILE: any;
    export const UV_DIRENT_DIR: any;
    export const UV_DIRENT_LINK: any;
    export const UV_DIRENT_FIFO: any;
    export const UV_DIRENT_SOCKET: any;
    export const UV_DIRENT_CHAR: any;
    export const UV_DIRENT_BLOCK: any;
    export const UV_FS_SYMLINK_DIR: any;
    export const UV_FS_SYMLINK_JUNCTION: any;
    export const O_RDONLY: any;
    export const O_WRONLY: any;
    export const O_RDWR: any;
    export const O_APPEND: any;
    export const O_ASYNC: any;
    export const O_CLOEXEC: any;
    export const O_CREAT: any;
    export const O_DIRECT: any;
    export const O_DIRECTORY: any;
    export const O_DSYNC: any;
    export const O_EXCL: any;
    export const O_LARGEFILE: any;
    export const O_NOATIME: any;
    export const O_NOCTTY: any;
    export const O_NOFOLLOW: any;
    export const O_NONBLOCK: any;
    export const O_NDELAY: any;
    export const O_PATH: any;
    export const O_SYNC: any;
    export const O_TMPFILE: any;
    export const O_TRUNC: any;
    export const S_IFMT: any;
    export const S_IFREG: any;
    export const S_IFDIR: any;
    export const S_IFCHR: any;
    export const S_IFBLK: any;
    export const S_IFIFO: any;
    export const S_IFLNK: any;
    export const S_IFSOCK: any;
    export const S_IRWXU: any;
    export const S_IRUSR: any;
    export const S_IWUSR: any;
    export const S_IXUSR: any;
    export const S_IRWXG: any;
    export const S_IRGRP: any;
    export const S_IWGRP: any;
    export const S_IXGRP: any;
    export const S_IRWXO: any;
    export const S_IROTH: any;
    export const S_IWOTH: any;
    export const S_IXOTH: any;
    export const F_OK: any;
    export const R_OK: any;
    export const W_OK: any;
    export const X_OK: any;
    export default exports;
    import * as exports from "socket:fs/constants";
    
}
declare module "socket:fs/flags" {
    export function normalizeFlags(flags: any): any;
    export default exports;
    import * as exports from "socket:fs/flags";
    
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
        static from(stat?: object | Stats, fromBigInt?: any): Stats;
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
        static get DEFAULT_ACCESS_MODE(): any;
        static get DEFAULT_OPEN_FLAGS(): string;
        static get DEFAULT_OPEN_MODE(): number;
        /**
         * Creates a `FileHandle` from a given `id` or `fd`
         * @param {string|number|FileHandle|object} id
         * @return {FileHandle}
         */
        static from(id: string | number | FileHandle | object): FileHandle;
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
        flags: any;
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
        appendFile(data: string | Buffer | TypedArray | any[], options?: object | undefined): Promise<void>;
        /**
         * Change permissions of file handle.
         * @param {number} mode
         * @param {object=} [options]
         */
        chmod(mode: number, options?: object | undefined): Promise<void>;
        /**
         * Change ownership of file handle.
         * @param {number} uid
         * @param {number} gid
         * @param {object=} [options]
         */
        chown(uid: number, gid: number, options?: object | undefined): Promise<void>;
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
        datasync(): Promise<void>;
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
        write(buffer: Buffer | object, offset: number, length: number, position: number, options?: object | undefined): Promise<{
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
        writeFile(data: string | Buffer | TypedArray | any[], options?: object | undefined): Promise<void>;
        [exports.kOpening]: any;
        [exports.kClosing]: any;
        [exports.kClosed]: boolean;
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
         * Creates a `FileHandle` from a given `id` or `fd`
         * @param {string|number|DirectoryHandle|object} id
         * @return {DirectoryHandle}
         */
        static from(id: string | number | DirectoryHandle | object): DirectoryHandle;
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
         * Reads and returns directory entry.
         * @param {object|function} options
         * @param {function=} callback
         * @return {Dirent|string}
         */
        read(options: object | Function, callback?: Function | undefined): Dirent | string;
        /**
         * AsyncGenerator which yields directory entries.
         * @param {object=} options
         */
        entries(options?: object | undefined): AsyncGenerator<any, void, unknown>;
        /**
         * `for await (...)` AsyncGenerator support.
         */
        get [Symbol.asyncIterator](): (options?: object | undefined) => AsyncGenerator<any, void, unknown>;
    }
    /**
     * A container for a directory entry.
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#class-fsdirent}
     */
    export class Dirent {
        static get UNKNOWN(): any;
        static get FILE(): any;
        static get DIR(): any;
        static get LINK(): any;
        static get FIFO(): any;
        static get SOCKET(): any;
        static get CHAR(): any;
        static get BLOCK(): any;
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
     * @param {function} callback
     * @return {function}
     */
    export function onApplicationURL(callback: Function): Function;
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
        #private;
    }
    export default hooks;
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
        encoding: 'utf8' | 'buffer';
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
    }
    export default Watcher;
    import { EventEmitter } from "socket:events";
}
declare module "socket:fs/promises" {
    /**
     * Asynchronously check access a file.
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromisesaccesspath-mode}
     * @param {string | Buffer | URL} path
     * @param {string?} [mode]
     * @param {object?} [options]
     */
    export function access(path: string | Buffer | URL, mode?: string | null, options?: object | null): Promise<boolean>;
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
    export function copyFile(src: string, dest: string, flags: number): Promise<any>;
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
     * @return {Promise<any>} - Upon success, fulfills with undefined if recursive is false, or the first directory path created if recursive is true.
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
     * @param {object?} [options]
     * @param {string?} [options.encoding = 'utf8']
     * @param {number?} [options.bufferSize = 32]
     * @return {Promise<Dir>}
     */
    export function opendir(path: string | Buffer | URL, options?: object | null): Promise<Dir>;
    /**
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromisesreaddirpath-options}
     * @param {string | Buffer | URL} path
     * @param {object?} options
     * @param {string?} [options.encoding = 'utf8']
     * @param {boolean?} [options.withFileTypes = false]
     */
    export function readdir(path: string | Buffer | URL, options: object | null): Promise<any[]>;
    /**
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromisesreadfilepath-options}
     * @param {string} path
     * @param {object?} [options]
     * @param {(string|null)?} [options.encoding = null]
     * @param {string?} [options.flag = 'r']
     * @param {AbortSignal?} [options.signal]
     * @return {Promise<Buffer | string>}
     */
    export function readFile(path: string, options?: object | null): Promise<Buffer | string>;
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
     * @see {@link https://nodejs.org/api/fs.html#fspromisesstatpath-options}
     * @param {string | Buffer | URL} path
     * @param {object?} [options]
     * @param {boolean?} [options.bigint = false]
     * @return {Promise<Stats>}
     */
    export function stat(path: string | Buffer | URL, options?: object | null): Promise<Stats>;
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
     * @param {string | Buffer | URL | FileHandle} path - filename or FileHandle
     * @param {string|Buffer|Array|DataView|TypedArray} data
     * @param {object?} [options]
     * @param {string|null} [options.encoding = 'utf8']
     * @param {number} [options.mode = 0o666]
     * @param {string} [options.flag = 'w']
     * @param {AbortSignal?} [options.signal]
     * @return {Promise<void>}
     */
    export function writeFile(path: string | Buffer | URL | FileHandle, data: string | Buffer | any[] | DataView | TypedArray, options?: object | null): Promise<void>;
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
    import { Stats } from "socket:fs/stats";
    import { Watcher } from "socket:fs/watcher";
    import * as constants from "socket:fs/constants";
    import { DirectoryHandle } from "socket:fs/handle";
    import { Dirent } from "socket:fs/dir";
    import fds from "socket:fs/fds";
    import { ReadStream } from "socket:fs/stream";
    import { WriteStream } from "socket:fs/stream";
    import * as exports from "socket:fs/promises";
    
    export { constants, Dir, DirectoryHandle, Dirent, fds, FileHandle, ReadStream, Watcher, WriteStream };
}
declare module "socket:fs/index" {
    /**
     * Asynchronously check access a file for a given mode calling `callback`
     * upon success or error.
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsopenpath-flags-mode-callback}
     * @param {string | Buffer | URL} path
     * @param {string?|function(Error?)?} [mode = F_OK(0)]
     * @param {function(Error?)?} [callback]
     */
    export function access(path: string | Buffer | URL, mode: any, callback?: ((arg0: Error | null) => any) | null): void;
    /**
     * @ignore
     */
    export function appendFile(path: any, data: any, options: any, callback: any): void;
    /**
     *
     * Asynchronously changes the permissions of a file.
     * No arguments other than a possible exception are given to the completion callback
     *
     * @see {@link https://nodejs.org/api/fs.html#fschmodpath-mode-callback}
     *
     * @param {string | Buffer | URL} path
     * @param {number} mode
     * @param {function(Error?)} callback
     */
    export function chmod(path: string | Buffer | URL, mode: number, callback: (arg0: Error | null) => any): void;
    /**
     * Changes ownership of file or directory at `path` with `uid` and `gid`.
     * @param {string} path
     * @param {number} uid
     * @param {number} gid
     * @param {function} callback
     */
    export function chown(path: string, uid: number, gid: number, callback: Function): void;
    /**
     * Asynchronously close a file descriptor calling `callback` upon success or error.
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsclosefd-callback}
     * @param {number} fd
     * @param {function(Error?)?} [callback]
     */
    export function close(fd: number, callback?: ((arg0: Error | null) => any) | null): void;
    /**
     * Asynchronously copies `src` to `dest` calling `callback` upon success or error.
     * @param {string} src - The source file path.
     * @param {string} dest - The destination file path.
     * @param {number} flags - Modifiers for copy operation.
     * @param {function(Error=)=} [callback] - The function to call after completion.
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fscopyfilesrc-dest-mode-callback}
     */
    export function copyFile(src: string, dest: string, flags: number, callback?: ((arg0: Error | undefined) => any) | undefined): void;
    /**
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fscreatewritestreampath-options}
     * @param {string | Buffer | URL} path
     * @param {object?} [options]
     * @returns {ReadStream}
     */
    export function createReadStream(path: string | Buffer | URL, options?: object | null): ReadStream;
    /**
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fscreatewritestreampath-options}
     * @param {string | Buffer | URL} path
     * @param {object?} [options]
     * @returns {WriteStream}
     */
    export function createWriteStream(path: string | Buffer | URL, options?: object | null): WriteStream;
    /**
     * Invokes the callback with the <fs.Stats> for the file descriptor. See
     * the POSIX fstat(2) documentation for more detail.
     *
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsfstatfd-options-callback}
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
    export function lchown(path: string, uid: number, gid: number, callback: Function): void;
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
     * Asynchronously open a file calling `callback` upon success or error.
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsopenpath-flags-mode-callback}
     * @param {string | Buffer | URL} path
     * @param {string?} [flags = 'r']
     * @param {string?} [mode = 0o666]
     * @param {object?|function?} [options]
     * @param {function(Error?, number?)?} [callback]
     */
    export function open(path: string | Buffer | URL, flags?: string | null, mode?: string | null, options?: any, callback?: ((arg0: Error | null, arg1: number | null) => any) | null): void;
    /**
     * Asynchronously open a directory calling `callback` upon success or error.
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsreaddirpath-options-callback}
     * @param {string | Buffer | URL} path
     * @param {object?|function(Error?, Dir?)} [options]
     * @param {string?} [options.encoding = 'utf8']
     * @param {boolean?} [options.withFileTypes = false]
     * @param {function(Error?, Dir?)?} callback
     */
    export function opendir(path: string | Buffer | URL, options: {}, callback: ((arg0: Error | null, arg1: Dir | null) => any) | null): void;
    /**
     * Asynchronously read from an open file descriptor.
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsreadfd-buffer-offset-length-position-callback}
     * @param {number} fd
     * @param {object | Buffer | TypedArray} buffer - The buffer that the data will be written to.
     * @param {number} offset - The position in buffer to write the data to.
     * @param {number} length - The number of bytes to read.
     * @param {number | BigInt | null} position - Specifies where to begin reading from in the file. If position is null or -1 , data will be read from the current file position, and the file position will be updated. If position is an integer, the file position will be unchanged.
     * @param {function(Error?, number?, Buffer?)} callback
     */
    export function read(fd: number, buffer: object | Buffer | TypedArray, offset: number, length: number, position: number | BigInt | null, options: any, callback: (arg0: Error | null, arg1: number | null, arg2: Buffer | null) => any): void;
    /**
     * Asynchronously read all entries in a directory.
     * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsreaddirpath-options-callback}
     * @param {string | Buffer | URL } path
     * @param {object?|function(Error?, object[])} [options]
     * @param {string?} [options.encoding ? 'utf8']
     * @param {boolean?} [options.withFileTypes ? false]
     * @param {function(Error?, object[])} callback
     */
    export function readdir(path: string | Buffer | URL, options: {}, callback: (arg0: Error | null, arg1: object[]) => any): void;
    /**
     * @param {string | Buffer | URL | number } path
     * @param {object?|function(Error?, Buffer?)} [options]
     * @param {string?} [options.encoding ? 'utf8']
     * @param {string?} [options.flag ? 'r']
     * @param {AbortSignal?} [options.signal]
     * @param {function(Error?, Buffer?)} callback
     */
    export function readFile(path: string | Buffer | URL | number, options: {}, callback: (arg0: Error | null, arg1: Buffer | null) => any): void;
    /**
     * Reads link at `path`
     * @param {string} path
     * @param {function(err, string)} callback
     */
    export function readlink(path: string, callback: (arg0: err, arg1: string) => any): void;
    /**
     * Computes real path for `path`
     * @param {string} path
     * @param {function(err, string)} callback
     */
    export function realpath(path: string, callback: (arg0: err, arg1: string) => any): void;
    /**
     * Renames file or directory at `src` to `dest`.
     * @param {string} src
     * @param {string} dest
     * @param {function} callback
     */
    export function rename(src: string, dest: string, callback: Function): void;
    /**
     * Removes directory at `path`.
     * @param {string} path
     * @param {function} callback
     */
    export function rmdir(path: string, callback: Function): void;
    /**
     *
     * @param {string | Buffer | URL | number } path - filename or file descriptor
     * @param {object?} options
     * @param {string?} [options.encoding ? 'utf8']
     * @param {string?} [options.flag ? 'r']
     * @param {AbortSignal?} [options.signal]
     * @param {function(Error?, Stats?)} callback
     */
    export function stat(path: string | Buffer | URL | number, options: object | null, callback: (arg0: Error | null, arg1: Stats | null) => any): void;
    /**
     * Creates a symlink of `src` at `dest`.
     * @param {string} src
     * @param {string} dest
     */
    export function symlink(src: string, dest: string, type: any, callback: any): void;
    /**
     * Unlinks (removes) file at `path`.
     * @param {string} path
     * @param {function} callback
     */
    export function unlink(path: string, callback: Function): void;
    /**
     * @see {@url https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fswritefilefile-data-options-callback}
     * @param {string | Buffer | URL | number } path - filename or file descriptor
     * @param {string | Buffer | TypedArray | DataView | object } data
     * @param {object?} options
     * @param {string?} [options.encoding ? 'utf8']
     * @param {string?} [options.mode ? 0o666]
     * @param {string?} [options.flag ? 'w']
     * @param {AbortSignal?} [options.signal]
     * @param {function(Error?)} callback
     */
    export function writeFile(path: string | Buffer | URL | number, data: string | Buffer | TypedArray | DataView | object, options: object | null, callback: (arg0: Error | null) => any): void;
    /**
     * Watch for changes at `path` calling `callback`
     * @param {string}
     * @param {function|object=} [options]
     * @param {string=} [options.encoding = 'utf8']
     * @param {?function} [callback]
     * @return {Watcher}
     */
    export function watch(path: any, options?: (Function | object) | undefined, callback?: Function | null): Watcher;
    export default exports;
    export type Buffer = import("socket:buffer").Buffer;
    export type TypedArray = Uint8Array | Int8Array;
    import { ReadStream } from "socket:fs/stream";
    import { WriteStream } from "socket:fs/stream";
    import { Dir } from "socket:fs/dir";
    import { Stats } from "socket:fs/stats";
    import { Watcher } from "socket:fs/watcher";
    import * as constants from "socket:fs/constants";
    import { DirectoryHandle } from "socket:fs/handle";
    import { Dirent } from "socket:fs/dir";
    import fds from "socket:fs/fds";
    import { FileHandle } from "socket:fs/handle";
    import * as promises from "socket:fs/promises";
    import * as exports from "socket:fs/index";
    
    export { constants, Dir, DirectoryHandle, Dirent, fds, FileHandle, promises, ReadStream, Stats, Watcher, WriteStream };
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
declare module "socket:ipc" {
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
        let enabled: any;
        function log(...args: any[]): any;
    }
    /**
     * @ignore
     */
    export function postMessage(message: any, ...args: any[]): Promise<any>;
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
    export function sendSync(command: string, value?: any | null, options?: object | null): Result;
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
        static from(input: any): any;
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
         * @param {any} defaultValue
         * @return {any}
         * @ignore
         */
        get(key: string, defaultValue: any): any;
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
    /**
     * @ignore
     */
    export const primordials: any;
    export default exports;
    import { Buffer } from "socket:buffer";
    import { URL } from "socket:url/index";
    import * as exports from "socket:ipc";
    
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
    export * as _default from "socket:window/constants";
    
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
        constructor({ index, ...options }: {
            [x: string]: any;
            index: any;
        });
        /**
         * Get the index of the window
         * @return {number} - the index of the window
         */
        get index(): number;
        /**
         * Get the size of the window
         * @return {{ width: number, height: number }} - the size of the window
         */
        getSize(): {
            width: number;
            height: number;
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
         * This is a high-level API that you should use instead of `ipc.send` when
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
        }): Promise<ipc.Result>;
        /**
         * Opens an URL in the default browser.
         * @param {object} options
         * @returns {Promise<ipc.Result>}
         */
        openExternal(options: object): Promise<ipc.Result>;
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
}
declare module "socket:application" {
    /**
     * Returns the current window index
     * @return {number}
     */
    export function getCurrentWindowIndex(): number;
    /**
     * Creates a new window and returns an instance of ApplicationWindow.
     * @param {object} opts - an options object
     * @param {number} opts.index - the index of the window
     * @param {string} opts.path - the path to the HTML file to load into the window
     * @param {string=} opts.title - the title of the window
     * @param {(number|string)=} opts.width - the width of the window. If undefined, the window will have the main window width.
     * @param {(number|string)=} opts.height - the height of the window. If undefined, the window will have the main window height.
     * @param {(number|string)=} [opts.minWidth = 0] - the minimum width of the window
     * @param {(number|string)=} [opts.minHeight = 0] - the minimum height of the window
     * @param {(number|string)=} [opts.maxWidth = '100%'] - the maximum width of the window
     * @param {(number|string)=} [opts.maxHeight = '100%'] - the maximum height of the window
     * @param {boolean=} [opts.resizable=true] - whether the window is resizable
     * @param {boolean=} [opts.frameless=false] - whether the window is frameless
     * @param {boolean=} [opts.utility=false] - whether the window is utility (macOS only)
     * @param {boolean=} [opts.canExit=false] - whether the window can exit the app
     * @return {Promise<ApplicationWindow>}
     */
    export function createWindow(opts: {
        index: number;
        path: string;
        title?: string | undefined;
        width?: (number | string) | undefined;
        height?: (number | string) | undefined;
        minWidth?: (number | string) | undefined;
        minHeight?: (number | string) | undefined;
        maxWidth?: (number | string) | undefined;
        maxHeight?: (number | string) | undefined;
        resizable?: boolean | undefined;
        frameless?: boolean | undefined;
        utility?: boolean | undefined;
        canExit?: boolean | undefined;
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
     * @return {Promise<Object.<number, ApplicationWindow>>}
     * @throws {Error} - if indices is not an array of integer numbers
     */
    export function getWindows(indices?: number[]): Promise<{
        [x: number]: ApplicationWindow;
    }>;
    /**
     * Returns the ApplicationWindow instance for the given index
     * @param {number} index - the index of the window
     * @throws {Error} - if index is not a valid integer number
     * @returns {Promise<ApplicationWindow>} - the ApplicationWindow instance or null if the window does not exist
     */
    export function getWindow(index: number): Promise<ApplicationWindow>;
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
     * Set the enabled state of the system menu.
     * @param {object} value - an options object
     * @return {Promise<ipc.Result>}
     */
    export function setSystemMenuItemEnabled(value: object): Promise<ipc.Result>;
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
     * @type {object}
     */
    export const config: object;
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
    import ApplicationWindow from "socket:window";
    import ipc from "socket:ipc";
    import * as exports from "socket:application";
    
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
declare module "socket:net" {
    export default exports;
    export class Server extends EventEmitter {
        constructor(options: any, handler: any);
        _connections: number;
        id: BigInt;
        onconnection(data: any): void;
        listen(port: any, address: any, cb: any): this;
        _address: {
            port: any;
            address: any;
            family: any;
        };
        connections: {};
        address(): {
            port: any;
            address: any;
            family: any;
        };
        close(cb: any): void;
        getConnections(cb: any): void;
        unref(): this;
    }
    export class Socket extends Duplex {
        _server: any;
        _address: any;
        allowHalfOpen: boolean;
        _flowing: boolean;
        setNoDelay(enable: any): void;
        setKeepAlive(enabled: any): void;
        _onTimeout(): void;
        address(): any;
        _final(cb: any): this;
        destroySoon(): void;
        __write(data: any): void;
        _read(cb: any): any;
        pause(): this;
        resume(): this;
        connect(...args: any[]): this;
        id: BigInt;
        remotePort: any;
        remoteAddress: any;
        unref(): this;
        [kLastWriteQueueSize]: any;
    }
    export function connect(...args: any[]): exports.Socket;
    export function createServer(...args: any[]): exports.Server;
    export function getNetworkInterfaces(o: any): any;
    export function isIPv4(s: any): boolean;
    import * as exports from "socket:net";
    import { EventEmitter } from "socket:events";
    import { Duplex } from "socket:stream";
    const kLastWriteQueueSize: unique symbol;
    
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
    import { InternalError } from "socket:errors";
    import * as exports from "socket:dgram";
    
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
declare module "socket:mime/index" {
    /**
     * Look up a MIME type in various MIME databases.
     * @param {string} query
     * @return {Promise<DatabaseQueryResult[]>}
     */
    export function lookup(query: string): Promise<DatabaseQueryResult[]>;
    /**
     * A container for a database lookup query.
     */
    export class DatabaseQueryResult {
        /**
         * `DatabaseQueryResult` class constructor.
         * @ignore
         * @param {Database} database
         * @param {string} name
         * @param {string} mime
         */
        constructor(database: Database, name: string, mime: string);
        /**
         * @type {string}
         */
        name: string;
        /**
         * @type {string}
         */
        mime: string;
        database: Database;
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
         * Lookup MIME type by name or content type
         * @param {string} query
         * @return {Promise<DatabaseQueryResult>}
         */
        lookup(query: string): Promise<DatabaseQueryResult>;
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
    namespace _default {
        export { Database };
        export { databases };
        export { lookup };
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
    }): File;
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
            getFile(): void;
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
        static type(name: string): Promise<'shared' | 'wasm32' | 'unknown' | null>;
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
    export type ExtensionLoadOptions = {
        allow: string[] | string;
        imports?: object;
        type?: 'shared' | 'wasm32';
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
     * {Pointer}
     */
    type $loaded = number;
    /**
     * @typedef {number} {Pointer}
     */
    const $loaded: unique symbol;
    import path from "socket:path";
}
declare module "socket:fetch/fetch" {
    export class DOMException {
        private constructor();
    }
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
    export namespace fetch {
        let polyfill: boolean;
    }
}
declare module "socket:fetch/index" {
    export * from "socket:fetch/fetch";
    export default fetch;
    import { fetch } from "socket:fetch/fetch";
}
declare module "socket:fetch" {
    export * from "socket:fetch/index";
    export default fetch;
    import fetch from "socket:fetch/index";
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
declare module "socket:test/fast-deep-equal" {
    export default function equal(a: any, b: any): boolean;
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
        notDeepEqual<T_1>(actual: T_1, expected: T_1, msg?: string): void;
        /**
         * @template T
         * @param {T} actual
         * @param {T} expected
         * @param {string} [msg]
         * @returns {void}
         */
        equal<T_2>(actual: T_2, expected: T_2, msg?: string): void;
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
        waitForText(selector: string | HTMLElement | Element, opts?: string | RegExp | {
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
        }, msg?: string): Promise<HTMLElement | Element | void>;
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
declare module "socket:module" {
    export function isBuiltin(name: any): boolean;
    /**
     * Creates a `require` function from a source URL.
     * @param {URL|string} sourcePath
     * @return {function}
     */
    export function createRequire(sourcePath: URL | string): Function;
    export default exports;
    /**
     * A limited set of builtins exposed to CommonJS modules.
     */
    export const builtins: {
        buffer: typeof buffer;
        console: import("socket:console").Console;
        dgram: typeof dgram;
        dns: typeof dns;
        'dns/promises': typeof dns.promises;
        events: typeof events;
        extension: {
            load: typeof import("socket:extension").load;
            stats: typeof import("socket:extension").stats;
        };
        fs: typeof fs;
        'fs/promises': typeof fs.promises;
        gc: any;
        ipc: typeof ipc;
        module: typeof exports;
        os: typeof os;
        path: typeof path;
        process: any;
        stream: typeof stream;
        test: typeof test;
        util: typeof util;
        url: any;
    };
    export const builtinModules: {
        buffer: typeof buffer;
        console: import("socket:console").Console;
        dgram: typeof dgram;
        dns: typeof dns;
        'dns/promises': typeof dns.promises;
        events: typeof events;
        extension: {
            load: typeof import("socket:extension").load;
            stats: typeof import("socket:extension").stats;
        };
        fs: typeof fs;
        'fs/promises': typeof fs.promises;
        gc: any;
        ipc: typeof ipc;
        module: typeof exports;
        os: typeof os;
        path: typeof path;
        process: any;
        stream: typeof stream;
        test: typeof test;
        util: typeof util;
        url: any;
    };
    /**
     * CommonJS module scope source wrapper.
     * @type {string}
     */
    export const COMMONJS_WRAPPER: string;
    /**
     * The main entry source origin.
     * @type {string}
     */
    export const MAIN_SOURCE_ORIGIN: string;
    export namespace scope {
        let current: any;
        let previous: any;
    }
    /**
     * A container for a loaded CommonJS module. All errors bubble
     * to the "main" module and global object (if possible).
     */
    export class Module extends EventTarget {
        static set current(module: exports.Module);
        /**
         * A reference to the currently scoped module.
         * @type {Module?}
         */
        static get current(): exports.Module;
        static set previous(module: exports.Module);
        /**
         * A reference to the previously scoped module.
         * @type {Module?}
         */
        static get previous(): exports.Module;
        /**
         * Module cache.
         * @ignore
         */
        static cache: any;
        /**
         * Custom module resolvers.
         * @type {Array<ModuleResolver>}
         */
        static resolvers: Array<ModuleResolver>;
        /**
         * CommonJS module scope source wrapper.
         * @ignore
         */
        static wrapper: string;
        /**
         * Creates a `require` function from a source URL.
         * @param {URL|string} sourcePath
         * @return {function}
         */
        static createRequire(sourcePath: URL | string): Function;
        /**
         * The main entry module, lazily created.
         * @type {Module}
         */
        static get main(): exports.Module;
        /**
         * Wraps source in a CommonJS module scope.
         */
        static wrap(source: any): string;
        /**
         * Creates a `Module` from source URL and optionally a parent module.
         * @param {string|URL|Module} [sourcePath]
         * @param {string|URL|Module} [parent]
         */
        static from(sourcePath?: string | URL | Module, parent?: string | URL | Module): any;
        /**
         * `Module` class constructor.
         * @ignore
         */
        constructor(id: any, parent?: any, sourcePath?: any);
        /**
         * The module id, most likely a file name.
         * @type {string}
         */
        id: string;
        /**
         * The parent module, if given.
         * @type {Module?}
         */
        parent: Module | null;
        /**
         * `true` if the module did load successfully.
         * @type {boolean}
         */
        loaded: boolean;
        /**
         * The module's exports.
         * @type {any}
         */
        exports: any;
        /**
         * The filename of the module.
         * @type {string}
         */
        filename: string;
        /**
         * Modules children to this one, as in they were required in this
         * module scope context.
         * @type {Array<Module>}
         */
        children: Array<Module>;
        /**
         * The original source URL to load this module.
         * @type {string}
         */
        sourcePath: string;
        /**
         * `true` if the module is the main module.
         * @type {boolean}
         */
        get isMain(): boolean;
        /**
         * `true` if the module was loaded by name, not file path.
         * @type {boolean}
         */
        get isNamed(): boolean;
        /**
         * @type {URL}
         */
        get url(): URL;
        /**
         * @type {string}
         */
        get pathname(): string;
        /**
         * @type {string}
         */
        get path(): string;
        /**
         * Loads the module, synchronously returning `true` upon success,
         * otherwise `false`.
         * @return {boolean}
         */
        load(): boolean;
        /**
         * Creates a require function for loaded CommonJS modules
         * child to this module.
         * @return {function(string): any}
         */
        createRequire(): (arg0: string) => any;
        /**
         * Requires a module at `filename` that will be loaded as a child
         * to this module.
         * @param {string} filename
         * @return {any}
         */
        require(filename: string): any;
        /**
         * @ignore
         */
        [Symbol.toStringTag](): string;
    }
    export type ModuleResolver = (arg0: string, arg1: Module, arg2: Function) => undefined;
    import { URL } from "socket:url/index";
    import * as exports from "socket:module";
    import buffer from "socket:buffer";
    import dgram from "socket:dgram";
    import dns from "socket:dns";
    import events from "socket:events";
    import fs from "socket:fs";
    import ipc from "socket:ipc";
    import os from "socket:os";
    import { posix as path } from "socket:path";
    import stream from "socket:stream";
    import test from "socket:test";
    import util from "socket:util";
    
}
declare module "socket:stream-relay/packets" {
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
    export function validatePacket(o: any, constraints: {
        [key: string]: {
            required: boolean;
            type: string;
        };
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
        constructor({ message, clusterId, subclusterId }: {
            message: any;
            clusterId: any;
            subclusterId: any;
        });
    }
    export class PacketPong extends Packet {
        static type: number;
        constructor({ message, clusterId, subclusterId }: {
            message: any;
            clusterId: any;
            subclusterId: any;
        });
    }
    export class PacketIntro extends Packet {
        static type: number;
        constructor({ clock, hops, clusterId, subclusterId, message }: {
            clock: any;
            hops: any;
            clusterId: any;
            subclusterId: any;
            message: any;
        });
    }
    export class PacketJoin extends Packet {
        static type: number;
        constructor({ clock, hops, clusterId, subclusterId, message }: {
            clock: any;
            hops: any;
            clusterId: any;
            subclusterId: any;
            message: any;
        });
    }
    export class PacketPublish extends Packet {
        static type: number;
        constructor({ message, sig, packetId, clusterId, subclusterId, nextId, clock, hops, usr1, usr2, ttl, previousId }: {
            message: any;
            sig: any;
            packetId: any;
            clusterId: any;
            subclusterId: any;
            nextId: any;
            clock: any;
            hops: any;
            usr1: any;
            usr2: any;
            ttl: any;
            previousId: any;
        });
    }
    export class PacketStream extends Packet {
        static type: number;
        constructor({ message, sig, packetId, clusterId, subclusterId, nextId, clock, ttl, usr1, usr2, usr3, usr4, previousId }: {
            message: any;
            sig: any;
            packetId: any;
            clusterId: any;
            subclusterId: any;
            nextId: any;
            clock: any;
            ttl: any;
            usr1: any;
            usr2: any;
            usr3: any;
            usr4: any;
            previousId: any;
        });
    }
    export class PacketSync extends Packet {
        static type: number;
        constructor({ packetId, message }: {
            packetId: any;
            message?: any;
        });
    }
    export class PacketQuery extends Packet {
        static type: number;
        constructor({ packetId, previousId, subclusterId, usr1, usr2, usr3, usr4, message }: {
            packetId: any;
            previousId: any;
            subclusterId: any;
            usr1: any;
            usr2: any;
            usr3: any;
            usr4: any;
            message?: {};
        });
    }
    export default Packet;
    import { Buffer } from "socket:buffer";
}
declare module "socket:stream-relay/encryption" {
    export class Encryption {
        static createSharedKey(seed: any): Promise<any>;
        static createKeyPair(seed: any): Promise<any>;
        static createId(str?: Buffer): Promise<string>;
        static createClusterId(value: any): Promise<any>;
        static createSubclusterId(value: any): Promise<any>;
        /**
         * @param {Buffer} b - The message to sign
         * @param {Uint8Array} sk - The secret key to use
         */
        static sign(b: Buffer, sk: Uint8Array): any;
        /**
         * @param {Buffer} b - The message to verify
         * @param {Uint8Array} pk - The public key to use
         * @return {number} - Returns non zero if the buffer could not be verified
         */
        static verify(b: Buffer, sig: any, pk: Uint8Array): number;
        keys: {};
        add(publicKey: any, privateKey: any): void;
        remove(publicKey: any): void;
        has(to: any): boolean;
        /**
         * `Open(message, receiver)` performs a _decrypt-verify-decrypt_ (DVD) on a
         * ciphertext `message` for a `receiver` identity. Receivers who open the
         * `message` ciphertext can be guaranteed non-repudiation without relying on
         * the packet chain integrity.
         *
         * let m = Open(in, pk, sk)
         * let sig = Slice(m, 0, 64)
         * let ct = Slice(m, 64)
         * if (verify(sig, ct, pk)) {
         *   let out = open(ct, pk, sk)
         * }
         */
        open(message: any, v: any): any;
        openMessage(message: any, v: any): any;
        sealMessage(message: any, publicKey: any): any;
        /**
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
         */
        seal(message: any, v: any): any;
    }
    import Buffer from "socket:buffer";
}
declare module "socket:stream-relay/cache" {
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
    export function trim(buf: Buffer): any;
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
    import { Buffer } from "socket:buffer";
    import { Packet } from "socket:stream-relay/packets";
}
declare module "socket:stream-relay/nat" {
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
declare module "socket:stream-relay/index" {
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
    export function debug(pid: any, ...args: any[]): void;
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
        write(sharedKey: any, args: any): Promise<any>;
    }
    export function wrap(dgram: any): {
        new (persistedState?: object | null): {
            port: any;
            address: any;
            natType: number;
            nextNatType: number;
            clusters: {};
            reflectionId: any;
            reflectionTimeout: any;
            reflectionStage: number;
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
                    REJECTED: number;
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
            _scheduleSend(): void;
            sendTimeout: number;
            /**
             * @private
             */
            _dequeue(): void;
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
            probeReflectionTimeout: any;
            /**
             * Ping another peer
             * @return {PacketPing}
             * @ignore
             */
            ping(peer: any, withRetry: any, props: any, socket: any): PacketPing;
            getPeer(id: any): any;
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
            sync(peer: any): undefined;
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
            cachePredicate(packet: any): boolean;
            /**
             * A connection was made, add the peer to the local list of known
             * peers and call the onConnection if it is defined by the user.
             *
             * @return {undefined}
             * @ignore
             */
            _onConnection(packet: any, peerId: any, port: any, address: any, proxy: any, socket: any): undefined;
            connections: Map<any, any>;
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
        };
    };
    export default wrap;
    import { Packet } from "socket:stream-relay/packets";
    import { sha256 } from "socket:stream-relay/packets";
    import { Cache } from "socket:stream-relay/cache";
    import { Encryption } from "socket:stream-relay/encryption";
    import * as NAT from "socket:stream-relay/nat";
    import { Buffer } from "socket:buffer";
    import { PacketPing } from "socket:stream-relay/packets";
    import { PacketPublish } from "socket:stream-relay/packets";
    export { Packet, sha256, Cache, Encryption, NAT };
}
declare module "socket:stream-relay/sugar" {
    function _default(dgram: any, events: any): (options?: {}) => Promise<any>;
    export default _default;
}
declare module "socket:network" {
    export default network;
    export const network: (options?: {}) => Promise<any>;
    import { Cache } from "socket:stream-relay/index";
    import { sha256 } from "socket:stream-relay/index";
    import { Encryption } from "socket:stream-relay/index";
    import { Packet } from "socket:stream-relay/index";
    import { NAT } from "socket:stream-relay/index";
    export { Cache, sha256, Encryption, Packet, NAT };
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
        });
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
        static requestPermission(options?: object | undefined): Promise<'granted' | 'default' | 'denied'>;
        /**
         * `Notification` class constructor.
         * @param {string} title
         * @param {NotificationOptions=} [options]
         */
        constructor(title: string, options?: NotificationOptions | undefined, ...args: any[]);
        /**
         * A unique identifier for this notification.
         * @type {string}
         */
        get id(): string;
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
declare module "socket:stream-relay" {
    export * from "socket:stream-relay/index";
    export default def;
    import def from "socket:stream-relay/index";
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
    namespace _default {
        export { ApplicationURLEvent };
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
declare module "socket:internal/globals" {
    /**
     * Gets a global by name.
     * @ignore
     */
    export function get(name: any): any;
    export default registry;
    /**
     * Symbolic global registry
     * @ignore
     */
    const registry: {
        readonly global: any;
        symbol(name: any): symbol;
        register(name: any, value: any): any;
        get(name: any): any;
    };
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
declare module "socket:internal/pickers" {
    /**
     * @typedef {{
     *   id?: string,
     *   mode?: 'read' | 'readwrite'
     *   startIn?: FileSystemHandle | 'desktop' | 'documents' | 'downloads' | 'music' | 'pictures' | 'videos',
     * }} ShowDirectoryPickerOptions
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
    export function showOpenFilePicker(options?: ShowOpenFilePickerOptions): Promise<FileSystemFileHandle[]>;
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
    export function showSaveFilePicker(options?: ShowSaveFilePickerOptions): Promise<FileSystemHandle>;
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
        mode?: 'read' | 'readwrite';
        startIn?: FileSystemHandle | 'desktop' | 'documents' | 'downloads' | 'music' | 'pictures' | 'videos';
    };
    /**
     * ]?: string[]
     *   }>
     * }} ShowOpenFilePickerOptions
     */
    export type object = {
        id?: string;
        excludeAcceptAllOption?: boolean;
        startIn?: FileSystemHandle | 'desktop' | 'documents' | 'downloads' | 'music' | 'pictures' | 'videos';
        types?: Array<{
            description?: string;
            [keyof];
        }>;
    };
    import { FileSystemHandle } from "socket:fs/web";
}
declare module "socket:internal/monkeypatch" {
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
    const _default: any;
    export default _default;
}
declare module "socket:internal/worker" {
    export function onWorkerMessage(event: any): any;
    export function addEventListener(eventName: any, callback: any, ...args: any[]): any;
    export function removeEventListener(eventName: any, callback: any, ...args: any[]): any;
    export function dispatchEvent(event: any): any;
    export function postMessage(message: any, ...args: any[]): any;
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
    namespace _default {
        export { RUNTIME_WORKER_ID };
        export { removeEventListener };
        export { addEventListener };
        export { dispatchEvent };
        export { postMessage };
        export { source };
        export { url };
    }
    export default _default;
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
    export function wrapHarness<T extends exports.Harness>(tapzero: typeof import("socket:test/index"), harnessClass: new (options: object) => T): exports.TapeTestFn<T>;
    export default exports;
    /**
     * @template {Harness} T
     */
    export class TapeHarness<T extends exports.Harness> {
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
    export type TapeTestFn<T extends exports.Harness> = {
        (name: string, cb?: (harness: T, test: Test) => (void | Promise<void>)): void;
        (name: string, opts: object, cb: (harness: T, test: Test) => (void | Promise<void>)): void;
        only(name: string, cb?: (harness: T, test: Test) => (void | Promise<void>)): void;
        only(name: string, opts: object, cb: (harness: T, test: Test) => (void | Promise<void>)): void;
        skip(name: string, cb?: (harness: T, test: Test) => (void | Promise<void>)): void;
        skip(name: string, opts: object, cb: (harness: T, test: Test) => (void | Promise<void>)): void;
    };
    import * as exports from "socket:test/harness";
    
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