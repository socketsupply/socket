const path = require('path')
const fs = require('fs/promises')

const CleanCSS = require('clean-css')
const stylus = require('stylus')
const esbuild = require('esbuild')

//
// The output target is passed by the build tool,
// it's where we want to write all of our files.
//
const target = path.resolve(process.argv[2]);

if (!target) {
  console.log('  - Did not receive the build target path as an argument')
  process.exit(1)
}

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

async function cp (src, dest) {
  const entries = await fs.readdir(src, { withFileTypes: true })
  await fs.mkdir(dest, { recursive: true })

  for (const entry of entries) {
    await (entry.isDirectory() ? cp : fs.copyFile)(
      path.join(src, entry.name),
      path.join(dest, entry.name)
    )
  }
}

async function main () {
  await esbuild.build({
    entryPoints: ['src/render/index.js'],
    bundle: true,
    keepNames: true,
    minify: true,
    outfile: path.join(target, 'render.js'),
    platform: 'browser'
  })

  await esbuild.build({
    entryPoints: ['src/main/index.js'],
    bundle: true,
    keepNames: true,
    minify: true,
    format: 'cjs',
    outfile: path.join(target, 'main.js'),
    platform: 'node'
  })

  await css(
    path.join('src', 'render/index.styl'),
    path.join(target, 'bundle.css')
  )

  await fs.copyFile('src/render/index.html', path.join(target, 'index.html'))
  await fs.copyFile(`src/icons/index.png`, path.join(target, `index.png`))
  await cp('src/images', path.join(target, 'images'))
}

main()
