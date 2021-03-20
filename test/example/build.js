import path from 'node:path'
import CleanCSS from 'clean-css'
import stylus from 'stylus'
import esbuild from 'esbuild'

const target = process.argv.slice(2)[0];

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

async function main () {
  await esbuild.build({
    entryPoints: ['src/render.js'],
    bundle: true,
    keepNames: true,
    minify: true,
    outfile: path.join(target, 'bundle.js'),
    platform: 'browser'
  })

  await css(
    path.join('src', 'index.styl'),
    path.join(target, 'bundle.css')
  )
}

main()
