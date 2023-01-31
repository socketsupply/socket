#!/usr/bin/env node
import path from 'node:path'
import fs from 'node:fs/promises'
import os from 'node:os'

import esbuild from 'esbuild'

const cp = async (a, b) => fs.cp(
  path.resolve(a),
  path.join(b, path.basename(a)),
  { recursive: true, force: true }
)

async function copy (target) {
  await Promise.all([
    cp('src/frontend/index.html', target),
    cp('src/frontend/index_second_window.html', target),
    cp('src/frontend/index_second_window2.html', target),
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
    external: ['socket:*', 'node:*'],
    outdir: path.resolve(process.argv[2]),
    plugins: []
  }

  if (os.platform() === 'win32') {
    params.plugins.push({
      name: 'socket-runtime-import-path',
      setup (build) {
        build.onResolve({ filter: /^socket:.*$/ }, (args) => {
          return {
            external: true,
            path: args.path.replace('socket:', `.${path.sep}socket${path.sep}`).replace(/.js$/, '') + '.js'
          }
        })
      }
    })
  }

  await Promise.all([
    esbuild.build(params),
    esbuild.build({ ...params, entryPoints: ['src/frontend/index_second_window.js'] }),
    copy(params.outdir)
  ])
}

main()
