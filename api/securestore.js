/**
 * Module for accessing native secure store.
 * @module SecureStore
 */

/**
 * Retrieve a value from the native secure store.
 * @async
 * @param {string} account - The account identifier.
 * @param {string} service - The service identifier.
 * @returns {Promise<string|null>} The retrieved value, or null if not found.
 */
export async function get(account, service) {
  return await ipc.send('securestore.get', { account, service });
}

/**
 * Set a value in the native secure store.
 * @async
 * @param {string} account - The account identifier.
 * @param {string} service - The service identifier.
 * @param {string} value - The value to store.
 * @returns {Promise<boolean>} A promise resolving to true if successful, otherwise false.
 */
export async function set(account, service, value) {
  return await ipc.write('securestore.set', {
    account,
    service,
    length: value.length
  }, value);
}

import * as exports from './securestore.js';
export default exports;
