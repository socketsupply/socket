import { URLPattern } from './urlpattern/urlpattern.js'
import url from './url/url.js'

const { URL, URLSearchParams, parseURL } = url

export default URL
export { URLPattern, URL, URLSearchParams, parseURL }

export const parse = parseURL

// lifted from node
export function resolve (from, to) {
  const resolvedUrl = new URL(to, new URL(from, 'resolve://'));
  if (resolvedUrl.protocol === 'resolve:') {
    const { pathname, search, hash } = resolvedUrl;
    return pathname + search + hash;
  }
  return resolvedUrl.toString();
}
