#ifndef vtkm_worklet_contourtree_augmented_process_contourtree_inc_euler_tour_list_h
#define vtkm_worklet_contourtree_augmented_process_contourtree_inc_euler_tour_list_h

#include <vtkm/cont/Algorithm.h>
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
class ComputeEulerTourList : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn next, WholeArrayIn first, WholeArrayOut succ);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3);
  using InputDomain = _1;

  vtkm::cont::ArrayHandle<Vec<Id, 2>> edges;

  // Default Constructor
  VTKM_EXEC_CONT ComputeEulerTourList(vtkm::cont::ArrayHandle<Vec<Id, 2>> _edges)
    : edges(_edges)
  {
  }

  template <typename NextArrayPortalType,
            typename FirstArrayPortalType,
            typename OutputArrayPortalType>
  VTKM_EXEC void operator()(const vtkm::Id i,
                            const NextArrayPortalType& next,
                            const FirstArrayPortalType& first,
                            const OutputArrayPortalType& succ) const
  {
    cont::ArrayHandle<Vec<Id, 2>> value;
    value.Allocate(1);
    value.GetPortalControl().Set(0,
                                 Vec<Id, 2>{ edges.GetPortalConstControl().Get(i)[1],
                                             edges.GetPortalConstControl().Get(i)[0] });

    cont::ArrayHandle<Id> output;
    vtkm::cont::Algorithm::LowerBounds(edges, value, output, vtkm::SortLess());

    int oppositeIndex = output.GetPortalControl().Get(0);

    if (NO_SUCH_ELEMENT == next.Get(oppositeIndex))
    {
      succ.Set(i, first.Get(edges.GetPortalConstControl().Get(i)[1]));
    }
    else
    {
      succ.Set(i, next.Get(oppositeIndex));
    }
  }
};

} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm


#endif
