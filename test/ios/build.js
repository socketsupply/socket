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

const cp = async (a, b) => fs.cp(
  path.resolve(a),
  path.join(b, path.basename(a)),
  { recursive: true, force: true }
)

async function main () {
  await esbuild.build({
    entryPoints: ['src/render/index.js'],
    bundle: true,
    keepNames: true,
    minify: true,
    outfile: path.join(target, 'render.js'),
    platform: 'browser'
  })

  await css(
    path.join('src', 'render/index.styl'),
    path.join(target, 'bundle.css')
  )

  await cp('src/embedded.mobileprovision', target)
  // await cp('src/PkgInfo', target)
  await cp('src/render/index.html', target)
  await cp('src/Default-568h@2x.png', target)
  await cp(`src/icons/index.png`, target)
  await cp('src/images', target)
}

main()
