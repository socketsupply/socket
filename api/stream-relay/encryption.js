import { sodium, randomBytes } from '../crypto.js'
import Buffer from '../buffer.js'
import { sha256 } from './packets.js'
import { isArrayLike, isBufferLike } from '../util.js'

export class Encryption {
  keys = {}

  static async createSharedKey (seed) {
    await sodium.ready

    if (seed) return sodium.crypto_generichash(32, seed)
    return sodium.randombytes_buf(32)
  }

  static async createKeyPair (seed) {
    await sodium.ready
    seed = seed || sodium.randombytes_buf(32)

    if (typeof seed === 'string') {
      seed = sodium.crypto_generichash(32, seed)
    }

    return sodium.crypto_sign_seed_keypair(seed)
  }

  static async createId (str = randomBytes(32)) {
    return await sha256(str)
  }

  static async createClusterId (value) {
    await sodium.ready
    value = value || sodium.randombytes_buf(32)
    return Buffer.from(value)
  }

  static async createSubclusterId (value) {
    return Encryption.createClusterId(value)
  }

  add (publicKey, privateKey) {
    const to = Buffer.from(publicKey).toString('base64')
    this.keys[to] = { publicKey, privateKey, ts: Date.now() }
  }

  remove (publicKey) {
    delete this.keys[Buffer.from(publicKey).toString('base64')]
  }

  has (to) {
    if (to.constructor.name === 'Uint8Array') {
      to = Buffer.from(to).toString('base64')
    }

    return !!this.keys[to]
  }

  /**
   * @param {Buffer} b - The message to sign
   * @param {Uint8Array} sk - The secret key to use
   */
  static sign (b, sk) {
    const ct = b.subarray(sodium.crypto_sign_BYTES)
    const sig = sodium.crypto_sign_detached(ct, sk)
    return sig
  }

  /**
   * @param {Buffer} b - The message to verify
   * @param {Uint8Array} pk - The public key to use
   * @return {number} - Returns non zero if the buffer could not be verified
   */
  static verify (b, sig, pk) {
    if (sig.length !== sodium.crypto_sign_BYTES) return false
    const ct = b.subarray(sodium.crypto_sign_BYTES)
    return sodium.crypto_sign_verify_detached(sig, ct, pk)
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
  open (message, v) {
    if (typeof v === 'string') v = this.keys[v]
    if (!v) throw new Error(`ENOKEY (key=${v})`)

    const pk = toPK(v.publicKey)
    const sk = toSK(v.privateKey)
    const buf = sodium.crypto_box_seal_open(message, pk, sk)

    if (buf.byteLength <= sodium.crypto_sign_BYTES) {
      throw new Error('EMALFORMED')
    }

    const sig = buf.subarray(0, sodium.crypto_sign_BYTES)
    const ct = buf.subarray(sodium.crypto_sign_BYTES)

    if (!sodium.crypto_sign_verify_detached(sig, ct, v.publicKey)) {
      throw new Error('ENOTVERIFIED')
    }

    return Buffer.from(sodium.crypto_box_seal_open(ct, pk, sk))
  }

  openMessage (message, v) {
    if (typeof v === 'string') v = this.keys[v]
    if (!v) throw new Error(`ENOKEY (key=${v})`)

    const pk = toPK(v.publicKey)
    const sk = toSK(v.privateKey)
    return Buffer.from(sodium.crypto_box_seal_open(message, pk, sk))
  }

  sealMessage (message, publicKey) {
    return sodium.crypto_box_seal(toUint8Array(message), toPK(publicKey))
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
  seal (message, v) {
    if (typeof v === 'string') v = this.keys[v]
    if (!v) throw new Error(`ENOKEY (key=${v})`)

    this.add(v.publicKey, v.privateKey)

    const pk = toPK(v.publicKey)
    const pt = toUint8Array(message)
    const ct = sodium.crypto_box_seal(pt, pk)
    const sig = sodium.crypto_sign_detached(ct, v.privateKey)
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
