//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_cont_CellLocatorUniformGrid_h
#define vtkm_cont_CellLocatorUniformGrid_h

#include <vtkm/cont/internal/CellLocatorBase.h>

#include <vtkm/exec/CellLocatorUniformGrid.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT CellLocatorUniformGrid
  : public vtkm::cont::internal::CellLocatorBase<CellLocatorUniformGrid>
{
  using Superclass = vtkm::cont::internal::CellLocatorBase<CellLocatorUniformGrid>;

public:
  VTKM_CONT vtkm::exec::CellLocatorUniformGrid PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const;

private:
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;
  vtkm::Vec3f Origin;
  vtkm::Vec3f InvSpacing;
  vtkm::Vec3f MaxPoint;
  bool Is3D = true;

  friend Superclass;
  VTKM_CONT void Build();
};
}
} // vtkm::cont

#endif //vtkm_cont_CellLocatorUniformGrid_h
