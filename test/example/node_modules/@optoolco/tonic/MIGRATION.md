# Migration from v12 to v13

We enabled escaping on strings returned by `render()`

If your `render()` method returns a `'string'` or uses a plain
template tag like "return \`&lt;div>&lt;/div>\`" then this will
be escaped and rendered as text content.

You will have to update your `render() {}` methods to use
the `this.html` method for rendering.

We've renamed the method `Tonic.raw` => `Tonic.unsafeRawString`.
This makes it clear that the method is unsafe and exposes your
application to security issues.

We strongly recommend replacing all uses of `Tonic.raw` with
`this.html` instead. For the use case of repeating templates
you can pass an array of `TonicTemplate` objects returned
from `this.html` into another `this.html`.

If you truly do need an `unsafeRawString` that is assigned as
raw HTML we recommend that you use `Tonic.escape()` to build
that string and review it very carefully, add a comment explaining
it too.

We renamed a field from `isTonicRaw` to `isTonicTemplate` on
the `TonicRaw` / `TonicTemplate` class. This is unlikely to break
your app.

# Migrating from v11 to v12

We made a breaking change where the `id` attribute is mandatory
for any tonic components that access `this.id` or `this.state`.

Previously `id` attribute was not mandatory but the component
was in a half broken state since the `this.state` was not
stored upon re-render.

Now tonic warns you that the component is stateful and that the
`id` is mandatory as its the primary key by which we store the
`state` and get the previous state upon re-render.

Basically `this.state` is semi-broken without an `id` attribute.

You will get exceptions and you will have to refactor; for example

```js
-            <app-search-filter-combo>
+            <app-search-filter-combo id="app-search-filter-combo">

   render () {
-    return `
+    return this.html`
       <header>New Folder</header>
       <main>
         <tonic-input
+          id="new-folder"
           name="new-folder"
           label="Name"
```

Basically just add the `id` attributes.

# Migrating from v10 to v11

The implementation of HTML escaping changed between v10 and v11
of `@optoolco/tonic`.

The main takeaway is that v10 had a potential XSS injection as
we only escaped strings that exist on `this.props.someKey`,
we've now changed the implementation to sanitize all strings
that are passed into ``this.html`<div>${someStr}<div>`;``.

This breaks some existing patterns that are common in applications
like the following

```js
class Comp extends Tonic {
  renderLabel () {
    return `<label>${this.props.label}</label>`
  }

  render () {
    return this.html`
      <div>
        <header>Some header</header>
        ${this.renderLabel()}
      </div>
    `
  }
}
```

In this case the HTML returned from `this.renderLabel()` is now
being escaped which is probably not what you meant.

You will have to patch the code to use `this.html` for the
implementation of `renderLabel()` like

```js
  renderLabel () {
    return this.html`<label>${this.props.label}</label>`
  }
```

Or to call `Tonic.raw()` manually like

```js
  render () {
    return this.html`
      <div>
        <header>Some header</header>
        ${Tonic.raw(this.renderLabel())}
      </div>
    `
  }
```

If you want to quickly find all occurences of the above patterns
you can run the following git grep on your codebase.

```sh
git grep -C10 '${' | grep ')}'
```

The fix is to add `this.html` calls in various places.

We have updated `@optoolco/components` and you will have to
update to version `7.4.0` as well

```sh
npm install @optoolco/components@^7.4.0 -ES
```

There are other situations in which the increased escaping from
`Tonic.escape()` like for example escaping the `"` character if
you dynamically generate optional attributes

Like:

```js
class Icon extends Tonic {
  render () {
    return this.html`<svg ${tabAttr} styles="icon">
      <use
        width="${size}"
        ${fill ? `fill="${fill}" color="${fill}"` : ''}
        height="${size}">
    </svg>`
  }
}
```

In the above example we do ``fill ? `fill="${fill}"` : ''`` which
leads to `"` getting escaped to `&quot;` and leads to the value
of `use.getAttribute('fill')` to be `"${fill}"` instead of `${fill}`

Here is a regex you can use to find the one-liner use cases.

```
git grep -E '`(.+)="'
```

When building dynamic attribute lists `Tonic` has a spread feature
in the `this.html()` function you can use instead to make it easier.

For example, you can refactor the above `Icon` class to:

```js
class Icon extends Tonic {
  render () {
    return this.html`<svg ${tabAttr} styles="icon">
      <use ...${{
        width: size,
        fill,
        color: fill,
        height: size
      }}>
    </svg>`
  }
}
```

Here we use `...${{ ... }}` to expand an object of attributes to
attribute key value pairs in the HTML. You can also pull out the attrs
into a reference if you prefer, like:

```js
class Icon extends Tonic {
  render () {
    const useAttrs = {
      width: size,
      fill,
      color: fill,
      height: size
    }
    return this.html`<svg ${tabAttr} styles="icon">
      <use ...${useAttrs}>
    </svg>`
  }
}
```
