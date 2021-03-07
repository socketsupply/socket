#!/usr/bin/env node
import fs from 'node:fs/promises'
import cp from 'node:child_process'
import esbuild from 'esbuild'
import CleanCSS from 'clean-css'
import stylus from 'stylus'

const exec = s => new Promise(resolve => {
  const params = {
    stdio: 'pipe',
    cwd: process.cwd(),
    env: process.env,
    windowsHide: true,
    timeout: 6e4,
    encoding: 'utf-8',
  }

  cp.exec(s, params, (err, stdout, stderr) => {
    resolve({
      err,
      stderr: String(stderr),
      data: String(stdout)
    })
  })
})

const cleanCSS = new CleanCSS({ advanced: true })

const css = async (src, dest) => {
  const s = await fs.readFile(src, 'utf8')

  const css = await new Promise((resolve, reject) => {
    return stylus.render(s, { filename: src }, (err, css) => {
      if (err) return reject(err)
      return resolve(css)
    })
  })

  const minified = cleanCSS.minify(css)
  return fs.writeFile(dest, minified.styles)
}

const getLibraries = async (os) => {
  switch (os) {
    case 'darwin': return '-luv -lpthread -std=c++2a -framework WebKit'
    case 'linux': return exec('pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0')
    case 'win32': return '-mwindows -L./dll/x64 -lwebview -lWebView2Loader'
  }
}

async function main () {
  console.time('build')
  await fs.rmdir('build', { recursive: true })
  await fs.mkdir('build')

  await esbuild.build({
    entryPoints: ['src/index.js'],
    bundle: true,
    keepNames: true,
    minify: true,
    outfile: 'build/bundle.js',
    platform: 'browser'
  })

  await css('src/index.styl', 'build/bundle.css')

  await exec(`cp src/render.html build`)
  await exec(`cp src/main.js build`)

  const config = {
    compiler: 'g++',
    src: 'main.cc',
    libs: await getLibraries(process.platform),
    dest: '-o build/operator',
    flags: '-O3'
  }

  const res = await exec(Object.values(config).join(' '))

  if (res.stderr) {
    console.log(res.stderr)
    process.exit(1)
  }
  
  console.timeEnd('build')
}

main()
