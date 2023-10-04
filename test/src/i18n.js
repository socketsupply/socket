import test from 'socket:test'
import i18n from 'socket:i18n'

test('i18n.getMessage(messageName[,substitutions = [][, options]])', (t) => {
  const en = {
    key1: i18n.getMessage('key1', { locale: 'en' }),
    key2: i18n.getMessage('key2', { locale: 'en' }),
    'key-with-indexed-substitutions': i18n.getMessage(
      'key-with-indexed-substitutions',
      ['first', 'second', 'third'],
      { locale: 'en' }
    ),
    'key-with-placeholder-substitutions': i18n.getMessage(
      'key-with-placeholder-substitutions',
      ['first', 'second', 'third'],
      { locale: 'en' }
    )
  }

  const fr = {
    key1: i18n.getMessage('key1', { locale: 'fr' }),
    key2: i18n.getMessage('key2', { locale: 'fr-fr' }),
    'key-with-indexed-substitutions': i18n.getMessage(
      'key-with-indexed-substitutions',
      ['premier!', 'deuxième!', 'troisième!'],
      { locale: 'fr' }
    ),
    'key-with-placeholder-substitutions': i18n.getMessage(
      'key-with-placeholder-substitutions',
      ['premier!', 'deuxième!', 'troisième!'],
      { locale: 'fr-FR' }
    )
  }

  t.equal(en.key1, 'A message for key1', 'en.key1')
  t.equal(en.key2, 'A message for key2', 'en.key2')
  t.equal(en['key-with-indexed-substitutions'], 'first=first, second=second, third=third', 'en[key-with-indexed-substitutions]')
  t.equal(en['key-with-placeholder-substitutions'], 'first=first, second=second, third=third', 'en[key-with-placeholder-substitutions]')

  t.equal(fr.key1, 'Un message pour la clé 1', 'fr.key1')
  t.equal(fr.key2, 'Un message pour la clé 2', 'fr.key2')
  t.equal(fr['key-with-indexed-substitutions'], 'premier=premier!, deuxième=deuxième!, troisième=troisième!', 'fr[key-with-indexed-substitutions]')
  t.equal(fr['key-with-placeholder-substitutions'], 'premier=premier!, deuxième=deuxième!, troisième=troisième!', 'fr[key-with-placeholder-substitutions]')
})
