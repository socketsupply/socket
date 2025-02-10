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

export default globalThis.navigation ? globalThis.navigation : new Navigation()
