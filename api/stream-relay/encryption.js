import { sodium } from '../crypto.js'
import Buffer from '../buffer.js'
import { isArrayLike, isBufferLike } from '../util.js'

export class Encryption {
  keys = {}
  publicKey = null
  privateKey = null

  constructor (config = {}) {
    if (!config.publicKey && !config.privateKey) {
      Object.assign(config, sodium.crypto_sign_keypair())
    }

    Object.assign(this, config)
  }

  add (publicKey, privateKey) {
    const to = Buffer.from(publicKey).toString('base64')
    this.keys[to] = { publicKey, privateKey, ts: Date.now() }
  }

  get (to) {
    const publicKey = Buffer.from(to, 'base64')
    const id = publicKey.toString('base64')

    if (publicKey.compare(this.publicKey) === 0) {
      return { publicKey: this.publicKey, privateKey: this.privateKey }
    }

    if (this.keys[id]) {
      return this.keys[id]
    }

    return null
  }

  has (to) {
    return this.get(to) !== null
  }

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
  open (message, to) {
    const pair = this.get(to)

    if (!pair) {
      throw new Error('ENOKEYS')
    }

    const pk = toPK(pair.publicKey)
    const sk = toSK(pair.privateKey)
    const buf = sodium.crypto_box_seal_open(toUint8Array(message), pk, sk)

    if (buf.byteLength <= sodium.crypto_sign_BYTES) {
      throw new Error('EMALFORMED')
    }

    const sig = buf.subarray(0, sodium.crypto_sign_BYTES)
    const ct = buf.subarray(sodium.crypto_sign_BYTES)

    if (!sodium.crypto_sign_verify_detached(sig, ct, pair.publicKey)) {
      throw new Error('ENOTVERIFIED')
    }

    return Buffer.from(sodium.crypto_box_seal_open(ct, pk, sk))
  }

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
  seal (message, to) {
    const pair = this.get(to)

    if (!pair) {
      throw new Error('ENOKEYS')
    }

    const pk = toPK(pair.publicKey)
    const pt = toUint8Array(message)
    const ct = sodium.crypto_box_seal(pt, pk)
    const sig = sodium.crypto_sign_detached(ct, pair.privateKey)
    return sodium.crypto_box_seal(toUint8Array(Buffer.concat([sig, ct])), pk)
  }
}

function toPK (pk) {
  try {
    return sodium.crypto_sign_ed25519_pk_to_curve25519(pk)
  } catch {}

  return new Uint8Array(0)
}

function toSK (sk) {
  try {
    return sodium.crypto_sign_ed25519_sk_to_curve25519(sk)
  } catch {}

  return new Uint8Array(0)
}

const textEncoder = new TextEncoder()

function toUint8Array (buffer) {
  if (buffer instanceof Uint8Array) {
    return buffer
  } else if (typeof buffer === 'string') {
    return textEncoder.encode(buffer)
  } else if (buffer?.buffer) {
    return new Uint8Array(buffer.buffer)
  } else if (isArrayLike(buffer) || isBufferLike(buffer)) {
    return new Uint8Array(buffer)
  } else if (buffer instanceof ArrayBuffer) {
    return new Uint8Array(buffer)
  }

  return buffer && Symbol.iterator in buffer
    ? new Uint8Array(buffer)
    : new Uint8Array(0)
}
