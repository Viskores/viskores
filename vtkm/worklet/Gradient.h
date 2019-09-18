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

#include <vtkm/worklet/gradient/CellGradient.h>
#include <vtkm/worklet/gradient/Divergence.h>
#include <vtkm/worklet/gradient/GradientOutput.h>
#include <vtkm/worklet/gradient/PointGradient.h>
#include <vtkm/worklet/gradient/QCriterion.h>
#include <vtkm/worklet/gradient/StructuredPointGradient.h>
#include <vtkm/worklet/gradient/Transpose.h>
#include <vtkm/worklet/gradient/Vorticity.h>

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
  void operator()(const CellSetType& cellset) const
  {
    vtkm::worklet::DispatcherMapTopology<PointGradient<T>> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      cellset, //whole cellset in
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  void operator()(const vtkm::cont::CellSetStructured<3>& cellset) const
  {
    vtkm::worklet::DispatcherPointNeighborhood<StructuredPointGradient<T>> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  template <typename PermIterType>
  void operator()(const vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<3>,
                                                       PermIterType>& cellset) const
  {
    vtkm::worklet::DispatcherPointNeighborhood<StructuredPointGradient<T>> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  void operator()(const vtkm::cont::CellSetStructured<2>& cellset) const
  {
    vtkm::worklet::DispatcherPointNeighborhood<StructuredPointGradient<T>> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  template <typename PermIterType>
  void operator()(const vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<2>,
                                                       PermIterType>& cellset) const
  {
    vtkm::worklet::DispatcherPointNeighborhood<StructuredPointGradient<T>> dispatcher;
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
                                               const vtkm::cont::ArrayHandle<T, S>& field)
  {
    vtkm::worklet::GradientOutputFields<T> extraOutput(true, false, false, false);
    return this->Run(cells, coords, field, extraOutput);
  }

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
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> Run(const CellSetType& cells,
                                               const CoordinateSystem& coords,
                                               const vtkm::cont::ArrayHandle<T, S>& field)
  {
    vtkm::worklet::GradientOutputFields<T> extra(true, false, false, false);
    return this->Run(cells, coords, field, extra);
  }

  template <typename CellSetType, typename CoordinateSystem, typename T, typename S>
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> Run(const CellSetType& cells,
                                               const CoordinateSystem& coords,
                                               const vtkm::cont::ArrayHandle<T, S>& field,
                                               GradientOutputFields<T>& extraOutput)
  {
    using DispatcherType =
      vtkm::worklet::DispatcherMapTopology<vtkm::worklet::gradient::CellGradient<T>>;

    vtkm::worklet::gradient::CellGradient<T> worklet;
    DispatcherType dispatcher(worklet);

    dispatcher.Invoke(cells, coords, field, extraOutput);
    return extraOutput.Gradient;
  }
};
}
} // namespace vtkm::worklet
#endif
