import { test } from 'socket:test'
import path from 'socket:path'
import os from 'socket:os'

test('path', (t) => {
  const isUnix = os.platform() !== 'win32'
  t.ok(path.posix, 'path.posix exports')
  t.ok(path.win32, 'path.win32 exports')
  const expectedSep = isUnix ? '/' : '\\'
  t.equal(path.sep, expectedSep, 'path.sep is correct')
  const expectedDelimiter = isUnix ? ':' : ';'
  t.equal(path.delimiter, expectedDelimiter, 'path.delimiter is correct')
})

test('path.posix.resolve', (t) => {
  const cwd = path.posix.cwd()
  const dot = path.posix.resolve('.')
  const a = path.posix.resolve('a')
  const ab = path.posix.resolve('a', 'b')
  const abc = path.posix.resolve('a', 'b', 'c')
  const abd = path.posix.resolve('a', 'b', '../b', 'd')
  const a___ = path.posix.resolve('./a', 'b', './c', 'd', '../../..')
  t.equal(cwd, dot, 'path.posix.resolve(.) resolves to cwd')
  t.equal([cwd, 'a'].join('/'), a, 'path.posix.resolve() resolves path with 1 component')
  t.equal([cwd, 'a', 'b'].join('/'), ab, 'path.posix.resolve() resolves path 2 components')
  t.equal([cwd, 'a', 'b', 'c'].join('/'), abc, 'path.posix.resolve() resolves path 3 components')
  t.equal([cwd, 'a', 'b', 'd'].join('/'), abd, 'path.posix.resolve() resolves path 4 components')
  t.equal([cwd, 'a'].join('/'), a___, 'path.posix.resolve() resolves path with 5 component')
})

test('path.posix.join', (t) => {
  t.equal(path.posix.join('a', 'b', 'c'), 'a/b/c', 'join(a, b, c)')
  t.equal(path.posix.join('a', 'b', 'c', '../d'), 'a/b/d', 'join(a, b, c, ../d)')
  t.equal(path.posix.join('a', 'b', 'c', '../d', '../../b'), 'a/b', 'join(a, b, c, ../d, ../../b)')
})

test('path.posix.dirname', (t) => {
  t.equal(path.posix.dirname('a/b/c'), 'a/b', 'a/b')
  t.equal(path.posix.dirname('a/b/c/d.js'), 'a/b/c', 'a/b/c')
  t.equal(path.posix.dirname('/a/b/c/d.js'), '/a/b/c', '/a/b/c')
  t.equal(path.posix.dirname('./a/b/c/d.js'), './a/b/c', './a/b/c')
  t.equal(path.posix.dirname('a/b.js'), 'a', 'a')
  t.equal(path.posix.dirname('/a.js'), '/', '/')
  t.equal(path.posix.dirname('a.js'), '.', '.')
})

test('path.posix.basename', (t) => {
  t.equal(path.posix.basename('a/b/c'), 'c', 'c')
  t.equal(path.posix.basename('a/b/c/d.js'), 'd.js', 'd.js')
  t.equal(path.posix.basename('/a/b/c/d.js'), 'd.js', 'd.js')
  t.equal(path.posix.basename('./a/b/c/d.js'), 'd.js', 'd.js')
  t.equal(path.posix.basename('a/b.js'), 'b.js', 'b.js')
  t.equal(path.posix.basename('/a.js'), 'a.js', 'a.js')
  t.equal(path.posix.basename('a.js'), 'a.js', 'a.js')
})

test('path.posix.extname', (t) => {
  t.equal(path.posix.extname('a/b/c'), '', 'no extension')
  t.equal(path.posix.extname('a/b/c/d.js'), '.js', '.js')
  t.equal(path.posix.extname('/a/b/c/d.js'), '.js', '.js')
  t.equal(path.posix.extname('./a/b/c/d.js'), '.js', '.js')
  t.equal(path.posix.extname('a/b.js'), '.js', '.js')
  t.equal(path.posix.extname('/a.js'), '.js', '.js')
  t.equal(path.posix.extname('a.js'), '.js', '.js')
})

test('path.win32.resolve', (t) => {
  const cwd = path.win32.cwd()
  const dot = path.win32.resolve('.')
  const a = path.win32.resolve('a')
  const ab = path.win32.resolve('a', 'b')
  const abc = path.win32.resolve('a', 'b', 'c')
  const abd = path.win32.resolve('a', 'b', '..\\b', 'd')
  const a___ = path.win32.resolve('.\\a', 'b', '.\\c', 'd', '..\\..\\..')
  t.equal(cwd, dot, 'path.win32.resolve(.) resolves to cwd')
  t.equal([cwd, 'a'].join('\\'), a, 'path.win32.resolve() resolves path with 1 component')
  t.equal([cwd, 'a', 'b'].join('\\'), ab, 'path.win32.resolve() resolves path 2 components')
  t.equal([cwd, 'a', 'b', 'c'].join('\\'), abc, 'path.win32.resolve() resolves path 3 components')
  t.equal([cwd, 'a', 'b', 'd'].join('\\'), abd, 'path.win32.resolve() resolves path 4 components')
  t.equal([cwd, 'a'].join('\\'), a___, 'path.win32.resolve() resolves path with 5 component')
})

