//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_Gradient_h
#define vtk_m_worklet_Gradient_h

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/DispatcherPointNeighborhood.h>

#include <vtkm/filter/vector_analysis/worklet/gradient/CellGradient.h>
#include <vtkm/filter/vector_analysis/worklet/gradient/Divergence.h>
#include <vtkm/filter/vector_analysis/worklet/gradient/GradientOutput.h>
#include <vtkm/filter/vector_analysis/worklet/gradient/PointGradient.h>
#include <vtkm/filter/vector_analysis/worklet/gradient/QCriterion.h>
#include <vtkm/filter/vector_analysis/worklet/gradient/StructuredPointGradient.h>
#include <vtkm/filter/vector_analysis/worklet/gradient/Transpose.h>
#include <vtkm/filter/vector_analysis/worklet/gradient/Vorticity.h>

// Required for instantiations
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/UnknownCellSet.h>
#include <vtkm/internal/Instantiations.h>

#ifndef vtkm_GradientInstantiation
// Turn this on to check to see if all the instances of the gradient worklet are covered
// in external instances. If they are not, you will get a compile error.
//#define VTKM_GRADIENT_CHECK_WORKLET_INSTANCES
#endif

namespace vtkm
{
namespace worklet
{

template <typename T>
struct GradientOutputFields;

namespace gradient
{

//-----------------------------------------------------------------------------
template <typename CoordinateSystem, typename T, typename S>
struct DeducedPointGrad
{
  DeducedPointGrad(const CoordinateSystem& coords,
                   const vtkm::cont::ArrayHandle<T, S>& field,
                   GradientOutputFields<T>* result)
    : Points(&coords)
    , Field(&field)
    , Result(result)
  {
  }

  template <typename CellSetType>
  void operator()(const CellSetType& cellset) const;

