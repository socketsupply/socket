type MenuItemSelection = {
    title: string
    parent: string
    state: '0'
}
declare interface Window {
    addEventListener(
        type: "menuItemSelected",
        listener: (event: CustomEvent<MenuItemSelection>) => void,
        options?: boolean | AddEventListenerOptions
    ): void;
    addEventListener(
        type: "process-error",
        listener: (event: CustomEvent<string>) => void,
        options?: boolean | AddEventListenerOptions
    ): void;
    addEventListener(
        type: "backend-exit",
        listener: (event: CustomEvent<string>) => void,
        options?: boolean | AddEventListenerOptions
    ): void;
}