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

#ifndef vtk_m_filter_CellAverage_h
#define vtk_m_filter_CellAverage_h

#include <vtkm/filter/CellFilter.h>
#include <vtkm/worklet/CellAverage.h>

namespace vtkm {
namespace filter {

class CellAverage : public vtkm::filter::CellFilter<CellAverage>
{
public:
  VTKM_CONT_EXPORT
  CellAverage();

  template<typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT_EXPORT
  vtkm::filter::FieldResult DoExecute(const vtkm::cont::DataSet &input,
                                      const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                      const vtkm::filter::FieldMetadata& fieldMeta,
                                      const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                      const DeviceAdapter& tag);

private:
  vtkm::worklet::CellAverage Worklet;

};

}
} // namespace vtkm::filter


#include <vtkm/filter/CellAverage.hxx>

#endif // vtk_m_filter_CellAverage_h
