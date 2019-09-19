#ifndef vtkm_worklet_contourtree_augmented_process_contourtree_inc_euler_tour_compute_first_next_h
#define vtkm_worklet_contourtree_augmented_process_contourtree_inc_euler_tour_compute_first_next_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{
class ComputeEulerTourFirstNext : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn edges, WholeArrayOut first, WholeArrayOut next);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3);
  using InputDomain = _1;

  // Default Constructor
  VTKM_EXEC_CONT ComputeEulerTourFirstNext() {}

  template <typename EdgesArrayPortalType, typename OutputArrayPortalType>
  VTKM_EXEC void operator()(const vtkm::Id i,
                            const EdgesArrayPortalType& edges,
                            const OutputArrayPortalType& first,
                            const OutputArrayPortalType& next) const
  {
    if (i == 0)
    {
      first.Set(0, 0);
    }
    else
    {
      if (edges.Get(i)[0] != edges.Get(i - 1)[0])
      {
        first.Set(edges.Get(i)[0], i);
      }
      else
      {
        next.Set(i - 1, i);
      }
    }
  }
};

} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm


#endif
