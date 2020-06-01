#pragma once


#ifdef __GNUC__
#  ifndef UNLIKELY
#    define UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#  endif
#  ifndef LIKELY
#    define LIKELY(expr) __builtin_expect(!!(expr), 1)
#  endif
#else
#  ifndef UNLIKELY
#    define UNLIKELY
#  endif
#  ifndef LIKELY
#    define LIKELY
#  endif
#endif
