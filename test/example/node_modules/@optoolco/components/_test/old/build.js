const fs = require('fs')
const path = require('path')

const root = `${__dirname}/../..`

const components = fs
  .readFileSync(path.join(root, 'components.txt'), 'utf8')
  .split('\n')
  .filter(Boolean)

const sections = components
  .map(f => {
    const fileName = path.join(root, f, 'test.html')

    let htmlText
    try {
      htmlText = fs.readFileSync(fileName, 'utf8')
    } catch (err) {
      // Missing file, skip.
      return ''
    }

    return '      ' + htmlText
  })
  .join('\n')

const index = `
  <!DOCTYPE html>
    <head>
      <title>Tonic - Component Based Architecture</title>
      <link href="index.css" rel="stylesheet">
      <link href="theme.css" rel="stylesheet">
      <link href="test.css" rel="stylesheet">
      <link rel="icon" type="image/png" href="/favicon-32x32.png" sizes="32x32">
      <link rel="icon" type="image/png" href="/favicon-96x96.png" sizes="96x96">
      <link rel="icon" type="image/png" href="/favicon-96x96.png" sizes="120x120">
      <link rel="icon" type="image/png" href="/favicon-96x96.png" sizes="128x128">
      <link rel="apple-touch-icon" type="image/png" href="favicon-152x152.png" sizes="152x152">
      <link rel="apple-touch-icon" type="image/png" href="favicon-167x167.png" sizes="167x167">
      <link rel="apple-touch-icon" type="image/png" href="favicon-180x180.png" sizes="180x180">
      <meta http-equiv="Content-Type" charset="utf-8" content="text/html">
      <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1 user-scalable=no">
      <meta name="description" content="Component Based Architecture">
      <meta property="og:site_name", content="Operator Tool Co.">
      <meta property="og:title", content="Tonic Components">
      <meta property="og:description" content="Component Based Architecture">
      <meta property="og:type", content="website">
      <meta property="og:url", content="https://optoolco.github.io/components">
      <meta property="og:image", content="https://optoolco.github.io/components/tonic_preview.png">
      <meta name="twitter:card", content="summary">
      <meta name="twitter:site", content="@optoolco">
      <meta name="twitter:image", content="https://optoolco.github.io/components/tonic_preview.png">
      <meta
        http-equiv="Content-Security-Policy"
        content="
          default-src 'self';
          font-src 'self' https:;
          img-src 'self' http: https: data:;
          style-src 'self' 'unsafe-inline' https:;
          script-src 'self' 'nonce-U29tZSBzdXBlciBzZWNyZXQ=';
          connect-src 'self' https:;">
    </head>
    <body data-page="test" id="test">
      <main>
        <tonic-sprite></tonic-sprite>
        <aside id="test-column">
          <div class="test-summary">
            <div id="passing"><h1 class="value">0</h1><label>Passing</label></div>
            <div id="total"><h1 class="value">0</h1><label>Total</label></div>
            <div id="status"><h1 class="value">--</h1><label>Status</label></div>
            <hr/>
          </div>
          <div id="test-output"></div>
        </aside>
        ${sections}
      </main>
      <script nonce="U29tZSBzdXBlciBzZWNyZXQ=" crossorigin="anonymous" src="test.js"></script>
    </body>
  </html>
`

fs.writeFileSync(path.join(__dirname, '..', '..', 'docs', 'test.html'), index)
