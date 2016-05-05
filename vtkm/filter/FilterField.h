//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_filter_FieldFilter_h
#define vtk_m_filter_FieldFilter_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Field.h>

#include <vtkm/filter/PolicyBase.h>
#include <vtkm/filter/internal/RuntimeDeviceTracker.h>

namespace vtkm {
namespace filter {

class FieldResult
{
public:
  VTKM_CONT_EXPORT
  FieldResult(): Valid(false), Field()
    { }

  VTKM_CONT_EXPORT
  FieldResult(const vtkm::cont::Field& f): Valid(true), Field(f)
    { }

  VTKM_CONT_EXPORT
  bool IsValid() const { return this->Valid; }

  VTKM_CONT_EXPORT
  const vtkm::cont::Field& GetField() const { return this->Field; }

  template<typename T, typename StorageTag>
  VTKM_CONT_EXPORT
  bool FieldAs(vtkm::cont::ArrayHandle<T, StorageTag>& dest) const;

  template<typename T, typename StorageTag, typename DerivedPolicy>
  VTKM_CONT_EXPORT
  bool FieldAs(vtkm::cont::ArrayHandle<T, StorageTag>& dest,
               const vtkm::filter::PolicyBase<DerivedPolicy>& policy) const;

private:
  bool Valid;
  vtkm::cont::Field Field;
};

template<class Derived>
class FilterField
{
public:
  VTKM_CONT_EXPORT
  void SetOutputFieldName( const std::string& name )
    { this->OutputFieldName = name; }

  VTKM_CONT_EXPORT
  const std::string& GetOutputFieldName() const
    { return this->OutputFieldName; }

  VTKM_CONT_EXPORT
  FieldResult Execute(const vtkm::cont::DataSet &input, const std::string &inFieldName);

  VTKM_CONT_EXPORT
  FieldResult Execute(const vtkm::cont::DataSet &input, const vtkm::cont::Field &field);

  VTKM_CONT_EXPORT
  FieldResult Execute(const vtkm::cont::DataSet &input, const vtkm::cont::CoordinateSystem &field);


  template<typename DerivedPolicy>
  VTKM_CONT_EXPORT
  FieldResult Execute(const vtkm::cont::DataSet &input,
                      const std::string &inFieldName,
                      const vtkm::filter::PolicyBase<DerivedPolicy>& policy );

  template<typename DerivedPolicy>
  VTKM_CONT_EXPORT
  FieldResult Execute(const vtkm::cont::DataSet &input,
                      const vtkm::cont::Field &field,
                      const vtkm::filter::PolicyBase<DerivedPolicy>& policy );

  template<typename DerivedPolicy>
  VTKM_CONT_EXPORT
  FieldResult Execute(const vtkm::cont::DataSet &input,
                      const vtkm::cont::CoordinateSystem &field,
                      const vtkm::filter::PolicyBase<DerivedPolicy>& policy );


private:

  template<typename DerivedPolicy>
  VTKM_CONT_EXPORT
  FieldResult PrepareForExecution(const vtkm::cont::DataSet& input,
                                  const vtkm::cont::Field& field,
                                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  template<typename DerivedPolicy>
  VTKM_CONT_EXPORT
  FieldResult PrepareForExecution(const vtkm::cont::DataSet& input,
                                  const vtkm::cont::CoordinateSystem& field,
                                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  std::string OutputFieldName;
  vtkm::filter::internal::RuntimeDeviceTracker Tracker;
};

}
} // namespace vtkm::filter


#include <vtkm/filter/FilterField.hxx>

#endif // vtk_m_filter_FieldFilter_h
