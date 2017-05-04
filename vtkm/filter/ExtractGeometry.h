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

#ifndef vtk_m_filter_ExtractGeometry_h
#define vtk_m_filter_ExtractGeometry_h

#include <vtkm/cont/ImplicitFunction.h>
#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/ExtractGeometry.h>

namespace vtkm {
namespace filter {

class ExtractGeometry : public vtkm::filter::FilterDataSet<ExtractGeometry>
{
public:
  VTKM_CONT
  ExtractGeometry();

  // Set the volume of interest to extract
  template <typename ImplicitFunctionType, typename DerivedPolicy>
  void SetImplicitFunction(const std::shared_ptr<ImplicitFunctionType>& func,
                           const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  template <typename ImplicitFunctionType>
  void SetImplicitFunction(const std::shared_ptr<ImplicitFunctionType> &func)
  {
    this->Function = func;
  }

  std::shared_ptr<vtkm::cont::ImplicitFunction> GetImplicitFunction() const
  {
    return this->Function;
  }

  VTKM_CONT
  bool GetExtractInside()                       { return this->ExtractInside; }
  VTKM_CONT
  void SetExtractInside(bool value)             { this->ExtractInside = value; }
  VTKM_CONT
  void ExtractInsideOn()                        { this->ExtractInside = true; }
  VTKM_CONT
  void ExtractInsideOff()                       { this->ExtractInside = false; }

  VTKM_CONT
  bool GetExtractBoundaryCells()                { return this->ExtractBoundaryCells; }
  VTKM_CONT
  void SetExtractBoundaryCells(bool value)      { this->ExtractBoundaryCells = value; }
  VTKM_CONT
  void ExtractBoundaryCellsOn()                 { this->ExtractBoundaryCells = true; }
  VTKM_CONT
  void ExtractBoundaryCellsOff()                { this->ExtractBoundaryCells = false; }
  
  VTKM_CONT
  bool GetExtractOnlyBoundaryCells()            { return this->ExtractOnlyBoundaryCells; }
  VTKM_CONT
  void SetExtractOnlyBoundaryCells(bool value)  { this->ExtractOnlyBoundaryCells = value; }
  VTKM_CONT
  void ExtractOnlyBoundaryCellsOn()             { this->ExtractOnlyBoundaryCells = true; }
  VTKM_CONT
  void ExtractOnlyBoundaryCellsOff()            { this->ExtractOnlyBoundaryCells = false; }
  

  template<typename DerivedPolicy, typename DeviceAdapter>
  vtkm::filter::ResultDataSet DoExecute(const vtkm::cont::DataSet& input,
                                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                        const DeviceAdapter& tag);

  //Map a new field onto the resulting dataset after running the filter
  template<typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  bool DoMapField(vtkm::filter::ResultDataSet& result,
                  const vtkm::cont::ArrayHandle<T, StorageType>& input,
                  const vtkm::filter::FieldMetadata& fieldMeta,
                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                  const DeviceAdapter& tag);

private:
  bool ExtractInside;
  bool ExtractBoundaryCells;
  bool ExtractOnlyBoundaryCells;
  std::shared_ptr<vtkm::cont::ImplicitFunction> Function;

  //vtkm::worklet::ExtractGeometry Worklet;
  vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};

template<>
class FilterTraits<ExtractGeometry>
{ //currently the ExtractGeometry filter only works on scalar data.
public:
  typedef TypeListTagScalarAll InputFieldTypeList;
};

}
} // namespace vtkm::filter


#include <vtkm/filter/ExtractGeometry.hxx>

#endif // vtk_m_filter_ExtractGeometry_h
