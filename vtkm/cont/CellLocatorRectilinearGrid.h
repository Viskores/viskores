//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_cont_celllocatorrectilineargrid_h
#define vtkm_cont_celllocatorrectilineargrid_h

#include <vtkm/cont/CellLocator.h>
#include <vtkm/cont/CellSetStructured.h>

namespace vtkm
{

namespace cont
{

class VTKM_CONT_EXPORT CellLocatorRectilinearGrid : public vtkm::cont::CellLocator
{
public:
  using StructuredType = vtkm::cont::CellSetStructured<3>;
  using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using RectilinearType =
    vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;

  VTKM_CONT CellLocatorRectilinearGrid();

  VTKM_CONT ~CellLocatorRectilinearGrid() override;

  VTKM_CONT void Build() override;

  VTKM_CONT
  const HandleType PrepareForExecutionImpl(
    const vtkm::cont::DeviceAdapterId deviceId) const override;

private:
  vtkm::Bounds Bounds;
  vtkm::Id PlaneSize;
  vtkm::Id RowSize;
  mutable HandleType ExecHandle;

  struct PrepareForExecutionFunctor;
};

} //namespace cont

} //namespace vtkm

#endif //vtkm_cont_celllocatorrectilineargrid_h
