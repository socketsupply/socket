import path from 'path'
import fs from 'fs/promises'
import esbuild from 'esbuild'

//
// The output target is passed by the build tool,
// it's where we want to write all of our files.
//
const target = path.resolve(process.argv[2]);

if (!target) {
  console.log('  - Did not receive the build target path as an argument')
  process.exit(1)
}

const cp = async (a, b) => fs.copyFile(
  path.resolve(a),
  path.join(b, path.basename(a))
)

async function main () {
  let ext = ''

  switch (process.platform) {
    case 'win32':
      ext = 'ico'
      await cp(`src/icons/icon.png`, target)
      break;
    case 'linux':
      ext = 'png'
      break
    case 'darwin':
      ext = 'icns'
  }

  await Promise.all([
    esbuild.build({
      entryPoints: ['src/render/index.jsx'],
      bundle: true,
      keepNames: true,
      // minify: true,
      outfile: path.join(target, 'render.js'),
      platform: 'browser'
    }),
    cp(`src/icons/icon.${ext}`, target), // app icon
    cp('src/render/index.html', target), // base HTML file
    cp('src/main/main.js', target),      // main process script
  ])
}

main()
