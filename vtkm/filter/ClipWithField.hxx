//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ClipWithField_hxx
#define vtk_m_filter_ClipWithField_hxx

#include <vtkm/filter/ClipWithField.h>

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>

// Do not instantiation common concrete types unless we are compiling the
// corresponding TU.
#if !defined(vtkm_filter_ClipWithFieldExecuteInteger_cxx) || \
  !defined(vtkm_filter_ClipWithFieldExecuteScalar_cxx)
#include <vtkm/filter/ClipWithFieldSkipInstantiations.hxx>
#endif

namespace vtkm
{
namespace filter
{

namespace detail
{

struct ClipWithFieldProcessCoords
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
template <typename T, typename StorageType, typename DerivedPolicy>
vtkm::cont::DataSet ClipWithField::DoExecute(const vtkm::cont::DataSet& input,
                                             const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                             const vtkm::filter::FieldMetadata& fieldMeta,
                                             vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  if (fieldMeta.IsPointField() == false)
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  vtkm::cont::CellSetExplicit<> outputCellSet = this->Worklet.Run(
    vtkm::filter::ApplyPolicyCellSet(cells, policy, *this), field, this->ClipValue, this->Invert);

  //create the output data
  vtkm::cont::DataSet output;
  output.SetCellSet(outputCellSet);

  // Compute the new boundary points and add them to the output:
  for (vtkm::IdComponent coordSystemId = 0; coordSystemId < input.GetNumberOfCoordinateSystems();
       ++coordSystemId)
  {
    vtkm::cont::CoordinateSystem coords = input.GetCoordinateSystem(coordSystemId);
    coords.GetData().CastAndCall(
      detail::ClipWithFieldProcessCoords{}, coords.GetName(), this->Worklet, output);
  }

  return output;
}
}
} // end namespace vtkm::filter

#endif
