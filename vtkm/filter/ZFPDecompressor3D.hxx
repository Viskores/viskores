//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>

#include <vtkm/worklet/DispatcherMapTopology.h>

namespace vtkm
{
namespace filter
{


//-----------------------------------------------------------------------------
inline VTKM_CONT ZFPDecompressor3D::ZFPDecompressor3D()
  : vtkm::filter::FilterField<ZFPDecompressor3D>()
{
}
//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ZFPDecompressor3D::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<T, StorageType>&,
  const vtkm::filter::FieldMetadata&,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  VTKM_ASSERT(true);
  vtkm::cont::DataSet ds;
  return ds;
}

//-----------------------------------------------------------------------------
template <typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ZFPDecompressor3D::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<vtkm::Int64, StorageType>& field,
  const vtkm::filter::FieldMetadata&,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  //  if (fieldMeta.IsPointField() == false)
  //  {
  //    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  //  }

  // Check the fields of the dataset to see what kinds of fields are present so
  // we can free the mapping arrays that won't be needed. A point field must
  // exist for this algorithm, so just check cells.
  const vtkm::Id numFields = input.GetNumberOfFields();
  bool hasCellFields = false;
  for (vtkm::Id fieldIdx = 0; fieldIdx < numFields && !hasCellFields; ++fieldIdx)
  {
    auto f = input.GetField(fieldIdx);
    if (f.GetAssociation() == vtkm::cont::Field::Association::CELL_SET)
    {
      hasCellFields = true;
    }
  }

  vtkm::cont::CellSetStructured<3> cellSet;
  input.GetCellSet(this->GetActiveCoordinateSystemIndex()).CopyTo(cellSet);
  vtkm::Id3 pointDimensions = cellSet.GetPointDimensions();

  vtkm::cont::ArrayHandle<vtkm::Float64> decompress;
  decompressor.Decompress(field, decompress, rate, pointDimensions);

  vtkm::cont::DataSet dataset;
  vtkm::cont::Field decompressField(
    "decompressed", vtkm::cont::Field::Association::POINTS, decompress);
  dataset.AddField(decompressField);
  return dataset;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool ZFPDecompressor3D::DoMapField(vtkm::cont::DataSet&,
                                                    const vtkm::cont::ArrayHandle<T, StorageType>&,
                                                    const vtkm::filter::FieldMetadata&,
                                                    const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  return false;
}
}
} // namespace vtkm::filter
