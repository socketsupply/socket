import extension from 'socket:extension'
import process from 'socket:process'
import test from 'socket:test'
import ipc from 'socket:ipc'
import fs from 'socket:fs'

test('extension.load(name) - sqlite3', async (t) => {
  const path = process.cwd() + '/data.db'
  let sqlite3
  let query

  await fs.unlink(path)

  try {
    sqlite3 = await extension.load('sqlite3')
  } catch (err) {
    t.ifError(err)
    return
  }

  var { err, data: db } = await sqlite3.binding.open({ path })
  if (err) return t.ifError(err)

  t.ok(db, 'sqlite3.binding.open result data')
  t.ok(db?.id, 'sqlite3.binding.open result data has id')
  t.equal(db?.path, path, 'sqlite3.binding.open result data has path')

  query = `
    CREATE TABLE IF NOT EXISTS users (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      name NVARCHAR NOT NULL
    );
  `

  var { err } = await sqlite3.binding.exec({ query, id: db.id })
  if (err) return t.ifError(err)

  query = 'DELETE FROM users'

  var { err, data: rows } = await sqlite3.binding.exec({ query, id: db.id })
  if (err) return t.ifError(err)

  query = `
    INSERT INTO users (name)
         VALUES ('jwerle')
              , ('heapwolf')
              , ('chicoxyzzy')
              , ('mribbons')`

  var { err } = await sqlite3.binding.exec({ query, id: db.id })
  if (err) return t.ifError(err)

  query = 'SELECT * FROM users'

  var { err, data: rows } = await sqlite3.binding.exec({ query, id: db.id })
  if (err) return t.ifError(err)

  t.equal(rows[0]?.name, 'jwerle', 'jwerle at index 0')
  t.equal(rows[1]?.name, 'heapwolf', 'heapwolf at index 1')
  t.equal(rows[2]?.name, 'chicoxyzzy', 'chicoxyzzy at index 2')
  t.equal(rows[3]?.name, 'mribbons', 'mribbons at index 3')

  query = `
    DELETE FROM users
          WHERE name = 'jwerle'
  `

  var { err } = await sqlite3.binding.exec({ query, id: db.id })
  if (err) return t.ifError(err)

  query = 'SELECT * FROM users'

  var { err, data: rows } = await sqlite3.binding.exec({ query, id: db.id })
  if (err) return t.ifError(err)

  t.equal(rows[0]?.name, 'heapwolf', 'heapwolf at index 0')
  t.equal(rows[1]?.name, 'chicoxyzzy', 'chicoxyzzy at index 1')
  t.equal(rows[2]?.name, 'mribbons', 'mribbons at index 2')

  query = 'DELETE FROM users'

  var { err, data: rows } = await sqlite3.binding.exec({ query, id: db.id })
  if (err) return t.ifError(err)

  query = 'SELECT * FROM users'

  var { err, data: rows } = await sqlite3.binding.exec({ query, id: db.id })
  if (err) return t.ifError(err)

  t.equal(rows.length, 0, 'all rows deleted')

  var { err } = await sqlite3.binding.close({ query, id: db.id })
  if (err) return t.ifError(err)
  t.pass('db closed')
})
