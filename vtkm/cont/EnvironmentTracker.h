//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_EnvironmentTracker_h
#define vtk_m_cont_EnvironmentTracker_h

#include <vtkm/Types.h>
#include <vtkm/cont/vtkm_cont_export.h>
#include <vtkm/internal/ExportMacros.h>
#include <vtkm/thirdparty/diy/Configure.h>

namespace diy
{
namespace mpi
{
class communicator;
}
}

namespace vtkm
{
namespace cont
{

/// \brief Maintain MPI controller, if any, for distributed operation.
///
/// `EnvironmentTracker` is a class that provides static API to track the global
/// MPI controller to use for operating in a distributed environment.
class VTKM_CONT_EXPORT EnvironmentTracker
{
public:
  VTKM_CONT
  static void SetCommunicator(const diy::mpi::communicator& comm);

  VTKM_CONT
  static const diy::mpi::communicator& GetCommunicator();
};
}
}


#endif // vtk_m_cont_EnvironmentTracker_h
