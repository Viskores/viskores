#ifndef vtk_m_worklet_particleadvection_EvaluatorStatus_h
#define vtk_m_worklet_particleadvection_EvaluatorStatus_h

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{
enum class EvaluatorStatus
{
  SUCCESS = 0,
  OUTSIDE_SPATIAL_BOUNDS,
  OUTSIDE_TEMPORAL_BOUNDS
};

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm


#endif // vtk_m_worklet_particleadvection_EvaluatorStatus_h
