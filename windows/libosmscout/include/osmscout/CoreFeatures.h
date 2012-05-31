#ifndef LIBOSMSCOUT_COREFEATURES_H
#define LIBOSMSCOUT_COREFEATURES_H

/* libosmscout uses special gcc compiler features to export symbols */
#define OSMSCOUT_EXPORT_SYMBOLS 1

/* system header <stdint.h> is available */
#undef OSMSCOUT_HAVE_STDINT_H

/* system header <unordered_map> is available */
#undef OSMSCOUT_HAVE_UNORDERED_MAP

/* system header <unordered_set> is available */
#undef OSMSCOUT_HAVE_UNORDERED_SET

/* std::wstring is available */
#define OSMSCOUT_HAVE_STD__WSTRING 1

/* int8_t is available */
#undef OSMSCOUT_HAVE_INT8_T

/* uint8_t is available */
#undef OSMSCOUT_HAVE_UINT8_T

/* int16_t is available */
#undef OSMSCOUT_HAVE_INT16_T

/* uint16_t is available */
#undef OSMSCOUT_HAVE_UINT16_T

/* int32_t is available */
#undef OSMSCOUT_HAVE_INT32_T

/* uint32_t is available */
#undef OSMSCOUT_HAVE_UINT32_T

/* int64_t is available */
#undef OSMSCOUT_HAVE_INT64_T

/* uint64_t is available */
#undef OSMSCOUT_HAVE_UINT64_T

#endif //LIBOSMSCOUT_COREFEATURES_H