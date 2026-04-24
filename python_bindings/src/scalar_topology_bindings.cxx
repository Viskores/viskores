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

namespace viskores::python::bindings
{

#if VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY
namespace
{

nb::dict ComputeVolumeBranchDecompositionDict(
  viskores::filter::scalar_topology::ContourTreeAugmented& self)
{
  using IdArrayType = viskores::worklet::contourtree_augmented::IdArrayType;

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
  branchDecomposition[nb::str("which_branch")] =
    IdArrayToNumPy(whichBranch);
  branchDecomposition[nb::str("branch_minimum")] =
    IdArrayToNumPy(branchMinimum);
  branchDecomposition[nb::str("branch_maximum")] =
    IdArrayToNumPy(branchMaximum);
  branchDecomposition[nb::str("branch_saddle")] =
    IdArrayToNumPy(branchSaddle);
  branchDecomposition[nb::str("branch_parent")] =
    IdArrayToNumPy(branchParent);
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

  using IdArrayType = viskores::worklet::contourtree_augmented::IdArrayType;
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
  erase_existing_name("ContourTreeMesh2D");
  nb::class_<viskores::filter::scalar_topology::ContourTreeMesh2D>(m, "ContourTreeMesh2D")
    .def(nb::init<>())
    .def("SetActiveField",
         [](viskores::filter::scalar_topology::ContourTreeMesh2D& self,
            const char* name,
            nb::handle associationObject) {
           self.SetActiveField(
             name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
         },
         nb::arg("name"),
         nb::arg("association") = nb::none())
    .def("GetActiveFieldName",
         &viskores::filter::scalar_topology::ContourTreeMesh2D::GetActiveFieldName)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::scalar_topology::ContourTreeMesh2D>);

  erase_existing_name("ContourTreeMesh3D");
  nb::class_<viskores::filter::scalar_topology::ContourTreeMesh3D>(m, "ContourTreeMesh3D")
    .def(nb::init<>())
    .def("SetActiveField",
         [](viskores::filter::scalar_topology::ContourTreeMesh3D& self,
            const char* name,
            nb::handle associationObject) {
           self.SetActiveField(
             name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
         },
         nb::arg("name"),
         nb::arg("association") = nb::none())
    .def("GetActiveFieldName",
         &viskores::filter::scalar_topology::ContourTreeMesh3D::GetActiveFieldName)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::scalar_topology::ContourTreeMesh3D>);

  erase_existing_name("ContourTreeAugmented");
  nb::class_<viskores::filter::scalar_topology::ContourTreeAugmented>(m, "ContourTreeAugmented")
    .def(nb::init<bool, unsigned int>(),
         nb::arg("use_marching_cubes") = false,
         nb::arg("compute_regular_structure") = 1U)
    .def("SetActiveField",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self,
            const char* name,
            nb::handle associationObject) {
           self.SetActiveField(
             name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
         },
         nb::arg("name"),
         nb::arg("association") = nb::none())
    .def("GetActiveFieldName",
         &viskores::filter::scalar_topology::ContourTreeAugmented::GetActiveFieldName)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::scalar_topology::ContourTreeAugmented>)
    .def("GetSortOrder",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self) {
           return IdArrayToNumPy(self.GetSortOrder());
         })
    .def("GetNumIterations",
         &viskores::filter::scalar_topology::ContourTreeAugmented::GetNumIterations)
    .def("GetSortedSuperarcs",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self) {
           viskores::worklet::contourtree_augmented::EdgePairArray saddlePeak;
           viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
             self.GetContourTree(), self.GetSortOrder(), saddlePeak);
           return EdgePairArrayToNumPy(saddlePeak);
         })
    .def("ComputeVolumeBranchDecomposition",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self) {
           return ComputeVolumeBranchDecompositionDict(self);
         })
    .def("ComputeRelevantValues",
         [](viskores::filter::scalar_topology::ContourTreeAugmented& self,
            nb::handle datasetObject,
            long long numLevels,
            int contourType,
            double eps,
            long long numComponents,
            int contourSelectMethod,
            bool usePersistenceSorter,
            bool dataFieldIsSorted) {
           auto dataset = RequireDataSet(datasetObject);
           if (!dataset)
           {
             throw nb::python_error();
           }

           const auto& field = dataset->GetField(self.GetActiveFieldName());
           nb::object result;
           field.GetData().CastAndCallForTypes<viskores::TypeListScalarAll,
                                               VISKORES_DEFAULT_STORAGE_LIST>(
             [&](const auto& dataField) {
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

  erase_existing_name("ContourTreeUniformDistributed");
  nb::class_<viskores::filter::scalar_topology::ContourTreeUniformDistributed>(
    m, "ContourTreeUniformDistributed")
    .def(nb::init<>())
    .def("SetActiveField",
         [](viskores::filter::scalar_topology::ContourTreeUniformDistributed& self,
            const char* name,
            nb::handle associationObject) {
           self.SetActiveField(
             name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
         },
         nb::arg("name"),
         nb::arg("association") = nb::none())
    .def("SetUseBoundaryExtremaOnly",
         [](viskores::filter::scalar_topology::ContourTreeUniformDistributed& self, bool value) {
           self.SetUseBoundaryExtremaOnly(value);
         },
         nb::arg("value"))
    .def("SetUseMarchingCubes",
         [](viskores::filter::scalar_topology::ContourTreeUniformDistributed& self, bool value) {
           self.SetUseMarchingCubes(value);
         },
         nb::arg("value"))
    .def("SetAugmentHierarchicalTree",
         [](viskores::filter::scalar_topology::ContourTreeUniformDistributed& self, bool value) {
           self.SetAugmentHierarchicalTree(value);
         },
         nb::arg("value"))
    .def("SetPresimplifyThreshold",
         [](viskores::filter::scalar_topology::ContourTreeUniformDistributed& self, long long value) {
           self.SetPresimplifyThreshold(static_cast<viskores::Id>(value));
         },
         nb::arg("value"))
    .def("SetSaveDotFiles",
         [](viskores::filter::scalar_topology::ContourTreeUniformDistributed& self, bool value) {
           self.SetSaveDotFiles(value);
         },
         nb::arg("value"))
    .def("SetBlockIndices",
         [](viskores::filter::scalar_topology::ContourTreeUniformDistributed& self,
            nb::handle blocksPerDimObject,
            nb::handle localBlockIndicesObject) {
           self.SetBlockIndices(ParseDimensions(blocksPerDimObject),
                                ParseId3Array(localBlockIndicesObject));
         },
         nb::arg("blocks_per_dim"),
         nb::arg("local_block_indices"))
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::scalar_topology::ContourTreeUniformDistributed>);

  erase_existing_name("DistributedBranchDecompositionFilter");
  nb::class_<viskores::filter::scalar_topology::DistributedBranchDecompositionFilter>(
    m, "DistributedBranchDecompositionFilter")
    .def(nb::init<>())
    .def("Execute",
         &ExecuteFilterToPython<
           viskores::filter::scalar_topology::DistributedBranchDecompositionFilter>);

  erase_existing_name("SelectTopVolumeBranchesFilter");
  nb::class_<viskores::filter::scalar_topology::SelectTopVolumeBranchesFilter>(
    m, "SelectTopVolumeBranchesFilter")
    .def(nb::init<>())
    .def("SetSavedBranches",
         [](viskores::filter::scalar_topology::SelectTopVolumeBranchesFilter& self, long long value) {
           self.SetSavedBranches(static_cast<viskores::Id>(value));
         },
         nb::arg("value"))
    .def("SetPresimplifyThreshold",
         [](viskores::filter::scalar_topology::SelectTopVolumeBranchesFilter& self, long long value) {
           self.SetPresimplifyThreshold(static_cast<viskores::Id>(value));
         },
         nb::arg("value"))
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::scalar_topology::SelectTopVolumeBranchesFilter>);

  erase_existing_name("ExtractTopVolumeContoursFilter");
  nb::class_<viskores::filter::scalar_topology::ExtractTopVolumeContoursFilter>(
    m, "ExtractTopVolumeContoursFilter")
    .def(nb::init<>())
    .def("SetMarchingCubes",
         [](viskores::filter::scalar_topology::ExtractTopVolumeContoursFilter& self, bool value) {
           self.SetMarchingCubes(value);
         },
         nb::arg("value"))
    .def("SetShiftIsovalueByEpsilon",
         [](viskores::filter::scalar_topology::ExtractTopVolumeContoursFilter& self, bool value) {
           self.SetShiftIsovalueByEpsilon(value);
         },
         nb::arg("value"))
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::scalar_topology::ExtractTopVolumeContoursFilter>);
}
#else
void RegisterNanobindScalarTopologyClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
