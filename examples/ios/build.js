const path = require('path')
const fs = require('fs/promises')

const Minifier = require('clean-css')
const { build: js } = require('esbuild')

//
// The output target is passed by the build tool,
// it's where we want to write all of our files.
//
const target = path.resolve(process.argv[2]);

if (!target) {
  console.log('Did not receive the build target path as an argument!')
  process.exit(1)
}

const minifier = new Minifier({ advanced: true })

const css = async (src, dest) => {
  let str = await fs.readFile(src, 'utf8')
  let RE = /@import ['"]([^'" ]+)['"];/g
  let reqs = []

  str.replace(RE, (_, p) => {
    reqs.push(css(path.resolve(path.dirname(src), p)))
  })

  const data = await Promise.all(reqs)
  str = str.replace(RE, () => data.shift())

  const min = minifier.minify(str)

  if (!dest) return min.styles
  return fs.writeFile(dest, min.styles)
}

const cp = async (a, b) => fs.cp(
  path.resolve(a),
  path.join(b, path.basename(a)),
  { recursive: true, force: true }
)

async function main () {
  //
  // Neither wkwebview or webview2 support local import. NBD,
  // esbuild is fast, keeps the product small and reduces io
  // in production.
  //
  await js({
    entryPoints: ['src/index.js'],
    bundle: true,
    keepNames: true,
    format: 'cjs',
    minify: true,
    outfile: path.join(target, 'bundle.js'),
    platform: 'browser'
  })

  //
  // We want compile time @import so we can organize and
  // minify non-component styles.
  //
  await css(
    path.join('src', 'index.css'),
    path.join(target, 'bundle.css')
  )

  //
  // Copy the rest of the files that we care about.
  //
  await cp('src/index.html', target)
  await cp(`src/icons/icon.png`, target)
  await cp('src/images', target)
}

main()