test('path.win32.join', (t) => {
  t.equal(path.win32.join('a', 'b', 'c'), 'a\\b\\c', 'join(a, b, c)')
  t.equal(path.win32.join('a', 'b', 'c', '..\\d'), 'a\\b\\d', 'join(a, b, c, ..\\d)')
  t.equal(path.win32.join('a', 'b', 'c', '..\\d', '..\\..\\b'), 'a\\b', 'join(a, b, c, ..\\d, ..\\..\\b)')
  t.equal(path.win32.join('c:\\dir\\', 'file.txt'), `c:\\dir\\file.txt`, `c:\\dir\\, file.txt`)
  t.equal(path.win32.join('c:\\test', '', '', 'hello'), `c:\\test\\hello`, `c:\\test, '', '', hello`)
  t.equal(path.win32.join('c:\\test', '\\', '\\', 'hello'), `c:\\test\\hello`, `c:\\test, '\\', '\\', hello`)
})

test('path.win32.dirname', (t) => {
  t.equal(path.win32.dirname('a\\b\\c'), 'a\\b', 'a\\b')
  t.equal(path.win32.dirname('a\\b\\c\\d.js'), 'a\\b\\c', 'a\\b\\c')
  t.equal(path.win32.dirname('C:\\a\\b\\c\\d.js'), 'C:\\a\\b\\c', 'C:\\a\\b\\c')
  t.equal(path.win32.dirname('\\a\\b\\c\\d.js'), '\\a\\b\\c', '\\a\\b\\c')
  t.equal(path.win32.dirname('z:\\.\\a\\b\\c\\d.js'), 'z:\\.\\a\\b\\c', 'z:\\.\\a\\b\\c')
  t.equal(path.win32.dirname('a\\b.js'), 'a', 'a')
  t.equal(path.win32.dirname('J:\\a.js'), 'J:\\', 'J:\\')
  t.equal(path.win32.dirname('a.js'), '.', '.')
  t.equal(path.win32.dirname('c:\\a\\b\\c'), 'c:\\a\\b', 'c:\\a\\b')
})

test('path.win32.basename', (t) => {
  t.equal(path.win32.basename('c:\\a\\b\\c'), 'c', 'c')
  t.equal(path.win32.basename('z:\\a\\b\\c\\d.js'), 'd.js', 'd.js')
  t.equal(path.win32.basename('\\a\\b\\c\\d.js'), 'd.js', 'd.js')
  t.equal(path.win32.basename('.\\a\\b\\c\\d.js'), 'd.js', 'd.js')
  t.equal(path.win32.basename('a\\b.js'), 'b.js', 'b.js')
  t.equal(path.win32.basename('\\a.js'), 'a.js', 'a.js')
  t.equal(path.win32.basename('a.js'), 'a.js', 'a.js')
})

test('path.win32.extname', (t) => {
  t.equal(path.win32.extname('C:\\a\\b\\c'), '', 'no extension')
  t.equal(path.win32.extname('x:\\a\\b\\c\\d.js'), '.js', '.js')
  t.equal(path.win32.extname('d:\\a\\b\\c\\d.js'), '.js', '.js')
  t.equal(path.win32.extname('.\\a\\b\\c\\d.js'), '.js', '.js')
  t.equal(path.win32.extname('a\\b.js'), '.js', '.js')
  t.equal(path.win32.extname('\\a.js'), '.js', '.js')
  t.equal(path.win32.extname('a.js'), '.js', '.js')
})

test('path.isAbsolute', (t) => {
  t.equal(path.isAbsolute('.'), false, 'path.isAbsolute(.) === false')
  t.equal(path.isAbsolute('./'), false, 'path.isAbsolute(./) === false')
  t.equal(path.isAbsolute('foo/bar'), false, 'path.isAbsolute(foo/bar) === false')
  t.equal(path.isAbsolute('foo\\bar'), false, 'path.isAbsolute(foo\\bar) === false')
  t.equal(path.isAbsolute('.\\foo\\bar'), false, 'path.isAbsolute(.\\foo\\bar) === false')

  t.equal(path.isAbsolute('/'), true, 'path.isAbsolute(/) === true')
  t.equal(path.isAbsolute('/foo/bar'), true, 'path.isAbsolute(/foo/bar) === true')
  t.equal(path.isAbsolute('\\'), true, 'path.isAbsolute(\\) === true')
  t.equal(path.isAbsolute('\\foo'), true, 'path.isAbsolute(\\foo) === true')
  t.equal(path.isAbsolute('\\foo\\bar'), true, 'path.isAbsolute(\\foo\\bar) === true')
  t.equal(path.isAbsolute('C:\\'), true, 'path.isAbsolute(C:\\) === true')
  t.equal(path.isAbsolute('C:\\foo'), true, 'path.isAbsolute(C:\\foo) === true')
  t.equal(path.isAbsolute('C:\\foo\\bar'), true, 'path.isAbsolute(C:\\foo\\bar) === true')
})

