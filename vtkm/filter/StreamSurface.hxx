//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ErrorFilterExecution.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT StreamSurface::StreamSurface()
  : vtkm::filter::FilterDataSet<StreamSurface>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
template <typename Policy>
inline VTKM_CONT vtkm::cont::DataSet StreamSurface::DoExecute(const vtkm::cont::DataSet& input,
                                                              vtkm::filter::PolicyBase<Policy>)
{
  vtkm::worklet::StreamSurface streamSrf;

  vtkm::cont::ArrayHandle<vtkm::Vec3f> newPoints;
  vtkm::cont::CellSetSingleType<> newCells;

  streamSrf.Run(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
                input.GetCellSet(this->GetActiveCellSetIndex()),
                newPoints,
                newCells);

  vtkm::cont::DataSet outData;
  vtkm::cont::CoordinateSystem outCoords("coordinates", newPoints);
  outData.AddCellSet(newCells);
  outData.AddCoordinateSystem(outCoords);

  return outData;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool StreamSurface::DoMapField(vtkm::cont::DataSet&,
                                                const vtkm::cont::ArrayHandle<T, StorageType>&,
                                                const vtkm::filter::FieldMetadata&,
                                                vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace vtkm::filter
