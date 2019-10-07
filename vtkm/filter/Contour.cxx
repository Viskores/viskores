//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define vtkm_filter_Contour_cxx
#include <vtkm/filter/Contour.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
VTKM_FILTER_EXPORT Contour::Contour()
  : vtkm::filter::FilterDataSetWithField<Contour>()
  , IsoValues()
  , GenerateNormals(false)
  , AddInterpolationEdgeIds(false)
  , ComputeFastNormalsForStructured(false)
  , ComputeFastNormalsForUnstructured(true)
  , NormalArrayName("normals")
  , InterpolationEdgeIdsArrayName("edgeIds")
  , Worklet()
{
  // todo: keep an instance of marching cubes worklet as a member variable
}

//-----------------------------------------------------------------------------
VTKM_FILTER_EXPORT void Contour::SetNumberOfIsoValues(vtkm::Id num)
{
  if (num >= 0)
  {
    this->IsoValues.resize(static_cast<std::size_t>(num));
  }
}

//-----------------------------------------------------------------------------
VTKM_FILTER_EXPORT vtkm::Id Contour::GetNumberOfIsoValues() const
{
  return static_cast<vtkm::Id>(this->IsoValues.size());
}

//-----------------------------------------------------------------------------
VTKM_FILTER_EXPORT void Contour::SetIsoValue(vtkm::Id index, vtkm::Float64 v)
{
  std::size_t i = static_cast<std::size_t>(index);
  if (i >= this->IsoValues.size())
  {
    this->IsoValues.resize(i + 1);
  }
  this->IsoValues[i] = v;
}

//-----------------------------------------------------------------------------
VTKM_FILTER_EXPORT void Contour::SetIsoValues(const std::vector<vtkm::Float64>& values)
{
  this->IsoValues = values;
}

//-----------------------------------------------------------------------------
VTKM_FILTER_EXPORT vtkm::Float64 Contour::GetIsoValue(vtkm::Id index) const
{
  return this->IsoValues[static_cast<std::size_t>(index)];
}

//-----------------------------------------------------------------------------
VTKM_FILTER_INSTANTIATE_EXECUTE_METHOD(Contour);
}
}
