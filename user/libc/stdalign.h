#ifndef _STDALIGN_H
#define _STDALIGN_H

#if defined (__cplusplus)
extern "C" {
#endif

#ifndef __cplusplus

#if __STDC_VERSION__ < 201112L && defined(__GNUC__)
#define _Alignas(t) __attribute__((__aligned__(t)))
#define _Alignof(t) __alignof__(t)
#endif /* C11 or compiler extensions */

#define alignas _Alignas
#define alignof _Alignof

#endif /* __cplusplus */

#define __alignas_is_defined 1
#define __alignof_is_defined 1

#if defined (__cplusplus)
} /* extern "C" */
#endif

#endif /* _STDALIGN_H */
