#ifndef VTKMDIY_MPI_EXPORT_H
#define VTKMDIY_MPI_EXPORT_H

#if defined(_MSC_VER)
#  ifdef VTKMDIY_MPI_STATIC_BUILD
     /* This is a static component and has no need for exports
        elf based static libraries are able to have hidden/default visibility
        controls on symbols so we should propagate this information in that
        use case
     */
#    define VTKMDIY_MPI_EXPORT_DEFINE
#    define VTKMDIY_MPI_IMPORT_DEFINE
#    define VTKMDIY_MPI_NO_EXPORT_DEFINE
#  else
#    define VTKMDIY_MPI_EXPORT_DEFINE __declspec(dllexport)
#    define VTKMDIY_MPI_IMPORT_DEFINE __declspec(dllimport)
#    define VTKMDIY_MPI_NO_EXPORT_DEFINE
#  endif
#else
#  define VTKMDIY_MPI_EXPORT_DEFINE __attribute__((visibility("default")))
#  define VTKMDIY_MPI_IMPORT_DEFINE __attribute__((visibility("default")))
#  define VTKMDIY_MPI_NO_EXPORT_DEFINE __attribute__((visibility("hidden")))
#endif

#ifndef VTKMDIY_MPI_EXPORT
#  if !defined(VTKMDIY_MPI_AS_LIB)
#    define VTKMDIY_MPI_EXPORT
#    define VTKMDIY_MPI_EXPORT_FUNCTION inline
#  else
#    if defined(VTKMDIY_HAS_MPI)
       /* We are building this library */
#      define VTKMDIY_MPI_EXPORT VTKMDIY_MPI_EXPORT_DEFINE
#    else
       /* We are using this library */
#      define VTKMDIY_MPI_EXPORT VTKMDIY_MPI_IMPORT_DEFINE
#    endif
#    define VTKMDIY_MPI_EXPORT_FUNCTION VTKMDIY_MPI_EXPORT
#  endif
#endif

#ifndef VTKMDIY_MPI_EXPORT_FUNCTION
#error "VTKMDIY_MPI_EXPORT_FUNCTION not defined"
#endif

#ifndef VTKMDIY_MPI_NO_EXPORT
#  define VTKMDIY_MPI_NO_EXPORT VTKMDIY_MPI_NO_EXPORT_DEFINE
#endif

#endif // VTKMDIY_MPI_EXPORT_H
