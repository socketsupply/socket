import * as toaster from "./toaster"

/**
 * Typescript has a bug where it cannot import JS files correctly
 * Sub classing them works for reasons unknown so just do it.
 */
class CTonicToaster extends toaster.TonicToaster {}

declare global {
  interface HTMLElementTagNameMap {
    "tonic-toaster": CTonicToaster;
  }
}

declare function components (Tonic: object): void
declare namespace components {}

export = components;
