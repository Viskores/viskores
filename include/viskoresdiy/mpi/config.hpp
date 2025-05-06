#ifndef VISKORESDIY_MPI_CONFIG_HPP
#define VISKORESDIY_MPI_CONFIG_HPP

#include <utility>

/// We want to allow the use of `diy::mpi` in either header-only or library mode.
/// VISKORESDIY_MPI_AS_LIB is defined when using library mode.
/// This file contains some configuration macros. To maintain backwards compatibility
/// suitable default values should be defined when using header-only mode.

/// VISKORESDIY_HAS_MPI should always be defined when VISKORESDIY_MPI_AS_LIB is defined, but only for
/// the compilation units that are part of the library.
/// VISKORESDIY_HAS_MPI=1 means MPI library is availalbe.
/// For header-only, the default is to assume MPI is available
#if !defined(VISKORESDIY_MPI_AS_LIB) && !defined(VISKORESDIY_HAS_MPI)
#  define VISKORESDIY_HAS_MPI 1
#endif

/// Include appropriate mpi header. Since VISKORESDIY_HAS_MPI is only defined for
/// the compilation units of the library, when in library mode, the header is
/// only included for the library's compilation units.
#ifdef VISKORESDIY_HAS_MPI
#  if VISKORESDIY_HAS_MPI
#    include <mpi.h>
#  else
#    include "no-mpi.hpp"
#  endif
#endif

/// Classes and objects that need to be visible to clients of the library should be
/// marked as VISKORESDIY_MPI_EXPORT. Similarly API functions should be marked as
/// VISKORESDIY_MPI_EXPORT_FUNCTION.
#include "diy-mpi-export.h" // defines VISKORESDIY_MPI_EXPORT and VISKORESDIY_MPI_EXPORT_FUNCTION

/// Define alisases for MPI types
#ifdef VISKORESDIY_MPI_AS_LIB
#  include "mpitypes.hpp" // only configured in library mode
#else // ifdef VISKORESDIY_MPI_AS_LIB

namespace diy
{
namespace mpi
{

#define DEFINE_DIY_MPI_TYPE(mpitype)                                          \
struct DIY_##mpitype {                                                        \
  DIY_##mpitype() = default;                                                  \
  DIY_##mpitype(const mpitype& obj) : data(obj) {}                            \
  DIY_##mpitype& operator=(const mpitype& obj) { data = obj; return *this; }  \
  operator mpitype() { return data; }                                         \
  mpitype data;                                                               \
};

#define DEFINE_DIY_MPI_TYPE_MOVE(mpitype)                                           \
struct DIY_##mpitype {                                                              \
  DIY_##mpitype() = default;                                                        \
  DIY_##mpitype(const mpitype&) = delete;                                           \
  DIY_##mpitype(mpitype&& obj) : data(std::move(obj)) {}                            \
  DIY_##mpitype& operator=(const mpitype&) = delete;                                \
  DIY_##mpitype& operator=(mpitype&& obj) { data = std::move(obj); return *this; }  \
  operator const mpitype&() const { return data; }                                  \
  void reset() { data = mpitype(); }                                                \
private:                                                                            \
  mpitype data;                                                                     \
};

DEFINE_DIY_MPI_TYPE(MPI_Comm)
DEFINE_DIY_MPI_TYPE(MPI_Datatype)
DEFINE_DIY_MPI_TYPE(MPI_Status)
DEFINE_DIY_MPI_TYPE(MPI_Request)
DEFINE_DIY_MPI_TYPE(MPI_Op)
DEFINE_DIY_MPI_TYPE(MPI_File)
DEFINE_DIY_MPI_TYPE_MOVE(MPI_Win)

#undef DEFINE_DIY_MPI_TYPE

}
} // diy::mpi
#endif // ifdef VISKORESDIY_MPI_AS_LIB

#ifdef VISKORESDIY_HAS_MPI
#  include "mpi_cast.hpp"
#endif

#endif // VISKORESDIY_MPI_CONFIG_HPP
