import { test } from 'socket:test'
import mime from 'socket:mime'

test.only('mime.lookup', async (t) => {
  const results = await mime.lookup('html')
  t.ok(results)
  const tests = [
    { ext: 'aac', expect: ['audio/aac'] },
    { ext: 'abw', expect: ['application/x-abiword'] },
    { ext: 'arc', expect: ['application/x-freearc'] },
    { ext: 'avif', expect: ['image/avif'] },
    { ext: 'avi', expect: ['video/x-msvideo'] },
    { ext: 'azw', expect: ['application/vnd.amazon.ebook'] },
    { ext: 'bin', expect: ['application/octet-stream'] },
    { ext: 'bmp', expect: ['image/bmp'] },
    { ext: 'bz', expect: ['application/x-bzip'] },
    { ext: 'bz2', expect: ['application/x-bzip2'] },
    { ext: 'cda', expect: ['application/x-cdf'] },
    { ext: 'csh', expect: ['application/x-csh'] },
    { ext: 'css', expect: ['text/css'] },
    { ext: 'csv', expect: ['text/csv'] },
    { ext: 'doc', expect: ['application/msword'] },
    { ext: 'docx', expect: ['application/vnd.openxmlformats-officedocument.wordprocessingml.document'] },
    { ext: 'eot', expect: ['application/vnd.ms-fontobject'] },
    { ext: 'epub', expect: ['application/epub+zip'] },
    { ext: 'gz', expect: ['application/gzip'] },
    { ext: 'gif', expect: ['image/gif'] },
    { ext: 'htm', expect: ['text/html'] },
    { ext: 'ico', expect: ['image/vnd.microsoft.icon'] },
    { ext: 'ics', expect: ['text/calendar'] },
    { ext: 'jar', expect: ['application/java-archive'] },
    { ext: 'jpeg', expect: ['image/jpeg'] },
    { ext: 'js', expect: ['text/javascript'] },
    { ext: 'json', expect: ['application/json'] },
    { ext: 'jsonld', expect: ['application/ld+json'] },
    { ext: 'mid', expect: ['audio/midi'] },
    { ext: 'mjs', expect: ['text/javascript'] },
    { ext: 'mp3', expect: ['audio/mpeg'] },
    { ext: 'mp4', expect: ['video/mp4'] },
    { ext: 'mpeg', expect: ['video/mpeg'] },
    { ext: 'mpkg', expect: ['application/vnd.apple.installer+xml'] },
    { ext: 'odp', expect: ['application/vnd.oasis.opendocument.presentation'] },
    { ext: 'ods', expect: ['application/vnd.oasis.opendocument.spreadsheet'] },
    { ext: 'odt', expect: ['application/vnd.oasis.opendocument.text'] },
    { ext: 'oga', expect: ['audio/ogg'] },
    { ext: 'ogv', expect: ['video/ogg'] },
    { ext: 'ogx', expect: ['application/ogg'] },
    { ext: 'opus', expect: ['audio/opus'] },
    { ext: 'otf', expect: ['font/otf'] },
    { ext: 'png', expect: ['image/png'] },
    { ext: 'pdf', expect: ['application/pdf'] },
    { ext: 'php', expect: ['application/x-httpd-php'] },
    { ext: 'ppt', expect: ['application/vnd.ms-powerpoint'] },
    { ext: 'pptx', expect: ['application/vnd.openxmlformats-officedocument.presentationml.presentation'] },
    { ext: 'rar', expect: ['application/vnd.rar'] },
    { ext: 'rtf', expect: ['application/rtf'] },
    { ext: 'sh', expect: ['application/x-sh'] },
    { ext: 'svg', expect: ['image/svg+xml'] },
    { ext: 'tar', expect: ['application/x-tar'] },
    { ext: 'tif', expect: ['image/tiff'] },
    { ext: 'ts', expect: ['video/mp2t'] },
    { ext: 'ttf', expect: ['font/ttf'] },
    { ext: 'txt', expect: ['text/plain'] },
    { ext: 'vsd', expect: ['application/vnd.visio'] },
    { ext: 'wav', expect: ['audio/wav'] },
    { ext: 'weba', expect: ['audio/webm'] },
    { ext: 'webm', expect: ['video/webm'] },
    { ext: 'webp', expect: ['image/webp'] },
    { ext: 'woff', expect: ['font/woff'] },
    { ext: 'woff2', expect: ['font/woff2'] },
    { ext: 'xhtml', expect: ['application/xhtml+xml'] },
    { ext: 'xls', expect: ['application/vnd.ms-excel'] },
    { ext: 'xlsx', expect: ['application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'] },
    { ext: 'xml', expect: ['application/xml'] },
    { ext: 'xul', expect: ['application/vnd.mozilla.xul+xml'] },
    { ext: 'zip', expect: ['application/zip'] },
    { ext: '3gp', expect: ['video/3gpp'] },
    { ext: '3g2', expect: ['video/3gpp2'] },
    { ext: '7z', expect: ['application/x-7z-compressed'] }
  ]

  for (const { ext, expect } of tests) {
    const results = await mime.lookup(ext)
    t.equal(results.length, expect.length)
    for (const result of results) {
      expect.includes(result)
    }
  }
})
