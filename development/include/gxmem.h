#ifndef GX_MEM
#define GX_MEM


#include <stdlib.h> // malloc and free needs this or error is generated.

#ifndef gxfree
#define gxfree(p)            free(p)
#endif

#ifndef gxmalloc
#define gxmalloc(s)          malloc(s)
#endif

#ifndef gxcalloc
#define gxcalloc(p, s)       calloc(p, s)
#endif


#ifndef gxrealloc
#define gxrealloc(p, s)      realloc(p, s)
#endif

#endif //!defined(DLMS_USE_CUSTOM_MALLOC) && !defined(DLMS_IGNORE_MALLOC)
