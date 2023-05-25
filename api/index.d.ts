declare module "socket:errors" {
    export default exports;
    export const ABORT_ERR: 20;
    export const ENCODING_ERR: 32;
    export const INVALID_ACCESS_ERR: 15;
    export const INDEX_SIZE_ERR: 1;
    export const NETWORK_ERR: 19;
    export const NOT_ALLOWED_ERR: 31;
    export const NOT_FOUND_ERR: 8;
    export const NOT_SUPPORTED_ERR: 9;
    export const OPERATION_ERR: 30;
    export const TIMEOUT_ERR: 23;
    /**
     * An `AbortError` is an error type thrown in an `onabort()` level 0
     * event handler on an `AbortSignal` instance.
     */
    export class AbortError extends Error {
        /**
         * The code given to an `ABORT_ERR` `DOMException`
         * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): number;
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
        static get code(): number;
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
         * The code given to an `NOT_FOUND_ERR` `DOMException`
         */
        static get code(): number;
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
        set code(arg: number | "INTERNAL_ERR");
        get code(): number | "INTERNAL_ERR";
        [exports.kInternalErrorCode]: number;
    }
    /**
     * An `InvalidAccessError` is an error type thrown when an internal exception
     * has occurred, such as in the native IPC layer.
     */
    export class InvalidAccessError extends Error {
        /**
         * The code given to an `INVALID_ACCESS_ERR` `DOMException`
         * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): number;
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
         * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): number;
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
         * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): number;
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
         * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): number;
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
         * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): number;
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
         * The code given to an `NOT_FOUND_ERR` `DOMException`
         */
        static get code(): number;
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
     * An `TimeoutError` is an error type thrown when an operation timesout.
     */
    export class TimeoutError extends Error {
        /**
         * The code given to an `TIMEOUT_ERR` `DOMException`
         * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
         */
        static get code(): number;
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
        swap16(): Buffer;
        swap32(): Buffer;
        swap64(): Buffer;
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
        fill(val: any, start: any, end: any, encoding: any): Buffer;
    }
    export namespace Buffer {
        export const TYPED_ARRAY_SUPPORT: boolean;
        export const poolSize: number;
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
        const custom: symbol;
        const ignore: symbol;
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
        function log(): any;
        const enabled: any;
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
     * @param {object|string?} [params]
     * @param {object?} [options]
     * @return {Result}
     * @ignore
     */
    export function sendSync(command: string, params?: object | (string | null), options?: object | null): Result;
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
     * @param {object=} params
     * @param {(Buffer|Uint8Array|ArrayBuffer|string|Array)=} buffer
     * @param {object=} options
     * @ignore
     */
    export function write(command: string, params?: object | undefined, buffer?: (Buffer | Uint8Array | ArrayBuffer | string | any[]) | undefined, options?: object | undefined): Promise<any>;
    /**
     * Sends an async IPC command request with parameters requesting a response
     * with buffered bytes.
     * @param {string} command
     * @param {object=} params
     * @param {object=} options
     * @ignore
     */
    export function request(command: string, params?: object | undefined, options?: object | undefined): Promise<any>;
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
            [k: string]: any;
        };
    }
    /**
     * A container for a IPC message based on a `ipc://` URI scheme.
     * @ignore
     */
    export class Message extends URL {
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
         * @param {object|Uint8Array?} [bytes]
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
         * @ignore
         */
        get command(): string;
        /**
         * Computed IPC message name.
         * @ignore
         */
        get name(): string;
        /**
         * Computed `id` value for the command.
         * @ignore
         */
        get id(): string;
        /**
         * Computed `seq` (sequence) value for the command.
         * @ignore
         */
        get seq(): any;
        /**
         * Computed message value potentially given in message parameters.
         * This value is automatically decoded, but not treated as JSON.
         * @ignore
         */
        get value(): any;
        /**
         * Computed `index` value for the command potentially referring to
         * the window index the command is scoped to or originating from. If not
         * specified in the message parameters, then this value defaults to `-1`.
         * @ignore
         */
        get index(): number;
        /**
         * Computed value parsed as JSON. This value is `null` if the value is not present
         * or it is invalid JSON.
         * @ignore
         */
        get json(): any;
        /**
         * Computed readonly object of message parameters.
         * @ignore
         */
        get params(): any;
        /**
         * Gets unparsed message parameters.
         * @ignore
         */
        get rawParams(): {
            [k: string]: any;
        };
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
        set(key: string, value: any): void;
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
         * @param {object|Error|any?} result
         * @param {Error|object} [maybeError]
         * @param {string} [maybeSource]
         * @param {object|string|Headers} [maybeHeaders]
         * @return {Result}
         * @ignore
         */
        static from(result: object | Error | (any | null), maybeError?: Error | object, maybeSource?: string, maybeHeaders?: object | string | Headers): Result;
        /**
         * `Result` class constructor.
         * @private
         * @param {Error?} [err = null]
         * @param {object?} [data = null]
         * @param {string?} [source = null]
         * @param {object|string|Headers?} [headers = null]
         * @ignore
         */
        private constructor();
        /**
         * @type {Error?}
         * @ignore
         */
        err: Error | null;
        /**
         * @type {string|object|Uint8Array}
         * @ignore
         */
        data: string | object | Uint8Array;
        /**
         * @type {string?}
         * @ignore
         */
        source: string | null;
        /**
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
            headers: Headers;
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
    import * as exports from "socket:ipc";
    
}
declare module "socket:polyfills" {
    export function applyPolyfills(): void;
    export default exports;
    import * as exports from "socket:polyfills";
    
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
    export function formatFileUrl(url: string): string;
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
         * Sends an IPC message to the window or to qthe backend.
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
    export const constants: typeof statuses;
    import ipc from "socket:ipc";
    import * as statuses from "socket:window/constants";
}
declare module "socket:application" {
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
     * @returns {Promise<ipc.Result>}
     */
    export function getScreenSize(): Promise<ipc.Result>;
    /**
     * Returns the ApplicationWindow instances for the given indices or all windows if no indices are provided.
     * @param {number[]|undefined} indices - the indices of the windows
     * @return {Promise<Object.<number, ApplicationWindow>>}
     * @throws {Error} - if indices is not an array of integer numbers
     */
    export function getWindows(indices: number[] | undefined): Promise<{
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
     * @param {object} code - an exit code
     * @return {Promise<ipc.Result>}
     */
    export function exit(code: object): Promise<ipc.Result>;
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
    export const CustomEvent: {
        new <T>(type: string, eventInitDict?: CustomEventInit<T>): CustomEvent<T>;
        prototype: CustomEvent<any>;
    } | {
        new (type: any, options: any): {
            "__#2@#detail": any;
            readonly detail: any;
        };
    };
    export const MessageEvent: {
        new <T>(type: string, eventInitDict?: MessageEventInit<T>): MessageEvent<T>;
        prototype: MessageEvent<any>;
    } | {
        new (type: any, options: any): {
            "__#3@#detail": any;
            "__#3@#data": any;
            readonly detail: any;
            readonly data: any;
        };
    };
    export const ErrorEvent: {
        new (type: string, eventInitDict?: ErrorEventInit): ErrorEvent;
        prototype: ErrorEvent;
    } | {
        new (type: any, options: any): {
            "__#4@#detail": any;
            "__#4@#error": any;
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
        setMaxListeners(n: any): EventEmitter;
        getMaxListeners(): any;
        emit(type: any, ...args: any[]): boolean;
        addListener(type: any, listener: any): any;
        on: any;
        prependListener(type: any, listener: any): any;
        once(type: any, listener: any): EventEmitter;
        prependOnceListener(type: any, listener: any): EventEmitter;
        removeListener(type: any, listener: any): EventEmitter;
        off: any;
        removeAllListeners(type: any, ...args: any[]): EventEmitter;
        listeners(type: any): any[];
        rawListeners(type: any): any[];
        listenerCount: typeof listenerCount;
        eventNames(): any;
    }
    export namespace EventEmitter {
        export { EventEmitter };
        export const defaultMaxListeners: number;
        export function init(): void;
        export function listenerCount(emitter: any, type: any): any;
        export { once };
    }
    export function once(emitter: any, name: any): Promise<any>;
    import * as exports from "socket:events";
    function listenerCount(type: any): any;
    
}
declare module "socket:os" {
    export function arch(): any;
    export function cpus(): any;
    export function networkInterfaces(): any;
    export function platform(): any;
    export function type(): string;
    export function isWindows(): boolean;
    export function tmpdir(): string;
    export function rusage(): any;
    export function uptime(): any;
    export function uname(): string;
    export function hrtime(): any;
    export function availableMemory(): any;
    export const EOL: "\n" | "\r\n";
    export default exports;
    import * as exports from "socket:os";
    
}
declare module "socket:process" {
    export function nextTick(callback: any): void;
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
    export function memoryUsage(): {
        rss: any;
    };
    export namespace memoryUsage {
        function rss(): any;
    }
    export default process;
    const process: any;
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
        subscribers: any[];
        name: any;
        group: any;
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
        subscribe(onMessage: any): boolean;
        /**
         * Removes an `onMessage` subscription callback from the channel.
         * @param {function} onMessage
         * @return {boolean}
         */
        unsubscribe(onMessage: Function): boolean;
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
         * Computed subscribers for all channels in this group.
         */
        get subscribers(): any[];
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
        /**
         * Iterator iterface.
         * @ignore
         */
        get [Symbol.iterator](): Channel[];
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
        update(): void;
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
        update(value: any): void;
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
    import * as exports from "socket:diagnostics";
    
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
declare module "socket:path/path" {
    /**
     * @typedef {(string|Path|URL|{ pathname: string }|{ url: string)} PathComponent
     */
    /**
     * A container for a parsed Path.
     */
    export class Path extends URL {
        /**
         * Computes current working directory for a path
         * @param {object=} [opts]
         * @param {boolean=} [opts.posix] Set to `true` to force POSIX style path
         * @return {string}
         */
        static cwd(opts?: object | undefined): string;
        /**
         * Resolves path components to an absolute path.
         * @param {object} options
         * @param {...PathComponent} components
         * @return {string}
         */
        static resolve(options: object, ...components: PathComponent[]): string;
        /**
         * Computes the relative path from `from` to `to`.
         * @param {object} options
         * @param {PathComponent} from
         * @param {PathComponent} to
         * @return {string}
         */
        static relative(options: object, from: PathComponent, to: PathComponent): string;
        /**
         * Joins path components. This function may not return an absolute path.
         * @param {object} options
         * @param {...PathComponent} components
         * @return {string}
         */
        static join(options: object, ...components: PathComponent[]): string;
        /**
         * Computes directory name of path.
         * @param {object} options
         * @param {...PathComponent} components
         * @return {string}
         */
        static dirname(options: object, path: any): string;
        /**
         * Computes base name of path.
         * @param {object} options
         * @param {...PathComponent} components
         * @return {string}
         */
        static basename(options: object, path: any): string;
        /**
         * Computes extension name of path.
         * @param {object} options
         * @param {PathComponent} path
         * @return {string}
         */
        static extname(options: object, path: PathComponent): string;
        /**
         * Computes normalized path
         * @param {object} options
         * @param {PathComponent} path
         * @return {string}
         */
        static normalize(options: object, path: PathComponent): string;
        /**
         * Formats `Path` object into a string.
         * @param {object} options
         * @param {object|Path} path
         * @return {string}
         */
        static format(options: object, path: object | Path): string;
        /**
         * Parses input `path` into a `Path` instance.
         * @param {PathComponent} path
         * @return {object}
         */
        static parse(path: PathComponent): object;
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
        /**
         * `true` if the path is relative, otherwise `false.
         * @type {boolean}
         */
        get isRelative(): boolean;
        /**
         * The working value of this path.
         */
        get value(): string;
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
}
declare module "socket:path/win32" {
    /**
     * Computes current working directory for a path
     * @param {string}
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
     * Normalizes `path` resolving `..` and `.\` preserving trailing
     * slashes.
     * @param {string} path
     */
    export function normalize(path: string): string;
    /**
     * Computes the relative path from `from` to `to`.
     * @param {string} from
     * @param {string} to
     * @return {string}
     */
    export function relative(from: string, to: string): string;
    export * as _default from "socket:path/win32";
    export * as win32 from "socket:path/win32";
    export const sep: "\\";
    export const delimiter: ";";
    export type PathComponent = import("socket:path/path").PathComponent;
    import posix from "socket:path/posix";
    import { Path } from "socket:path/path";
    
    export { posix, Path };
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
    export function normalize(path: string): string;
    /**
     * Computes the relative path from `from` to `to`.
     * @param {string} from
     * @param {string} to
     * @return {string}
     */
    export function relative(from: string, to: string): string;
    export * as _default from "socket:path/posix";
    export * as posix from "socket:path/posix";
    export const sep: "/";
    export const delimiter: ":";
    export type PathComponent = import("socket:path/path").PathComponent;
    import win32 from "socket:path/win32";
    import { Path } from "socket:path/path";
    
    export { win32, Path };
}
declare module "socket:path/index" {
    export * as _default from "socket:path/index";
    
    import posix from "socket:path/posix";
    import win32 from "socket:path/win32";
    import { Path } from "socket:path/path";
    export { posix, win32, Path };
}
declare module "socket:path" {
    export const sep: "\\" | "/";
    export const delimiter: ";";
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
    export { Path, posix, win32 };
}
declare module "socket:fs" {
    export * from "socket:fs/index";
    export default exports;
    import * as exports from "socket:fs";
    
}
declare module "socket:external/libsodium/index" {
    export {};
}
declare module "socket:crypto/sodium" {
    const _default: any;
    export default _default;
}
declare module "socket:crypto" {
    /**
     * Generate cryptographically strong random values into the `buffer`
     * @param {TypedArray} buffer
     * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Crypto/getRandomValues}
     * @return {TypedArray}
     */
    export function getRandomValues(buffer: TypedArray, ...args: any[]): TypedArray;
    export function rand64(): bigint;
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
     * WebCrypto API
     * @see {https://developer.mozilla.org/en-US/docs/Web/API/Crypto}
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
    import { Buffer } from "socket:buffer";
    export namespace sodium {
        const ready: Promise<any>;
    }
    import * as exports from "socket:crypto";
    
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
        resume(): exports.Readable;
        pause(): exports.Readable;
    }
    export class Writable extends exports.Stream {
        static isBackpressured(ws: any): boolean;
        _writableState: exports.WritableState;
        _writev(batch: any, cb: any): void;
        _write(data: any, cb: any): void;
        _final(cb: any): void;
        write(data: any): boolean;
        end(data: any): exports.Writable;
    }
    export class Duplex extends exports.Readable {
        _writableState: exports.WritableState;
        _writev(batch: any, cb: any): void;
        _write(data: any, cb: any): void;
        _final(cb: any): void;
        write(data: any): boolean;
        end(data: any): exports.Duplex;
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
    import * as exports from "socket:fs/stream";
    import { Writable } from "socket:stream";
    
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
     * @TODO
     */
    export class Stats {
        /**
         * @TODO
         */
        static from(stat: any, fromBigInt: any): exports.Stats;
        /**
         * `Stats` class constructor.
         * @param {object} stat
         */
        constructor(stat: object);
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
        isDirectory(): boolean;
        isFile(): boolean;
        isBlockDevice(): boolean;
        isCharacterDevice(): boolean;
        isSymbolicLink(): boolean;
        isFIFO(): boolean;
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
declare module "socket:fs/handle" {
    export const kOpening: unique symbol;
    export const kClosing: unique symbol;
    export const kClosed: unique symbol;
    /**
     * A container for a descriptor tracked in `fds` and opened in the native layer.
     * This class implements the Node.js `FileHandle` interface
     * @see {https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#class-filehandle}
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
         * @return {boolean}
         */
        static access(path: string, mode?: number, options?: object | undefined): boolean;
        /**
         * Asynchronously open a file.
         * @see {https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fspromisesopenpath-flags-mode}
         * @param {string | Buffer | URL} path
         * @param {string=} [flags = 'r']
         * @param {string=} [mode = 0o666]
         * @param {object=} [options]
         */
        static open(path: string | Buffer | URL, flags?: string | undefined, mode?: string | undefined, options?: object | undefined): Promise<exports.FileHandle>;
        /**
         * `FileHandle` class constructor
         * @private
         * @param {object} options
         */
        private constructor();
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
        createReadStream(options?: object | undefined): any;
        /**
         * Creates a `WriteStream` for the underlying file.
         * @param {object=} [options] - An options object
         */
        createWriteStream(options?: object | undefined): any;
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
         * @param {number} offset
         * @param {number} length
         * @param {number} position
         * @param {object=} [options]
         */
        read(buffer: Buffer | object, offset: number, length: number, position: number, options?: object | undefined): Promise<{
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
         * @param {object=} [options]
         */
        readv(buffers: any, position: any): Promise<void>;
        /**
         * Returns the stats of the underlying file.
         * @param {object=} [options]
         */
        stat(options?: object | undefined): Promise<Stats>;
        /**
         * @param {object=} [options]
         */
        sync(): Promise<void>;
        /**
         * @param {object=} [options]
         */
        truncate(length: any): Promise<void>;
        /**
         * @param {object=} [options]
         */
        utimes(atime: any, mtime: any): Promise<void>;
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
        /**
         * @param {object=} [options]
         */
        writev(buffers: any, position: any): Promise<void>;
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
         */
        static open(path: string | Buffer | URL, options?: object | undefined): Promise<exports.DirectoryHandle>;
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
         */
        open(options?: object | undefined): Promise<any>;
        /**
         * Close underlying directory handle
         * @param {object=} [options]
         */
        close(options?: object | undefined): Promise<any>;
        /**
         * Reads
         * @param {object=} [options]
         */
        read(options?: object | undefined): Promise<any>;
        [exports.kOpening]: any;
        [exports.kClosing]: any;
        [exports.kClosed]: boolean;
    }
    export default exports;
    import { EventEmitter } from "socket:events";
    import { Buffer } from "socket:buffer";
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
     * @see {https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#class-fsdir}
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
         * Closes container and underlying handle.
         * @param {object|function} options
         * @param {function=} callback
         */
        close(options: object | Function, callback?: Function | undefined): Promise<any>;
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
     * @see {https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#class-fsdirent}
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
declare module "socket:fs/promises" {
    /**
     * Asynchronously check access a file.
     * @see {@link https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fspromisesaccesspath-mode}
     * @param {string | Buffer | URL} path
     * @param {string=} [mode]
     * @param {object=} [options]
     */
    export function access(path: string | Buffer | URL, mode?: string | undefined, options?: object | undefined): Promise<boolean>;
    /**
     * @TODO
     * @ignore
     */
    export function appendFile(path: any, data: any, options: any): Promise<void>;
    /**
     * @see {@link https://nodejs.org/api/fs.html#fspromiseschmodpath-mode}
     * @param {string | Buffer | URL} path
     * @param {number} mode
     * @returns {Promise<void>}
     */
    export function chmod(path: string | Buffer | URL, mode: number): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function chown(path: any, uid: any, gid: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function copyFile(src: any, dst: any, mode: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function lchmod(path: any, mode: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function lchown(path: any, uid: any, gid: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function lutimes(path: any, atime: any, mtime: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function link(existingPath: any, newPath: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function lstat(path: any, options: any): Promise<void>;
    /**
     * Asynchronously creates a directory.
     * @todo recursive option is not implemented yet.
     *
     * @param {String} path - The path to create
     * @param {Object} options - The optional options argument can be an integer specifying mode (permission and sticky bits), or an object with a mode property and a recursive property indicating whether parent directories should be created. Calling fs.mkdir() when path is a directory that exists results in an error only when recursive is false.
     * @return {Primise<any>} - Upon success, fulfills with undefined if recursive is false, or the first directory path created if recursive is true.
     */
    export function mkdir(path: string, options?: any): Primise<any>;
    /**
     * Asynchronously open a file.
     * @see {@link https://nodejs.org/api/fs.html#fspromisesopenpath-flags-mode }
     *
     * @param {string | Buffer | URL} path
     * @param {string} flags - default: 'r'
     * @param {string} mode - default: 0o666
     * @return {Promise<FileHandle>}
     */
    export function open(path: string | Buffer | URL, flags: string, mode: string): Promise<FileHandle>;
    /**
     * @see {@link https://nodejs.org/api/fs.html#fspromisesopendirpath-options}
     * @param {string | Buffer | URL} path
     * @param {object=} [options]
     * @param {string=} [options.encoding = 'utf8']
     * @param {number=} [options.bufferSize = 32]
     * @return {Promise<FileSystem,Dir>}
     */
    export function opendir(path: string | Buffer | URL, options?: object | undefined): Promise<FileSystem, Dir>;
    /**
     * @see {@link https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fspromisesreaddirpath-options}
     * @param {string | Buffer | URL} path
     * @param {object=} options
     * @param {string=} [options.encoding = 'utf8']
     * @param {boolean=} [options.withFileTypes = false]
     */
    export function readdir(path: string | Buffer | URL, options?: object | undefined): Promise<any[]>;
    /**
     * @see {@link https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fspromisesreadfilepath-options}
     * @param {string} path
     * @param {object=} [options]
     * @param {(string|null)=} [options.encoding = null]
     * @param {string=} [options.flag = 'r']
     * @param {AbortSignal=} [options.signal]
     * @return {Promise<Buffer | string>}
     */
    export function readFile(path: string, options?: object | undefined): Promise<Buffer | string>;
    /**
     * @TODO
     * @ignore
     */
    export function readlink(path: any, options: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function realpath(path: any, options: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function rename(oldPath: any, newPath: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function rmdir(path: any, options: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function rm(path: any, options: any): Promise<void>;
    /**
     * @see {@link https://nodejs.org/api/fs.html#fspromisesstatpath-options}
     * @param {string | Buffer | URL} path
     * @param {object=} [options]
     * @param {boolean=} [options.bigint = false]
     * @return {Promise<Stats>}
     */
    export function stat(path: string | Buffer | URL, options?: object | undefined): Promise<Stats>;
    /**
     * @TODO
     * @ignore
     */
    export function symlink(target: any, path: any, type: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function truncate(path: any, length: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function unlink(path: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function utimes(path: any, atime: any, mtime: any): Promise<void>;
    /**
     * @TODO
     * @ignore
     */
    export function watch(path: any, options: any): Promise<void>;
    /**
     * @see {@link https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fspromiseswritefilefile-data-options}
     * @param {string | Buffer | URL | FileHandle} path - filename or FileHandle
     * @param {string|Buffer|Array|DataView|TypedArray|Stream} data
     * @param {object=} [options]
     * @param {string|null} [options.encoding = 'utf8']
     * @param {number} [options.mode = 0o666]
     * @param {string} [options.flag = 'w']
     * @param {AbortSignal=} [options.signal]
     * @return {Promise<void>}
     */
    export function writeFile(path: string | Buffer | URL | FileHandle, data: string | Buffer | any[] | DataView | TypedArray | Stream, options?: object | undefined): Promise<void>;
    export * as constants from "socket:fs/constants";
    export default exports;
    import { FileHandle } from "socket:fs/handle";
    import { Dir } from "socket:fs/dir";
    import * as exports from "socket:fs/promises";
    
}
declare module "socket:fs/binding" {
    const _default: ProxyConstructor;
    export default _default;
}
declare module "socket:fs/index" {
    /**
     * Asynchronously check access a file for a given mode calling `callback`
     * upon success or error.
     * @see {@link https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fsopenpath-flags-mode-callback}
     * @param {string | Buffer | URL} path
     * @param {string=} [mode = F_OK(0)]
     * @param {function(err, fd)} callback
     */
    export function access(path: string | Buffer | URL, mode?: string | undefined, callback: (arg0: err, arg1: fd) => any): void;
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
     * @param {function(err)} callback
     */
    export function chmod(path: string | Buffer | URL, mode: number, callback: (arg0: err) => any): void;
    /**
     * @ignore
     */
    export function chown(path: any, uid: any, gid: any, callback: any): void;
    /**
     * Asynchronously close a file descriptor calling `callback` upon success or error.
     * @see {https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fsclosefd-callback}
     * @param {number} fd
     * @param {function(err)=} callback
     */
    export function close(fd: number, callback?: ((arg0: err) => any) | undefined): void;
    export function copyFile(src: any, dst: any, mode: any, callback: any): void;
    /**
     * @see {@link https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fscreatewritestreampath-options}
     * @param {string | Buffer | URL} path
     * @param {object=} options
     * @returns {fs.ReadStream}
     */
    export function createReadStream(path: string | Buffer | URL, options?: object | undefined): fs.ReadStream;
    /**
     * @see {@link https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fscreatewritestreampath-options}
     * @param {string | Buffer | URL} path
     * @param {object=} options
     * @returns {fs.WriteStream}
     */
    export function createWriteStream(path: string | Buffer | URL, options?: object | undefined): fs.WriteStream;
    /**
     * Invokes the callback with the <fs.Stats> for the file descriptor. See
     * the POSIX fstat(2) documentation for more detail.
     *
     * @see {@link https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fsfstatfd-options-callback}
     *
     * @param {number} fd - A file descriptor.
     * @param {Object=} options - An options object.
     * @param {function} callback - The function to call after completion.
     */
    export function fstat(fd: number, options?: any | undefined, callback: Function): void;
    /**
     * @ignore
     */
    export function lchmod(path: any, mode: any, callback: any): void;
    /**
     * @ignore
     */
    export function lchown(path: any, uid: any, gid: any, callback: any): void;
    /**
     * @ignore
     */
    export function lutimes(path: any, atime: any, mtime: any, callback: any): void;
    /**
     * @ignore
     */
    export function link(existingPath: any, newPath: any, callback: any): void;
    /**
     * @ignore
     */
    export function lstat(path: any, options: any, callback: any): void;
    /**
     * @ignore
     */
    export function mkdir(path: any, options: any, callback: any): void;
    /**
     * Asynchronously open a file calling `callback` upon success or error.
     * @see {https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fsopenpath-flags-mode-callback}
     * @param {string | Buffer | URL} path
     * @param {string=} [flags = 'r']
     * @param {string=} [mode = 0o666]
     * @param {function(err, fd)} callback
     */
    export function open(path: string | Buffer | URL, flags?: string | undefined, mode?: string | undefined, options: any, callback: (arg0: err, arg1: fd) => any): void;
    /**
     * Asynchronously open a directory calling `callback` upon success or error.
     * @see {https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fsreaddirpath-options-callback}
     * @param {string | Buffer | URL} path
     * @param {Object=} options
     * @param {string=} [options.encoding = 'utf8']
     * @param {boolean=} [options.withFileTypes = false]
     * @param {function(err, fd)} callback
     */
    export function opendir(path: string | Buffer | URL, options?: any | undefined, callback: (arg0: err, arg1: fd) => any): void;
    /**
     * Asynchronously read from an open file descriptor.
     * @see {https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fsreadfd-buffer-offset-length-position-callback}
     * @param {number} fd
     * @param {object | Buffer | TypedArray} buffer - The buffer that the data will be written to.
     * @param {number} offset - The position in buffer to write the data to.
     * @param {number} length - The number of bytes to read.
     * @param {number | BigInt | null} position - Specifies where to begin reading from in the file. If position is null or -1 , data will be read from the current file position, and the file position will be updated. If position is an integer, the file position will be unchanged.
     * @param {function(err, bytesRead, buffer)} callback
     */
    export function read(fd: number, buffer: object | Buffer | TypedArray, offset: number, length: number, position: number | BigInt | null, options: any, callback: (arg0: err, arg1: bytesRead, arg2: any) => any): void;
    /**
     * Asynchronously read all entries in a directory.
     * @see {https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fsreaddirpath-options-callback}
     * @param {string | Buffer | URL } path
     * @param {object=} [options]
     * @param {string=} [options.encoding = 'utf8']
     * @param {boolean=} [options.withFileTypes = false]
     * @param {function(err, buffer)} callback
     */
    export function readdir(path: string | Buffer | URL, options?: object | undefined, callback: (arg0: err, arg1: buffer) => any): void;
    /**
     * @param {string | Buffer | URL | number } path
     * @param {object=} [options]
     * @param {string=} [options.encoding = 'utf8']
     * @param {string=} [options.flag = 'r']
     * @param {AbortSignal=} [options.signal]
     * @param {function(err, buffer)} callback
     */
    export function readFile(path: string | Buffer | URL | number, options?: object | undefined, callback: (arg0: err, arg1: buffer) => any): void;
    /**
     * @ignore
     */
    export function readlink(path: any, options: any, callback: any): void;
    /**
     * @ignore
     */
    export function realpath(path: any, options: any, callback: any): void;
    /**
     * @ignore
     */
    export function rename(oldPath: any, newPath: any, callback: any): void;
    /**
     * @ignore
     */
    export function rmdir(path: any, options: any, callback: any): void;
    /**
     * @ignore
     */
    export function rm(path: any, options: any, callback: any): void;
    /**
     *
     * @param {string | Buffer | URL | number } path - filename or file descriptor
     * @param {object=} options
     * @param {string=} [options.encoding = 'utf8']
     * @param {string=} [options.flag = 'r']
     * @param {AbortSignal=} [options.signal]
     * @param {function(err, data)} callback
     */
    export function stat(path: string | Buffer | URL | number, options?: object | undefined, callback: (arg0: err, arg1: data) => any): void;
    /**
     * @ignore
     */
    export function symlink(target: any, path: any, type: any, callback: any): void;
    /**
     * @ignore
     */
    export function truncate(path: any, length: any, callback: any): void;
    /**
     * @ignore
     */
    export function unlink(path: any, callback: any): void;
    /**
     * @ignore
     */
    export function utimes(path: any, atime: any, mtime: any, callback: any): void;
    /**
     * @ignore
     */
    export function watch(path: any, options: any, callback: any): void;
    /**
     * @ignore
     */
    export function write(fd: any, buffer: any, offset: any, length: any, position: any, callback: any): void;
    /**
     * @see {@url https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fswritefilefile-data-options-callback}
     * @param {string | Buffer | URL | number } path - filename or file descriptor
     * @param {string | Buffer | TypedArray | DataView | object } data
     * @param {object=} options
     * @param {string=} [options.encoding = 'utf8']
     * @param {string=} [options.mode = 0o666]
     * @param {string=} [options.flag = 'w']
     * @param {AbortSignal=} [options.signal]
     * @param {function(err)} callback
     */
    export function writeFile(path: string | Buffer | URL | number, data: string | Buffer | TypedArray | DataView | object, options?: object | undefined, callback: (arg0: err) => any): void;
    export function writev(fd: any, buffers: any, position: any, callback: any): void;
    export { default as binding } from "socket:./binding.js";
    export default exports;
    import * as constants from "socket:fs/constants";
    import { Dir } from "socket:fs/dir";
    import { DirectoryHandle } from "socket:fs/handle";
    import { Dirent } from "socket:fs/dir";
    import fds from "socket:fs/fds";
    import { FileHandle } from "socket:fs/handle";
    import * as promises from "socket:fs/promises";
    import { ReadStream } from "socket:fs/stream";
    import { Stats } from "socket:fs/stats";
    import { WriteStream } from "socket:fs/stream";
    import * as exports from "socket:fs/index";
    
    export { constants, Dir, DirectoryHandle, Dirent, fds, FileHandle, promises, ReadStream, Stats, WriteStream };
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
        id: bigint;
        onconnection(data: any): void;
        listen(port: any, address: any, cb: any): exports.Server;
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
        unref(): exports.Server;
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
        _final(cb: any): exports.Socket;
        destroySoon(): void;
        __write(data: any): void;
        _read(cb: any): any;
        pause(): exports.Socket;
        resume(): exports.Socket;
        connect(...args: any[]): exports.Socket;
        id: bigint;
        remotePort: any;
        remoteAddress: any;
        unref(): exports.Socket;
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
     * @param {number|string} [opts.family=0] - The record family. Must be 4, 6, or 0. For backward compatibility reasons,'IPv4' and 'IPv6' are interpreted as 4 and 6 respectively. The value 0 indicates that IPv4 and IPv6 addresses are both returned. Default: 0.
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
     * @param {Object=} opts - An options object.
     * @param {number|string} [opts.family=0] - The record family. Must be 4, 6, or 0. For backward compatibility reasons,'IPv4' and 'IPv6' are interpreted as 4 and 6 respectively. The value 0 indicates that IPv4 and IPv6 addresses are both returned. Default: 0.
     * @param {function} cb - The function to call after the method is complete.
     * @returns {void}
     */
    export function lookup(hostname: string, opts?: any | undefined, cb: Function): void;
    export { promises };
    export default exports;
    import * as promises from "socket:dns/promises";
    import * as exports from "socket:dns/index";
    
}
declare module "socket:dns" {
    export * from "socket:dns/index";
    export default exports;
    import * as exports from "socket:dns";
    
}
declare module "socket:dgram" {
    export function createSocket(options: string | any, callback?: Function | undefined): Socket;
    /**
     * New instances of dgram.Socket are created using dgram.createSocket().
     * The new keyword is not to be used to create dgram.Socket instances.
     */
    export class Socket extends EventEmitter {
        constructor(options: any, callback: any);
        id: bigint;
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
        bind(arg1: any, arg2: any, arg3: any): exports.Socket;
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
        close(cb: any): exports.Socket;
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
        ref(): exports.Socket;
        unref(): exports.Socket;
    }
    /**
     * Generic error class for an error occurring on a `Socket` instance.
     * @ignore
     */
    export class SocketError extends InternalError {
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
    import * as exports from "socket:dgram";
    import { InternalError } from "socket:errors";
    
}
declare module "socket:hooks" {
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
     * An interface for registering callbacks for various hooks in
     * the runtime.
     */
    export class Hooks extends EventTarget {
        /**
         * Reference to global object
         * @type {Global}
         */
        get global(): Global;
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
        #private;
    }
    const _default: Hooks;
    export default _default;
}
declare module "socket:test/fast-deep-equal" {
    export default function equal(a: any, b: any): boolean;
}
declare module "socket:test/index" {
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
     * @type {{
     *    (name: string, fn?: TestFn): void
     *    only(name: string, fn?: TestFn): void
     *    skip(name: string, fn?: TestFn): void
     * }}
     *
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
        /** @type {string} */
        name: string;
        /** @type {TestFn} */
        fn: TestFn;
        /** @type {TestRunner} */
        runner: TestRunner;
        /** @type {{ pass: number, fail: number }} */
        _result: {
            pass: number;
            fail: number;
        };
        /** @type {boolean} */
        done: boolean;
        /** @type {boolean} */
        strict: boolean;
        /**
         * @param {string} msg
         * @returns {void}
         */
        comment(msg: string): void;
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
         * @param {boolean} pass
         * @param {unknown} actual
         * @param {unknown} expected
         * @param {string} description
         * @param {string} operator
         * @returns {void}
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
        /** @type {(lines: string) => void} */
        report: (lines: string) => void;
        /** @type {Test[]} */
        tests: Test[];
        /** @type {Test[]} */
        onlyTests: Test[];
        /** @type {boolean} */
        scheduled: boolean;
        /** @type {number} */
        _id: number;
        /** @type {boolean} */
        completed: boolean;
        /** @type {boolean} */
        rethrowExceptions: boolean;
        /** @type {boolean} */
        strict: boolean;
        /** @type {function | void} */
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
    export const GLOBAL_TEST_RUNNER: TestRunner;
    export default test;
    export type TestFn = (t: Test) => (void | Promise<void>);
}
declare module "socket:test" {
    export * from "socket:test/index";
    export default test;
    import test from "socket:test/index";
}
declare module "socket:module" {
    /**
     * Creates a `require` function from a source URL.
     * @param {URL|string} sourceURL
     * @return {function}
     */
    export function createRequire(sourceURL: URL | string): Function;
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
    };
    /**
     * CommonJS module scope source wrapper.
     * @type {string}
     */
    export const COMMONJS_WRAPPER: string;
    /**
     * The main entry source URL.
     * @type {string}
     */
    export const MAIN_SOURCE_URL: string;
    export namespace scope {
        const current: any;
        const previous: any;
    }
    /**
     * A container for a loaded CommonJS module. All errors bubble
     * to the "main" module and global object (if possible).
     */
    export class Module extends EventTarget {
        static set current(arg: exports.Module);
        /**
         * A reference to the currently scoped module.
         * @type {Module?}
         */
        static get current(): exports.Module;
        static set previous(arg: exports.Module);
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
         * @param {URL|string} sourceURL
         * @return {function}
         */
        static createRequire(sourceURL: URL | string): Function;
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
         * @param {string|URL|Module} [sourceURL]
         * @param {string|URL|Module} [parent]
         */
        static from(sourceURL?: string | URL | Module, parent?: string | URL | Module): any;
        /**
         * `Module` class constructor.
         * @ignore
         */
        constructor(id: any, parent?: any, sourceURL?: any);
        /**
         * The module id, most likely a file name.
         * @type {string}
         */
        id: string;
        /**
         * The path to the module.
         * @type {string}
         */
        path: string;
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
        sourceURL: string;
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
         * The `URL` for this module.
         * @type {URL}
         */
        get url(): URL;
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
declare module "socket:node-esm-loader" {
    export function resolve(specifier: any, ctx: any, next: any): Promise<any>;
    export default resolve;
}
declare module "socket:stream-relay/encryption" {
    export class Encryption {
        constructor(config?: {});
        keys: {};
        publicKey: any;
        privateKey: any;
        add(publicKey: any, privateKey: any): void;
        get(to: any): any;
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
        open(message: any, to: any): any;
        /**
         * `Seal(message, receiver)` performs an _encrypt-sign-encrypt_ (ESE) on
         * a plaintext `message` for a `receiver` identity. This prevents repudiation
         * attacks and doesn't rely on packet chain guarantees.
         *
         * let ct = Seal(sender | pt, receiver)
         * let sig = Sign(ct, sk)
         * let out = Seal(sig | ct)
         *
         * Essentially, in an setup between Alice & Bob, this means:
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
        seal(message: any, to: any): any;
    }
}
declare module "socket:stream-relay/packets" {
    export function trim(buf: Buffer): any;
    /**
     * The magic bytes prefixing every packet. They are the
     * 2nd, 3rd, 5th, and 7th, prime numbers.
     * @type {number[]}
     */
    export const MAGIC_BYTES_PREFIX: number[];
    /**
     * The version of the protocol.
     */
    export const VERSION: 2;
    /**
     * The size in bytes of the prefix magic bytes.
     */
    export const MAGIC_BYTES: 4;
    /**
     * The size in bytes of the `type` field.
     */
    export const TYPE_BYTES: 1;
    /**
     * The size in bytes of the `version` field.
     */
    export const VERSION_BYTES: 1;
    /**
     * The size in bytes of the `gops` field.
     */
    export const HOPS_BYTES: 4;
    /**
     * The size in bytes of the `clock` field.
     */
    export const CLOCK_BYTES: 4;
    /**
     * The size in bytes of the `index` field.
     */
    export const INDEX_BYTES: 4;
    /**
     * The size in bytes of the `message_id` field.
     */
    export const MESSAGE_ID_BYTES: 32;
    /**
     * The size in bytes of the `cluster_id` field.
     */
    export const CLUSTER_ID_BYTES: 32;
    /**
     * The size in bytes of the `previous_id` field.
     */
    export const PREVIOUS_ID_BYTES: 32;
    /**
     * The size in bytes of the `next_id` field.
     */
    export const NEXT_ID_BYTES: 32;
    /**
     * The size in bytes of the `to` field.
     */
    export const TO_BYTES: 32;
    /**
     * The size in bytes of the `usr1` field.
     */
    export const USR1_BYTES: 32;
    /**
     * The size in bytes of the `message_length` field.
     */
    export const MESSAGE_LENGTH_BYTES: 2;
    /**
     * The size in bytes of the `message` field.
     */
    export const MESSAGE_BYTES: 1024;
    /**
     * The size in bytes of the total packet frame.
     */
    export const FRAME_BYTES: number;
    /**
     * The size in bytes of the total packet frame and message.
     */
    export const PACKET_BYTES: number;
    /**
     * The cache TTL in milliseconds.
     */
    export const CACHE_TTL: number;
    export function validatePacket(message: object, constraints: {
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
    export function addHops(buf: any, offset?: number): any;
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
        type: number;
        version: number;
        clock: number;
        index: number;
        clusterId: string;
        previousId: string;
        packetId: string;
        nextId: string;
        to: string;
        usr1: string;
        message: string;
        #private;
    }
    export class PacketPing extends Packet {
        static type: number;
        constructor({ message }: {
            message: any;
        });
    }
    export class PacketPong extends Packet {
        static type: number;
        constructor({ message }: {
            message: any;
        });
    }
    export class PacketIntro extends Packet {
        static type: number;
        constructor({ clock, message }: {
            clock: any;
            message: any;
        });
    }
    export class PacketJoin extends Packet {
        static type: number;
        constructor({ clock, message }: {
            clock: any;
            message: any;
        });
    }
    export class PacketPublish extends Packet {
        static type: number;
        constructor({ message, packetId, clusterId, nextId, clock, to, usr1, previousId }: {
            message: any;
            packetId: any;
            clusterId: any;
            nextId: any;
            clock: any;
            to: any;
            usr1: any;
            previousId: any;
        });
    }
    export class PacketSync extends Packet {
        static type: number;
        constructor({ packetId, clusterId, message }: {
            packetId: any;
            clusterId: any;
            message?: string;
        });
    }
    export class PacketQuery extends Packet {
        static type: number;
        constructor({ packetId, clusterId, message }: {
            packetId: any;
            clusterId: any;
            message?: {};
        });
    }
    export default Packet;
    import { Buffer } from "socket:buffer";
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
     * A class for storing a cache of packets by ID.
     */
    export class Cache {
        static HASH_SIZE_BYTES: number;
        /**
         * encodeSummary provide a compact binary encoding of the output of summary()
         *
         * @param {Object} summary - the output of calling summary()
         * @return {Buffer}
        **/
        static encodeSummary(summary: any): Buffer;
        /**
         * decodeSummary decodes the output of encodeSummary()
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
        toJSON(): [any, any][];
        /**
         * Inserts a `CacheEntry` value `v` into the cache at key `k`.
         * @param {string} k
         * @param {CacheEntry} v
         * @return {Promise<boolean>}
         */
        insert(k: string, v: CacheEntry): Promise<boolean>;
        /**
         * Gets a `CacheEntry` value at key `k`.
         * @param {string} k
         * @return {Promise<CacheEntry?>}
         */
        get(k: string): Promise<CacheEntry | null>;
        /**
         * @param {string} k
         * @return {Promise<boolean>}
         */
        delete(k: string): Promise<boolean>;
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
        compose(packet: Packet): Promise<Packet>;
        sha1(value: any, toHex: any): Promise<any>;
        /**
         * summarize returns a terse yet comparable summary of the cache contents.
         *
         * thinking of the cache as a trie of hex characters, the summary returns
         * a checksum for the current level of the trie and for its 16 children.
         *
         * this is similar to a merkel tree as equal subtrees can easily be detected
         * without the need for further recursion. When the subtree checksums are
         * inequivalent then further negotiation at lower levels may be required, this
         * process continues until the two trees become synchonized.
         *
         * when the prefix is empty, the summary will return an array of 16 checksums
         * these checksums provide a way of comparing that subtree with other peers.
         *
         * when a variable-length hexidecimal prefix is provided, then only cache
         * member hashes sharing this prefix will be considered.
         *
         * for each hex character provided in the prefix, the trie will decend by one
         * level, each level divides the 2^128 address space by 16.
         *
         * example:
         *
         * level  0   1   2
         * --------------------------
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
         *
         * @param {string} prefix - a string of lowercased hexidecimal characters
         * @return {Object}
         */
        summarize(prefix?: string): any;
    }
    export default Cache;
    export type CacheEntry = Packet;
    export type CacheEntrySiblingResolver = (arg0: CacheEntry, arg1: CacheEntry) => number;
    import { Buffer } from "socket:buffer";
    import { Packet } from "socket:stream-relay/packets";
}
declare module "socket:stream-relay/scheme-ptp" {
    export function formatPeerId(forPubKey: any): any;
    export function generateNetworkIdentity(): {
        publicKey: any;
        privateKey: any;
        encPublicKey: any;
        encPrivateKey: any;
    };
    export function registerNetworkIdentity(keyInfo: any): {
        err: Error;
        data?: undefined;
    } | {
        data: boolean;
        err?: undefined;
    };
    export function lookupNetworkIdentities(forPubKey?: any): {
        privateKey: any;
        encPrivateKey: any;
        label: any;
        publicKey: any;
        encPublicKey: any;
    }[];
    export function readNetworkMessage(message: any, forPubKey?: any): {
        err: Error;
        data?: undefined;
    } | {
        data: any;
        err?: undefined;
    };
    export function buildIdentityGroupMessage(groupPubKey: any, label?: string): {
        err: Error;
        data?: undefined;
    } | {
        data: any;
        err?: undefined;
    };
    export function verifyIdentityGroupMessage(groupMsg: any): {
        err: Error;
        data?: undefined;
    } | {
        data: any;
        err?: undefined;
    };
    export function registerRemoteIdentityGroup(groupMsg: any): {
        err: Error;
        data?: undefined;
    } | {
        data: boolean;
        err?: undefined;
    };
    export function _clearRegistry(confirmStr: any): {
        err: Error;
        data?: undefined;
    } | {
        data: boolean;
        err?: undefined;
    };
    export class PacketIdentityGroup extends Packet {
        static type: number;
    }
    export class PTP {
        static init(peer: any, config: any, peerType?: string): void;
    }
    namespace _default {
        export { PTP };
    }
    export default _default;
    import { Packet } from "socket:stream-relay/packets";
}
declare module "socket:stream-relay/index" {
    /**
     * Computes rate limit predicate value for a port and address pair for a given threshold
     * updating an input rates map.
     * @param {Map} rates
     * @param {number} type
     * @param {number} port
     * @param {string} address
     * @param {number?} [threshold = DEFAULT_RATE_LIMIT_THRESHOLD]
     * @return {boolean}
     */
    export function rateLimit(rates: Map<any, any>, type: number, port: number, address: string, threshold?: number | null): boolean;
    /**
     * Retry delay in milliseconds for ping.
     * @type {number}
     */
    export const PING_RETRY: number;
    /**
     * Default port to bind peer UDP socket port to.
     * @type {number}
     */
    export const DEFAULT_PORT: number;
    /**
     * Default static test port to bind peer UDP socket port to.
     * @type {number}
     */
    export const DEFAULT_TEST_PORT: number;
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
         *   natType?: string,
         *   clusterId?: string,
         *   pingId?: string,
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
            natType?: string;
            clusterId?: string;
            pingId?: string;
            distance?: number;
            publicKey?: string;
            privateKey?: string;
            clock?: number;
            lastUpdate?: number;
            lastRequest?: number;
        });
        peerId: any;
        address: any;
        port: number;
        natType: any;
        clusterId: any;
        pingId: any;
        distance: number;
        publicKey: any;
        privateKey: any;
        clock: number;
        syncing: boolean;
        lastUpdate: number;
        lastRequest: number;
    }
    export function createPeer(options: {
        createSocket: (arg0: 'udp4', arg1: null, arg2: object | null) => object;
    }): {
        new (persistedState?: object | null): {
            port: any;
            address: any;
            natType: any;
            clusterId: any;
            reflectionId: any;
            reflectionFirstResponder: any;
            peerId: string;
            pingStart: number;
            ctime: number;
            lastUpdate: number;
            closing: boolean;
            listening: boolean;
            unpublished: {};
            cache: any;
            queries: {};
            controlPackets: {};
            uptime: number;
            maxHops: number;
            bdpCache: number[];
            onListening: any;
            onDelete: any;
            firewall: any;
            peers: any;
            encryption: Encryption;
            config: any;
            onError: (err: Error) => void;
            socket: any;
            testSocket: any;
            init(): Promise<any>;
            connecting: number;
            send(data: any, ...args: any[]): Promise<boolean>;
            getState(): {
                config: any;
                data: any[];
                unpublished: {};
            };
            getPeers(packet: any, peers: any, n?: number, filter?: (o: any) => any): any;
            mcast(packet: any, packetId: any, clusterId: any, isTaxed: any, ignorelist?: any[]): Promise<void>;
            timer(delay: any, repeat: any, fn: any): Promise<boolean>;
            requestReflection(reflectionId: any): Promise<boolean>;
            ping(peer: any, props?: {}): Promise<PacketPing>;
            setPeer(peerId: any, packet: any, port: any, address: any): any;
            join(): Promise<void>;
            publish(args: any): Promise<void | PacketPublish[]>;
            sync(peer: any, packet: any, port: any, address: any): Promise<void>;
            onSync(packet: any, port: any, address: any): Promise<void>;
            onQuery(packet: any, port: any, address: any): Promise<void>;
            close(): void;
            onConnection(peer: any, packet: any, port: any, address: any): Promise<void>;
            onPing(packet: any, port: any, address: any): Promise<void>;
            onPong(packet: any, port: any, address: any): Promise<void>;
            onIntro(packet: any, port: any, address: any): Promise<void>;
            onJoin(packet: any, port: any, address: any, data: any): Promise<void>;
            onPublish(packet: any, port: any, address: any, data: any): Promise<void>;
            rateLimit(data: any, port: any, address: any): boolean;
            /**
             * When a packet is received it is decoded, the packet contains the type
             * of the message. Based on the message type it is routed to a function.
             * like WebSockets, don't answer queries unless we know its another SRP peer.
             *
             * @param {Buffer|Uint8Array} data
             * @param {{ port: number, address: string }} info
             */
            onMessage(data: Buffer | Uint8Array, { port, address }: {
                port: number;
                address: string;
            }): any;
        };
        /**
         * Factory for creating a `Peer` instances. This function ensures all
         * internal dependencies are loaded and ready.
         * @param {object?} [options]
         * @return {Promise<Peer>}
         */
        create(options?: object | null): Promise<{
            port: any;
            address: any;
            natType: any;
            clusterId: any;
            reflectionId: any;
            reflectionFirstResponder: any;
            peerId: string;
            pingStart: number;
            ctime: number;
            lastUpdate: number;
            closing: boolean;
            listening: boolean;
            unpublished: {};
            cache: any;
            queries: {};
            controlPackets: {};
            uptime: number;
            maxHops: number;
            bdpCache: number[];
            onListening: any;
            onDelete: any;
            firewall: any;
            peers: any;
            encryption: Encryption;
            config: any;
            onError: (err: Error) => void;
            socket: any;
            testSocket: any;
            init(): Promise<any>;
            connecting: number;
            send(data: any, ...args: any[]): Promise<boolean>;
            getState(): {
                config: any;
                data: any[];
                unpublished: {};
            };
            getPeers(packet: any, peers: any, n?: number, filter?: (o: any) => any): any;
            mcast(packet: any, packetId: any, clusterId: any, isTaxed: any, ignorelist?: any[]): Promise<void>;
            timer(delay: any, repeat: any, fn: any): Promise<boolean>;
            requestReflection(reflectionId: any): Promise<boolean>;
            ping(peer: any, props?: {}): Promise<PacketPing>;
            setPeer(peerId: any, packet: any, port: any, address: any): any;
            join(): Promise<void>;
            publish(args: any): Promise<void | PacketPublish[]>;
            sync(peer: any, packet: any, port: any, address: any): Promise<void>;
            onSync(packet: any, port: any, address: any): Promise<void>;
            onQuery(packet: any, port: any, address: any): Promise<void>;
            close(): void;
            onConnection(peer: any, packet: any, port: any, address: any): Promise<void>;
            onPing(packet: any, port: any, address: any): Promise<void>;
            onPong(packet: any, port: any, address: any): Promise<void>;
            onIntro(packet: any, port: any, address: any): Promise<void>;
            onJoin(packet: any, port: any, address: any, data: any): Promise<void>;
            onPublish(packet: any, port: any, address: any, data: any): Promise<void>;
            rateLimit(data: any, port: any, address: any): boolean;
            /**
             * When a packet is received it is decoded, the packet contains the type
             * of the message. Based on the message type it is routed to a function.
             * like WebSockets, don't answer queries unless we know its another SRP peer.
             *
             * @param {Buffer|Uint8Array} data
             * @param {{ port: number, address: string }} info
             */
            onMessage(data: Buffer | Uint8Array, { port, address }: {
                port: number;
                address: string;
            }): any;
        }>;
    };
    export default createPeer;
    import { sha256 } from "socket:stream-relay/packets";
    import { Cache } from "socket:stream-relay/cache";
    import { Encryption } from "socket:stream-relay/encryption";
    import { PacketPing } from "socket:stream-relay/packets";
    import { PacketPublish } from "socket:stream-relay/packets";
    import { Buffer } from "socket:buffer";
    export { sha256, Cache, Encryption };
}
declare module "socket:peer" {
    /**
     *
     * @class
     * @public
     *
     *The Peer class is an EventEmitter. It emits events when new network events
     *are received (.on), it can also emit new events to the network (.emit).
     *
     *```js
     *import { Peer } from 'socket:peer'
     *
     *const pair = await Peer.createKeys()
     *const clusterId = await Peer.createClusterId()
     *
     *const peer = new Peer({ ...pair, clusterId })
     *
     *peer.on('greeting', (value, peer, address, port) => {
     *  console.log(value)
     *})
     *
     *window.onload = () => {
     *  const value = { english: 'hello, world' }
     *  const packet = await peer.emit('greeting', value)
     *}
     *```
     *
     */
    export class Peer extends EventEmitter {
        /**
         * A method that will generate a public and private key pair.
         * The ed25519 pair can be stored by an app with a secure API.
         *
         * @static
         * @return {Object<Pair>} pair - A pair of keys
         *
         */
        static createKeys(): any;
        /**
         * Create a clusterId from random bytes
         * @return {string} id - a hex encoded sha256 hash
         */
        static createClusterId(): string;
        /**
         * `Peer` class constructor.
         * @param {Object} options - All options for the Peer constructor
         * @param {string} options.publicKey - The public key required to sign and read
         * @param {string} options.privateKey - The private key required to sign and read
         * @param {string} options.clusterId - A unique appliction identity
         * @param {string} options.scheme - Specify which encryption scheme to use (ie, 'PTP')
         * @param {Array} options.peers - An array of RemotePeer
         */
        constructor(opts?: {});
        peer: {
            port: any;
            address: any;
            natType: any;
            clusterId: any;
            reflectionId: any;
            reflectionFirstResponder: any;
            peerId: string;
            pingStart: number;
            ctime: number;
            lastUpdate: number;
            closing: boolean;
            listening: boolean;
            unpublished: {};
            cache: any;
            queries: {};
            controlPackets: {};
            uptime: number;
            maxHops: number;
            bdpCache: number[];
            onListening: any;
            onDelete: any;
            firewall: any;
            peers: any;
            encryption: Encryption;
            config: any;
            onError: (err: Error) => void;
            socket: any;
            testSocket: any;
            init(): Promise<any>;
            connecting: number;
            send(data: any, ...args: any[]): Promise<boolean>;
            getState(): {
                config: any;
                data: any[];
                unpublished: {};
            };
            getPeers(packet: any, peers: any, n?: number, filter?: (o: any) => any): any;
            mcast(packet: any, packetId: any, clusterId: any, isTaxed: any, ignorelist?: any[]): Promise<void>;
            timer(delay: any, repeat: any, fn: any): Promise<boolean>;
            requestReflection(reflectionId: any): Promise<boolean>;
            ping(peer: any, props?: {}): Promise<import("socket:stream-relay/packets").PacketPing>;
            setPeer(peerId: any, packet: any, port: any, address: any): any;
            join(): Promise<void>;
            publish(args: any): Promise<void | import("socket:stream-relay/packets").PacketPublish[]>;
            sync(peer: any, packet: any, port: any, address: any): Promise<void>;
            onSync(packet: any, port: any, address: any): Promise<void>;
            onQuery(packet: any, port: any, address: any): Promise<void>;
            close(): void;
            onConnection(peer: any, packet: any, port: any, address: any): Promise<void>;
            onPing(packet: any, port: any, address: any): Promise<void>;
            onPong(packet: any, port: any, address: any): Promise<void>;
            onIntro(packet: any, port: any, address: any): Promise<void>;
            onJoin(packet: any, port: any, address: any, data: any): Promise<void>;
            onPublish(packet: any, port: any, address: any, data: any): Promise<void>;
            rateLimit(data: any, port: any, address: any): boolean;
            onMessage(data: Uint8Array | import("socket:buffer").default, { port, address }: {
                port: number;
                address: string;
            }): any;
        };
        _emit: (type: any, ...args: any[]) => boolean;
        opts: {};
        /**
         * Emits a message to the network
         *
         * @param {string} event - The name of the event to emit to the network
         * @param {Buffer} message - The data to emit to the network
         * @return {Object<Packet>} The packet that will be sent when possible
         */
        emit(eventName: any, message: Buffer, ...args: any[]): any;
        query(): Promise<void>;
        /**
         * Starts the process of connecting to the network.
         *
         * @return {Peer} Returns an instance of the underlying network peer
         */
        join(): Peer;
        getPeerId(): string;
        close(): void;
        #private;
    }
    export default Peer;
    import { EventEmitter } from "socket:events";
    import { Encryption } from "socket:stream-relay/index";
}
declare module "socket:stream-relay" {
    export * from "socket:stream-relay/index";
    export default def;
    import def from "socket:stream-relay/index";
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
declare module "socket:internal/init" {
    const _default: void;
    export default _default;
}
declare module "socket:internal/worker" {
    export function onWorkerMessage(event: any): any;
    const undefined: any;
    export default undefined;
}
declare module "socket:test/context" {
    const _default: any;
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
