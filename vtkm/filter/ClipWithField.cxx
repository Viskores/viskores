//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define vtkm_filter_Clip_cxx

#include <vtkm/filter/ClipWithField.h>

#include <vtkm/filter/MapFieldPermutation.h>

namespace vtkm
{
namespace filter
{

VTKM_FILTER_EXPORT bool ClipWithField::MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                                          const vtkm::cont::Field& field)
{
  if (field.IsFieldPoint())
  {
    // Handled by DoMapField, which the superclass will call.
    // Actually already done by other version of MapFieldOntoOutput. (Stupid policies.)
    //return this->FilterDataSetWithField<ClipWithField>::MapFieldOntoOutput(result, field, policy);
    VTKM_ASSERT(false && "Should not be here");
    return false;
  }
  else if (field.IsFieldCell())
  {
    vtkm::cont::ArrayHandle<vtkm::Id> permutation = this->Worklet.GetCellMapOutputToInput();
    return vtkm::filter::MapFieldPermutation(field, permutation, result);
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
VTKM_FILTER_INSTANTIATE_EXECUTE_METHOD(ClipWithField);
}
}
