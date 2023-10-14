import test from 'socket:test'
import URL from 'socket:url'

const dirname = URL.resolve(import.meta.url, './router-resolution')

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
      redirectBodyTest: ''
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
      redirectBodyTest: ''
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

    const relPath = response.url.split(dirname)[1] || response.url.split('/router-resolution')[1]

    t.ok(response.ok, `response is ok for ${testCase.url}`)
    t.ok(response.status, `response status is 200 for ${testCase.url}`)
    t.equal(response.url, `${dirname}${testCase.expectedRelPath}`, `response url is fully qualified for ${testCase.url}`)
    // t.equal(testCase.expectedRelPath, relPath, `Relpath matchs ${testCase.expectedRelPath}`)

    if (testCase.redirect) {
      t.equal(testCase.redirectBodyTest, responseBody, `response body matches ${testCase.redirectBodyTest}`)
      // TODO: resolve the redirect URL and make asserts
    } else {
      t.equal(testCase.bodyTest, responseBody, `response body matches ${testCase.bodyTest}`)
    }
  }
})
