//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_EnvironmentTracker_h
#define vtk_m_cont_EnvironmentTracker_h

#include <vtkm/Types.h>
#include <vtkm/cont/vtkm_cont_export.h>
#include <vtkm/internal/ExportMacros.h>

#include <vtkm/thirdparty/diy/diy.h>

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
  static void SetCommunicator(const vtkmdiy::mpi::communicator& comm);

  VTKM_CONT
  static const vtkmdiy::mpi::communicator& GetCommunicator();
};
}
}


#endif // vtk_m_cont_EnvironmentTracker_h
