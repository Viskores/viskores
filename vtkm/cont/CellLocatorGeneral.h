//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellLocatorGeneral_h
#define vtk_m_cont_CellLocatorGeneral_h

#include <vtkm/cont/CellLocatorRectilinearGrid.h>
#include <vtkm/cont/CellLocatorTwoLevel.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>

#include <vtkm/exec/CellLocatorMultiplexer.h>

#include <vtkm/cont/internal/Variant.h>

#include <functional>
#include <memory>

namespace vtkm
{
namespace cont
{

/// \brief A CellLocator that works generally well for any supported cell set.
///
/// `CellLocatorGeneral` creates a `CellLocator` that acts like a multiplexer to
/// switch at runtime to any supported cell set. It is a convenient class to use
/// when the type of `CellSet` cannot be determined at runtime.
///
/// Note that `CellLocatorGeneral` only supports a finite amount of `CellSet` types.
/// Thus, it is possible to give it a cell set type that is not supported.
///
/// Also note that `CellLocatorGeneral` can add a significant amount of code inside
/// of worklet that uses it, and this might cause some issues with some compilers.
///
class VTKM_CONT_EXPORT CellLocatorGeneral
  : public vtkm::cont::internal::CellLocatorBase<CellLocatorGeneral>
{
  using Superclass = vtkm::cont::internal::CellLocatorBase<CellLocatorGeneral>;

public:
  using ContLocatorList = vtkm::List<vtkm::cont::CellLocatorUniformGrid,
                                     vtkm::cont::CellLocatorRectilinearGrid,
                                     vtkm::cont::CellLocatorTwoLevel>;

  using ExecLocatorList =
    vtkm::List<vtkm::cont::internal::ExecutionObjectType<vtkm::cont::CellLocatorUniformGrid>,
               vtkm::cont::internal::ExecutionObjectType<vtkm::cont::CellLocatorRectilinearGrid>,
               vtkm::cont::internal::ExecutionObjectType<vtkm::cont::CellLocatorTwoLevel>>;

  using ExecObjType = vtkm::ListApply<ExecLocatorList, vtkm::exec::CellLocatorMultiplexer>;

  VTKM_CONT ExecObjType PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                            vtkm::cont::Token& token) const;

private:
  vtkm::cont::internal::ListAsVariant<ContLocatorList> LocatorImpl;

  friend Superclass;
  VTKM_CONT void Build();

  struct PrepareFunctor;
};
}
} // vtkm::cont

#endif // vtk_m_cont_CellLocatorGeneral_h
