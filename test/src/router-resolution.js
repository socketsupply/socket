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
      redirectBodyTest: "<meta http-equiv=\"refresh\" content=\"0; url='/router-resolution/a-conflict-index/'\" />"
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
      redirectBodyTest: "<meta http-equiv=\"refresh\" content=\"0; url='/router-resolution/an-index-file/'\" />"
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
    const response = await fetch(requestUrl)
    const responseBody = (await response.text()).trim()

    t.ok(response.ok, `response is ok for ${testCase.url}`)
    t.ok(response.status, `response status is 200 for ${testCase.url}`)

    if (testCase.redirect) {
      t.equal(responseBody, testCase.redirectBodyTest, `Redirect response body matches ${testCase.redirectBodyTest}`)
      const extractedRedirectURL = extractUrl(responseBody)

      const redirectResponse = await fetch(extractedRedirectURL)
      const redirectResponseBody = (await redirectResponse.text()).trim()
      t.equal(redirectResponseBody, testCase.bodyTest, `Redirect response body matches ${testCase.bodyTest}`)
    } else {
      t.equal(responseBody, testCase.bodyTest, `response body matches ${testCase.bodyTest}`)
    }
  }
})

function extractUrl (content) {
  const regex = /url\s*=\s*(["'])([^"']+)\1/i
  const match = content.match(regex)
  return match ? match[2] : null
}
