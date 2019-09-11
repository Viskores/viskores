//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_CreateResult_h
#define vtk_m_filter_CreateResult_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Field.h>

#include <vtkm/filter/FieldMetadata.h>

namespace vtkm
{
namespace filter
{
//@{
/// These are utility functions defined to use in filters when creating an
/// output dataset to return from `DoExecute` methods. The various overloads
/// provides different ways of creating the output dataset (copying the input
/// without any of the fields) and adding additional field(s).

/// Use this if you have built a \c Field object. An output
/// \c DataSet will be created by adding the field to the input.
inline VTKM_CONT vtkm::cont::DataSet CreateResult(const vtkm::cont::DataSet& inDataSet,
                                                  const vtkm::cont::Field& field)
{
  vtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(field);
  VTKM_ASSERT(!field.GetName().empty());
  VTKM_ASSERT(clone.HasField(field.GetName(), field.GetAssociation()));
  return clone;
}

/// Use this function if you have an ArrayHandle that holds the data for
/// the field. You also need to specify a name for the field.
template <typename T, typename Storage>
inline VTKM_CONT vtkm::cont::DataSet CreateResult(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, Storage>& fieldArray,
  const std::string& fieldName,
  const vtkm::filter::FieldMetadata& metaData)
{
  VTKM_ASSERT(!fieldName.empty());

  vtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(metaData.AsField(fieldName, fieldArray));

  // Sanity check.
  VTKM_ASSERT(clone.HasField(fieldName, metaData.GetAssociation()));
  return clone;
}

/// Use this function if you have a VariantArrayHandle that holds the data
/// for the field.
inline VTKM_CONT vtkm::cont::DataSet CreateResult(const vtkm::cont::DataSet& inDataSet,
                                                  const vtkm::cont::VariantArrayHandle& fieldArray,
                                                  const std::string& fieldName,
                                                  const vtkm::filter::FieldMetadata& metaData)
{
  VTKM_ASSERT(!fieldName.empty());

  vtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(metaData.AsField(fieldName, fieldArray));

  // Sanity check.
  VTKM_ASSERT(clone.HasField(fieldName, metaData.GetAssociation()));
  return clone;
}

/// Use this function if you want to explicit construct a Cell field and have a ArrayHandle
/// that holds the data for the field.
template <typename T, typename Storage>
inline VTKM_CONT vtkm::cont::DataSet CreateResultFieldCell(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, Storage>& fieldArray,
  const std::string& fieldName)
{
  VTKM_ASSERT(!fieldName.empty());

  vtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(vtkm::cont::make_FieldCell(fieldName, fieldArray));

  // Sanity check.
  VTKM_ASSERT(clone.HasCellField(fieldName));
  return clone;
}

/// Use this function if you want to explicit construct a Cell field and have a VariantArrayHandle
/// that holds the data for the field.
inline VTKM_CONT vtkm::cont::DataSet CreateResultFieldCell(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::VariantArrayHandle& fieldArray,
  const std::string& fieldName)
{
  VTKM_ASSERT(!fieldName.empty());

  vtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(vtkm::cont::make_FieldCell(fieldName, fieldArray));

  // Sanity check.
  VTKM_ASSERT(clone.HasCellField(fieldName));
  return clone;
}

/// Use this function if you want to explicit construct a Point field and have a ArrayHandle
/// that holds the data for the field.
template <typename T, typename Storage>
inline VTKM_CONT vtkm::cont::DataSet CreateResultFieldPoint(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, Storage>& fieldArray,
  const std::string& fieldName)
{
  VTKM_ASSERT(!fieldName.empty());

  vtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(vtkm::cont::make_FieldPoint(fieldName, fieldArray));

  // Sanity check.
  VTKM_ASSERT(clone.HasPointField(fieldName));
  return clone;
}

/// Use this function if you want to explicit construct a Point field and have a VariantArrayHandle
/// that holds the data for the field.
inline VTKM_CONT vtkm::cont::DataSet CreateResultFieldPoint(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::VariantArrayHandle& fieldArray,
  const std::string& fieldName)
{
  VTKM_ASSERT(!fieldName.empty());

  vtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(vtkm::cont::make_FieldPoint(fieldName, fieldArray));

  // Sanity check.
  VTKM_ASSERT(clone.HasPointField(fieldName));
  return clone;
}

//@}
} // namespace vtkm::filter
} // namespace vtkm

#endif
