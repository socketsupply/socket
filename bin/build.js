#!/usr/bin/env node
import fs from 'node:fs/promises'
import path from 'node:path'
import esbuild from 'esbuild'
import { css, exec, mkdir } from './util.js'

const pkg = async f => JSON.parse(await fs.readFile(f, 'utf8'))
const PLATFORM = process.platform

async function main () {
  const config = await pkg('package.json')

  console.time('• Build complete')

  console.log('• Cleaning up')
  await fs.rmdir('build', { recursive: true })

  const pathToBuild = await mkdir('build')
  const pathToApp = await mkdir(pathToBuild, 'Operator.app')
  const pathToPackage = await mkdir(pathToApp, 'Contents')
  const pathToBinary = await mkdir(pathToPackage, 'MacOS')
  const pathToResources = await mkdir(pathToPackage, 'Resources')

  console.log('• Creating directories')

  console.log('• Compiling js')

  await esbuild.build({
    entryPoints: ['src/render.js'],
    bundle: true,
    keepNames: true,
    minify: true,
    outfile: `${pathToResources}/bundle.js`,
    platform: 'browser'
  })

  console.log('• Building css')

  await css('src/index.styl', `${pathToResources}/bundle.css`)

  console.log('• Copying files')

  await exec(`cp src/render.html ${pathToResources}`)
  await exec(`cp src/main.js ${pathToResources}`)
  await exec(`cp src/ipc.js ${pathToResources}`)
  await exec(`cp src/package.json ${pathToResources}`)
  await exec(`cp src/icons/icon.icns ${pathToResources}`)
  await exec(`cp settings/${PLATFORM}/* ${pathToResources}`)
  await exec(`cp settings/${PLATFORM}/Info.plist ${pathToResources}`)

  const binaryName = 'Operator'
  const targetBinary = path.join(pathToBinary, binaryName)

  const compile = {
    compiler: 'g++',
    src: config.files[PLATFORM],
    flags: config.flags[PLATFORM],
    target: `-o ${targetBinary}`,
    optimizations: '-O3'
  }

  console.log('• Compiling native binary')

  const res = await exec(Object.values(compile).join(' '))

  if (res.stderr) {
    console.log(res.stderr)
  }
  
  console.timeEnd('• Build complete')

  if (process.argv[1] === 'run') {
    await exec('open Operator.app', { cwd: pathToBuild })
  }
}

main()
