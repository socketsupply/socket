#!/usr/bin/env node
import path from 'node:path'
import fs from 'node:fs/promises'

import esbuild from 'esbuild'

const cp = async (a, b) => fs.cp(
  path.resolve(a),
  path.join(b, path.basename(a)),
  { recursive: true, force: true }
)

async function copy (target) {
  await Promise.all([
    cp('src/frontend/index.html', target),
    cp('src/frontend/index_send_event.html', target),
    cp('src/frontend/index_no_js.html', target),
    cp('src/frontend/index_no_js2.html', target),
    cp('fixtures', target),
    // for testing purposes
    cp('socket.ini', target),
    // backend
    cp('src/backend/backend.js', target)
  ])
}

async function main () {
  const params = {
    entryPoints: ['src/index.js'],
    format: 'esm',
    bundle: true,
    keepNames: true,
    platform: 'browser',
    sourcemap: 'inline',
    external: ['node:*', 'socket:*'],
    outdir: path.resolve(process.argv[2]),
    plugins: []
  }

  await Promise.all([
    esbuild.build(params),
    esbuild.build({ ...params, entryPoints: ['src/frontend/index_send_event.js'] }),
    copy(params.outdir)
  ])
}

main()
