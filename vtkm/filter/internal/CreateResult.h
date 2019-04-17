//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_internal_CreateResult_h
#define vtk_m_filter_internal_CreateResult_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace filter
{
namespace internal
{

//@{
/// These are utility functions defined to use in filters when creating an
/// output dataset to return from `DoExecute` methods. The various overloads
/// provides different ways of creating the output dataset (copying the input
/// without any of the fields) and optionally adding additional field(s).

/// Use this for DataSet filters (not Field filters).
inline VTKM_CONT vtkm::cont::DataSet CreateResult(const vtkm::cont::DataSet& dataSet)
{
  vtkm::cont::DataSet clone;
  clone.CopyStructure(dataSet);
  return clone;
}

/// Use this if the field has already been added to the data set.
/// In this case, just tell us what the field name is (and optionally its
/// association).
inline VTKM_CONT vtkm::cont::DataSet CreateResult(
  const vtkm::cont::DataSet& dataSet,
  const std::string& fieldName,
  vtkm::cont::Field::Association fieldAssociation = vtkm::cont::Field::Association::ANY)
{
  VTKM_ASSERT(fieldName != "");
  VTKM_ASSERT(dataSet.HasField(fieldName, fieldAssociation));
  return dataSet;
}

/// Use this if you have built a \c Field object. An output
/// \c DataSet will be created by adding the field to the input.
inline VTKM_CONT vtkm::cont::DataSet CreateResult(const vtkm::cont::DataSet& inDataSet,
                                                  const vtkm::cont::Field& field)
{
  vtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(field);
  VTKM_ASSERT(field.GetName() != "");
  VTKM_ASSERT(clone.HasField(field.GetName(), field.GetAssociation()));
  return clone;
}

/// Use this function if you have an ArrayHandle that holds the data for
/// the field. You also need to specify a name and an association for the
/// field. If the field is associated with a particular element set (for
/// example, a cell association is associated with a cell set), the name of
/// that associated set must also be given. The element set name is ignored
/// for \c Association::WHOLE_MESH and \c Association::POINTS associations.
template <typename T, typename Storage>
inline VTKM_CONT vtkm::cont::DataSet CreateResult(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, Storage>& fieldArray,
  const std::string& fieldName,
  vtkm::cont::Field::Association fieldAssociation,
  const std::string& elementSetName = "")
{
  VTKM_ASSERT(fieldName != "");
  VTKM_ASSERT(fieldAssociation != vtkm::cont::Field::Association::ANY);
  VTKM_ASSERT(fieldAssociation != vtkm::cont::Field::Association::LOGICAL_DIM);

  vtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  if ((fieldAssociation == vtkm::cont::Field::Association::WHOLE_MESH) ||
      (fieldAssociation == vtkm::cont::Field::Association::POINTS))
  {
    vtkm::cont::Field field(fieldName, fieldAssociation, fieldArray);
    clone.AddField(field);
  }
  else
  {
    vtkm::cont::Field field(fieldName, fieldAssociation, elementSetName, fieldArray);
    clone.AddField(field);
  }

  // Sanity check.
  VTKM_ASSERT(clone.HasField(fieldName, fieldAssociation));
  return clone;
}

/// Use this function if you have a VariantArrayHandle that holds the data
/// for the field. You also need to specify a name and an association for the
/// field. If the field is associated with a particular element set (for
/// example, a cell association is associated with a cell set), the name of
/// that associated set must also be given. The element set name is ignored
/// for \c Association::WHOLE_MESH and \c Association::POINTS associations.
///
inline VTKM_CONT vtkm::cont::DataSet CreateResult(const vtkm::cont::DataSet& inDataSet,
                                                  const vtkm::cont::VariantArrayHandle& fieldArray,
                                                  const std::string& fieldName,
                                                  vtkm::cont::Field::Association fieldAssociation,
                                                  const std::string& elementSetName = "")
{
  VTKM_ASSERT(fieldName != "");
  VTKM_ASSERT(fieldAssociation != vtkm::cont::Field::Association::ANY);
  VTKM_ASSERT(fieldAssociation != vtkm::cont::Field::Association::LOGICAL_DIM);

  vtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  if ((fieldAssociation == vtkm::cont::Field::Association::WHOLE_MESH) ||
      (fieldAssociation == vtkm::cont::Field::Association::POINTS))
  {
    vtkm::cont::Field field(fieldName, fieldAssociation, fieldArray);
    clone.AddField(field);
  }
  else
  {
    vtkm::cont::Field field(fieldName, fieldAssociation, elementSetName, fieldArray);
    clone.AddField(field);
  }

  // Sanity check.
  VTKM_ASSERT(clone.HasField(fieldName, fieldAssociation));
  return clone;
}

//@}
} // namespace vtkm::filter::internal
} // namespace vtkm::filter
} // namespace vtkm

#endif