test('path.parse', (t) => {
  function compare (a, b, m) {
    const { root, dir, base, ext, name } = a
    t.deepEqual({ root, dir, base, ext, name }, b, m)
  }

  compare(
    path.parse('foo'),
    { root: '', dir: '', base: 'foo', ext: '', name: 'foo' }
  )

  compare(
    path.posix.parse('foo.js'),
    { root: '', dir: '', base: 'foo.js', ext: '.js', name: 'foo' }
  )

  compare(
    path.posix.parse('./foo.js'),
    { root: '', dir: '.', base: 'foo.js', ext: '.js', name: 'foo' }
  )

  compare(
    path.win32.parse('.\\foo.js'),
    { root: '', dir: '.', base: 'foo.js', ext: '.js', name: 'foo' }
  )

  compare(
    path.win32.parse('/foo.js'),
    { root: '/', dir: '/', base: 'foo.js', ext: '.js', name: 'foo' }
  )

  compare(
    path.win32.parse('\\foo.js'),
    { root: '\\', dir: '\\', base: 'foo.js', ext: '.js', name: 'foo' }
  )

  compare(
    path.win32.parse('C:\\foo.js'),
    { root: 'C:\\', dir: 'C:\\', base: 'foo.js', ext: '.js', name: 'foo' }
  )

  compare(
    path.posix.parse('/bar/foo.js'),
    { root: '/', dir: '/bar', base: 'foo.js', ext: '.js', name: 'foo' }
  )

  compare(
    path.posix.parse('/../bar/foo.js'),
    { root: '/', dir: '/../bar', base: 'foo.js', ext: '.js', name: 'foo' }
  )

  compare(
    path.posix.parse('../bar/../../foo.js'),
    { root: '', dir: '../bar/../..', base: 'foo.js', ext: '.js', name: 'foo' }
  )

  compare(
    path.posix.parse('./foo.js'),
    { root: '', dir: '.', base: 'foo.js', ext: '.js', name: 'foo' }
  )
})

test('path.format', (t) => {
  t.equal(path.posix.format({ base: 'foo.js' }), 'foo.js')
  t.equal(path.posix.format({ dir: 'bar', base: 'foo.js' }), 'bar/foo.js')
  t.equal(path.posix.format({ root: '/', dir: '/home/bar', name: 'foo', ext: '.js' }), '/home/bar/foo.js')
  t.equal(path.win32.format({ sep: '\\', root: '\\', dir: '\\home\\bar', name: 'foo', ext: '.js' }), '\\home\\bar\\foo.js')
})

test('path.normalize', (t) => {
  t.equal(
    path.posix.normalize('file:///path/to/a/b/../../file.txt'),
    'file:/path/to/file.txt',
    'normalize file: URL'
  )

  t.equal(
    path.posix.normalize('protocol:path/to/a/b/../../file.txt'),
    'protocol:path/to/file.txt',
    'normalize protocol: URL'
  )

  t.equal(
    path.posix.normalize('https://example.com/path/to/a/b/../../file.txt'),
    'https://example.com/path/to/file.txt',
    'normalize https: URL'
  )

  t.equal(
    path.posix.normalize('/home/user/../../foo/bar.js'),
    '/foo/bar.js',
    'relative POSIX directory'
  )

  t.equal(
    path.posix.normalize('/home/user/../../foo/'),
    '/foo/',
    'preserve POSIX trailing slash'
  )

  t.equal(
    path.win32.normalize('c:\\beep\\boop\\a\\b\\..\\..\\foo\\bar.js'),
    'c:\\beep\\boop\\foo\\bar.js',
    'relative Windows directory'
  )

  t.equal(
    path.win32.normalize('c:\\beep\\boop\\a\\b\\..\\..\\foo\\bar\\'),
    'c:\\beep\\boop\\foo\\bar\\',
    'preserve Windows trailing slash'
  )
})

test('path.relative', (t) => {
  t.equal(path.posix.relative('/a/b/c', '/d/e/f'), '../../../d/e/f', '../../../d/e/f')
  t.equal(path.posix.relative('/a/b/c', '/a'), '../..', '../..')
  t.equal(path.posix.relative('/a/b/c', '/a/b/e'), '../e', ' ../e')
  t.equal(path.posix.relative('/a/b/c', '/a/b/c/d'), 'd', 'd')
  t.equal(path.posix.relative('/a/b/c', '/a/b/c/d/e'), 'd/e', 'd/e')

  t.equal(path.win32.relative('\\a\\b\\c', '\\d\\e\\f'), '..\\..\\..\\d\\e\\f', '..\\..\\..\\d\\e\\f')
  t.equal(path.win32.relative('\\a\\b\\c', '\\a'), '..\\..', '..\\..')
  t.equal(path.win32.relative('\\a\\b\\c', '\\a\\b\\e'), '..\\e', ' ..\\e')
  t.equal(path.win32.relative('\\a\\b\\c', '\\a\\b\\c\\d'), 'd', 'd')
  t.equal(path.win32.relative('\\a\\b\\c', '\\a\\b\\c\\d\\e'), 'd\\e', 'd\\e')
})
