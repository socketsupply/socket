import { createRequire } from '../module.js'
import process from '../process.js'
import module from './module.js'
import http from '../http.js'
import util from '../util.js'

export async function onRequest (request, env, ctx) {
  if (process.env.SOCKET_RUNTIME_NPM_DEBUG) {
    console.debug(request.url)
  }

  const url = new URL(request.url)
  const origin = url.origin.replace('npm://', 'socket://')
  const specifier = url.pathname.replace('/socket/npm/', '')
  const importOrigins = url.searchParams.getAll('origin').concat(url.searchParams.getAll('origin[]'))

  let resolved = null
  let origins = []

  for (const value of importOrigins) {
    if (value.startsWith('npm:')) {
      origins.push(value)
    } else if (value.startsWith('.')) {
      origins.push(new URL(value, `socket://${url.host}${url.pathname}`).href.replace(/\/$/, ''))
    } else if (value.startsWith('/')) {
      origins.push(new URL(value, origin).href)
    } else if (URL.canParse(value)) {
      origins.push(value)
    } else if (URL.canParse(`socket://${value}`)) {
      origins.push(`socket://${value}`)
    }
  }

  origins.push(origin)
  origins = Array.from(new Set(origins))

  if (process.env.SOCKET_RUNTIME_NPM_DEBUG) {
    console.debug('resolving: npm:%s', specifier)
  }

  while (origins.length && resolved === null) {
    const potentialOrigins = []
    const origin = origins.shift()

    if (origin.startsWith('npm:')) {
      const potentialSpeciifier = new URL(origin).pathname
      for (const potentialOrigin of origins) {
        try {
          const resolution = await module.resolve(potentialSpeciifier, potentialOrigin)
          if (resolution) {
            potentialOrigins.push(new URL(resolution.path, resolution.origin).href)
            break
          }
        } catch { }
      }
    } else {
      potentialOrigins.push(origin)
    }

    while (potentialOrigins.length && resolved === null) {
      const importOrigin = new URL('./', potentialOrigins.shift()).href
      try {
        resolved = await module.resolve(specifier, importOrigin)
      } catch (err) {
        globalThis.reportError(err)
        return new Response(util.inspect(err), {
          status: 500
        })
      }
    }
  }

  // not found
  if (!resolved) {
    if (process.env.SOCKET_RUNTIME_NPM_DEBUG) {
      console.debug('not found: npm:%s', specifier)
    }
    return
  }

  const source = new URL(resolved.path, resolved.origin)

  if (resolved.type === 'module') {
    const response = await fetch(source.href)
    const text = await response.text()
    const proxy = text.includes('export default')
      ? `
        import module from '${source.href}'
        export * from '${source.href}'
        export default module
        `
      : `
        import * as module from '${source.href}'
        export * from '${source.href}'
        export default module
        `

    return new Response(proxy, {
      headers: {
        'content-type': 'text/javascript'
      }
    })
  }

  if (resolved.type === 'commonjs') {
    const pathspec = (source.pathname + source.search).replace(/^\/node_modules\//, '')
    console.log({ origin: source.origin, pathspec })
    const proxy = `
      import { createRequire } from 'socket:module'
      const require = createRequire('${source.origin + '/'}', {
        headers: {
          'Runtime-ServiceWorker-Fetch-Mode': 'ignore'
        }
      })
      const exports = require('${pathspec}')
      console.log('${pathspec}', exports)
      export default exports?.default ?? exports ?? null
      `

    return new Response(proxy, {
      headers: {
        'content-type': 'text/javascript'
      }
    })
  }
}

/**
 * Handles incoming 'npm://<module_name>/<pathspec...>' requests.
 * @param {Request} request
 * @param {object} env
 * @param {import('../service-worker/context.js').Context} ctx
 * @return {Response?}
 */
export default async function (request, env, ctx) {
  if (request.method !== 'GET') {
    return new Response('Invalid HTTP method', {
      status: http.BAD_REQUEST
    })
  }

  try {
    return await onRequest(request, env, ctx)
  } catch (err) {
    globalThis.reportError(err)

    if (process.env.SOCKET_RUNTIME_NPM_DEBUG) {
      console.debug(err)
    }
  }
}
