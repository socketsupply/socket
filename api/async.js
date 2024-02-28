export class Deferred {
  constructor () {
    this._promise = new Promise((resolve, reject) => {
      this.resolve = resolve
      this.reject = reject
    })
    this.then = this._promise.then.bind(this._promise)
    this.catch = this._promise.catch.bind(this._promise)
    this.finally = this._promise.finally.bind(this._promise)
    this[Symbol.toStringTag] = 'Promise'
  }
}

export default {
  Deferred
}
