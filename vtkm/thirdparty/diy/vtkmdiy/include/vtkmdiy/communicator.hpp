#ifndef VTKMDIY_COMMUNICATOR_HPP
#define VTKMDIY_COMMUNICATOR_HPP

#warning "diy::Communicator (in diy/communicator.hpp) is deprecated, use diy::mpi::communicator directly"

#include "mpi.hpp"

namespace diy
{
  typedef mpi::communicator         Communicator;
}

#endif
