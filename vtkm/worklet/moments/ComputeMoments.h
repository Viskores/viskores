//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=======================================================================
#ifndef vtk_m_worklet_moments_ComputeMoments_h
#define vtk_m_worklet_moments_ComputeMoments_h

#include <vtkm/worklet/DispatcherPointNeighborhood.h>
#include <vtkm/worklet/WorkletPointNeighborhood.h>

namespace vtkm
{
namespace worklet
{
namespace moments
{

struct ComputeMoments2D : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  ComputeMoments2D(const vtkm::Vec<vtkm::Int32, 3>& _radius, int _p, int _q)
    : Radius(_radius)
    , p(_p)
    , q(_q)
  {
    assert(_radius[0] >= 1);
    assert(_radius[1] >= 1);
    //    assert(_radius[2] >= 1);

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
    // FIXME: Radius as Vec3<int>, however, radius_z may be 0 which cause divide by 0.
    // We do need to have separate versions for 2D/3D, since we couldn't have Radius_z == 0
    // which will cause devision byzero.
    auto sum = vtkm::TypeTraits<T>::ZeroInitialization();
    vtkm::Vec<vtkm::Float64, 2> recp{ 1.0 / Radius[0], 1.0 / Radius[1] };

    // Clamp the radius to the dataset bounds (discard out-of-bounds points).
    const auto minRadius = boundary.ClampNeighborIndex(-this->Radius);
    const auto maxRadius = boundary.ClampNeighborIndex(this->Radius);

    for (vtkm::IdComponent j = minRadius[1]; j <= maxRadius[1]; ++j)
    {
      if (j > -this->Radius[1] && boundary.IJK[1] + j == 0)
      { // Don't double count samples that exist on other nodes:
        continue;
      }

      for (vtkm::IdComponent i = minRadius[0]; i <= maxRadius[0]; ++i)
      {
        if (i > -this->Radius[0] && boundary.IJK[0] + i == 0)
        { // Don't double count samples that exist on other nodes:
          continue;
        }

        vtkm::Float64 r0 = i * recp[0];
        vtkm::Float64 r1 = j * recp[1];

        if (r0 * r0 + r1 * r1 <= 1)
        {
          sum += pow(r0, p) * pow(r1, q) * image.Get(i, j, 0);
        }
      }
    }

    moment = T(sum * recp[0] * recp[1]); // extend to recp_x * recp_y * recp_z
  }

private:
  const vtkm::Vec<vtkm::Int32, 3> Radius;
  const int p;
  const int q;
};

struct ComputeMoments3D : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  ComputeMoments3D(int _radius, int _p, int _q, int _r)
    : Radius(_radius)
    , p(_p)
    , q(_q)
    , r(_r)
  {
    assert(_radius >= 0);
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
    // FIXME: Radius as Vec3<int>, however, radius_z may be 0 which cause divide by 0.
    // We do need to have separate versions for 2D/3D, since we couldn't have Radius_z == 0
    // which will cause devision byzero.
    auto sum = vtkm::TypeTraits<T>::ZeroInitialization();
    vtkm::Float64 recp = 1.0 / Radius; // extend to Vec3<double>

    // Clamp the radius to the dataset bounds (discard out-of-bounds points).
    const auto minRadius = boundary.ClampNeighborIndex({ -this->Radius });
    const auto maxRadius = boundary.ClampNeighborIndex({ this->Radius });

    for (vtkm::IdComponent k = minRadius[2]; k <= maxRadius[2]; ++k)
    {
      if (k > -this->Radius && boundary.IJK[2] + k == 0)
      { // Don't double count samples that exist on other nodes:
        continue;
      }

      for (vtkm::IdComponent j = minRadius[1]; j <= maxRadius[1]; ++j)
      {
        if (j > -this->Radius && boundary.IJK[1] + j == 0)
        { // Don't double count samples that exist on other nodes:
          continue;
        }

        for (vtkm::IdComponent i = minRadius[0]; i <= maxRadius[0]; ++i)
        {
          if (i > -this->Radius && boundary.IJK[0] + i == 0)
          { // Don't double count samples that exist on other nodes:
            continue;
          }

          if (i * i + j * j + k * k <= Radius * Radius)
            sum += pow(i * recp, p) * pow(j * recp, q) * pow(k * recp, r) * image.Get(i, j, k);
        }
      }
    }

    moment = T(sum * recp * recp * recp); // extend to recp_x * recp_y * recp_z
  }

private:
  const int Radius; // extend to Vec3<int>
  const int p;
  const int q;
  const int r;
};

class ComputeMoments
{
public:
  ComputeMoments(vtkm::Vec<vtkm::Int32, 3> _radius)
    : Radius(_radius)
  {
  }

  class ResolveDynamicCellSet
  {
  public:
    // TODO: when and where should I dispatch on the Dimension?
    template <typename T, typename S, typename OutputPortalType>
    void operator()(const vtkm::cont::CellSetStructured<2>& input,
                    const vtkm::cont::ArrayHandle<T, S>& pixels,
                    vtkm::Vec<vtkm::Int32, 3> Radius,
                    int p,
                    int q,
                    OutputPortalType& momentsOut) const
    {
      using DispatcherType =
        vtkm::worklet::DispatcherPointNeighborhood<vtkm::worklet::moments::ComputeMoments2D>;
      DispatcherType dispatcher(vtkm::worklet::moments::ComputeMoments2D{ Radius, p, q });
      dispatcher.Invoke(input, pixels, momentsOut);
    }

    template <typename T, typename S, typename OutputPortalType>
    void operator()(const vtkm::cont::CellSetStructured<3>& input,
                    const vtkm::cont::ArrayHandle<T, S>& pixels,
                    vtkm::Vec<vtkm::Int32, 3> Radius,
                    int p,
                    int q,
                    OutputPortalType& momentsOut) const
    {
      // TBD, don't know how to deal with 3D.
    }
  };

  template <typename T, typename S, typename OutputPortalType>
  void Run(const vtkm::cont::DynamicCellSet& input,
           const vtkm::cont::ArrayHandle<T, S>& pixels,
           int p,
           int q,
           OutputPortalType& momentsOut) const
  {
    input.ResetCellSetList(vtkm::cont::CellSetListTagStructured())
      .CastAndCall(ResolveDynamicCellSet(), pixels, this->Radius, p, q, momentsOut);
  }

private:
  const vtkm::Vec<vtkm::Int32, 3> Radius = { 1, 1, 1 };
};
}
}
}

#endif // vtk_m_worklet_moments_ComputeMoments_h
