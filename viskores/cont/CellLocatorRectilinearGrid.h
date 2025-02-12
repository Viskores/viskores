//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_CellLocatorRectilinearGrid_h
#define viskores_cont_CellLocatorRectilinearGrid_h

#include <viskores/cont/CellLocatorBase.h>

#include <viskores/exec/CellLocatorRectilinearGrid.h>

namespace viskores
{
namespace cont
{

class VISKORES_CONT_EXPORT CellLocatorRectilinearGrid : public viskores::cont::CellLocatorBase
{
  using Structured2DType = viskores::cont::CellSetStructured<2>;
  using Structured3DType = viskores::cont::CellSetStructured<3>;
  // Might want to handle cartesian product of both Float32 and Float64.
  using AxisHandle = viskores::cont::ArrayHandle<viskores::FloatDefault>;
  using RectilinearType =
    viskores::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;

public:
  CellLocatorRectilinearGrid() = default;

  ~CellLocatorRectilinearGrid() = default;

  using LastCell = viskores::exec::CellLocatorRectilinearGrid::LastCell;

  VISKORES_CONT viskores::exec::CellLocatorRectilinearGrid PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const;

private:
  viskores::Bounds Bounds;
  viskores::Id PlaneSize;
  viskores::Id RowSize;
  bool Is3D = true;

protected:
  VISKORES_CONT void Build() override;
};

} //namespace cont
} //namespace viskores

#endif //viskores_cont_CellLocatorRectilinearGrid_h
