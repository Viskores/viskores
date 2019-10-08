//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ZFPDecompressor2D_hxx
#define vtk_m_filter_ZFPDecompressor2D_hxx

#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>

namespace vtkm
{
namespace filter
{


//-----------------------------------------------------------------------------
inline VTKM_CONT ZFPDecompressor2D::ZFPDecompressor2D()
  : vtkm::filter::FilterField<ZFPDecompressor2D>()
{
}
//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ZFPDecompressor2D::DoExecute(
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
inline VTKM_CONT vtkm::cont::DataSet ZFPDecompressor2D::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<vtkm::Int64, StorageType>& field,
  const vtkm::filter::FieldMetadata&,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  vtkm::cont::CellSetStructured<2> cellSet;
  input.GetCellSet().CopyTo(cellSet);
  vtkm::Id2 pointDimensions = cellSet.GetPointDimensions();

  vtkm::cont::ArrayHandle<vtkm::Float64> decompress;
  decompressor.Decompress(field, decompress, rate, pointDimensions);

  vtkm::cont::DataSet dataset;
  dataset.AddField(vtkm::cont::make_FieldPoint("decompressed", decompress));
  return dataset;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool ZFPDecompressor2D::DoMapField(vtkm::cont::DataSet&,
                                                    const vtkm::cont::ArrayHandle<T, StorageType>&,
                                                    const vtkm::filter::FieldMetadata&,
                                                    const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  return false;
}
}
} // namespace vtkm::filter
#endif
