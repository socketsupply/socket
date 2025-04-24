![image](https://github.com/socketsupply/socket/assets/136109/93abfcbe-e880-4548-b3e0-dc7e09292ca6)


### Description

Web Developers use `Socket runtime` to create apps for any OS, desktop, and mobile. You can use plain old HTML, CSS, and JavaScript, as well as your favorite front-end libraries like Next.js, React, Svelte, or Vue.  

The `Socket runtime CLI` outputs hybrid native-web apps that combine your code with the runtime. Your code is rendered using the OS's native "WebView" component. Platform features are implemented natively and made available to the JavaScript environment in a way that is secure and fully sandboxed on every platform. Native APIs like Bluetooth and UDP make local-first and peer-to-peer software design patterns as first class considerations.


### Features

* Any backend &mdash; Business logic can be written in any language, Python, Rust, Node.js, etc. The backend is even completely optional.
* Any frontend &mdash; Use your favorite frontend framework to create your UIs: React, Svelte, Vue, and more.
* Batteries Included &mdash; Native Add-ons are supported, but we ship everything you need for the majority of use cases.
* Local-first &mdash; A full-featured, familiar File system API, native add-ons and full cross platform support for Bluetooth.
* Not just Cloud &mdash; P2P helps you reliably move work out of the cloud, beyond the edge, and onto the devices that can communicate directly with each other.
* Maintainable &mdash; Socket runtime has Zero external dependencies, and a smaller code base than any other competing project.
* Lean & Fast &mdash; Socket runtime has a smaller memory footprint and creates smaller binaries than any other competing project.


### Compatibility Matrix

Socket Supports both ESM and CommonJS

> [!NOTE]
> Socket supports many of the Node.js APIs. It is **NOT** a drop in replacement for Node.js, it most likely wont ever be since Socket is for building software and node.js is for building servers. Below is a high level overview of fully or partially supported APIs and modules.

| Module              | Node.js       | Socket   |
| -----------------   | ----------    | -------- |
| assert              | √             | ⏱        |
| async_hooks         | √             | √        |
| buffer              | √             | √        |
| child_process       | √             | √        |
| cluster             | √             | §        |
| console             | √             | √        |
| crypto              | √             | √ \*     |
| dgram (udp)         | √             | √        |
| diagnostics_channel | √             | √        |
| dns                 | √             | √        |
| encoding            | √             | √        |
| events              | √             | √        |
| fetch               | √             | √        |
| fs                  | √             | √        |
| fs/promises         | √             | √        |
| http                | √             | √        |
| http2               | √             | √        |
| https               | √             | √        |
| inspector           | √             | ⏱        |
| module              | √             | √        |
| net                 | √             | ⏱        |
| os                  | √             | √        |
| path                | √             | √        |
| process             | √             | √        |
| streams             | √             | √        |
| string_decoder      | √             | √        |
| test                | √             | √        |
| timers              | √             | √        |
| tls                 | √             | ⏱        |
| tty                 | √             | √        |
| URL                 | √             | √        |
| uuid                | √             | ⏱ ☀︎      |
| vm                  | √             | √        |
| worker_threads      | √             | √        |

| Symbol | Meaning                                                                     |
| :----: | :-------------------------------------------------------------------------- |
| ⏱      | Planned support                                                             |
| §      | Not Relevant or not necessary since socket doesn't require high-concurrency |
| \*     | Supported but Works differently; may be refactored to match the nodejs API  |
| ☀︎      | Use `crypto.randomUUID()` instead                                           |


### FAQ

Check the FAQs on our [Website](https://socketsupply.co/guides/#faq) to learn more.


### Building your first Socket app!

`Create Socket App` is similar to React's `Create React App`, we provide a few basic boilerplates and some strong opinions so you can get coding on a production-quality app as quickly as possible.  
Please check [create-socket-app Repo](https://github.com/socketsupply/create-socket-app) to get started and to learn more.  
You can also check our `Examples` in the [Examples Repo](https://github.com/socketsupply/socket-examples).  
§§§


### Development

If you are developing socket, and you want your apps to point to your dev branch.


#### Step 1

```bash
cd ~/projects/socket            # navigate into the location where you cloned this repo
./bin/uninstall.sh              # remove existing installation
npm rm -g @socketsupply/socket  # remove any global links or prior global npm installs
npm run relink                  # this will call `./bin/publish-npm-modules.sh --link` (accepts NO_ANDROID=1, NO_IOS=1, and DEBUG=1)
```

#### Step 2

```bash
cd ~/projects/<myapp>           # navigate into your project (replacing <myapp> with whatever it's actually called
npm link @socketsupply/socket   # link socket and you'll be ready to go.
```


### Testing

`Socket` provides a built-in `test runner` similar to `node:test` which outputs the test results in [TAP](https://testanything.org/) format.
 You can also check [`test/`](test/) for the unit and integration test suite.


### Contributing

We welcome contributions from everyone! Please check our [Contribution Guide](CONTRIBUTING.md) to learn more.
Don't hesitate to stop by [Discord](https://discord.com/invite/YPV32gKCsH) and ask the team about your issue and if someone is already working on it.  
Please connect with any current project contributors: [@heapwolf][0], [@jwerle][1], and [@chicoxyzzy][2] if you want to contribute to the `Socket Runtime` project itself.  
Thank you for your interest in reporting/fixing issues and contributing to `Socket`!  

[0]:https://github.com/heapwolf
[1]:https://github.com/jwerle
[2]:https://github.com/chicoxyzzy

