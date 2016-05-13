//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_filter_ResultField_h
#define vtk_m_filter_ResultField_h

#include <vtkm/filter/ResultBase.h>

#include <vtkm/cont/Field.h>

namespace vtkm {
namespace filter {

/// \brief Results for filters that create a field
///
/// \c ResultField contains the results for a filter that generates a new
/// field. The geometry of the underlying data is a shallow copy of the
/// original input.
///
/// This class also holds the field that has been added.
///
class ResultField : public vtkm::filter::ResultBase
{
public:
  VTKM_CONT_EXPORT
  ResultField() {  }

  /// Use this constructor if the field has already been added to the data set.
  /// In this case, just tell us what the field name is (and optionally its
  /// association).
  ///
  VTKM_CONT_EXPORT
  ResultField(const vtkm::cont::DataSet &dataSet,
              const std::string &fieldName,
              vtkm::cont::Field::AssociationEnum fieldAssociation
                = vtkm::cont::Field::ASSOC_ANY)
    : ResultBase(dataSet),
      FieldName(fieldName),
      FieldAssociation(fieldAssociation)
  {
    VTKM_ASSERT(fieldName != "");
    VTKM_ASSERT(dataSet.HasField(fieldName, fieldAssociation));
  }

  /// Use this constructor if you have build a \c Field object. An output
  /// \c DataSet will be created by adding the field to the input.
  ///
  VTKM_CONT_EXPORT
  ResultField(const vtkm::cont::DataSet &inDataSet,
              const vtkm::cont::Field &field)
    : FieldName(field.GetName()), FieldAssociation(field.GetAssociation())
  {
    VTKM_ASSERT(this->FieldName != "");

    vtkm::cont::DataSet outDataSet = inDataSet;
    outDataSet.AddField(field);
    this->SetDataSet(outDataSet);

    // Sanity check.
    VTKM_ASSERT(this->GetDataSet().HasField(this->FieldName,
                                            this->FieldAssociation));
  }

  /// Use this constructor if you have an ArrayHandle that holds the data for
  /// the field. You also need to specify a name and an association for the
  /// field. If the field is associated with a particular element set (for
  /// example, a cell association is associated with a cell set), the name of
  /// that associated set must also be given. The element set name is ignored
  /// for \c ASSOC_WHOLE_MESH and \c ASSOC_POINTS associations.
  ///
  template<typename T, typename Storage>
  VTKM_CONT_EXPORT
  ResultField(const vtkm::cont::DataSet &inDataSet,
              const vtkm::cont::ArrayHandle<T, Storage> &fieldArray,
              const std::string &fieldName,
              vtkm::cont::Field::AssociationEnum fieldAssociation,
              const std::string &elementSetName = "")
    : FieldName(fieldName), FieldAssociation(fieldAssociation)
  {
    VTKM_ASSERT(fieldName != "");
    VTKM_ASSERT(fieldAssociation != vtkm::cont::Field::ASSOC_ANY);
    VTKM_ASSERT(fieldAssociation != vtkm::cont::Field::ASSOC_LOGICAL_DIM);

    vtkm::cont::DataSet outDataSet = inDataSet;

    if ((fieldAssociation == vtkm::cont::Field::ASSOC_WHOLE_MESH) ||
        (fieldAssociation == vtkm::cont::Field::ASSOC_POINTS))
    {
      vtkm::cont::Field field(fieldName, fieldAssociation, fieldArray);
      outDataSet.AddField(field);
    }
    else
    {
      vtkm::cont::Field field(fieldName,
                              fieldAssociation,
                              elementSetName,
                              fieldArray);
      outDataSet.AddField(field);
    }

    this->SetDataSet(outDataSet);

    // Sanity check.
    VTKM_ASSERT(this->GetDataSet().HasField(this->FieldName,
                                            this->FieldAssociation));
  }

  /// Use this constructor if you have a DynamicArrayHandle that holds the data
  /// for the field. You also need to specify a name and an association for the
  /// field. If the field is associated with a particular element set (for
  /// example, a cell association is associated with a cell set), the name of
  /// that associated set must also be given. The element set name is ignored
  /// for \c ASSOC_WHOLE_MESH and \c ASSOC_POINTS associations.
  ///
  VTKM_CONT_EXPORT
  ResultField(const vtkm::cont::DataSet &inDataSet,
              const vtkm::cont::DynamicArrayHandle &fieldArray,
              const std::string &fieldName,
              vtkm::cont::Field::AssociationEnum fieldAssociation,
              const std::string &elementSetName = "")
    : FieldName(fieldName), FieldAssociation(fieldAssociation)
  {
    VTKM_ASSERT(fieldName != "");
    VTKM_ASSERT(fieldAssociation != vtkm::cont::Field::ASSOC_ANY);
    VTKM_ASSERT(fieldAssociation != vtkm::cont::Field::ASSOC_LOGICAL_DIM);

    vtkm::cont::DataSet outDataSet = inDataSet;

    if ((fieldAssociation == vtkm::cont::Field::ASSOC_WHOLE_MESH) ||
        (fieldAssociation == vtkm::cont::Field::ASSOC_POINTS))
    {
      vtkm::cont::Field field(fieldName, fieldAssociation, fieldArray);
      outDataSet.AddField(field);
    }
    else
    {
      vtkm::cont::Field field(fieldName,
                              fieldAssociation,
                              elementSetName,
                              fieldArray);
      outDataSet.AddField(field);
    }

    this->SetDataSet(outDataSet);

    // Sanity check.
    VTKM_ASSERT(this->GetDataSet().HasField(this->FieldName,
                                            this->FieldAssociation));
  }

  VTKM_CONT_EXPORT
  const vtkm::cont::Field &GetField() const
  {
    return this->GetDataSet().GetField(this->FieldName, this->FieldAssociation);
  }

  template<typename T, typename Storage>
  VTKM_CONT_EXPORT
  bool FieldAs(vtkm::cont::ArrayHandle<T, Storage> &dest) const
  {
    try
    {
      this->GetField().GetData().CopyTo(dest);
      return true;
    }
    catch(vtkm::cont::Error)
    {
      return false;
    }
  }

private:
  std::string FieldName;
  vtkm::cont::Field::AssociationEnum FieldAssociation;
};

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ResultField_h
