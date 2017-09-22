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

#ifndef vtk_m_filter_Streamline_h
#define vtk_m_filter_Streamline_h

#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/worklet/ParticleAdvection.h>

namespace vtkm
{
namespace filter
{
/// \brief generate isosurface(s) from a Volume

/// Takes as input a volume (e.g., 3D structured point set) and generates on
/// output one or more isosurfaces.
/// Multiple contour values must be specified to generate the isosurfaces.
/// @warning
/// This filter is currently only supports 3D volumes.
class Streamline : public vtkm::filter::FilterDataSetWithField<Streamline>
{
public:
  VTKM_CONT
  Streamline();

  VTKM_CONT
  void SetStepSize(vtkm::Float64 s) { this->StepSize = s; }

  VTKM_CONT
  void SetNumberOfSteps(vtkm::Id n) { this->NumberOfSteps = n; }

  VTKM_CONT
  void SetSeeds(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& seeds);

  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT vtkm::filter::Result DoExecute(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
    const vtkm::filter::FieldMetadata& fieldMeta,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
    const DeviceAdapter& tag);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT bool DoMapField(vtkm::filter::Result& result,
                            const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                            const DeviceAdapter& tag);

private:
  vtkm::worklet::Streamline Worklet;
  vtkm::Float64 StepSize;
  vtkm::Id NumberOfSteps;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> Seeds;
};

template <>
class FilterTraits<Streamline>
{
public:
  struct TypeListTagStreamline
    : vtkm::ListTagBase<vtkm::Vec<vtkm::Float32, 3>, vtkm::Vec<vtkm::Float64, 3>>
  {
  };
  typedef TypeListTagStreamline InputFieldTypeList;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/Streamline.hxx>

#endif // vtk_m_filter_Streamline_h
