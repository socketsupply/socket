import test from 'socket:test'
import URL from 'socket:url'
import os from 'socket:os'

const basePath = 'router-resolution'
const dirname = URL.resolve(import.meta.url, basePath)
const isWindows = os.platform() === 'win32'

test('router-resolution', async (t) => {
  const response = await fetch(dirname + '/invalid')
  t.equal(response.status, 404, '404 on invalid route')

  const cases = [
    {
      url: '/',
      bodyTest: '/index.html',
      redirect: false
    },
    {
      url: '/index.html',
      bodyTest: '/index.html',
      redirect: false
    },
    {
      url: '/a-conflict-index',
      bodyTest: '/a-conflict-index/index.html',
      redirect: true,
      redirectBodyTest: "<meta http-equiv=\"refresh\" content=\"0; url='/router-resolution/a-conflict-index/'\" />",
      redirectUrl: '/router-resolution/a-conflict-index/'
    },
    {
      url: '/another-file',
      bodyTest: '/another-file.html',
      redirect: false
    },
    {
      url: '/another-file.html',
      bodyTest: '/another-file.html',
      redirect: false
    },
    {
      url: '/an-index-file/',
      bodyTest: '/an-index-file/index.html',
      redirect: false
    },
    {
      url: '/an-index-file',
      bodyTest: '/an-index-file/index.html',
      redirect: true,
      redirectBodyTest: "<meta http-equiv=\"refresh\" content=\"0; url='/router-resolution/an-index-file/'\" />",
      redirectUrl: '/router-resolution/an-index-file/'
    },
    {
      url: '/an-index-file/a-html-file',
      bodyTest: '/an-index-file/a-html-file.html',
      redirect: false
    },
    {
      url: '/an-index-file/a-html-file.html',
      bodyTest: '/an-index-file/a-html-file.html',
      redirect: false
    }
  ]

  for (const testCase of cases) {
    const requestUrl = dirname + testCase.url
    const response = await fetch(requestUrl, { redirect: 'manual' })
    const responseBody = (await response.text()).trim()

    t.ok(response.ok, `response is ok for ${testCase.url}`)
    t.ok(response.status, `response status is 200 for ${testCase.url}`)

    if (testCase.redirect) {
      if (isWindows) {
        const extractedRedirectURL = extractUrl(response, responseBody)
        t.equal(extractedRedirectURL, testCase.redirectUrl, `Redirect response url matches ${testCase.redirectUrl} (windows)`)
      } else {
        t.equal(responseBody, testCase.redirectBodyTest, `Redirect response matches ${testCase.redirectBodyTest}`)
        const extractedRedirectURL = extractUrl(response, responseBody)
        const redirectResponse = await fetch(extractedRedirectURL)
        const redirectResponseBody = (await redirectResponse.text()).trim()
        t.ok(redirectResponseBody.includes(testCase.bodyTest), `Redirect response body includes ${testCase.bodyTest}`)
      }
    } else {
      t.ok(responseBody.includes(testCase.bodyTest), `response body includes ${testCase.bodyTest}`)
    }
  }
})

function extractUrl (response, content) {
  const location = response.headers.get('content-location') || response.headers.get('location')
  const regex = /url\s*=\s*(["'])([^"']+)\1/i
  const match = content.match(regex)
  if (match) {
    return new URL(match[2], dirname).pathname
  } else if (response.url) {
    return new URL(response.url, dirname).pathname
  } else if (location) {
    return new URL(location, dirname).pathname
  } else if (content) {
    try {
      return new URL(`.${content}`, dirname).pathname
    } catch {}
  }

  return null
}
