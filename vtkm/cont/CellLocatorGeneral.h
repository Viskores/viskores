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
