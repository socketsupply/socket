const { SOCKET_MODULES = 'node_modules' } = process.env

export async function resolve (specifier, ctx, next) {
  if (/^socket:modules/.test(specifier)) {
    let moduleName = specifier.replace('socket:modules/', '')
    if (moduleName.endsWith('.js')) {
      moduleName = moduleName.slice(0, -3)
    }

    specifier = `${SOCKET_MODULES}/${moduleName}.js`
  } else if (/^socket:/.test(specifier)) {
    let moduleName = specifier.replace('socket:', '')
    if (moduleName.endsWith('.js')) {
      moduleName = moduleName.slice(0, -3)
    }

    specifier = `@socketsupply/socket/${moduleName}.js`
  }

  return next(specifier)
}

export default resolve
