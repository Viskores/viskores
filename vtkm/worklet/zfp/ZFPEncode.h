#ifndef vtk_m_worklet_zfp_encode_h
#define vtk_m_worklet_zfp_encode_h

#include <vtkm/Types.h>
#include <vtkm/internal/ExportMacros.h>

namespace vtkm
{
namespace worklet
{
namespace zfp
{

template <typename Scalar>
VTKM_EXEC void PadBlock(Scalar* p, vtkm::UInt32 n, vtkm::UInt32 s)
{
  switch (n)
  {
    case 0:
      p[0 * s] = 0;
    /* FALLTHROUGH */
    case 1:
      p[1 * s] = p[0 * s];
    /* FALLTHROUGH */
    case 2:
      p[2 * s] = p[1 * s];
    /* FALLTHROUGH */
    case 3:
      p[3 * s] = p[0 * s];
    /* FALLTHROUGH */
    default:
      break;
  }
}
}
}
} // namespace vtkm::worklet::zfp
#endif
