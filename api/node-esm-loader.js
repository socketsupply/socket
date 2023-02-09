export async function resolve (specifier, ctx, next) {
  if (/^socket:/.test(specifier)) {
    let moduleName = specifier.replace('socket:', '')
    if (moduleName.endsWith('.js')) {
      moduleName = moduleName.slice(0, -3)
    }

    specifier = `@socketsupply/socket/${moduleName}.js`
  }

  return next(specifier)
}

export default resolve
