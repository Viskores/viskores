//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_MapFieldMergeAverage_h
#define vtk_m_filter_MapFieldMergeAverage_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/Keys.h>

#include <vtkm/filter/vtkm_filter_common_export.h>

namespace vtkm
{
namespace filter
{

/// \brief Maps a field by merging entries based on a keys object.
///
/// This method will create a new field containing the data from the provided `inputField` but but
/// with groups of entities merged together. The input `keys` object encapsulates which elements
/// should be merged together. A group of elements merged together will be averaged. The result is
/// placed in `outputField`.
///
/// The intention of this method is to implement the `MapFieldOntoOutput` methods in filters (many
/// of which require this merge of a field), but can be used in other places as well.
///
/// `outputField` is set to have the same metadata as the input. If the metadata needs to change
/// (such as the name or the association) that should be done after the function returns.
///
/// This function returns whether the field was successfully merged. If the returned result is
/// `true`, then the results in `outputField` are valid. If it is `false`, then `outputField`
/// should not be used.
///
VTKM_FILTER_COMMON_EXPORT VTKM_CONT bool MapFieldMergeAverage(
  const vtkm::cont::Field& inputField,
  const vtkm::worklet::internal::KeysBase& keys,
  vtkm::cont::Field& outputField);

/// \brief Maps a field by merging entries based on a keys object.
///
/// This method will create a new field containing the data from the provided `inputField` but but
/// with groups of entities merged together. The input `keys` object encapsulates which elements
/// should be merged together. A group of elements merged together will be averaged.
///
/// The intention of this method is to implement the `MapFieldOntoOutput` methods in filters (many
/// of which require this merge of a field), but can be used in other places as well. The
/// resulting field is put in the given `DataSet`.
///
/// The returned `Field` has the same metadata as the input. If the metadata needs to change (such
/// as the name or the association), then a different form of `MapFieldMergeAverage` should be used.
///
/// This function returns whether the field was successfully merged. If the returned result is
/// `true`, then `outputData` has the merged field. If it is `false`, then the field is not
/// placed in `outputData`.
///
VTKM_FILTER_COMMON_EXPORT VTKM_CONT bool MapFieldMergeAverage(
  const vtkm::cont::Field& inputField,
  const vtkm::worklet::internal::KeysBase& keys,
  vtkm::cont::DataSet& outputData);
}
} // namespace vtkm::filter

#endif //vtk_m_filter_MapFieldMergeAverage_h
