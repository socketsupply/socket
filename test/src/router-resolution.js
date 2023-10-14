import test from 'socket:test'
import URL from 'socket:url'

const basePath = 'router-resolution'
const dirname = URL.resolve(import.meta.url, basePath)

test('router-resolution', async (t) => {
  const response = await fetch(dirname + '/invalid')
  t.equal(response.status, 404, '404 on invalid route')

  const cases = [
    {
      url: '/',
      bodyTest: '/index.html',
      expectedRelPath: '/',
      redirect: false
    },
    {
      url: '/index.html',
      bodyTest: '/index.html',
      expectedRelPath: '/index.html',
      redirect: false
    },
    {
      url: '/a-conflict-index',
      bodyTest: '/a-conflict-index/index.html',
      expectedRelPath: '/a-conflict-index/',
      redirect: true,
      redirectBodyTest: "<meta http-equiv=\"refresh\" content=\"0; url='/router-resolution/a-conflict-index/'\" />"
    },
    {
      url: '/another-file',
      bodyTest: '/another-file.html',
      expectedRelPath: '/another-file',
      redirect: false
    },
    {
      url: '/another-file.html',
      bodyTest: '/another-file.html',
      expectedRelPath: '/another-file.html',
      redirect: false
    },
    {
      url: '/an-index-file/',
      bodyTest: '/an-index-file/index.html',
      expectedRelPath: '/an-index-file/',
      redirect: false
    },
    {
      url: '/an-index-file',
      bodyTest: '/an-index-file/index.html',
      expectedRelPath: '/an-index-file/',
      redirect: true,
      redirectBodyTest: "<meta http-equiv=\"refresh\" content=\"0; url='/router-resolution/an-index-file/'\" />"
    },
    {
      url: '/an-index-file/a-html-file',
      bodyTest: '/an-index-file/a-html-file.html',
      expectedRelPath: '/an-index-file/a-html-file',
      redirect: false
    },
    {
      url: '/an-index-file/a-html-file.html',
      bodyTest: '/an-index-file/a-html-file.html',
      expectedRelPath: '/an-index-file/a-html-file.html',
      redirect: false
    }
  ]

  for (const testCase of cases) {
    const requestUrl = dirname + testCase.url
    const response = await fetch(requestUrl)
    const responseBody = (await response.text()).trim()

    t.ok(response.ok, `response is ok for ${testCase.url}`)
    t.ok(response.status, `response status is 200 for ${testCase.url}`)
    t.ok(response.url.startsWith(dirname), `response url is fully qualified for ${testCase.url} (actual: ${response.url})`)

    if (testCase.redirect) {
      t.equal(responseBody, testCase.redirectBodyTest, `Redirect response body matches ${testCase.redirectBodyTest}`)
      const extractedRedirectURL = extractUrl(responseBody)
      const fqdRedirectURL = URL.resolve(dirname, extractedRedirectURL)
      t.equal(fqdRedirectURL, `${dirname}${testCase.expectedRelPath}`, `Redirect to ${dirname}${testCase.expectedRelPath}`)

      const redirectResponse = await fetch(fqdRedirectURL)
      const redirectResponseBody = (await redirectResponse.text()).trim()
      t.equal(redirectResponseBody, testCase.bodyTest, `Redirect response body matches ${testCase.bodyTest}`)
      const redirectRelPath = removeStartingSubstring(redirectResponse.url, dirname)
      t.equal(redirectRelPath, testCase.expectedRelPath, `Redirect Relpath matchs ${testCase.expectedRelPath}`)
    } else {
      t.equal(responseBody, testCase.bodyTest, `response body matches ${testCase.bodyTest}`)
      const relPath = removeStartingSubstring(response.url, dirname)
      t.equal(relPath, testCase.expectedRelPath, `Relpath matchs ${testCase.expectedRelPath}`)
    }
  }
})

function removeStartingSubstring (mainString, startString) {
  if (mainString.startsWith(startString)) {
    return mainString.slice(startString.length)
  }
  return mainString
}

function extractUrl (content) {
  const regex = /url\s*=\s*(["'])([^"']+)\1/i
  const match = content.match(regex)
  return match ? match[2] : null
}
