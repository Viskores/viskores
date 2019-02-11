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
#ifndef vtk_m_filter_AddGhostZone_h
#define vtk_m_filter_AddGhostZone_h

#include <vtkm/filter/FilterDataSet.h>

namespace vtkm
{
namespace filter
{

struct AddGhostZonePolicy : vtkm::filter::PolicyBase<AddGhostZonePolicy>
{
  using FieldTypeList = vtkm::ListTagBase<vtkm::UInt8>;
};

class AddGhostZone : public vtkm::filter::FilterDataSet<AddGhostZone>
{
public:
  VTKM_CONT
  AddGhostZone();

  template <typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData,
                                          vtkm::filter::PolicyBase<Policy> policy);

  template <typename ValueType, typename Storage, typename Policy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<ValueType, Storage>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<Policy>);

private:
};
}
} // namespace vtkm::filter

#include <vtkm/filter/AddGhostZone.hxx>

#endif //vtk_m_filter_AddGhostZone_h
