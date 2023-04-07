const { Module } = require('socket:module')
const dgram = require('socket:dgram')
const test = require('socket:test')
const fs = require('socket:fs')

test('commonjs - custom resolver - mapping', (t) => {
  Module.resolvers.push((specifier, ctx, next) => {
    if (specifier === 'filesystem') {
      return next('socket:fs')
    }

    if (specifier === 'udp') {
      return next('socket:dgram')
    }

    return next(specifier)
  })

  t.equal(require('filesystem'), fs, 'filesystem -> fs')
  t.equal(require('udp'), dgram, 'udp -> dgram')
})

test('commonjs - custom resolver - virtual', (t) => {
  const virtual = { key: 'value' }
  Module.resolvers.push((specifier, ctx, next) => {
    if (specifier === 'virtual') {
      return virtual
    }

    return next(specifier)
  })

  t.equal(require('virtual'), virtual, 'require(\'virtual\') -> virtual')
})
