//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ClipWithImplicitFunction_hxx
#define vtk_m_filter_ClipWithImplicitFunction_hxx

#include <vtkm/filter/ClipWithImplicitFunction.h>

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DynamicCellSet.h>

namespace vtkm
{
namespace filter
{

namespace detail
{

struct ClipWithImplicitFunctionProcessCoords
{
  template <typename T, typename Storage>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, Storage>& inCoords,
                            const std::string& coordsName,
                            const vtkm::worklet::Clip& worklet,
                            vtkm::cont::DataSet& output) const
  {
    vtkm::cont::ArrayHandle<T> outArray = worklet.ProcessPointField(inCoords);
    vtkm::cont::CoordinateSystem outCoords(coordsName, outArray);
    output.AddCoordinateSystem(outCoords);
  }
};

} // namespace detail

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline vtkm::cont::DataSet ClipWithImplicitFunction::DoExecute(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  const vtkm::cont::CoordinateSystem& inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::cont::CellSetExplicit<> outputCellSet =
    this->Worklet.Run(vtkm::filter::ApplyPolicyCellSet(cells, policy, *this),
                      this->Function,
                      inputCoords,
                      this->Invert);

  //create the output data
  vtkm::cont::DataSet output;
  output.SetCellSet(outputCellSet);

  // compute output coordinates
  for (vtkm::IdComponent coordSystemId = 0; coordSystemId < input.GetNumberOfCoordinateSystems();
       ++coordSystemId)
  {
    vtkm::cont::CoordinateSystem coords = input.GetCoordinateSystem(coordSystemId);
    coords.GetData().CastAndCall(
      detail::ClipWithImplicitFunctionProcessCoords{}, coords.GetName(), this->Worklet, output);
  }

  return output;
}
}
} // end namespace vtkm::filter

#endif
