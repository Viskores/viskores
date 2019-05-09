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
struct ComputeMoments : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  ComputeMoments(int _r, int _p, int _q)
    : Radius(_r)
    , p(_p)
    , q(_q)
  {
    assert(_r >= 0);
    assert(_p >= 0);
    assert(_q >= 0);
  }

  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);

  using ExecutionSignature = void(_2, _3);

  template <typename NeighIn, typename T>
  VTKM_EXEC void operator()(const NeighIn& image, T& moment) const
  {
    // TODO: type safety and numerical precision
    // FIXME: how exactly does 3D work, do we need seperate 2D/3D version?
    // FIXME: Radius as Vec3<int>, however, radius_z may be 0 which cause divide by 0.
    T sum{};
    vtkm::Float64 recp = 1.0 / Radius; // extend to Vec3<double>
    for (int k = -Radius; k <= Radius; ++k)
    {
      for (int j = -Radius; j <= Radius; ++j)
      {
        for (int i = -Radius; i <= Radius; ++i)
        {
          if (i * i + j * j + k * k <= Radius * Radius)
            //sum += pow(i * recp_x, p) * pow(j * recp_y, q) * pow(k * recp_z) * image.Get(i, j, k);
            sum += pow(i * recp, p) * pow(j * recp, q) * image.Get(i, j, k);
        }
      }
    }

    moment = T(sum * recp * recp); // extend to recp_x * recp_y * recp_z
  }

private:
  const int Radius; // extend to Vec3<int>
  const int p;
  const int q;
};
}
}
}

#endif // vtk_m_worklet_moments_ComputeMoments_h
