#ifndef SSC_ANDROID_H
#define SSC_ANDROID_H

/**
 * Java Native Interface
 * @see https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/jniTOC.html
 * @see https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/functions.html
 */
#include <jni.h>
#include <stdint.h>

/**
 * Defined by the Socket SDK preprocessor
 */
#define PACKAGE_NAME {{bundle_identifier}}

/**
 * Creates a named package function configured for the configured bundle
 * suitable for definition or invocation.
 * @param The name of the package function
 */
#define package_function(name) Java_##_PACKAGE_NAME_##name

/**
 * Creates a named native package export for the configured bundle suitable for
 * for definition only.
 * @param name The name of the package function to export
 */
#define package_export(name) JNIEXPORT JNICALL package_function(name)

#endif
