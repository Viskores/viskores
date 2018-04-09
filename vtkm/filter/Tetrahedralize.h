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

#ifndef vtk_m_filter_Tetrahedralize_h
#define vtk_m_filter_Tetrahedralize_h

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/Tetrahedralize.h>

namespace vtkm
{
namespace filter
{

class Tetrahedralize : public vtkm::filter::FilterDataSet<Tetrahedralize>
{
public:
  VTKM_CONT
  Tetrahedralize();

  template <typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                          const DeviceAdapter& tag);

  // Map new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                            const DeviceAdapter& tag);

private:
  vtkm::worklet::Tetrahedralize Worklet;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/Tetrahedralize.hxx>

#endif // vtk_m_filter_Tetrahedralize_h
