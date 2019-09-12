//============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=======================================================================
#ifndef vtk_m_worklet_moments_ComputeMoments_h
#define vtk_m_worklet_moments_ComputeMoments_h

#include <vtkm/Math.h>
#include <vtkm/worklet/WorkletPointNeighborhood.h>

#include <vtkm/cont/Field.h>

#include <vtkm/exec/BoundaryState.h>

#include <cassert>
#include <string>

namespace vtkm
{
namespace worklet
{
namespace moments
{

struct ComputeMoments2D : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  ComputeMoments2D(const vtkm::Vec<double, 3>& _spacing, const double _radius, int _p, int _q)
    : Spacing(_spacing)
    , Radius(_radius)
    , p(_p)
    , q(_q)
  {
    assert(_spacing[0] > 1e-10);
    assert(_spacing[1] > 1e-10);
    assert(_spacing[2] > 1e-10);

    assert(_p >= 0);
    assert(_q >= 0);
  }

  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);

  using ExecutionSignature = void(_2, Boundary, _3);

  template <typename NeighIn, typename T>
  VTKM_EXEC void operator()(const NeighIn& image,
                            const vtkm::exec::BoundaryState& boundary,
                            T& moment) const
  {
    // TODO: type safety and numerical precision
    auto sum = vtkm::TypeTraits<T>::ZeroInitialization();
    //    vtkm::Vec<vtkm::Float64, 2> recp{ 1.0 / RadiusReal[0], 1.0 / RadiusReal[1] };
    vtkm::Vec<vtkm::Int32, 3> RadiusDiscrete = { this->Radius / (this->Spacing[0] - 1e-10),
                                                 this->Radius / (this->Spacing[1] - 1e-10),
                                                 this->Radius / (this->Spacing[2] - 1e-10) };

    // Clamp the radius to the dataset bounds (discard out-of-bounds points).
    const auto minRadius = boundary.ClampNeighborIndex(-RadiusDiscrete);
    const auto maxRadius = boundary.ClampNeighborIndex(RadiusDiscrete);

    for (vtkm::IdComponent j = minRadius[1]; j <= maxRadius[1]; ++j)
    {
      if (j > -RadiusDiscrete[1] && boundary.IJK[1] + j == 0)
      { // Don't double count samples that exist on other nodes:
        continue;
      }

      for (vtkm::IdComponent i = minRadius[0]; i <= maxRadius[0]; ++i)
      {
        if (i > -RadiusDiscrete[0] && boundary.IJK[0] + i == 0)
        { // Don't double count samples that exist on other nodes:
          continue;
        }

        const vtkm::Float64 r0 = i * 1. / RadiusDiscrete[0];
        const vtkm::Float64 r1 = j * 1. / RadiusDiscrete[1];

        if (r0 * r0 + r1 * r1 <= 1)
        {
          sum += pow(r0, p) * pow(r1, q) * image.Get(i, j, 0);
        }
      }
    }

    moment = T(sum * Spacing[0] * Spacing[1]);
  }

private:
  vtkm::Vec<vtkm::Int32, 3> RadiusDiscrete;
  const double Radius;
  const vtkm::Vec<double, 3>& Spacing;
  const int p;
  const int q;
};

struct ComputeMoments3D : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  ComputeMoments3D(const vtkm::Vec<double, 3>& _spacing,
                   const double _radius,
                   int _p,
                   int _q,
                   int _r)
    : Spacing(_spacing)
    , Radius(_radius)
    , p(_p)
    , q(_q)
    , r(_r)
  {
    assert(_spacing[0] > 1e-10);
    assert(_spacing[1] > 1e-10);
    assert(_spacing[2] > 1e-10);

    assert(_p >= 0);
    assert(_q >= 0);
    assert(_r >= 0);
  }

  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);

  using ExecutionSignature = void(_2, Boundary, _3);

  template <typename NeighIn, typename T>
  VTKM_EXEC void operator()(const NeighIn& image,
                            const vtkm::exec::BoundaryState& boundary,
                            T& moment) const
  {
    // TODO: type safety and numerical precision
    auto sum = vtkm::TypeTraits<T>::ZeroInitialization();
    //    const vtkm::Vec<vtkm::Float64, 3> recp{ 1.0 / this->RadiusReal[0],
    //                                            1.0 / this->RadiusReal[1],
    //                                            1.0 / this->RadiusReal[2] };
    vtkm::Vec<vtkm::Int32, 3> RadiusDiscrete = { this->Radius / (this->Spacing[0] - 1e-10),
                                                 this->Radius / (this->Spacing[1] - 1e-10),
                                                 this->Radius / (this->Spacing[2] - 1e-10) };

    // Clamp the radius to the dataset bounds (discard out-of-bounds points).
    const auto minRadius = boundary.ClampNeighborIndex(-RadiusDiscrete);
    const auto maxRadius = boundary.ClampNeighborIndex(RadiusDiscrete);

    for (vtkm::IdComponent k = minRadius[2]; k <= maxRadius[2]; ++k)
    {
      if (k > -RadiusDiscrete[2] && boundary.IJK[2] + k == 0)
      { // Don't double count samples that exist on other nodes:
        continue;
      }

      for (vtkm::IdComponent j = minRadius[1]; j <= maxRadius[1]; ++j)
      {
        if (j > -RadiusDiscrete[1] && boundary.IJK[1] + j == 0)
        { // Don't double count samples that exist on other nodes:
          continue;
        }

        for (vtkm::IdComponent i = minRadius[0]; i <= maxRadius[0]; ++i)
        {
          if (i > -RadiusDiscrete[0] && boundary.IJK[0] + i == 0)
          { // Don't double count samples that exist on other nodes:
            continue;
          }

          const vtkm::Float64 r0 = i * 1. / RadiusDiscrete[0];
          const vtkm::Float64 r1 = j * 1. / RadiusDiscrete[1];
          const vtkm::Float64 r2 = k * 1. / RadiusDiscrete[2];

          if (r0 * r0 + r1 * r1 + r2 * r2 <= 1)
          {
            sum += pow(r0, p) * pow(r1, q) * pow(r2, r) * image.Get(i, j, k);
          }
        }
      }
    }

    moment = T(sum * Spacing[0] * Spacing[1] * Spacing[2]);
  }

