#ifndef vtkm_worklet_contourtree_augmented_process_contourtree_inc_compute_best_up_down_h
#define vtkm_worklet_contourtree_augmented_process_contourtree_inc_compute_best_up_down_h

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
class ComputeBestUpDown : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn _1,
                                WholeArrayIn _2,
                                WholeArrayIn _3,
                                WholeArrayIn _4,
                                WholeArrayIn _5,
                                WholeArrayIn _6,
                                WholeArrayIn _7,
                                WholeArrayIn _8,
                                WholeArrayIn _9,
                                WholeArrayIn _10,
                                WholeArrayOut _11,
                                WholeArrayOut _12);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12);
  using InputDomain = _3;

  // Default Constructor
  VTKM_EXEC_CONT ComputeBestUpDown() {}

  template <typename FirstArrayPortalType,
            typename NodesArrayPortalType,
            typename SupernodesArrayPortalType,
            typename MinValuesArrayPortalType,
            typename MinParentsArrayPortalType,
            typename MaxValuesArrayPortalType,
            typename MaxParentsArrayPortalType,
            typename SortOrderArrayPortalType,
            typename EdgesLinearArrayPortalType,
            typename FieldValuesArrayPortalType,
            typename OutputArrayPortalType>

  VTKM_EXEC void operator()(const vtkm::Id i,
                            const FirstArrayPortalType& first,
                            const NodesArrayPortalType& nodes,
                            const SupernodesArrayPortalType& supernodes,
                            const MinValuesArrayPortalType& minValues,
                            const MinParentsArrayPortalType& minParents,
                            const MaxValuesArrayPortalType& maxValues,
                            const MaxParentsArrayPortalType& maxParents,
                            const SortOrderArrayPortalType& ctSortOrder,
                            const EdgesLinearArrayPortalType& edgesLinear,
                            const FieldValuesArrayPortalType& fieldValues,
                            const OutputArrayPortalType& bestUp,  // output
                            const OutputArrayPortalType& bestDown // output
                            ) const
  {
    //Id k = first[i];
    Id k = first.Get(i);
    Float64 maxUpSubtreeHeight = 0;
    Float64 maxDownSubtreeHeight = 0;

    //while (edgesLinear[k].first == i)
    while (k < edgesLinear.GetNumberOfValues() && edgesLinear.Get(k).first == i)
    {
      Id j = edgesLinear.Get(k++).second;

      Id regularVertexValueI = maskedIndex(supernodes.Get(i));
      Id regularVertexValueJ = maskedIndex(supernodes.Get(j));

      //
      // Get subtree T(j) + i minimum
      //

      // If the arc is point the right way use the subtree min value
      Id minValueInSubtree = maskedIndex(supernodes.Get(minValues.Get(j)));

      // Include the current vertex along with the subtree it points at
      if (minValueInSubtree > regularVertexValueI)
      {
        minValueInSubtree = maskedIndex(supernodes.Get(i));
      }

      // If it's pointing the opposite way the global min must be there
      if (j == minParents.Get(i))
      {
        minValueInSubtree = 0;
      }

      //
      // Get subtree T(j) + i maximum
      //

      // If the arc is point the right way use the subtree min value
      Id maxValueInSubtree = maskedIndex(supernodes.Get(maxValues.Get(j)));

      // Include the current vertex along with the subtree it points at
      if (maxValueInSubtree < regularVertexValueI)
      {
        maxValueInSubtree = maskedIndex(supernodes.Get(i));
      }

      // If it's pointing the opposite way the global max must be there
      if (j == maxParents.Get(i))
      {
        maxValueInSubtree = nodes.GetNumberOfValues() - 1;
      }

      Float64 minValue = fieldValues.Get(ctSortOrder.Get(minValueInSubtree));
      Float64 maxValue = fieldValues.Get(ctSortOrder.Get(maxValueInSubtree));

      Float64 subtreeHeight = maxValue - minValue;

      // Downward Edge
      if (regularVertexValueI > regularVertexValueJ)
      {
        if (subtreeHeight > maxDownSubtreeHeight)
        {
          maxDownSubtreeHeight = subtreeHeight;
          bestDown.Set(i, j);
        }
      }
      // UpwardsEdge
      else
      {
        if (subtreeHeight > maxUpSubtreeHeight)
        {
          maxUpSubtreeHeight = subtreeHeight;
          bestUp.Set(i, j);
        }
      }
    }

    // Make sure at least one of these was set
    assert(false == noSuchElement(bestUp.Get(i)) || false == noSuchElement(bestDown.Get(i)));
  }
};

} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm


#endif
