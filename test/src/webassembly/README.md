WebAssembly modules here are compiled from C source code using https://wasmfiddle.com

## program.wasm

```c
extern int myFunc(int);

int identity(int num) {
  return num;
}

int runMyFunc(int num) {
  return myFunc(num);
}
```
