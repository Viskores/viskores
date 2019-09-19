#ifndef vtkm_worklet_contourtree_augmented_process_contourtree_inc_compute_min_max_values_h
#define vtkm_worklet_contourtree_augmented_process_contourtree_inc_compute_min_max_values_h

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
class ComputeMinMaxValues : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn supernodes,
                                WholeArrayIn firstLast,
                                WholeArrayIn tourEdges,
                                WholeArrayOut output);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4);
  using InputDomain = _1;

  bool isMin = true;

  // Default Constructor
  VTKM_EXEC_CONT ComputeMinMaxValues(bool _isMin)
    : isMin(_isMin)
  {
  }

  template <typename SupernodesArrayPortalType,
            typename FirstLastArrayPortalType,
            typename TourEdgesArrayPortalType,
            typename OutputArrayPortalType>
  VTKM_EXEC void operator()(const vtkm::Id i,
                            const SupernodesArrayPortalType& supernodes,
                            const FirstLastArrayPortalType& firstLast,
                            const TourEdgesArrayPortalType& tourEdges,
                            const OutputArrayPortalType& output) const
  {
    Id optimal = tourEdges.Get(firstLast.Get(i).first)[1];

    for (int j = firstLast.Get(i).first; j < firstLast.Get(i).second; j++)
    {
      Id vertex = tourEdges.Get(j)[1];

      Id vertexValue = maskedIndex(supernodes.Get(vertex));
      Id optimalValue = maskedIndex(supernodes.Get(optimal));

      //if (compare(optimalValue, vertexValue))
      if ((true == isMin && vertexValue < optimalValue) ||
          (false == isMin && vertexValue > optimalValue))
      {
        optimal = vertex;
      }
      //printf("[%d, %d]\n", tourEdges[j][0], tourEdges[j][1]);
    }


    //printf("The optimal is %d\n", optimal);
    output.Set(i, optimal);
  }
};

} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm


#endif
