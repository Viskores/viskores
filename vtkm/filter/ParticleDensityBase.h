//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particle_density_base_h
#define vtk_m_filter_particle_density_base_h

#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace filter
{
// We only need the CoordinateSystem and scalar fields of the input dataset thus a FilterField
template <typename Derived>
class ParticleDensityBase : public vtkm::filter::FilterDataSetWithField<Derived>
{
public:
  // deposit scalar field associated with particles, e.g. mass/charge to mesh cells
  using SupportedTypes = vtkm::TypeListFieldScalar;

protected:
  ParticleDensityBase(const vtkm::Id3& dimension,
                      const vtkm::Vec3f& origin,
                      const vtkm::Vec3f& spacing)
    : Dimension(dimension)
    , Origin(origin)
    , Spacing(spacing)
    , ComputeNumberDensity(false)
    , DivideByVolume(true)
  {
  }

  ParticleDensityBase(const vtkm::Id3& dimension, const vtkm::Bounds& bounds)
    : Dimension(dimension)
    , Origin({ static_cast<vtkm::FloatDefault>(bounds.X.Min),
               static_cast<vtkm::FloatDefault>(bounds.Y.Min),
               static_cast<vtkm::FloatDefault>(bounds.Z.Min) })
    , Spacing(vtkm::Vec3f{ static_cast<vtkm::FloatDefault>(bounds.X.Length()),
                           static_cast<vtkm::FloatDefault>(bounds.Y.Length()),
                           static_cast<vtkm::FloatDefault>(bounds.Z.Length()) } /
              Dimension)
    , ComputeNumberDensity(false)
    , DivideByVolume(true)
  {
  }

public:
  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet PrepareForExecution(const vtkm::cont::DataSet& input,
                                                    vtkm::filter::PolicyBase<DerivedPolicy> policy)
  {
    if (this->ComputeNumberDensity)
    {
      return static_cast<Derived*>(this)->DoExecute(
        input,
        vtkm::cont::make_ArrayHandleConstant(vtkm::FloatDefault{ 1 }, input.GetNumberOfPoints()),
        vtkm::filter::FieldMetadata{}, // Ignored
        policy);
    }
    else
    {
      return this->FilterDataSetWithField<Derived>::PrepareForExecution(input, policy);
    }
  }

  template <typename T, typename StorageType, typename Policy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet&,
                            const vtkm::cont::ArrayHandle<T, StorageType>&,
                            const vtkm::filter::FieldMetadata&,
                            vtkm::filter::PolicyBase<Policy>)
  {
    return false;
  }

  VTKM_CONT void SetComputeNumberDensity(bool yes) { this->ComputeNumberDensity = yes; }

  VTKM_CONT bool GetComputeNumberDensity() const { return this->ComputeNumberDensity; }

  VTKM_CONT void SetDivideByVolume(bool yes) { this->DivideByVolume = yes; }

  VTKM_CONT bool GetDivideByVolume() const { return this->DivideByVolume; }

protected:
  vtkm::Id3 Dimension; // Cell dimension
  vtkm::Vec3f Origin;
  vtkm::Vec3f Spacing;
  bool ComputeNumberDensity;
  bool DivideByVolume;

public:
  // conceptually protected but CUDA needs this to be public
  class DivideByVolumeWorklet : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldInOut field);
    using ExecutionSignature = void(_1);

    VTKM_EXEC_CONT
    explicit DivideByVolumeWorklet(vtkm::Float64 volume)
      : Volume(volume)
    {
    }

    template <typename T>
    VTKM_EXEC void operator()(T& value) const
    {
      value = static_cast<T>(value / Volume);
    }

  private:
    vtkm::Float64 Volume;
  }; // class DivideByVolumeWorklet
};
}
}
#endif //vtk_m_filter_particle_density_base_h
