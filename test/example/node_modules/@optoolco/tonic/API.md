# APIs

## METHODS IMPLEMENTED BY THE COMPONENT DEVELOPER

| Name | Description |
| :--- | :--- |
| `render()` | Required, should return a template literal of HTML. There is no need to call this directly, the browser will call it. Can be `sync`, `async` or `async generator`. |
| `stylesheet()` | Optional, Should return a string of css to be added to a `style` tag in the component (ideal for components that use a shadow dom). |
| `static stylesheet()` | Optional, Should return a string of css to be lazily added to a `style` tag in the head (ideal for custom elements with no shadow dom). |
| `styles()` | Optional, Should return an object that represents inline-styles to be applied to the component. Styles are applied by adding a keys from the object to the `styles` attribute of an html tag in the render function, for example `styles="key1 key2"`. Each object's key-value pair are added to the element's style object. |

## STATIC METHODS

| Method | Description |
| :--- | :--- |
| `add(Class, String?)` | Register a class as a new custom-tag and provide options for it. You can pass an optional string that is the HTML tagName for the custom component.|
| `escape(String)` | Escapes HTML characters from a string (based on [he][3]). |
| `unsafeRawString(String)` | Insert raw text in html\`...\`. Be careful with calling `unsafeRawString` on untrusted text like user input as that is an XSS attack vector.
| `match(Node, Selector)` | Match the given node against a selector or any matching parent of the given node. This is useful when trying to locate a node from the actual node that was interacted with. |

## INSTANCE METHODS

| Method | Description |
| :--- | :--- |
| <code>reRender(Object &#124; Function)</code> | Set the properties of a component instance. Can also take a function which will receive the current props as an argument. |
| html\`...\` | Interpolated HTML string (use as a [tagged template][2]). Provides...<br/> 1. Pass object references as properties.<br/> 2. Spread operator (ie `<a ...${object}></a>`) which turns ojbects into html properties.<br/> 3. Automatic string escaping.<br/> 4. Render `NamedNodeMap`, `HTMLElement`, `HTMLCollection`, or `NodeList` as html (ie `<a>${span}</a>`).<br/>|
| `dispatch(String, Object?)` | Dispatch a custom event from the component with an optional detail. A parent component can listen for the event by calling `this.addEventListener(String, this)` and implementing the event handler method. |

## INSTANCE PROPERTIES

| Name | Description |
| :--- | :--- |
| <code>elements</code> | An array of the original child *elements* of the component. |
| <code>nodes</code> | An array of the original child *nodes* of the component. |
| <code>props</code> | An object that contains the properties that were passed to the component. |
| <code>state</code> | A plain-old JSON object that contains the state of the component. |

## "LIFECYCLE" INSTANCE METHODS

| Method | Description |
| :--- | :--- |
| `constructor(object)` | An instance of the element is created or upgraded. Useful for initializing state, setting up event listeners, or creating shadow dom. See the spec for restrictions on what you can do in the constructor. The constructor's arguments must be forwarded by calling `super(object)`. |
| `willConnect()` | Called prior to the element being inserted into the DOM. Useful for updating configuration, state and preparing for the render. |
| `connected()` | Called every time the element is inserted into the DOM. Useful for running setup code, such as fetching resources or rendering. Generally, you should try to delay work until this time. |
| `disconnected()` | Called every time the element is removed from the DOM. Useful for running clean up code. |
| `updated(oldProps)` | Called after reRender() is called. This method is not called on the initial render. |

[1]:https://developers.google.com/web/fundamentals/web-components/customelements
[2]:https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Template_literals
[3]:https://github.com/mathiasbynens/he
