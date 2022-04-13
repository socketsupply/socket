import React, { useEffect, useState } from 'react';
import './App.css';

function useEvent(event, handler, passive = false) {
  useEffect(() => {
    window.addEventListener(event, handler, passive);
    return function cleanup() {
      window.removeEventListener(event, handler);
    }
  });
}

function App() {
  const [messageCounter, setMessageCounter] = useState(0)
  const [timerCounter, setTimerCounter] = useState(0)
  const [clickCounter, setClickCounter] = useState(0)
  useEvent('counter increase', (e) => {
    setMessageCounter(e.detail.counter)
    switch (e.detail.type) {
      case 'timer': {
        setTimerCounter(timerCounter + 1)
        break;
      }
      case 'click': {
        setClickCounter(clickCounter + 1)
        break;
      }
    }
  })
  return (
    <>
      <div>Messages sent from the main process: {messageCounter} ({clickCounter} clicks, {timerCounter} automatic (once per 5 seconds))</div>
      <button className="button-increase" onClick={async () => {
        const result = await window.system.send('increase counter')
        console.log(`Result: ${result}`)
      }}>Increase counter from the render process</button>
    </>
  );
}

export default App;
