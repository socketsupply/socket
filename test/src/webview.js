import { test } from 'socket:test'

let loaded = false
window.onload = () => {
  loaded = true
}

test('window.onload', (t) => {
  t.ok(loaded, 'window.onload called')
})
