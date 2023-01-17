
#ifndef BOF2D_EXPORT_H
#define BOF2D_EXPORT_H

#ifdef BOF2D_STATIC_DEFINE
#  define BOF2D_EXPORT
#  define BOF2D_NO_EXPORT
#else
#  ifndef BOF2D_EXPORT
#    ifdef bof2d_EXPORTS
        /* We are building this library */
#      define BOF2D_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define BOF2D_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef BOF2D_NO_EXPORT
#    define BOF2D_NO_EXPORT 
#  endif
#endif

#ifndef BOF2D_DEPRECATED
#  define BOF2D_DEPRECATED __declspec(deprecated)
#endif

#ifndef BOF2D_DEPRECATED_EXPORT
#  define BOF2D_DEPRECATED_EXPORT BOF2D_EXPORT BOF2D_DEPRECATED
#endif

#ifndef BOF2D_DEPRECATED_NO_EXPORT
#  define BOF2D_DEPRECATED_NO_EXPORT BOF2D_NO_EXPORT BOF2D_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef BOF2D_NO_DEPRECATED
#    define BOF2D_NO_DEPRECATED
#  endif
#endif

#endif /* BOF2D_EXPORT_H */
