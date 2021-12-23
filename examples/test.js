client.write = async (data, encoding, cb) => {
  const params = {
    clientId: client.clientId,
    data: data
  }

  if (({}).toString.call(data).includes('Uint8Array')) {
    data = btoa(String.fromCharCode.apply(null, data))
    params.type = 'Uint8Array'
  } else {
    params.type = 'string'
  }

  const { err, data } = await window._ipc.send('tcpSend', params)

  if (err && cb) return cb(err)
  if (err) return client.emit('error', err)
  if (cb) return cb(null, data)

  return data
}
