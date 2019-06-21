
#include <vtkm/cont/ArrayPortalExtrudePlane.h>

namespace vtkm
{
namespace exec
{

template <typename PortalType>
vtkm::Vec<typename ArrayPortalExtrudePlane<PortalType>::ValueType, 6> ArrayPortalExtrudePlane<
  PortalType>::ArrayPortalExtrudePlane::GetWedge(const ToroidIndices& index) const
{
  vtkm::Vec<ValueType, 6> result;
  result[0] = this->Portal.Get(index.PointIds[0][0]);
  result[1] = this->Portal.Get(index.PointIds[0][1]);
  result[2] = this->Portal.Get(index.PointIds[0][2]);
  result[3] = this->Portal.Get(index.PointIds[1][0]);
  result[4] = this->Portal.Get(index.PointIds[1][1]);
  result[5] = this->Portal.Get(index.PointIds[1][2]);

  return result;
}
}
} // vtkm::exec
