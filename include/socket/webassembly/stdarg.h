#ifndef SOCKET_RUNTIME_WEBASSEMBLY_STDARG_H
#define SOCKET_RUNTIME_WEBASSEMBLY_STDARG_H

#if !defined(_STDARG_H)
#define _STDARG_H
#endif

#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end
#endif
