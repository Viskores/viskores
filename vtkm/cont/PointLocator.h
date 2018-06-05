#include <vtkm/Types.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ExecutionObjectBase.h>

namespace vtkm
{

namespace exec
{

class PointLocator
{
public:
  VTKM_EXEC virtual void FindNearestNeighbor(const vtkm::Vec<vtkm::FloatDefault, 3>& queryPoint,
                                             vtkm::Id& pointId,
                                             vtkm::FloatDefault& distanceSquared) const = 0;
};

} // namespace exec

namespace cont
{

class PointLocator : public ExecutionObjectBase
{

public:
  PointLocator()
    : Dirty(true)
  {
  }

  vtkm::cont::CoordinateSystem GetCoords() const { return Coords; }

  void SetCoords(const vtkm::cont::CoordinateSystem& coords)
  {
    Coords = coords;
    Dirty = true;
  }

  virtual void Build() = 0;

  void Update()
  {
    if (Dirty)
      Build();
    Dirty = false;
  }

  template <typename DeviceAdapter>
  VTKM_CONT std::unique_ptr<vtkm::exec::PointLocator> PrepareForExecution(DeviceAdapter device)
  {
    vtkm::cont::DeviceAdapterId deviceId = vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetId();
    return PrepareForExecution(deviceId);
  }

  VTKM_CONT virtual std::unique_ptr<vtkm::exec::PointLocator> PrepareForExecution(
    vtkm::cont::DeviceAdapterId device) = 0;

private:
  vtkm::cont::CoordinateSystem Coords;
  bool Dirty;
};

} // namespace cont
} // namespace vtkm
