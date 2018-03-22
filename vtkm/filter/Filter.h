//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_filter_Filter_h
#define vtk_m_filter_Filter_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/MultiBlock.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/filter/FieldSelection.h>
#include <vtkm/filter/PolicyBase.h>


namespace vtkm
{
namespace filter
{
template <typename Derived>
class Filter
{
public:
  VTKM_CONT
  Filter();

  VTKM_CONT
  ~Filter();

  //@{
  /// The runtime device tracker determines what devices the filter will be executed on.
  ///
  VTKM_CONT
  void SetRuntimeDeviceTracker(const vtkm::cont::RuntimeDeviceTracker& tracker)
  {
    this->Tracker = tracker;
  }

  VTKM_CONT
  const vtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker() const { return this->Tracker; }
  VTKM_CONT
  vtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker() { return this->Tracker; }
  //@}

  //@{
  /// \brief Specify which fields get passed from input to output.
  ///
  /// After a filter successfully executes and returns a new data set, fields are mapped from
  /// input to output. Depending on what operation the filter does, this could be a simple shallow
  /// copy of an array, or it could be a computed operation. You can control which fields are
  /// passed (and equivalently which are not) with this parameter.
  ///
  /// By default, all fields are passed during execution.
  ///
  VTKM_CONT
  void SetFieldsToPass(const vtkm::filter::FieldSelection& fieldsToPass)
  {
    this->FieldsToPass = fieldsToPass;
  }

  VTKM_CONT
  void SetFieldsToPass(const vtkm::filter::FieldSelection& fieldsToPass,
                       vtkm::filter::FieldSelection::ModeEnum mode)
  {
    this->FieldsToPass = fieldsToPass;
    this->FieldsToPass.SetMode(mode);
  }

  VTKM_CONT
  void SetFieldsToPass(
    const std::string& fieldname,
    vtkm::cont::Field::AssociationEnum association,
    vtkm::filter::FieldSelection::ModeEnum mode = vtkm::filter::FieldSelection::MODE_SELECT)
  {
    this->SetFieldsToPass({ fieldname, association }, mode);
  }

  VTKM_CONT
  const vtkm::filter::FieldSelection& GetFieldsToPass() const { return this->FieldsToPass; }
  VTKM_CONT
  vtkm::filter::FieldSelection& GetFieldsToPass() { return this->FieldsToPass; }
  //@}

  //@{
  /// Executes the filter on the input and producer an result dataset.
  ///
  /// On success, this the dataset produced. On error, vtkm::cont::ErrorExecution will be thrown.
  VTKM_CONT
  vtkm::cont::DataSet Execute(const vtkm::cont::DataSet& input);

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet Execute(const vtkm::cont::DataSet& input,
                                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy);
  //@}

  //@{
  /// MultiBlock variants of execute.
  VTKM_CONT
  vtkm::cont::MultiBlock Execute(const vtkm::cont::MultiBlock& input);

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::MultiBlock Execute(const vtkm::cont::MultiBlock& input,
                                           const vtkm::filter::PolicyBase<DerivedPolicy>& policy);
  //@}

private:
  vtkm::cont::RuntimeDeviceTracker Tracker;
  vtkm::filter::FieldSelection FieldsToPass;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/Filter.hxx>
#endif
