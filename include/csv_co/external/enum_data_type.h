#pragma once
namespace vince_csv {
    /** Enumerates the different CSV field types that are
     *  recognized by this library
     *
     *  @note Overflowing integers will be stored and classified as doubles.
     *  @note Unlike previous releases, integer enums here are platform agnostic.
     */
    enum class DataType {
        UNKNOWN = -1,
        CSV_NULL,   /**< Empty string */
        CSV_STRING, /**< Non-numeric string */
        CSV_INT8,   /**< 8-bit integer */
        CSV_INT16,  /**< 16-bit integer (short on MSVC/GCC) */
        CSV_INT32,  /**< 32-bit integer (int on MSVC/GCC) */
        CSV_INT64,  /**< 64-bit integer (long long on MSVC/GCC) */
        CSV_DOUBLE  /**< Floating point value */
    };
}

