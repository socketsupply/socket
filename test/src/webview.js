import process from 'socket:process'
import test from 'socket:test'

let loaded = false
window.addEventListener('load', () => {
  loaded = true
})

if (!process.env.SSC_ANDROID_CI) {
  test('window.onload', (t) => {
    t.ok(loaded, 'window.onload called')
  })
}
