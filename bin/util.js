import path from 'node:path'
import fs from 'node:fs/promises'
import cp from 'node:child_process'

import CleanCSS from 'clean-css'
import stylus from 'stylus'

const exec = (s, opts = {}, env = {}) => new Promise(resolve => {
  const params = {
    stdio: 'pipe',
    cwd: process.cwd(),
    env: { ...process.env, ...env },
    windowsHide: true,
    timeout: 6e4,
    encoding: 'utf-8',
    ...opts
  }

  cp.exec(s, params, (err, stdout, stderr) => {
    resolve({
      err,
      stderr: String(stderr),
      data: String(stdout)
    })
  })
})

const css = async (src, dest) => {
  const cleanCSS = new CleanCSS({ advanced: true })
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

const mkdir = (...args) => {
  const dir = path.join(...args)
  fs.mkdir(dir, { recursive: true })
  return dir
}

export {
  exec,
  css,
  mkdir
}
