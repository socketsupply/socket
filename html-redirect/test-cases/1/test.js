import test from 'node:test'
import assert from 'node:assert'
import desm from '../../desm.js'
import { redirect } from '../../redirect.js'

const __dirname = desm(import.meta.url)

test('index export', async (t) => {
  // assert.strictEqual(await redirect('/', __dirname), '/index.html')
  // assert.strictEqual(await redirect('/index.html', __dirname), '/index.html')
  assert.strictEqual(await redirect('/a-conflict-index', __dirname), '/a-conflict-index/index.html')
  // assert.strictEqual(await redirect('/another-file', __dirname), '/another-file.html')
  // assert.strictEqual(await redirect('/an-index-file/', __dirname), '/an-index-file/index.html')
  // assert.strictEqual(await redirect('/an-index-file', __dirname), '/an-index-file/index.html')
  // assert.strictEqual(await redirect('/an-index-file/a-html-file', __dirname), '/an-index-file/.html')
})
