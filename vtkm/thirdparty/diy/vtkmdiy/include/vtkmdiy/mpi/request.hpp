#ifndef VTKMDIY_MPI_REQUEST_HPP
#define VTKMDIY_MPI_REQUEST_HPP

#include "config.hpp"
#include "status.hpp"
#include "optional.hpp"

namespace diy
{
namespace mpi
{
  struct request
  {
    VTKMDIY_MPI_EXPORT_FUNCTION                  request();
    VTKMDIY_MPI_EXPORT_FUNCTION status           wait();
    VTKMDIY_MPI_EXPORT_FUNCTION optional<status> test();
    VTKMDIY_MPI_EXPORT_FUNCTION void             cancel();

    DIY_MPI_Request handle;
  };

}
} // diy::mpi

#ifndef VTKMDIY_MPI_AS_LIB
#include "request.cpp"
#endif

#endif // VTKMDIY_MPI_REQUEST_HPP
