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

  VTKM_CONT
  void SetRuntimeDeviceTracker(const vtkm::cont::RuntimeDeviceTracker& tracker)
  {
    this->Tracker = tracker;
  }

  VTKM_CONT
  const vtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker() const { return this->Tracker; }
  VTKM_CONT
  vtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker() { return this->Tracker; }

  //@{
  /// Executes the filter on the input and producer an result dataset.
  /// FieldSelection can be specified to indicate which fields should be passed
  /// on from the input to the output.
  ///
  /// On success, this the dataset produced. On error, vtkm::cont::ErrorExecution will be thrown.
  VTKM_CONT
  vtkm::cont::DataSet Execute(const vtkm::cont::DataSet& input,
                              const FieldSelection& fieldSelection = FieldSelection());

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet Execute(const vtkm::cont::DataSet& input,
                                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                        const FieldSelection& fieldSelection = FieldSelection());
  //@}

  //@{
  /// MultiBlock variants of execute.
  VTKM_CONT
  vtkm::cont::MultiBlock Execute(const vtkm::cont::MultiBlock& input,
                                 const FieldSelection& fieldSelection = FieldSelection());

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::MultiBlock Execute(const vtkm::cont::MultiBlock& input,
                                           const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                           const FieldSelection& fieldSelection = FieldSelection());
  //@}

private:
  vtkm::cont::RuntimeDeviceTracker Tracker;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/Filter.hxx>
#endif
