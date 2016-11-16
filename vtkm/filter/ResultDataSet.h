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

#ifndef vtk_m_filter_ResultDataSet_h
#define vtk_m_filter_ResultDataSet_h

#include <vtkm/filter/ResultBase.h>

namespace vtkm {
namespace filter {

/// \brief Results for filters that generate new geometry
///
/// \c ResultDataSet contains the results for a filter that generates
/// a wholly new data set (new geometry). Typically little if any data
/// is shared between the filter input and this result.
///
/// Also, data set filters often have secondary operations on the resulting
/// data structure (such as interpolating fields). Thus, this class also
/// allows you to get modifiable versions of the data set.
///
class ResultDataSet : public vtkm::filter::ResultBase
{
public:
  VTKM_CONT
  ResultDataSet() {  }

  VTKM_CONT
  ResultDataSet(const vtkm::cont::DataSet &dataSet)
    : ResultBase(dataSet) {  }

  VTKM_CONT
  const vtkm::cont::DataSet &GetDataSet() const
  {
    return this->ResultBase::GetDataSet();
  }

  VTKM_CONT
  vtkm::cont::DataSet &GetDataSet()
  {
    return this->ResultBase::GetDataSetReference();
  }
};

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ResultDataSet_h
