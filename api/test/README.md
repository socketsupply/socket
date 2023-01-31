# tapzero

Zero dependency test framework

## Source code

The implementation is <250 loc, (<500 with comments) ( https://github.com/Raynos/tapzero/blob/master/index.js ) and very readable.

## Migrating from tape

```js
const test = require('tape')
// Tapzero exports an object with a test function property.
const test = require('tapzero').test
```

```js
tape('my test', (t) => {
  t.equal(2, 2, 'ok')
  t.end()
})
// Auto ending behavior on function completion
tapzero('my test', (t) => {
  t.equal(2, 2, 'ok')
  // t.end() does not exist.
})
```

```js
// tapzero "auto" ends async tests when the async function completes
tapzero('my cb test', async (t) => {
  await new Promise((resolve) => {
    t.equal(2, 2, 'ok')
    setTimeout(() => {
      // instead of calling t.end(), resolve a promise
      resolve()
    }, 10)
  })
})
```

```js
tape('my test', (t) => {
  t.equals(2, 2)
  t.is(2, 2)
  t.isEqual(2, 2)
})
tapzero('my test', (t) => {
  // tapzero does not implement any aliases, very small surface area.
  t.equal(2, 2)
  t.equal(2, 2)
  t.equal(2, 2)
})
```

## Motivation

Small library, zero dependencies

```
$ package-size ./build/src/index.js zora baretest,assert qunit tape jasmine mocha

  package                      size       minified   gzipped
  ./build/src/index.js         8.97 KB    3.92 KB    1.53 KB
  zora@3.1.8                   32.44 KB   11.65 KB   4.08 KB
  baretest@1.0.0,assert@2.0.0  51.61 KB   16.48 KB   5.82 KB
  qunit@2.9.3                  195.83 KB  62.04 KB   20.38 KB
  tape@4.13.0                  304.71 KB  101.46 KB  28.8 KB
  jasmine@3.5.0                413.61 KB  145.2 KB   41.07 KB
  mocha@7.0.1                  811.55 KB  273.07 KB  91.61 KB

```

Small library, small install size.

|        |  tapzero  |  baretest  |  zora  |  pta  |  tape  |
|--------|:---------:|:----------:|:------:|:-----:|:------:|
|pkg size|  [![tapzero](https://packagephobia.now.sh/badge?p=tapzero@0.1.1)](https://packagephobia.now.sh/result?p=tapzero)  |  [![baretest](https://packagephobia.now.sh/badge?p=baretest)](https://packagephobia.now.sh/result?p=baretest)  |  [![zora](https://packagephobia.now.sh/badge?p=zora)](https://packagephobia.now.sh/result?p=zora)  |  [![pta](https://packagephobia.now.sh/badge?p=pta)](https://packagephobia.now.sh/result?p=pta)  |  [![tape](https://packagephobia.now.sh/badge?p=tape)](https://packagephobia.now.sh/result?p=tape)  |
|Min.js size|  [![tapzero](https://badgen.net/bundlephobia/min/tapzero)](https://bundlephobia.com/result?p=tapzero)  |  [![baretest](https://badgen.net/bundlephobia/min/baretest)](https://bundlephobia.com/result?p=baretest)  |  [![zora](https://badgen.net/bundlephobia/min/zora)](https://bundlephobia.com/result?p=zora)  |  [![pta](https://badgen.net/bundlephobia/min/pta)](https://bundlephobia.com/result?p=pta)  |  [![tape](https://badgen.net/bundlephobia/min/tape)](https://bundlephobia.com/result?p=tape)  |
|dep count|  [![tapzero](https://badgen.net/badge/dependencies/0/green)](https://www.npmjs.com/package/tapzero)  |  [![baretest](https://badgen.net/badge/dependencies/1/green)](https://www.npmjs.com/package/baretest)  |  [![zora](https://badgen.net/badge/dependencies/0/green)](https://www.npmjs.com/package/zora)  |  [![pta](https://badgen.net/badge/dependencies/23/orange)](https://www.npmjs.com/package/pta)  |  [![tape](https://badgen.net/badge/dependencies/44/orange)](https://www.npmjs.com/package/tape)  |

|        |  Mocha  |  Ava  |  Jest  |  tap  |
|:------:|:-------:|:-----:|:------:|:-----:|
|pkg size|  [![mocha](https://packagephobia.now.sh/badge?p=mocha)](https://packagephobia.now.sh/result?p=mocha)  |  [![ava](https://packagephobia.now.sh/badge?p=ava)](https://packagephobia.now.sh/result?p=ava) |  [![jest](https://packagephobia.now.sh/badge?p=jest)](https://packagephobia.now.sh/result?p=jest) |  [![tap](https://packagephobia.now.sh/badge?p=tap)](https://packagephobia.now.sh/result?p=tap) |
|Min.js size|  [![mocha](https://badgen.net/bundlephobia/min/mocha)](https://bundlephobia.com/result?p=mocha)  |  [![ava](https://badgen.net/bundlephobia/min/ava)](https://bundlephobia.com/result?p=ava)  |  [![jest](https://badgen.net/bundlephobia/min/jest)](https://bundlephobia.com/result?p=jest)  |  [![tap](https://badgen.net/bundlephobia/min/tap)](https://bundlephobia.com/result?p=tap)  |
|dep count|  [![mocha](https://badgen.net/badge/dependencies/104/red)](https://www.npmjs.com/package/mocha)  |  [![ava](https://badgen.net/badge/dependencies/300/red)](https://www.npmjs.com/package/ava)  |  [![jest](https://badgen.net/badge/dependencies/799/red)](https://www.npmjs.com/package/jest)  |  [![tap](https://badgen.net/badge/dependencies/390/red)](https://www.npmjs.com/package/tap)  |

## Docs

```js
const test = require('tapzero').test
```

### `test(name, [fn])`

Run a single named test case. The `fn` will be called with the `t` test object.

Tests run one at a time and complete when the `fn` completes, the `fn` can be async.

### `test.only(name, fn)`

Like `test(name, fn)` except if you use `.only` this is the only test case that will run for the entire process, all other test cases using tape will be ignored.

### `test.skip(name, [fn])`

Creates a test case that will be skipped

## Harness docs

```js
const testHarness = require('tapzero/harness')
```

See [HARNESS.md](./HARNESS.md)
