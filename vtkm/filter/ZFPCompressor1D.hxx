//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ZFPCompressor1D_hxx
#define vtk_m_filter_ZFPCompressor1D_hxx

#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>

namespace vtkm
{
namespace filter
{

namespace
{

template <typename CellSetList>
bool IsCellSetStructured(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellset)
{
  if (cellset.template IsType<vtkm::cont::CellSetStructured<1>>())

  {
    return true;
  }
  return false;
}
} // anonymous namespace

//-----------------------------------------------------------------------------
inline VTKM_CONT ZFPCompressor1D::ZFPCompressor1D()
  : vtkm::filter::FilterField<ZFPCompressor1D>()
  , rate(0)
{
}


//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ZFPCompressor1D::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata&,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  auto compressed = compressor.Compress(field, rate, field.GetNumberOfValues());

  vtkm::cont::DataSet dataset;
  dataset.AddField(vtkm::cont::make_FieldPoint("compressed", compressed));
  return dataset;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool ZFPCompressor1D::DoMapField(vtkm::cont::DataSet&,
                                                  const vtkm::cont::ArrayHandle<T, StorageType>&,
                                                  const vtkm::filter::FieldMetadata&,
                                                  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  return false;
}
}
} // namespace vtkm::filter
#endif
