import { dirname, join } from 'path';
import system from '@socketsupply/op-node';

async function main () {
  await system.show({ window: 0 });

  const screen = await system.getScreenSize();

  await system.setSize({
    window: 0,
    height: Math.min(900, screen.height * 0.80),
    width: Math.min(1440, screen.width * 0.80),
  });

  await system.setTitle({
    window: 0,
    value: 'React App',
  });

  const resourcesDirectory = dirname(process.argv[1]);
  const file = join(resourcesDirectory, 'index.html');
  await system.navigate({ window: 0, value: `file://${file}` });

  let counter = 0;
  
  function increaseCounterAndSendMessage(type) {
    counter += 1;
    system.send({
      window: 0,
      event: 'counter increase',
      value: { counter,type },
    });
    return counter;
  }

  system.receive = async (command, value) => {
    if (command === 'send' && value === 'increase counter') {
      return increaseCounterAndSendMessage('click');
    }
  };

  setInterval(() => {
    increaseCounterAndSendMessage('timer');
  }, 5000);
}

main()