private:
  const double Radius;
  const vtkm::Vec<double, 3>& Spacing;
  const int p;
  const int q;
  const int r;
};

class ComputeMoments
{
public:
  ComputeMoments(vtkm::Vec<double, 3>& _spacing, const double _radius)
    : Spacing(_spacing)
    , Radius(_radius)
  {
  }

  class ResolveDynamicCellSet
  {
  public:
    template <typename T, typename S>
    void operator()(const vtkm::cont::CellSetStructured<2>& input,
                    const vtkm::cont::ArrayHandle<T, S>& pixels,
                    vtkm::Vec<double, 3> Spacing,
                    double Radius,
                    int maxOrder,
                    vtkm::cont::DataSet& output) const
    {
      using WorkletType = vtkm::worklet::moments::ComputeMoments2D;
      using DispatcherType = vtkm::worklet::DispatcherPointNeighborhood<WorkletType>;

      for (int order = 0; order <= maxOrder; ++order)
      {
        for (int p = 0; p <= order; ++p)
        {
          const int q = order - p;

          vtkm::cont::ArrayHandle<T> moments;

          DispatcherType dispatcher(WorkletType{ Spacing, Radius, p, q });
          dispatcher.Invoke(input, pixels, moments);

          std::string fieldName = std::string("index") + std::string(p, '0') + std::string(q, '1');

          vtkm::cont::Field momentsField(
            fieldName, vtkm::cont::Field::Association::POINTS, moments);
          output.AddField(momentsField);
        }
      }
    }

    template <typename T, typename S>
    void operator()(const vtkm::cont::CellSetStructured<3>& input,
                    const vtkm::cont::ArrayHandle<T, S>& pixels,
                    vtkm::Vec<double, 3> Spacing,
                    double Radius,
                    int maxOrder,
                    vtkm::cont::DataSet& output) const
    {
      using WorkletType = vtkm::worklet::moments::ComputeMoments3D;
      using DispatcherType = vtkm::worklet::DispatcherPointNeighborhood<WorkletType>;

      for (int order = 0; order <= maxOrder; ++order)
      {
        for (int r = 0; r <= order; ++r)
        {
          const int qMax = order - r;
          for (int q = 0; q <= qMax; ++q)
          {
            const int p = order - r - q;

            vtkm::cont::ArrayHandle<T> moments;

            DispatcherType dispatcher(WorkletType{ Spacing, Radius, p, q, r });
            dispatcher.Invoke(input, pixels, moments);

            std::string fieldName = std::string("index") + std::string(p, '0') +
              std::string(q, '1') + std::string(r, '2');

            vtkm::cont::Field momentsField(
              fieldName, vtkm::cont::Field::Association::POINTS, moments);
            output.AddField(momentsField);
          }
        }
      }
    }
  };

  template <typename T, typename S>
  void Run(const vtkm::cont::DynamicCellSet& input,
           const vtkm::cont::ArrayHandle<T, S>& pixels,
           int maxOrder,
           vtkm::cont::DataSet& output) const
  {
    input.ResetCellSetList(vtkm::cont::CellSetListTagStructured())
      .CastAndCall(ResolveDynamicCellSet(), pixels, this->Spacing, this->Radius, maxOrder, output);
  }

private:
  const double Radius = 1;
  const vtkm::Vec<double, 3> Spacing = { 1, 1, 1 };
};
}
}
}

#endif // vtk_m_worklet_moments_ComputeMoments_h
