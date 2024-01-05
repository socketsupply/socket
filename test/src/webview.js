import test from 'socket:test'

import { fetch, Headers, Request, Response } from 'socket:fetch'
import { URL, URLPattern, URLSearchParams } from 'socket:url'
import { ApplicationURLEvent } from 'socket:internal/events'
import Notification from 'socket:notification'
import { Buffer } from 'socket:buffer'

import {
  File,
  FileSystemHandle,
  FileSystemFileHandle,
  FileSystemDirectoryHandle,
  FileSystemWritableFileStream
} from 'socket:fs/web'

import {
  showDirectoryPicker,
  showOpenFilePicker,
  showSaveFilePicker
} from 'socket:internal/pickers'

const loaded = new Promise((resolve) => {
  globalThis.addEventListener('load', () => {
    resolve(true)
  }, { once: true })
})

test('globalThis.onload', async (t) => {
  t.ok(await loaded, 'globalThis.onload called')
})

test('globalThis.isSocketRuntime', (t) => {
  t.ok(globalThis.isSocketRuntime === true, 'globalThis.isSocketRuntime')
})

test('globalThis.Buffer', (t) => {
  t.ok(typeof globalThis.Buffer === 'function' && globalThis.Buffer === Buffer, 'globalThis.Buffer')
})

test('globalThis.URL', (t) => {
  t.ok(typeof globalThis.URL === 'function' && globalThis.URL === URL, 'globalThis.URL')
})

test('globalThis.URLPattern', (t) => {
  t.ok(typeof globalThis.URLPattern === 'function' && globalThis.URLPattern === URLPattern, 'globalThis.URLPattern')
})

test('globalThis.URLSearchParams', (t) => {
  t.ok(typeof globalThis.URLSearchParams === 'function' && globalThis.URLSearchParams === URLSearchParams, 'globalThis.URLSearchParams')
})

test('globalThis.fetch', (t) => {
  t.ok(typeof globalThis.fetch === 'function' && globalThis.fetch === fetch, 'globalThis.fetch')
})

test('globalThis.Headers', (t) => {
  t.ok(typeof globalThis.Headers === 'function' && globalThis.Headers === Headers, 'globalThis.Headers')
})

test('globalThis.Request', (t) => {
  t.ok(typeof globalThis.Request === 'function' && globalThis.Request === Request, 'globalThis.Request')
})

test('globalThis.Response', (t) => {
  t.ok(typeof globalThis.Response === 'function' && globalThis.Response === Response, 'globalThis.Response')
})

test('globalThis.File', (t) => {
  t.ok(typeof globalThis.File === 'function' && globalThis.File === File, 'globalThis.File')
})

test('globalThis.FileSystemHandle', (t) => {
  t.ok(typeof globalThis.FileSystemHandle === 'function' && globalThis.FileSystemHandle === FileSystemHandle, 'globalThis.FileSystemHandle')
})

test('globalThis.FileSystemFileHandle', (t) => {
  t.ok(typeof globalThis.FileSystemFileHandle === 'function' && globalThis.FileSystemFileHandle === FileSystemFileHandle, 'globalThis.FileSystemFileHandle')
})

test('globalThis.FileSystemDirectoryHandle', (t) => {
  t.ok(typeof globalThis.FileSystemDirectoryHandle === 'function' && globalThis.FileSystemDirectoryHandle === FileSystemDirectoryHandle, 'globalThis.FileSystemDirectoryHandle')
})

test('globalThis.FileSystemWritableFileStream', (t) => {
  t.ok(typeof globalThis.FileSystemWritableFileStream === 'function' && globalThis.FileSystemWritableFileStream === FileSystemWritableFileStream, 'globalThis.FileSystemWritableFileStream')
})

test('globalThis.showDirectoryPicker', (t) => {
  t.ok(typeof globalThis.showDirectoryPicker === 'function' && globalThis.showDirectoryPicker === showDirectoryPicker, 'globalThis.showDirectoryPicker')
})

test('globalThis.showOpenFilePicker', (t) => {
  t.ok(typeof globalThis.showOpenFilePicker === 'function' && globalThis.showOpenFilePicker === showOpenFilePicker, 'globalThis.showOpenFilePicker')
})

test('globalThis.showSaveFilePicker', (t) => {
  t.ok(typeof globalThis.showSaveFilePicker === 'function' && globalThis.showSaveFilePicker === showSaveFilePicker, 'globalThis.showSaveFilePicker')
})

test('globalThis.ApplicationURLEvent', (t) => {
  t.ok(typeof globalThis.ApplicationURLEvent === 'function' && globalThis.ApplicationURLEvent === ApplicationURLEvent, 'globalThis.ApplicationURLEvent')
})

test('globalThis.Notification', (t) => {
  t.ok(typeof globalThis.Notification === 'function' && globalThis.Notification === Notification, 'globalThis.Notification')
})

test('globalThis.navigator.permissions', (t) => {
  t.ok(typeof globalThis.navigator.permissions.query === 'function', 'globalThis.navigator.permissions.query')
})
