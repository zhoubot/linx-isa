#ifndef PTO_LINX_TARGET_STUB_HPP
#define PTO_LINX_TARGET_STUB_HPP

#if defined(__LINXISA__)

#ifndef __global__
#define __global__
#endif
#ifndef AICORE
#define AICORE
#endif
#ifndef __aicore__
#define __aicore__
#endif

#ifndef __gm__
#define __gm__
#endif
#ifndef __ubuf__
#define __ubuf__
#endif
#ifndef __cbuf__
#define __cbuf__
#endif
#ifndef __ca__
#define __ca__
#endif
#ifndef __cb__
#define __cb__
#endif
#ifndef __cc__
#define __cc__
#endif
#ifndef __fbuf__
#define __fbuf__
#endif
#ifndef __tf__
#define __tf__
#endif
#ifndef __out__
#define __out__
#endif
#ifndef __in__
#define __in__
#endif

#ifndef __cce_get_tile_ptr
#define __cce_get_tile_ptr(x) (x)
#endif

#ifndef set_flag
#define set_flag(...)
#endif
#ifndef wait_flag
#define wait_flag(...)
#endif

#endif // __LINXISA__

#endif // PTO_LINX_TARGET_STUB_HPP
