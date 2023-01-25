import system from '@socketsupply/ssc-node'

system.receive = async (command, value) => {
  if (command === 'process.write') {
    await system.send({
      window: 0,
      event: 'character.backend',
      value: { character: { firstname: 'Summer', secondname: 'Smith' } }
    })
  }
};

(async () => {
  await system.send({
    window: 0,
    event: 'backend:ready'
  })
  await system.send({
    window: 0,
    event: 'character',
    value: { character: { firstname: 'Morty', secondname: 'Smith' } }
  })
})()
