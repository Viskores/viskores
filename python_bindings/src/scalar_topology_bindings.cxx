//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "pyviskores_common.h"
#include "pyviskores_bindings.h"

#include <nanobind/stl/string.h>

#include <stdexcept>
#include <vector>

namespace viskores::python::bindings
{

#if VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY
namespace
{

using ContourTreeAugmentedFilter = viskores::filter::scalar_topology::ContourTreeAugmented;
using ContourTreeType = viskores::worklet::contourtree_augmented::ContourTree;
using IdArrayType = viskores::worklet::contourtree_augmented::IdArrayType;

const ContourTreeType& RequireComputedContourTree(ContourTreeAugmentedFilter& self)
{
  const ContourTreeType& contourTree = self.GetContourTree();
  if (contourTree.Superarcs.GetNumberOfValues() == 0)
  {
    throw std::runtime_error("ContourTreeAugmented has no computed contour tree. Call Execute first.");
  }
  return contourTree;
}

const ContourTreeType& RequireComputedAugmentedContourTree(ContourTreeAugmentedFilter& self)
{
  const ContourTreeType& contourTree = RequireComputedContourTree(self);
  if (contourTree.Nodes.GetNumberOfValues() == 0 || contourTree.Superparents.GetNumberOfValues() == 0)
  {
    throw std::runtime_error(
      "ContourTreeAugmented regular structure is not available. Execute with "
      "compute_regular_structure=1 before requesting superarc membership or volume arrays.");
  }
  return contourTree;
}

viskores::Id PublicIndex(viskores::Id value)
{
  using namespace viskores::worklet::contourtree_augmented;
  return NoSuchElement(value) ? static_cast<viskores::Id>(NO_SUCH_ELEMENT) : MaskedIndex(value);
}

nb::object IdVectorToNumPy(std::vector<viskores::Id>&& values)
{
  IdArrayType array = viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
  return IdArrayToNumPy(array);
}

struct VolumeArrays
{
  IdArrayType IntrinsicVolume;
  IdArrayType DependentVolume;
  IdArrayType SupernodeTransferWeight;
  IdArrayType HyperarcDependentWeight;
};

VolumeArrays ComputeVolumeArrays(ContourTreeAugmentedFilter& self)
{
  VolumeArrays arrays;
  viskores::worklet::contourtree_augmented::ProcessContourTree::ComputeVolumeWeightsSerial(
    RequireComputedAugmentedContourTree(self),
    self.GetNumIterations(),
    arrays.IntrinsicVolume,
    arrays.DependentVolume,
    arrays.SupernodeTransferWeight,
    arrays.HyperarcDependentWeight);
  return arrays;
}

nb::object GetPointSuperarcIds(ContourTreeAugmentedFilter& self)
{
  const ContourTreeType& contourTree = RequireComputedAugmentedContourTree(self);
  const viskores::Id numberOfPoints = contourTree.Superparents.GetNumberOfValues();
  std::vector<viskores::Id> pointSuperarcs(static_cast<std::size_t>(numberOfPoints),
                                           viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT);

  auto superparentsPortal = contourTree.Superparents.ReadPortal();
  auto sortOrderPortal = self.GetSortOrder().ReadPortal();
  for (viskores::Id sortId = 0; sortId < numberOfPoints; ++sortId)
  {
    const viskores::Id pointId = sortOrderPortal.Get(sortId);
    if (pointId < 0 || pointId >= numberOfPoints)
    {
      throw std::runtime_error("ContourTreeAugmented sort order contains an invalid point id.");
    }
    pointSuperarcs[static_cast<std::size_t>(pointId)] =
      PublicIndex(superparentsPortal.Get(sortId));
  }

  return IdVectorToNumPy(std::move(pointSuperarcs));
}

nb::object GetSupernodePointIds(ContourTreeAugmentedFilter& self)
{
  const ContourTreeType& contourTree = RequireComputedContourTree(self);
  const viskores::Id numberOfSupernodes = contourTree.Supernodes.GetNumberOfValues();
  std::vector<viskores::Id> pointIds(static_cast<std::size_t>(numberOfSupernodes));

  auto supernodesPortal = contourTree.Supernodes.ReadPortal();
  auto sortOrderPortal = self.GetSortOrder().ReadPortal();
  for (viskores::Id supernode = 0; supernode < numberOfSupernodes; ++supernode)
  {
    pointIds[static_cast<std::size_t>(supernode)] =
      sortOrderPortal.Get(PublicIndex(supernodesPortal.Get(supernode)));
  }

  return IdVectorToNumPy(std::move(pointIds));
}

nb::dict ComputeVolumeBranchDecompositionDict(
  viskores::filter::scalar_topology::ContourTreeAugmented& self)
{
  IdArrayType superarcIntrinsicWeight;
  IdArrayType superarcDependentWeight;
  IdArrayType supernodeTransferWeight;
  IdArrayType hyperarcDependentWeight;
  viskores::worklet::contourtree_augmented::ProcessContourTree::ComputeVolumeWeightsSerial(
    self.GetContourTree(),
    self.GetNumIterations(),
    superarcIntrinsicWeight,
    superarcDependentWeight,
    supernodeTransferWeight,
    hyperarcDependentWeight);

  IdArrayType whichBranch;
  IdArrayType branchMinimum;
  IdArrayType branchMaximum;
  IdArrayType branchSaddle;
  IdArrayType branchParent;
  viskores::worklet::contourtree_augmented::ProcessContourTree::
    ComputeVolumeBranchDecompositionSerial(self.GetContourTree(),
                                           superarcDependentWeight,
                                           superarcIntrinsicWeight,
                                           whichBranch,
                                           branchMinimum,
                                           branchMaximum,
                                           branchSaddle,
                                           branchParent);

  nb::dict branchDecomposition;
  branchDecomposition[nb::str("which_branch")] = IdArrayToNumPy(whichBranch);
  branchDecomposition[nb::str("branch_minimum")] = IdArrayToNumPy(branchMinimum);
  branchDecomposition[nb::str("branch_maximum")] = IdArrayToNumPy(branchMaximum);
  branchDecomposition[nb::str("branch_saddle")] = IdArrayToNumPy(branchSaddle);
  branchDecomposition[nb::str("branch_parent")] = IdArrayToNumPy(branchParent);
  return branchDecomposition;
}

template <typename ArrayHandleType>
nb::object ComputeRelevantValuesForArray(
  viskores::filter::scalar_topology::ContourTreeAugmented& self,
  const ArrayHandleType& dataField,
  viskores::Id numLevels,
  int contourType,
  viskores::FloatDefault eps,
  viskores::Id numComponents,
  int contourSelectMethod,
  bool usePersistenceSorter,
  bool dataFieldIsSorted)
{
  using ValueType = typename ArrayHandleType::ValueType;
  using BranchType =
    viskores::worklet::contourtree_augmented::process_contourtree_inc::Branch<ValueType>;

  if (numLevels <= 0)
  {
    return VectorToNumPy({});
  }

  IdArrayType superarcIntrinsicWeight;
  IdArrayType superarcDependentWeight;
  IdArrayType supernodeTransferWeight;
  IdArrayType hyperarcDependentWeight;
  viskores::worklet::contourtree_augmented::ProcessContourTree::ComputeVolumeWeightsSerial(
    self.GetContourTree(),
    self.GetNumIterations(),
    superarcIntrinsicWeight,
    superarcDependentWeight,
    supernodeTransferWeight,
    hyperarcDependentWeight);

  IdArrayType whichBranch;
  IdArrayType branchMinimum;
  IdArrayType branchMaximum;
  IdArrayType branchSaddle;
  IdArrayType branchParent;
  viskores::worklet::contourtree_augmented::ProcessContourTree::
    ComputeVolumeBranchDecompositionSerial(self.GetContourTree(),
                                           superarcDependentWeight,
                                           superarcIntrinsicWeight,
                                           whichBranch,
                                           branchMinimum,
                                           branchMaximum,
                                           branchSaddle,
                                           branchParent);

  BranchType* root =
    viskores::worklet::contourtree_augmented::ProcessContourTree::ComputeBranchDecomposition<
      ValueType>(self.GetContourTree().Superparents,
                 self.GetContourTree().Supernodes,
                 whichBranch,
                 branchMinimum,
                 branchMaximum,
                 branchSaddle,
                 branchParent,
                 self.GetSortOrder(),
                 dataField,
                 dataFieldIsSorted);

  root->SimplifyToSize(numComponents, usePersistenceSorter);

  std::vector<ValueType> isoValues;
  switch (contourSelectMethod)
  {
    default:
    case 0:
      root->GetRelevantValues(contourType, static_cast<ValueType>(eps), isoValues);
      break;
    case 1:
    {
      viskores::worklet::contourtree_augmented::process_contourtree_inc::PiecewiseLinearFunction<
        ValueType>
        plf;
      root->AccumulateIntervals(contourType, static_cast<ValueType>(eps), plf);
      isoValues = plf.nLargest(static_cast<unsigned int>(numLevels));
      break;
    }
  }

  delete root;

  std::sort(isoValues.begin(), isoValues.end());
  auto uniqueEnd = std::unique(isoValues.begin(), isoValues.end());
  isoValues.erase(uniqueEnd, isoValues.end());

  std::vector<viskores::FloatDefault> isoValuesFloatDefault;
  isoValuesFloatDefault.reserve(isoValues.size());
  for (const auto& value : isoValues)
  {
    isoValuesFloatDefault.push_back(static_cast<viskores::FloatDefault>(value));
  }
  return VectorToNumPy(isoValuesFloatDefault);
}

} // namespace

void RegisterNanobindScalarTopologyClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("ContourTreeAugmented");
  nb::class_<viskores::filter::scalar_topology::ContourTreeAugmented>(
    m, "ContourTreeAugmented", doc::ClassDoc("ContourTreeAugmented"))
    .def(nb::init<bool, unsigned int>(),
         nb::arg("use_marching_cubes") = false,
         nb::arg("compute_regular_structure") = 1U)
    .def(
      "SetActiveField",
      [](viskores::filter::scalar_topology::ContourTreeAugmented& self,
         const char* name,
         viskores::cont::Field::Association association)
      { self.SetActiveField(name, association); },
      nb::arg("name"),
      nb::arg("association") = viskores::cont::Field::Association::Any)
    .def("GetActiveFieldName",
         &viskores::filter::scalar_topology::ContourTreeAugmented::GetActiveFieldName)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::scalar_topology::ContourTreeAugmented>,
         doc::ExecuteFilter)
    .def("GetSortOrder",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self)
         { return IdArrayToNumPy(self.GetSortOrder()); })
    .def("GetNumIterations",
         &viskores::filter::scalar_topology::ContourTreeAugmented::GetNumIterations)
    .def("GetSupernodes",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self)
         { return IdArrayToNumPy(RequireComputedContourTree(self).Supernodes); })
    .def("GetSuperarcs",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self)
         { return IdArrayToNumPy(RequireComputedContourTree(self).Superarcs); })
    .def("GetIntrinsicVolume",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self)
         {
           VolumeArrays arrays = ComputeVolumeArrays(self);
           return IdArrayToNumPy(arrays.IntrinsicVolume);
         })
    .def("GetDependentVolume",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self)
         {
           VolumeArrays arrays = ComputeVolumeArrays(self);
           return IdArrayToNumPy(arrays.DependentVolume);
         })
    .def("GetPointSuperarcIds",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self)
         { return GetPointSuperarcIds(self); })
    .def("GetSupernodePointIds",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self)
         { return GetSupernodePointIds(self); })
    .def("GetSortedSuperarcs",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self)
         {
           viskores::worklet::contourtree_augmented::EdgePairArray saddlePeak;
           viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
             self.GetContourTree(), self.GetSortOrder(), saddlePeak);
           return EdgePairArrayToNumPy(saddlePeak);
         })
    .def("ComputeVolumeBranchDecomposition",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self)
         { return ComputeVolumeBranchDecompositionDict(self); })
    .def(
      "ComputeRelevantValues",
      [](viskores::filter::scalar_topology::ContourTreeAugmented& self,
         nb::handle datasetObject,
         long long numLevels,
         int contourType,
         double eps,
         long long numComponents,
         int contourSelectMethod,
         bool usePersistenceSorter,
         bool dataFieldIsSorted)
      {
        auto dataset = RequireDataSet(datasetObject);
        if (!dataset)
        {
          throw nb::python_error();
        }

        const auto& field = dataset->GetField(self.GetActiveFieldName());
        nb::object result;
        field.GetData()
          .CastAndCallForTypes<viskores::TypeListScalarAll, VISKORES_DEFAULT_STORAGE_LIST>(
            [&](const auto& dataField)
            {
              result = ComputeRelevantValuesForArray(self,
                                                     dataField,
                                                     static_cast<viskores::Id>(numLevels),
                                                     contourType,
                                                     static_cast<viskores::FloatDefault>(eps),
                                                     static_cast<viskores::Id>(numComponents),
                                                     contourSelectMethod,
                                                     usePersistenceSorter,
                                                     dataFieldIsSorted);
            });
        return result;
      },
      nb::arg("dataset"),
      nb::arg("num_levels"),
      nb::arg("contour_type") = 0,
      nb::arg("eps") = 1.0e-5,
      nb::arg("num_components") = 0,
      nb::arg("contour_select_method") = 0,
      nb::arg("use_persistence_sorter") = true,
      nb::arg("data_field_is_sorted") = false);

}
#else
void RegisterNanobindScalarTopologyClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
