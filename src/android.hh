#ifndef SSC_ANDROID_H
#define SSC_ANDROID_H

/**
 * Java Native Interface
 * @see https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/jniTOC.html
 * @see https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/functions.html
 * @see https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/types.html#wp276
 */
#include <jni.h>
#include <stdint.h>

#include "core.hh"

/**
 * Defined by the Socket SDK preprocessor
 */
#define PACKAGE_NAME __BUNDLE_IDENTIFIER__

/**
 * Creates a named package function configured for the configured bundle
 * suitable for definition or invocation.
 * @param The name of the package function
 */
#define package_function(name) Java___BUNDLE_IDENTIFIER___##name

/**
 * Creates a named native package export for the configured bundle suitable for
 * for definition only.
 * @param name The name of the package function to export
 */
#define package_export(name) JNIEXPORT JNICALL package_function(name)

#endif
