import { resolve } from './module.js'
import process from '../process.js'
import debug from '../service-worker/debug.js'
import mime from '../mime.js'
import path from '../path.js'
import http from '../http.js'
import util from '../util.js'

const DEBUG_LABEL = '  <span\\sstyle="color:\\s#fb8817;"><b>npm</b></span>'

/**
 * @ignore
 * @param {Request}
 * @param {object} env
 * @param {import('../service-worker/context.js').Context} ctx
 * @return {Promise<Response|null>}
 */
export async function onRequest (request, env, ctx) {
  // eslint-disable-next-line
  void env, ctx;
  if (process.env.SOCKET_RUNTIME_NPM_DEBUG) {
    console.debug(request.url)
  }

  const url = new URL(request.url)
  const origin = url.origin.replace('npm://', 'socket://')
  const referer = request.headers.get('referer')
  let specifier = url.pathname.replace('/socket/npm/', '')
  const importOrigins = url.searchParams.getAll('origin').concat(url.searchParams.getAll('origin[]'))

  if (typeof specifier === 'string') {
    try {
      specifier = (new URL(require.resolve(specifier, { type: 'module' }))).toString()
    } catch {}
  }

  debug(`${DEBUG_LABEL}: fetch: %s`, specifier)

  let resolved = null
  let origins = []

  if (referer && !referer.startsWith('blob:')) {
    // @ts-ignore
    if (URL.canParse(referer, origin)) {
      const refererURL = new URL(referer, origin)
      if (refererURL.href.endsWith('/')) {
        importOrigins.push(refererURL.href)
      } else {
        importOrigins.push(new URL('./', refererURL).href)
      }
    }
  }

  for (const value of importOrigins) {
    if (value.startsWith('npm:')) {
      origins.push(value)
    } else if (value.startsWith('.')) {
      origins.push(new URL(value, `socket://${url.host}${url.pathname}`).href.replace(/\/$/, ''))
    } else if (value.startsWith('/')) {
      origins.push(new URL(value, origin).href)
      // @ts-ignore
    } else if (URL.canParse(value)) {
      origins.push(value)
      // @ts-ignore
    } else if (URL.canParse(`socket://${value}`)) {
      origins.push(`socket://${value}`)
    }
  }

  origins.push(origin)
  origins = Array.from(new Set(origins))

  if (process.env.SOCKET_RUNTIME_NPM_DEBUG) {
    console.debug('resolving: npm:%s (%o)', specifier, origins)
  }

  while (origins.length && resolved === null) {
    const potentialOrigins = []
    const origin = origins.shift()

    if (origin.startsWith('npm:')) {
      const potentialSpecifier = new URL(origin).pathname
      for (const potentialOrigin of origins) {
        const resolution = await resolve(potentialSpecifier, potentialOrigin)
        if (resolution) {
          potentialOrigins.push(resolution.url)
          break
        }
      }
    } else {
      potentialOrigins.push(origin)
    }

    while (potentialOrigins.length && resolved === null) {
      const importOrigin = new URL('./', potentialOrigins.shift()).href
      resolved = await resolve(specifier, importOrigin)
    }
  }

  // not found
  if (!resolved) {
    debug(`${DEBUG_LABEL}: not found: %s`, specifier)
    return
  }

  const extname = path.extname(resolved.url)
  const types = extname ? await mime.lookup(extname.slice(1)) : []

  if (types.length || resolved.type === 'file') {
    let redirect = true
    for (const type of types) {
      if (type.mime === 'text/javascript' || type.mime.endsWith('json')) {
        redirect = false
        break
      }
    }

    if (redirect) {
      debug(`${DEBUG_LABEL}: resolve: %s (file): %s`, specifier, resolved.url)
      return Response.redirect(resolved.url, 301)
    }
  }

  debug(`${DEBUG_LABEL}: resolve: %s (%s): %s`, specifier, resolved.type, resolved.url)

  if (resolved.type === 'module') {
    const response = await fetch(resolved.url)
    const text = await response.text()
    const proxy = /^(export default)/.test(text)
      ? `
        import module from '${resolved.url}'
        export * from '${resolved.url}'
        export default module
        `
      : `
        import * as module from '${resolved.url}'
        export * from '${resolved.url}'
        export default module
        `

    const source = proxy
      .trim()
      .split('\n')
      .map((line) => line.trim())
      .join('\n')
    return new Response(source, {
      headers: {
        'content-type': 'text/javascript'
      }
    })
  }

  if (resolved.type === 'commonjs') {
    const proxy = `
      import { createRequire } from 'socket:module'
      const headers = { 'Runtime-ServiceWorker-Fetch-Mode': 'ignore' }
      const require = createRequire('${resolved.origin}', { headers })
      const exports = require('${resolved.url}')
      export default exports?.default ?? exports ?? null
    `.trim()

    const source = proxy
      .trim()
      .split('\n')
      .map((line) => line.trim())
      .join('\n')
    return new Response(source, {
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
  if (request.method === 'OPTIONS') {
    return new Response('OK', { status: 204 })
  }

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

    return new Response(util.inspect(err), {
      status: 500
    })
  }
}
