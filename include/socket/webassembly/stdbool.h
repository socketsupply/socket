#ifndef SOCKET_RUNTIME_WEBASSEMBLY_STDBOOL_H
#define SOCKET_RUNTIME_WEBASSEMBLY_STDBOOL_H

#if !defined(_STDBOOL_H)
#define _STDBOOL_H
#endif

// as defined by the C99 standard
#define __bool_true_false_are_defined 1

#if !defined(__cplusplus)
#define true 1
#define false 0
#define bool _Bool
#endif

#endif
