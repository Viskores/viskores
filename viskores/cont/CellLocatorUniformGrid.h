//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_CellLocatorUniformGrid_h
#define viskores_cont_CellLocatorUniformGrid_h

#include <viskores/cont/CellLocatorBase.h>

#include <viskores/exec/CellLocatorUniformGrid.h>

namespace viskores
{
namespace cont
{

class VISKORES_CONT_EXPORT CellLocatorUniformGrid : public viskores::cont::CellLocatorBase
{
public:
  using LastCell = viskores::exec::CellLocatorUniformGrid::LastCell;

  VISKORES_CONT viskores::exec::CellLocatorUniformGrid PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const;

private:
  viskores::Id3 CellDims;
  viskores::Id3 PointDims;
  viskores::Vec3f Origin;
  viskores::Vec3f InvSpacing;
  viskores::Vec3f MaxPoint;
  bool Is3D = true;

  VISKORES_CONT void Build() override;
};
}
} // viskores::cont

#endif //viskores_cont_CellLocatorUniformGrid_h
