import test from 'node:test'
import assert from 'node:assert'
import desm from '../../desm.js'
import {join} from 'path'
import { redirect } from '../../redirect.js'

const __dirname = desm(import.meta.url)

test('index export', async (t) => {
  const results2 = await redirect(join(__dirname, 'does-not-exist.html'))
  const results = await redirect(join(__dirname, 'index.html'))
})
