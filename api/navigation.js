import './navigation/navigation.js'

export const Navigation = globalThis.navigation
  ? Object.getPrototypeOf(globalThis.navigation).constructor
  : class Navigation extends EventTarget {}

const currentEntry = globalThis.navigation
  ? globalThis.navigation.currentEntry
  : null

export const NavigationHistoryEntry = currentEntry
  ? Object.getPrototypeOf(currentEntry).constructor
  : class NavigationHistoryEntry extends EventTarget {}

export const navigation = globalThis.navigation ? globalThis.navigation : new Navigation()
export default navigation

if (!('activation' in navigation) && globalThis.window) {
  globalThis.addEventListener('pageswap', (e) => {
    if (e.activation) {
      try {
        navigation.activation = e.activation
      } catch {}
    }
  })
}
