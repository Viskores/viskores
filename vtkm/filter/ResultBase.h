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

#ifndef vtk_m_filter_ResultBase_h
#define vtk_m_filter_ResultBase_h

#include <vtkm/cont/DataSet.h>

namespace vtkm {
namespace filter {

/// \brief Base class for result returned from a filter.
///
/// \c ResultBase is the base class for the return value from any filter. It
/// contains a valid flag that signals whether the filter successfully
/// executed. Also, every filter produces some data on a data set. The
/// resulting data set is also available from this base clase.
///
/// Subclasses may define additional data (usually subparts of the data set)
/// specific to the type of operation.
///
class ResultBase
{
public:
  /// Returns true if these results are from a successful execution of a
  /// filter.
  ///
  VTKM_CONT
  bool IsValid() const { return this->Valid; }

  /// Returns the results of the filter in terms of a \c DataSet.
  ///
  VTKM_CONT
  const vtkm::cont::DataSet &GetDataSet() const { return this->Data; }

  /// Returns the results of the filter in terms of a writable \c DataSet.
  VTKM_CONT
  vtkm::cont::DataSet &GetDataSet() { return this->Data; }

protected:
  VTKM_CONT
  ResultBase(): Valid(false) {  }

  VTKM_CONT
  ResultBase(const vtkm::cont::DataSet &dataSet)
    : Valid(true), Data(dataSet) {  }

  VTKM_CONT
  void SetValid(bool valid)
  {
    this->Valid = valid;
  }

  VTKM_CONT
  void SetDataSet(const vtkm::cont::DataSet &dataSet)
  {
    this->Data = dataSet;
    this->SetValid(true);
  }

private:
  bool Valid;
  vtkm::cont::DataSet Data;
};

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ResultBase_h
