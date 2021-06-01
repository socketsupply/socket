constexpr auto gPreload = R"JS(

document.addEventListener('DOMContentLoaded', () => {
  window.external.invoke('ipc;0;ready;true');
});

const IPC = window._ipc = { nextSeq: 1 };

window._ipc.resolve = msg => {
  const data = msg.trim().split(';');
  const internal = data[0] === 'internal';
  const status = Number(data[1]);
  const seq = Number(data[2]);
  const method = status === 0 ? 'resolve' : 'reject';
  const value = internal ? data[3] : JSON.parse(decodeURIComponent(data[3]));

  if (!window._ipc[seq] || !window._ipc[seq][method]) return
  window._ipc[seq][method](value);
  window._ipc[seq] = undefined;
}

window._ipc.send = (name, value) => {
  const seq = window._ipc.nextSeq++

  const promise = new Promise((resolve, reject) => {
    window._ipc[seq] = {
      resolve: resolve,
      reject: reject,
    }
  })

  let encoded

  if (['setTitle', 'openExternal'].includes(name)) {
    encoded = value || ' '
  } else if (name === 'contextMenu') {
    encoded = Object
      .entries(value)
      .flatMap(o => o.join(':'))
      .join('_')
  } else {
    try {
      encoded = encodeURIComponent(JSON.stringify(value))
    } catch (err) {
      return Promise.reject(err.message)
    }
  }

  window.external.invoke(`ipc;${seq};${name};${encoded}`)
  return promise
}
)JS";
