import { sodium } from '../crypto.js'
import Buffer from '../buffer.js'
import { sha256 } from './packets.js'
import { isArrayLike, isBufferLike } from '../util.js'

/**
 * Class for handling encryption and key management.
 */
export class Encryption {
  /**
   * Mapping of public keys to key objects.
   * @type {Object.<string, { publicKey: Uint8Array, privateKey: Uint8Array, ts: number }>}
   */
  keys = {}

  /**
   * Creates a shared key based on the provided seed or generates a random one.
   * @param {Uint8Array|string} seed - Seed for key generation.
   * @returns {Promise<Uint8Array>} - Shared key.
   */
  static async createSharedKey (seed) {
    await sodium.ready

    if (seed) return sodium.crypto_generichash(32, seed)
    return sodium.randombytes_buf(32)
  }

  /**
   * Creates a key pair for signing and verification.
   * @param {Uint8Array|string} seed - Seed for key generation.
   * @returns {Promise<{ publicKey: Uint8Array, privateKey: Uint8Array }>} - Key pair.
   */
  static async createKeyPair (seed) {
    await sodium.ready
    seed = seed || sodium.randombytes_buf(32)

    if (typeof seed === 'string') {
      seed = sodium.crypto_generichash(32, seed)
    }

    return sodium.crypto_sign_seed_keypair(seed)
  }

  /**
   * Creates an ID using SHA-256 hash.
   * @param {string} str - String to hash.
   * @returns {Promise<Uint8Array>} - SHA-256 hash.
   */
  static async createId (str) {
    await sodium.ready
    return await sha256(str || sodium.randombytes_buf(32))
  }

  /**
   * Creates a cluster ID using SHA-256 hash with specified output size.
   * @param {string} str - String to hash.
   * @returns {Promise<Uint8Array>} - SHA-256 hash with specified output size.
   */
  static async createClusterId (str) {
    await sodium.ready
    return await sha256(str || sodium.randombytes_buf(32), { bytes: true })
  }

  /**
   * Adds a key pair to the keys mapping.
   * @param {Uint8Array|string} publicKey - Public key.
   * @param {Uint8Array} privateKey - Private key.
   */
  add (publicKey, privateKey) {
    if (!publicKey) throw new Error('encryption.add expects publicKey')
    if (!privateKey) throw new Error('encryption.add expects privateKey')

    const to = Buffer.from(publicKey).toString('base64')
    this.keys[to] = { publicKey, privateKey, ts: Date.now() }
  }

  /**
   * Removes a key from the keys mapping.
   * @param {Uint8Array|string} publicKey - Public key.
   */
  remove (publicKey) {
    delete this.keys[Buffer.from(publicKey).toString('base64')]
  }

  /**
   * Checks if a key is in the keys mapping.
   * @param {Uint8Array|string} to - Public key or Uint8Array.
   * @returns {boolean} - True if the key is present, false otherwise.
   */
  has (to) {
    if (to instanceof Uint8Array) {
      to = Buffer.from(to).toString('base64')
    }

    return !!this.keys[to]
  }

  /**
   * Signs a message using the given secret key.
   * @param {Buffer} b - The message to sign.
   * @param {Uint8Array} sk - The secret key to use.
   * @returns {Uint8Array} - Signature.
   */
  static sign (b, sk) {
    const ct = b.subarray(sodium.crypto_sign_BYTES)
    const sig = sodium.crypto_sign_detached(ct, sk)
    return sig
  }

  /**
   * Verifies the signature of a message using the given public key.
   * @param {Buffer} b - The message to verify.
   * @param {Uint8Array} sig - The signature to check.
   * @param {Uint8Array} pk - The public key to use.
   * @returns {number} - Returns non-zero if the buffer could not be verified.
   */
  static verify (b, sig, pk) {
    if (sig.length !== sodium.crypto_sign_BYTES) return false
    const ct = b.subarray(sodium.crypto_sign_BYTES)
    return sodium.crypto_sign_verify_detached(sig, ct, pk)
  }

  /**
   * Opens a sealed message using the specified key.
   * @param {Buffer} message - The sealed message.
   * @param {Object|string} v - Key object or public key.
   * @returns {Buffer} - Decrypted message.
   * @throws {Error} - Throws ENOKEY if the key is not found.
   */
  openUnsigned (message, v) {
    if (typeof v === 'string') v = this.keys[v]
    if (!v) throw new Error(`ENOKEY (key=${v})`)

    const pk = toPK(v.publicKey)
    const sk = toSK(v.privateKey)
    return Buffer.from(sodium.crypto_box_seal_open(message, pk, sk))
  }

  sealUnsigned (message, v) {
    if (typeof v === 'string') v = this.keys[v]
    if (!v) throw new Error(`ENOKEY (key=${v})`)

    this.add(v.publicKey, v.privateKey)

    const pk = toPK(v.publicKey)
    const pt = toUint8Array(message)
    const ct = sodium.crypto_box_seal(pt, pk)
    return sodium.crypto_box_seal(toUint8Array(ct), pk)
  }

  /**
   * Decrypts a sealed and signed message for a specific receiver.
   * @param {Buffer} message - The sealed message.
   * @param {Object|string} v - Key object or public key.
   * @returns {Buffer} - Decrypted message.
   * @throws {Error} - Throws ENOKEY if the key is not found, EMALFORMED if the message is malformed, ENOTVERIFIED if the message cannot be verified.
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

/**
 * Converts an Ed25519 public key to a Curve25519 public key.
 * @param {Uint8Array} pk - Ed25519 public key.
 * @returns {Uint8Array} - Curve25519 public key.
 */
function toPK (pk) {
  try {
    return sodium.crypto_sign_ed25519_pk_to_curve25519(pk)
  } catch {}

  return new Uint8Array(0)
}

/**
 * Converts an Ed25519 secret key to a Curve25519 secret key.
 * @param {Uint8Array} sk - Ed25519 secret key.
 * @returns {Uint8Array} - Curve25519 secret key.
 */
function toSK (sk) {
  try {
    return sodium.crypto_sign_ed25519_sk_to_curve25519(sk)
  } catch {}

  return new Uint8Array(0)
}

/**
 * Converts different types of input to a Uint8Array.
 * @param {Uint8Array|string|Buffer} buffer - Input buffer.
 * @returns {Uint8Array} - Uint8Array representation of the input.
 */
const textEncoder = new TextEncoder()

/**
 * Converts different types of input to a Uint8Array.
 * @param {Uint8Array|string|Buffer} buffer - Input buffer.
 * @returns {Uint8Array} - Uint8Array representation of the input.
 */
function toUint8Array (buffer) {
  if (buffer instanceof Uint8Array) {
    return buffer
  }

  if (typeof buffer === 'string') {
    return textEncoder.encode(buffer)
  }

  if (buffer?.buffer instanceof ArrayBuffer) {
    return new Uint8Array(buffer.buffer)
  }

  if (isArrayLike(buffer) || isBufferLike(buffer)) {
    return new Uint8Array(buffer)
  }

  if (buffer instanceof ArrayBuffer) {
    return new Uint8Array(buffer)
  }

  if (buffer && Symbol.iterator in buffer) {
    return new Uint8Array(buffer)
  }

  return new Uint8Array(0)
}
