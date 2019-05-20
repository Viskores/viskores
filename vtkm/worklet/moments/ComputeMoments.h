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
  ComputeMoments2D(int _radius, int _p, int _q, int _r)
    : Radius(_radius)
    , p(_p)
    , q(_q)
  {
    assert(_radius >= 0);
    assert(_p >= 0);
    assert(_q >= 0);
  }

  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);

  using ExecutionSignature = void(_2, _3);

  template <typename NeighIn, typename T>
  VTKM_EXEC void operator()(const NeighIn& image, T& moment) const
  {
    // TODO: type safety and numerical precision
    // FIXME: Radius as Vec3<int>, however, radius_z may be 0 which cause divide by 0.
    // We do need to have separate versions for 2D/3D, since we couldn't have Radius_z == 0
    // which will cause devision byzero.
    auto sum = vtkm::TypeTraits<T>::ZeroInitialization();
    vtkm::Float64 recp = 1.0 / Radius; // extend to Vec2<double>
    for (int j = -Radius; j <= Radius; ++j)
    {
      for (int i = -Radius; i <= Radius; ++i)
      {
        if (i * i + j * j + k * k <= Radius * Radius)
          sum += pow(i * recp, p) * pow(j * recp, q) * image.Get(i, j, 0);
      }
    }

    moment = T(sum * recp * recp); // extend to recp_x * recp_y * recp_z
  }

private:
  const int Radius; // extend to Vec2<int>
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

  using ExecutionSignature = void(_2, _3);

  template <typename NeighIn, typename T>
  VTKM_EXEC void operator()(const NeighIn& image, T& moment) const
  {
    // TODO: type safety and numerical precision
    // FIXME: Radius as Vec3<int>, however, radius_z may be 0 which cause divide by 0.
    // We do need to have separate versions for 2D/3D, since we couldn't have Radius_z == 0
    // which will cause devision byzero.
    auto sum = vtkm::TypeTraits<T>::ZeroInitialization();
    vtkm::Float64 recp = 1.0 / Radius; // extend to Vec3<double>
    for (int k = -Radius; k <= Radius; ++k)
    {
      for (int j = -Radius; j <= Radius; ++j)
      {
        for (int i = -Radius; i <= Radius; ++i)
        {
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
  class ResolveDynamicCellSet
  {
  public:
    // TODO: when and where should I dispatch on the Dimension?
    template <typename T, typename S, typename OutputPortalType>
    void operator()(const vtkm::cont::CellSetStructured<2>& input,
                    const vtkm::cont::ArrayHandle<T, S>& pixels,
                    OutputPortalType& momentsOut) const
    {
      using DispatcherType =
        vtkm::worklet::DispatcherPointNeighborhood<vtkm::worklet::moments::ComputeMoments>;
      DispatcherType dispatcher(vtkm::worklet::moments::ComputeMoments2D{ this->Radius, p, i - p });
      dispatcher.SetDevice(vtkm::cont::DeviceAdapterTagSerial());
      dispatcher.Invoke(vtkm::filter::ApplyPolicy(input, pixels, moments);
    }
    template <typename T, typename S, typename OutputPortalType>
    void operator()(const vtkm::cont::CellSetStructured<3>& input,
                    const vtkm::cont::ArrayHandle<T, S>& pixels,
                    OutputPortalType& momentsOut) const
    {
    }
  };

  template <typename T, typename S, typename OutputPortalType>
  void Run(const vtkm::cont::DynamicCellSet& input,
           const vtkm::cont::ArrayHandle<T, S>& pixels,
           OutputPortalType& momentsOut) const
  {
    input.ResetCellSetList(vtkm::cont::CellSetListTagStructured())
      .CastAndCall(ResolveDynamicCellSet(), pixels, momentsOut);
  }

private:
  const int p, q, r;
};
}
}
}

#endif // vtk_m_worklet_moments_ComputeMoments_h
