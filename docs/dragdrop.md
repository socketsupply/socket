# EVENTS

A limiation of "native drag and drop" is support for dragging *out* more than
one file, and providing useful meta data on dragging files *in*. This API fixes
both these problems.

## `drag`

Fired while dragging. Provides the pointer's `{x,y}` coordinates so you can
interact with elements as you're dragging. The event only fires if `mousedown`
occurs on an element that defines the `data-src` property (a string of absolute
local file paths or urls separated by `;`). Includes the `count` for how many
files are being dragged.

```js
window.addEventListener('drag', e => {
  const { x, y, count } = e.details
  const el = document.elementFromPoint(x, y)
})
```

## `dragend`

Fired when the user stops dragging. This is useful if you set up state in `drag`
so that while moving the mouse elements can change appearance.

```js
window.addEventListener('dragend', e => {
  // cleanup, reset state, etc.
});
```

## `dropout`

Fired when a drag event leaves the app. The drag must be started from an element
that defines the `data-src` property. One `dropout` event will fire for each
item in the `data-src` property. The `dest` value will be the path of the tmp
file that should be written to.

```html
<div data-src="https://foo.com/bar.png;/tmp/foo.txt">dragme</div>
```

```js
window.addEventListener('dropout', e => {
  const { src, dest } = e.details
})
```

## `dropin`

Fired when items are dropped *into* the app from somewhere else. Provides the
`{x,y}` coordinates and `src` for the absolute local path of the item being
dropped-in. This event may fire multiple times if multiple files are dropped.

```js
window.addEventListener('dropin', e => {
  const { x, y, src } = e.details
})
```
