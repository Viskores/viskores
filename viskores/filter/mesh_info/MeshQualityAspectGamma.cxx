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
//=========================================================================

#include <viskores/filter/mesh_info/MeshQualityAspectGamma.h>

#include <viskores/filter/mesh_info/worklet/MeshQualityWorklet.h>
#include <viskores/filter/mesh_info/worklet/cellmetrics/CellAspectGammaMetric.h>

namespace
{

struct AspectGammaWorklet : MeshQualityWorklet<AspectGammaWorklet>
{
  template <typename OutType, typename PointCoordVecType, typename CellShapeType>
  VISKORES_EXEC OutType ComputeMetric(const viskores::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  CellShapeType shape,
                                  viskores::ErrorCode& ec) const
  {
    return viskores::worklet::cellmetrics::CellAspectGammaMetric<OutType>(numPts, pts, shape, ec);
  }
};

} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace mesh_info
{

MeshQualityAspectGamma::MeshQualityAspectGamma()
{
  this->SetUseCoordinateSystemAsField(true);
  this->SetOutputFieldName("aspectGamma");
}

viskores::cont::DataSet MeshQualityAspectGamma::DoExecute(const viskores::cont::DataSet& input)
{
  viskores::cont::UnknownArrayHandle outArray =
    AspectGammaWorklet{}.Run(input, this->GetFieldFromDataSet(input));

  return this->CreateResultFieldCell(input, this->GetOutputFieldName(), outArray);
}

} // namespace mesh_info
} // namespace filter
} // namespace viskores
