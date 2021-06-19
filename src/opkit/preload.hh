constexpr auto gPreload = R"JS(

document.addEventListener('DOMContentLoaded', () => {
  window.external.invoke('ipc;0;ready;0');
});

const IPC = window._ipc = { nextSeq: 1 };

window._ipc.resolve = msg => {
  const data = msg.trim().split(';');
  const status = Number(data[1]);
  const seq = Number(data[2]);
  const method = status === 0 ? 'resolve' : 'reject';
  let value = data[3]

  try {
    value = JSON.parse(decodeURIComponent(value));
  } catch (err) {
    console.warn(err.message)
  }

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

  let encoded = value || ' '

  if (name === 'contextMenu') {
    encoded = Object
      .entries(value)
      .flatMap(o => o.join(':'))
      .join('_')
  } else if (typeof value === 'object') {
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

