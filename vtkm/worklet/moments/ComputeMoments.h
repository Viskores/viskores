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
    assert(_radius[2] >= 1);

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
    vtkm::Vec<vtkm::Float64, 2> recp{ 1.0 / Radius[0], 1.0 / Radius[1] };
    for (int j = -Radius[1]; j <= Radius[1]; ++j)
    {
      for (int i = -Radius[0]; i <= Radius[0]; ++i)
      {
        vtkm::Float64 r0 = i * recp[0];
        vtkm::Float64 r1 = i * recp[1];

        if (r0 * r0 + r1 * r1 <= 1)
          sum += pow(r0, p) * pow(r0, q) * image.Get(i, j, 0);
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
      dispatcher.SetDevice(vtkm::cont::DeviceAdapterTagSerial());
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
