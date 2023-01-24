import dns from '../../dns.js'

import { test } from '@socketsupply/tapzero'

// node compat
// import dns from 'node:dns'

const IPV4_REGEX = /^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/
const IPV6_REGEX = /^(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|fe80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::(ffff(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))$/

const isOnline = Boolean(globalThis?.navigator?.onLine || process?.versions?.node)

test('dns exports', t => {
  t.ok(typeof dns.lookup === 'function', 'lookup is available')
  t.equal(dns.lookup.length, 3, 'lookup expect 3 arguments')
  t.ok(typeof dns.promises.lookup === 'function', 'promises.lookup is available')
  t.equal(dns.promises.lookup.length, 2, 'promises.lookup expect 2 arguments')
})

test('dns.lookup', async t => {
  if (!isOnline) {
    return t.comment('skipping offline')
  }

  await Promise.all([
    new Promise(resolve => {
      dns.lookup('google.com', (err, address, family) => {
        if (err) return t.fail(err)

        const isValidFamily = (family === 4) || (family === 6)

        t.ok(isValidFamily, 'is either IPv4 or IPv6 family')

        const v4 = IPV4_REGEX.test(address)
        const v6 = IPV6_REGEX.test(address)

        t.ok(v4 || v6, 'has valid address')
        resolve()
      })
    }),
    new Promise(resolve => {
      dns.lookup('google.com', 4, (err, address, family) => {
        if (err) return t.fail(err)
        t.equal(family, 4, 'is IPv4 family')
        t.ok(IPV4_REGEX.test(address), 'has valid IPv4 address')
        resolve()
      })
    }),
    new Promise(resolve => {
      dns.lookup('cloudflare.com', 6, (err, address, family) => {
        if (err) return t.fail(err)
        t.equal(family, 6, 'is IPv6 family')
        t.ok(IPV6_REGEX.test(address), 'has valid IPv6 address')
        resolve()
      })
    }),
    new Promise(resolve => {
      dns.lookup('google.com', { family: 4 }, (err, address, family) => {
        if (err) return t.fail(err)
        t.equal(family, 4, 'is IPv4 family')
        t.ok(IPV4_REGEX.test(address), 'has valid IPv4 address')
        resolve()
      })
    }),
    new Promise(resolve => {
      dns.lookup('cloudflare.com', { family: 6 }, (err, address, family) => {
        if (err) return t.fail(err)
        t.equal(family, 6, 'is IPv6 family')
        t.ok(IPV6_REGEX.test(address), 'has valid IPv6 address')
        resolve()
      })
    })
    // TODO: call with other options
  ])
})

const BAD_HOSTNAME = 'thisisnotahostname'

test('dns.lookup bad hostname', async t => {
  if (!isOnline) {
    return t.comment('skipping offline')
  }

  await new Promise(resolve => {
    dns.lookup(BAD_HOSTNAME, (err, info) => {
      t.equal(err.message, `getaddrinfo EAI_AGAIN ${BAD_HOSTNAME}`, 'returns an error on unexisting hostname')
      resolve()
    })
  })
})

test('dns.promises.lookup', async t => {
  if (!isOnline) {
    return t.comment('skipping offline')
  }

  try {
    const info = await dns.promises.lookup('google.com', 4)
    t.ok(info && typeof info === 'object', 'returns a non-error object after resolving a hostname')
    t.equal(info.family, 4, 'is IPv4 family')
    t.ok(IPV4_REGEX.test(info.address), 'has valid IPv4 address')
  } catch (err) {
    t.fail(err)
  }
  try {
    const info = await dns.promises.lookup('google.com', 6)
    t.ok(info && typeof info === 'object', 'returns a non-error object after resolving a hostname')
    t.equal(info.family, 6, 'is IPv6 family')
    t.ok(IPV6_REGEX.test(info.address), 'has valid IPv4 address')
  } catch (err) {
    t.fail(err)
  }
  try {
    const info = await dns.promises.lookup('google.com', { family: 4 })
    t.ok(info && typeof info === 'object', 'returns a non-error object after resolving a hostname')
    t.equal(info.family, 4, 'is IPv4 family')
    t.ok(IPV4_REGEX.test(info.address), 'has valid IPv4 address')
  } catch (err) {
    t.fail(err)
  }
  try {
    const info = await dns.promises.lookup('cloudflare.com', 6)
    t.ok(info && typeof info === 'object', 'returns a non-error object after resolving a hostname')
    t.equal(info.family, 6, 'is IPv6 family')
    t.ok(IPV6_REGEX.test(info.address), 'has valid IPv6 address')
  } catch (err) {
    t.fail(err)
  }
  try {
    const info = await dns.promises.lookup('cloudflare.com', { family: 6 })
    t.ok(info && typeof info === 'object', 'returns a non-error object after resolving a hostname')
    t.equal(info.family, 6, 'is IPv6 family')
    t.ok(IPV6_REGEX.test(info.address), 'has valid IPv6 address')
  } catch (err) {
    t.fail(err)
  }
})

test('dns.promises.lookup bad hostname', async t => {
  if (!isOnline) {
    return t.comment('skipping offline')
  }

  try {
    await dns.promises.lookup(BAD_HOSTNAME)
  } catch (err) {
    t.equal(err.message, `getaddrinfo EAI_AGAIN ${BAD_HOSTNAME}`, 'returns an error on unexisting hostname')
  }
})
