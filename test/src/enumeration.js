import Enumeration from 'socket:enumeration'
import test from 'socket:test'

test('Enumeration.from(values)', (t) => {
  const abc = Enumeration.from(['a', 'b', 'c'])
  t.ok('a' in abc, 'a in abc')
  t.ok('b' in abc, 'b in abc')
  t.ok('c' in abc, 'c in abc')
})
