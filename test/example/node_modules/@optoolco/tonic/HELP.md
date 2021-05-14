# TROUBLE SHOOTING

### Class Name Mangling

If you are using Uglify (or something similar), it will mangle your class names.
To fix this, just pass the `keep_fnames` option babel-minify has something
similar.

```js
new UglifyJsPlugin({
  uglifyOptions: {
    keep_fnames: true
  },
  extractComments: true,
  parallel: true
})
```

## Webpack 4+ Mangling Error

If you get an error in the JS console around mangling it can likely be fixed. With Webpack 4+, minimizers, such as Uglify which is now bundled, are managed via [`optimization.minimizer`][0]. Setting Uglify settings via `plugins: {}` will probably not work as needed/expected. The following should work.

```js
...
optimization: {
  minimizer: [
    new UglifyJsPlugin({
      uglifyOptions: {
        keep_fnames: true
      },
      extractComments: true,
      parallel: true
    })
  ]
 }
 ...
```

## Babel Transpiler Issues

If you see a console error log similar to `class constructors must be invoked with |new|` then read on...

Built-in classes such as Date, Array, DOM etc cannot be properly subclassed due
to limitations in ES5. This babel plugin will usually fix this problem.

### Babel < 7

```js
{
  test: /\.js$/,
  exclude: /node_modules/,
  loader: 'babel-loader',
  query: {
    presets: [['env', { exclude: ['transform-es2015-classes'] }]]
  }
}
```

### Babel 7+

For Babel 7+ the syntax is slightly different. You'll need to use the new `@babel...` package name and provide some limitation on browser compatability. As per [Babel Docs][1] the following will net you support for browsers with >0.25% market share while also ignoring browsers that no longer receive security updates.

```js
  {
    test: /\.js$/,
    exclude: /node_modules/,
    loader: 'babel-loader',
    query: {
      presets: [
        ['@babel/preset-env', {
          exclude: [ 'transform-classes'],
          useBuiltIns: "entry" 
        }]
      ]
    }
  }
```

[0]: https://webpack.js.org/configuration/optimization/#optimization-minimizer
[1]: https://babeljs.io/docs/en/babel-preset-env#browserslist-integration
