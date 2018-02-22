#ifndef DIY_MPI_HPP
#define DIY_MPI_HPP

#ifndef DIY_NO_MPI
#include <mpi.h>
#else
#include "mpi/no-mpi.hpp"
#endif

#include "mpi/constants.hpp"
#include "mpi/datatypes.hpp"
#include "mpi/optional.hpp"
#include "mpi/status.hpp"
#include "mpi/request.hpp"
#include "mpi/point-to-point.hpp"
#include "mpi/communicator.hpp"
#include "mpi/collectives.hpp"
#include "mpi/io.hpp"

namespace diy
{
namespace mpi
{

//! \ingroup MPI
struct environment
{
  inline environment();
  inline environment(int argc, char* argv[]);
  inline ~environment();
};

}
}

diy::mpi::environment::
environment()
{
#ifndef DIY_NO_MPI
  int argc = 0; char** argv;
  MPI_Init(&argc, &argv);
#endif
}

diy::mpi::environment::
environment(int argc, char* argv[])
{
#ifndef DIY_NO_MPI
  MPI_Init(&argc, &argv);
#else
  (void) argc; (void) argv;
#endif
}

diy::mpi::environment::
~environment()
{
#ifndef DIY_NO_MPI
  MPI_Finalize();
#endif
}

#endif
