#include <functional>

namespace diy
{
namespace mpi
{
  //! \addtogroup MPI
  //!@{
  template<class U>
  struct maximum { const U& operator()(const U& x, const U& y) const { return std::max(x,y); } };
  template<class U>
  struct minimum { const U& operator()(const U& x, const U& y) const { return std::min(x,y); } };
  //!@}

namespace detail
{
  template<class T> struct mpi_op                           { static MPI_Op  get(const T&); };
  template<class U> struct mpi_op< maximum<U> >             { static MPI_Op  get(const maximum<U>&) { return MPI_MAX; }  };
  template<class U> struct mpi_op< minimum<U> >             { static MPI_Op  get(const minimum<U>&) { return MPI_MIN; }  };
  template<class U> struct mpi_op< std::plus<U> >           { static MPI_Op  get(const std::plus<U>&) { return MPI_SUM; }  };
  template<class U> struct mpi_op< std::multiplies<U> >     { static MPI_Op  get(const std::multiplies<U>&) { return MPI_PROD; }  };
  template<class U> struct mpi_op< std::logical_and<U> >    { static MPI_Op  get(const std::logical_and<U>&) { return MPI_LAND; }  };
  template<class U> struct mpi_op< std::logical_or<U> >     { static MPI_Op  get(const std::logical_or<U>&) { return MPI_LOR; }  };
}
}
}
