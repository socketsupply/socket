import { resolve } from './module.js'
import process from '../process.js'
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
    if (process.env.SOCKET_RUNTIME_NPM_DEBUG) {
      console.debug('not found: npm:%s', specifier)
    }
    return
  }

  if (resolved.type === 'module') {
    const response = await fetch(resolved.url)
    const text = await response.text()
    const proxy = text.includes('export default')
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

    return new Response(proxy, {
      headers: {
        'content-type': 'text/javascript'
      }
    })
  }

  if (resolved.type === 'commonjs') {
    const proxy = `
      import { Module } from 'socket:module'
      const exports = Module.main.require('${resolved.url}')
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

    return new Response(util.inspect(err), {
      status: 500
    })
  }
}