  template <typename CellSetType>
  void Go(const CellSetType& cellset) const
  {
    vtkm::worklet::DispatcherMapTopology<PointGradient> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      cellset, //whole cellset in
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  void Go(const vtkm::cont::CellSetStructured<3>& cellset) const
  {
    vtkm::worklet::DispatcherPointNeighborhood<StructuredPointGradient> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  template <typename PermIterType>
  void Go(const vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<3>, PermIterType>&
            cellset) const
  {
    vtkm::worklet::DispatcherPointNeighborhood<StructuredPointGradient> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  void Go(const vtkm::cont::CellSetStructured<2>& cellset) const
  {
    vtkm::worklet::DispatcherPointNeighborhood<StructuredPointGradient> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  template <typename PermIterType>
  void Go(const vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<2>, PermIterType>&
            cellset) const
  {
    vtkm::worklet::DispatcherPointNeighborhood<StructuredPointGradient> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }


  const CoordinateSystem* const Points;
  const vtkm::cont::ArrayHandle<T, S>* const Field;
  GradientOutputFields<T>* Result;

private:
  void operator=(const DeducedPointGrad<CoordinateSystem, T, S>&) = delete;
};

#ifndef VTKM_GRADIENT_CHECK_WORKLET_INSTANCES
// Declare the methods that get instances outside of the class so that they are not inline.
// If they are inline, the compiler may decide to compile them anyway.
template <typename CoordinateSystem, typename T, typename S>
template <typename CellSetType>
void DeducedPointGrad<CoordinateSystem, T, S>::operator()(const CellSetType& cellset) const
{
  this->Go(cellset);
}
#endif

} //namespace gradient

template <typename T>
struct GradientOutputFields : public vtkm::cont::ExecutionObjectBase
{

  using ValueType = T;
  using BaseTType = typename vtkm::VecTraits<T>::BaseComponentType;

  template <typename DeviceAdapter>
  struct ExecutionTypes
  {
    using Portal = vtkm::exec::GradientOutput<T>;
  };

  GradientOutputFields()
    : Gradient()
    , Divergence()
    , Vorticity()
    , QCriterion()
    , StoreGradient(true)
    , ComputeDivergence(false)
    , ComputeVorticity(false)
    , ComputeQCriterion(false)
  {
  }

  GradientOutputFields(bool store, bool divergence, bool vorticity, bool qc)
    : Gradient()
    , Divergence()
    , Vorticity()
    , QCriterion()
    , StoreGradient(store)
    , ComputeDivergence(divergence)
    , ComputeVorticity(vorticity)
    , ComputeQCriterion(qc)
  {
  }

  /// Add divergence field to the output data.
  /// The input array must have 3 components in order to compute this.
  /// The default is off.
  void SetComputeDivergence(bool enable) { ComputeDivergence = enable; }
  bool GetComputeDivergence() const { return ComputeDivergence; }

  /// Add voriticity/curl field to the output data.
  /// The input array must have 3 components in order to compute this.
  /// The default is off.
  void SetComputeVorticity(bool enable) { ComputeVorticity = enable; }
  bool GetComputeVorticity() const { return ComputeVorticity; }

  /// Add Q-criterion field to the output data.
  /// The input array must have 3 components in order to compute this.
  /// The default is off.
  void SetComputeQCriterion(bool enable) { ComputeQCriterion = enable; }
  bool GetComputeQCriterion() const { return ComputeQCriterion; }

  /// Add gradient field to the output data.
  /// The input array must have 3 components in order to disable this.
  /// The default is on.
  void SetComputeGradient(bool enable) { StoreGradient = enable; }
  bool GetComputeGradient() const { return StoreGradient; }

  //todo fix this for scalar
  vtkm::exec::GradientOutput<T> PrepareForOutput(vtkm::Id size)
  {
    vtkm::exec::GradientOutput<T> portal(this->StoreGradient,
                                         this->ComputeDivergence,
                                         this->ComputeVorticity,
                                         this->ComputeQCriterion,
                                         this->Gradient,
                                         this->Divergence,
                                         this->Vorticity,
                                         this->QCriterion,
                                         size);
    return portal;
  }

  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> Gradient;
  vtkm::cont::ArrayHandle<BaseTType> Divergence;
  vtkm::cont::ArrayHandle<vtkm::Vec<BaseTType, 3>> Vorticity;
  vtkm::cont::ArrayHandle<BaseTType> QCriterion;

private:
  bool StoreGradient;
  bool ComputeDivergence;
  bool ComputeVorticity;
  bool ComputeQCriterion;
};
class PointGradient
{
public:
  template <typename CellSetType, typename CoordinateSystem, typename T, typename S>
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> Run(const CellSetType& cells,
                                               const CoordinateSystem& coords,
                                               const vtkm::cont::ArrayHandle<T, S>& field,
                                               GradientOutputFields<T>& extraOutput)
  {
    //we are using cast and call here as we pass the cells twice to the invoke
    //and want the type resolved once before hand instead of twice
    //by the dispatcher ( that will cost more in time and binary size )
    gradient::DeducedPointGrad<CoordinateSystem, T, S> func(coords, field, &extraOutput);
    vtkm::cont::CastAndCall(cells, func);
    return extraOutput.Gradient;
  }
};

class CellGradient
{
public:
  template <typename CellSetType, typename CoordinateSystem, typename T, typename S>
  static vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> Run(const CellSetType& cells,
                                                      const CoordinateSystem& coords,
                                                      const vtkm::cont::ArrayHandle<T, S>& field,
                                                      GradientOutputFields<T>& extraOutput);
};

#ifndef VTKM_GRADIENT_CHECK_WORKLET_INSTANCES
// Declare the methods that get instances outside of the class so that they are not inline.
// If they are inline, the compiler may decide to compile them anyway.
template <typename CellSetType, typename CoordinateSystem, typename T, typename S>
vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> CellGradient::Run(
  const CellSetType& cells,
  const CoordinateSystem& coords,
  const vtkm::cont::ArrayHandle<T, S>& field,
  GradientOutputFields<T>& extraOutput)
{
  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::gradient::CellGradient> dispatcher;
  dispatcher.Invoke(cells, coords, field, extraOutput);
  return extraOutput.Gradient;
}
#endif

}
} // namespace vtkm::worklet

//==============================================================================
//---------------------------------------------------------------------------
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Float32,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetStructured<3>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Float64,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetStructured<3>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_32,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetStructured<3>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_64,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetStructured<3>&) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_32,
  vtkm::cont::StorageTagSOA>::operator()(const vtkm::cont::CellSetStructured<3>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_64,
  vtkm::cont::StorageTagSOA>::operator()(const vtkm::cont::CellSetStructured<3>&) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_32,
  vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic>>::
operator()(const vtkm::cont::CellSetStructured<3>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_64,
  vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic>>::
operator()(const vtkm::cont::CellSetStructured<3>&) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f,
  vtkm::cont::StorageTagUniformPoints>::operator()(const vtkm::cont::CellSetStructured<3>&) const;
VTKM_INSTANTIATION_END


//---------------------------------------------------------------------------
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Float32,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetStructured<2>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Float64,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetStructured<2>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_32,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetStructured<2>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_64,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetStructured<2>&) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_32,
  vtkm::cont::StorageTagSOA>::operator()(const vtkm::cont::CellSetStructured<2>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_64,
  vtkm::cont::StorageTagSOA>::operator()(const vtkm::cont::CellSetStructured<2>&) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_32,
  vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic>>::
operator()(const vtkm::cont::CellSetStructured<2>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_64,
  vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic>>::
operator()(const vtkm::cont::CellSetStructured<2>&) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f,
  vtkm::cont::StorageTagUniformPoints>::operator()(const vtkm::cont::CellSetStructured<2>&) const;
VTKM_INSTANTIATION_END


//---------------------------------------------------------------------------
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Float32,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetExplicit<>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Float64,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetExplicit<>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_32,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetExplicit<>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_64,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetExplicit<>&) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_32,
  vtkm::cont::StorageTagSOA>::operator()(const vtkm::cont::CellSetExplicit<>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_64,
  vtkm::cont::StorageTagSOA>::operator()(const vtkm::cont::CellSetExplicit<>&) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_32,
  vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic>>::
operator()(const vtkm::cont::CellSetExplicit<>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_64,
  vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic>>::
operator()(const vtkm::cont::CellSetExplicit<>&) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f,
  vtkm::cont::StorageTagUniformPoints>::operator()(const vtkm::cont::CellSetExplicit<>&) const;
VTKM_INSTANTIATION_END


//---------------------------------------------------------------------------
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Float32,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetSingleType<>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Float64,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetSingleType<>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_32,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetSingleType<>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_64,
  vtkm::cont::StorageTagBasic>::operator()(const vtkm::cont::CellSetSingleType<>&) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_32,
  vtkm::cont::StorageTagSOA>::operator()(const vtkm::cont::CellSetSingleType<>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_64,
  vtkm::cont::StorageTagSOA>::operator()(const vtkm::cont::CellSetSingleType<>&) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_32,
  vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic>>::
operator()(const vtkm::cont::CellSetSingleType<>&) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f_64,
  vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic>>::
operator()(const vtkm::cont::CellSetSingleType<>&) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::gradient::DeducedPointGrad<
  vtkm::cont::CoordinateSystem,
  vtkm::Vec3f,
  vtkm::cont::StorageTagUniformPoints>::operator()(const vtkm::cont::CellSetSingleType<>&) const;
VTKM_INSTANTIATION_END



//==============================================================================
//---------------------------------------------------------------------------
VTKM_INSTANTIATION_BEGIN
extern template vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>>
vtkm::worklet::CellGradient::Run(
  const vtkm::cont::UnknownCellSet&,
  const vtkm::cont::CoordinateSystem&,
  const vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>&,
  GradientOutputFields<vtkm::Float32>&);
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>>
vtkm::worklet::CellGradient::Run(
  const vtkm::cont::UnknownCellSet&,
  const vtkm::cont::CoordinateSystem&,
  const vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>&,
  GradientOutputFields<vtkm::Float64>&);
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec3f_32, 3>>
vtkm::worklet::CellGradient::Run(
  const vtkm::cont::UnknownCellSet&,
  const vtkm::cont::CoordinateSystem&,
  const vtkm::cont::ArrayHandle<vtkm::Vec3f_32, vtkm::cont::StorageTagBasic>&,
  GradientOutputFields<vtkm::Vec3f_32>&);
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec3f_64, 3>>
vtkm::worklet::CellGradient::Run(
  const vtkm::cont::UnknownCellSet&,
  const vtkm::cont::CoordinateSystem&,
  const vtkm::cont::ArrayHandle<vtkm::Vec3f_64, vtkm::cont::StorageTagBasic>&,
  GradientOutputFields<vtkm::Vec3f_64>&);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec3f_32, 3>>
vtkm::worklet::CellGradient::Run(
  const vtkm::cont::UnknownCellSet&,
  const vtkm::cont::CoordinateSystem&,
  const vtkm::cont::ArrayHandle<vtkm::Vec3f_32, vtkm::cont::StorageTagSOA>&,
  GradientOutputFields<vtkm::Vec3f_32>&);
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec3f_64, 3>>
vtkm::worklet::CellGradient::Run(
  const vtkm::cont::UnknownCellSet&,
  const vtkm::cont::CoordinateSystem&,
  const vtkm::cont::ArrayHandle<vtkm::Vec3f_64, vtkm::cont::StorageTagSOA>&,
  GradientOutputFields<vtkm::Vec3f_64>&);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec3f_32, 3>>
vtkm::worklet::CellGradient::Run(
  const vtkm::cont::UnknownCellSet&,
  const vtkm::cont::CoordinateSystem&,
  const vtkm::cont::ArrayHandle<
    vtkm::Vec3f_32,
    vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                           vtkm::cont::StorageTagBasic,
                                           vtkm::cont::StorageTagBasic>>&,
  GradientOutputFields<vtkm::Vec3f_32>&);
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec3f_64, 3>>
vtkm::worklet::CellGradient::Run(
  const vtkm::cont::UnknownCellSet&,
  const vtkm::cont::CoordinateSystem&,
  const vtkm::cont::ArrayHandle<
    vtkm::Vec3f_64,
    vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                           vtkm::cont::StorageTagBasic,
                                           vtkm::cont::StorageTagBasic>>&,
  GradientOutputFields<vtkm::Vec3f_64>&);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec3f, 3>> vtkm::worklet::CellGradient::Run(
  const vtkm::cont::UnknownCellSet&,
  const vtkm::cont::CoordinateSystem&,
  const vtkm::cont::ArrayHandle<vtkm::Vec3f, vtkm::cont::StorageTagUniformPoints>&,
  GradientOutputFields<vtkm::Vec3f>&);
VTKM_INSTANTIATION_END

#endif
