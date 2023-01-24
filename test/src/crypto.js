import crypto from '../../crypto.js'
import Buffer from '../../buffer.js'
import { test } from '@socketsupply/tapzero'

test('crypto', async (t) => {
  t.equal(crypto.webcrypto, window.crypto, 'crypto.webcrypto is window.crypto')
  const randomValues = crypto.getRandomValues(new Uint32Array(10))
  t.equal(randomValues.length, 10, 'crypto.getRandomValues returns an array of the correct length')
  t.ok(randomValues.some(value => value !== randomValues[9], 'crypto.getRandomValues returns an array of random values'))
  t.ok(randomValues.every(value => Number.isInteger(value)), 'crypto.getRandomValues returns an array of integers')

  t.equal(crypto.RANDOM_BYTES_QUOTA, 64 * 1024, 'crypto.RANDOM_BYTES_QUOTA is 65536')
  t.equal(crypto.MAX_RANDOM_BYTES, 0xFFFF_FFFF_FFFF, 'crypto.MAX_RANDOM_BYTES is 0xFFFF_FFFF_FFFF')
  t.equal(crypto.MAX_RANDOM_BYTES_PAGES, crypto.MAX_RANDOM_BYTES / crypto.RANDOM_BYTES_QUOTA, `crypto.MAX_RANDOM_BYTES_PAGES is ${crypto.MAX_RANDOM_BYTES / crypto.RANDOM_BYTES_QUOTA}`)

  const buffer = crypto.randomBytes(10)
  t.equal(buffer.length, 10, 'crypto.randomBytes returns a buffer of the correct length')
  t.ok(buffer.some(value => value !== buffer[9], 'crypto.randomBytes returns a buffer of random values'))
  t.ok(buffer.every(value => Number.isInteger(value)), 'crypto.randomBytes returns a buffer of integers')

  const digest = await crypto.createDigest('SHA-256', new Uint8Array(32))
  t.ok(digest instanceof Buffer, 'crypto.createDigest returns a buffer')
  t.equal(digest.length, 32, 'crypto.createDigest returns a buffer of the correct length')
})
