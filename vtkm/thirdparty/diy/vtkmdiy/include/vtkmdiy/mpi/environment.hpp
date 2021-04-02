#ifndef VTKMDIY_MPI_ENVIRONMENT_HPP
#define VTKMDIY_MPI_ENVIRONMENT_HPP

#include "config.hpp"

namespace diy
{
namespace mpi
{

//! \ingroup MPI
struct environment
{
  VTKMDIY_MPI_EXPORT_FUNCTION static bool initialized();

  VTKMDIY_MPI_EXPORT_FUNCTION environment();
  VTKMDIY_MPI_EXPORT_FUNCTION environment(int requested_threading);
  VTKMDIY_MPI_EXPORT_FUNCTION environment(int argc, char* argv[]);
  VTKMDIY_MPI_EXPORT_FUNCTION environment(int argc, char* argv[], int requested_threading);

  VTKMDIY_MPI_EXPORT_FUNCTION  ~environment();

  int   threading() const           { return provided_threading; }

  int   provided_threading;
};

}
} // diy::mpi

#ifndef VTKMDIY_MPI_AS_LIB
#include "environment.cpp"
#endif

#endif // VTKMDIY_MPI_ENVIRONMENT_HPP
