const Buffer = require('buffer')
const console = require('console')
const dgram = require('dgram')
const dns = require('dns')
const events = require('events')
const fs = require('fs')
const os = require('os')
const path = require('path')
const process = require('process')
const stream = require('stream')
const test = require('test')
const util = require('util')

test('builtins - buffer', (t) => {
  t.equal(typeof Buffer, 'function', 'Buffer is function')
  t.equal(typeof Buffer?.alloc, 'function', 'buffer.alloc is function')
})

test('builtins - console', (t) => {
  t.equal(typeof console, 'object', 'console is function')
  t.equal(typeof console?.log, 'function', 'console.log is function')
})

test('builtins - dgram', (t) => {
  t.equal(typeof dgram, 'object', 'dgram is function')
  t.equal(typeof dgram?.createSocket, 'function', 'dgram.createSocket is function')
})

test('builtins - dns', (t) => {
  t.equal(typeof dns, 'object', 'dns is function')
  t.equal(typeof dns?.lookup, 'function', 'dns.lookup is function')
})

test('builtins - events', (t) => {
  t.equal(typeof events, 'object', 'events is function')
  t.equal(typeof events?.EventEmitter, 'function', 'events.EventEmitter is function')
})

test('builtins - fs', (t) => {
  t.equal(typeof fs, 'object', 'fs is function')
  t.equal(typeof fs?.readdir, 'function', 'fs.readdir is function')
})

test('builtins - os', (t) => {
  t.equal(typeof os, 'object', 'os is function')
  t.equal(typeof os?.networkInterfaces, 'function', 'os.networkInterfaces is function')
})

test('builtins - path', (t) => {
  t.equal(typeof path, 'object', 'path is function')
  t.equal(typeof path?.normalize, 'function', 'path.normalize is function')
})

test('builtins - process', (t) => {
  t.equal(typeof process, 'object', 'process is function')
  t.equal(typeof process?.cwd, 'function', 'process.cwd is function')
})

test('builtins - stream', (t) => {
  t.equal(typeof stream, 'object', 'stream is function')
  t.equal(typeof stream?.PassThrough, 'function', 'stream.PassThrough is function')
})

test('builtins - util', (t) => {
  t.equal(typeof util, 'object', 'util is function')
  t.equal(typeof util?.format, 'function', 'util.format is function')
})
