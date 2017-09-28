//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_filter_Result_h
#define vtk_m_filter_Result_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace filter
{

/// \brief Storage for DataSet and/or Field results returned from a filter.
///
/// Use the \c IsValid() method to determine whether or not the filter execution
/// was successful.
///
/// \c Result can store either a \c DataSet, or a \c DataSet and \c Field, depending
/// on the type of filter that produced it. Use \c IsDataSetValid() and
/// \c IsFieldValid() to identify what is stored in the \c Result.
///
/// The \c DataSet and \c Field may be retrieved using \c GetDataSet() and
/// \c GetField() respectively. Attempts to retrieve an invalid result using
/// these methods will throw an \c vtkm::cont::ErrorValue exception.
///
class Result
{
public:
  VTKM_CONT
  Result()
    : DataSetValid(false)
    , FieldValid(false)
    , FieldAssociation(vtkm::cont::Field::ASSOC_ANY)
  {
  }

  /// Use this constructor for DataSet filters (not Field filters).
  ///
  VTKM_CONT
  Result(const vtkm::cont::DataSet& dataSet)
    : DataSetValid(true)
    , Data(dataSet)
    , FieldValid(false)
    , FieldAssociation(vtkm::cont::Field::ASSOC_ANY)
  {
  }

  /// Use this constructor if the field has already been added to the data set.
  /// In this case, just tell us what the field name is (and optionally its
  /// association).
  ///
  VTKM_CONT
  Result(const vtkm::cont::DataSet& dataSet,
         const std::string& fieldName,
         vtkm::cont::Field::AssociationEnum fieldAssociation = vtkm::cont::Field::ASSOC_ANY)
    : DataSetValid(true)
    , Data(dataSet)
    , FieldValid(true)
    , FieldName(fieldName)
    , FieldAssociation(fieldAssociation)
  {
    VTKM_ASSERT(fieldName != "");
    VTKM_ASSERT(dataSet.HasField(fieldName, fieldAssociation));
  }

  /// Use this constructor if you have built a \c Field object. An output
  /// \c DataSet will be created by adding the field to the input.
  ///
  VTKM_CONT
  Result(const vtkm::cont::DataSet& inDataSet, const vtkm::cont::Field& field)
    : DataSetValid(true)
    , Data(vtkm::cont::DataSet(inDataSet))
    , FieldValid(true)
    , FieldName(field.GetName())
    , FieldAssociation(field.GetAssociation())
  {
    this->Data.AddField(field);

    VTKM_ASSERT(this->FieldName != "");
    VTKM_ASSERT(this->GetDataSet().HasField(this->FieldName, this->FieldAssociation));
  }

  /// Use this constructor if you have an ArrayHandle that holds the data for
  /// the field. You also need to specify a name and an association for the
  /// field. If the field is associated with a particular element set (for
  /// example, a cell association is associated with a cell set), the name of
  /// that associated set must also be given. The element set name is ignored
  /// for \c ASSOC_WHOLE_MESH and \c ASSOC_POINTS associations.
  ///
  template <typename T, typename Storage>
  VTKM_CONT Result(const vtkm::cont::DataSet& inDataSet,
                   const vtkm::cont::ArrayHandle<T, Storage>& fieldArray,
                   const std::string& fieldName,
                   vtkm::cont::Field::AssociationEnum fieldAssociation,
                   const std::string& elementSetName = "")
    : DataSetValid(true)
    , Data(vtkm::cont::DataSet(inDataSet))
    , FieldValid(true)
    , FieldName(fieldName)
    , FieldAssociation(fieldAssociation)
  {
    VTKM_ASSERT(fieldName != "");
    VTKM_ASSERT(fieldAssociation != vtkm::cont::Field::ASSOC_ANY);
    VTKM_ASSERT(fieldAssociation != vtkm::cont::Field::ASSOC_LOGICAL_DIM);

    if ((fieldAssociation == vtkm::cont::Field::ASSOC_WHOLE_MESH) ||
        (fieldAssociation == vtkm::cont::Field::ASSOC_POINTS))
    {
      vtkm::cont::Field field(fieldName, fieldAssociation, fieldArray);
      this->Data.AddField(field);
    }
    else
    {
      vtkm::cont::Field field(fieldName, fieldAssociation, elementSetName, fieldArray);
      this->Data.AddField(field);
    }

    // Sanity check.
    VTKM_ASSERT(this->GetDataSet().HasField(this->FieldName, this->FieldAssociation));
  }

  /// Use this constructor if you have a DynamicArrayHandle that holds the data
  /// for the field. You also need to specify a name and an association for the
  /// field. If the field is associated with a particular element set (for
  /// example, a cell association is associated with a cell set), the name of
  /// that associated set must also be given. The element set name is ignored
  /// for \c ASSOC_WHOLE_MESH and \c ASSOC_POINTS associations.
  ///
  VTKM_CONT
  Result(const vtkm::cont::DataSet& inDataSet,
         const vtkm::cont::DynamicArrayHandle& fieldArray,
         const std::string& fieldName,
         vtkm::cont::Field::AssociationEnum fieldAssociation,
         const std::string& elementSetName = "")
    : DataSetValid(true)
    , Data(vtkm::cont::DataSet(inDataSet))
    , FieldValid(true)
    , FieldName(fieldName)
    , FieldAssociation(fieldAssociation)
  {
    VTKM_ASSERT(fieldName != "");
    VTKM_ASSERT(fieldAssociation != vtkm::cont::Field::ASSOC_ANY);
    VTKM_ASSERT(fieldAssociation != vtkm::cont::Field::ASSOC_LOGICAL_DIM);

    if ((fieldAssociation == vtkm::cont::Field::ASSOC_WHOLE_MESH) ||
        (fieldAssociation == vtkm::cont::Field::ASSOC_POINTS))
    {
      vtkm::cont::Field field(fieldName, fieldAssociation, fieldArray);
      this->Data.AddField(field);
    }
    else
    {
      vtkm::cont::Field field(fieldName, fieldAssociation, elementSetName, fieldArray);
      this->Data.AddField(field);
    }

    // Sanity check.
    VTKM_ASSERT(this->GetDataSet().HasField(this->FieldName, this->FieldAssociation));
  }

  /// Returns true if these results are from a successful execution of a
  /// filter.
  ///
  VTKM_CONT
  bool IsValid() const
  {
    // At this time, IsDataSetValid properly indicates whether or not a filter
    // ran successfully since all filters output a valid dataset on success.
    return this->IsDataSetValid();
  }

  /// Indicates whether or not it's safe to call GetDataSet().
  ///
  VTKM_CONT
  bool IsDataSetValid() const { return this->DataSetValid; }

  /// Returns the results of the filter in terms of a \c DataSet.
  ///
  VTKM_CONT
  const vtkm::cont::DataSet& GetDataSet() const
  {
    if (!this->DataSetValid)
    {
      throw vtkm::cont::ErrorBadValue("Result object does not contain a result dataset.");
    }

    return this->Data;
  }

  /// Returns the results of the filter in terms of a writable \c DataSet.
  VTKM_CONT
  vtkm::cont::DataSet& GetDataSet() { return this->Data; }

  /// Indicates whether or not it's safe to call GetField().
  ///
  VTKM_CONT
  bool IsFieldValid() const { return this->FieldValid; }

  VTKM_CONT
  const vtkm::cont::Field& GetField() const
  {
    if (!this->FieldValid)
    {
      throw vtkm::cont::ErrorBadValue(
        "Result object does not contain a result field, likely because it "
        "came from a DataSet filter.");
    }

    return this->GetDataSet().GetField(this->FieldName, this->FieldAssociation);
  }

  template <typename T, typename Storage>
  VTKM_CONT bool FieldAs(vtkm::cont::ArrayHandle<T, Storage>& dest) const
  {
    try
    {
      this->GetField().GetData().CopyTo(dest);
      return true;
    }
    catch (vtkm::cont::Error&)
    {
      return false;
    }
  }

private:
  bool DataSetValid;
  vtkm::cont::DataSet Data;

  bool FieldValid;
  std::string FieldName;
  vtkm::cont::Field::AssociationEnum FieldAssociation;
};
}
} // namespace vtkm::filter

#endif //vtk_m_filter_Result_h
