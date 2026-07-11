//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

// Minimal ContourTreeAugmented example.
//
// Computes the augmented contour tree and branch decomposition of a 5x5
// scalar field, prints a grid-shaped ASCII map of superarc IDs, and writes
// GraphViz .dot files for both the contour tree and its branch decomposition.
//
// The scalar field is the same as MakeTestDataSet().Make2DUniformDataSet1(),
// field "pointvar", from viskores/cont/testing/MakeTestDataSet.h.
//
// Render the .dot files:
//   dot -Tpdf contour_tree.dot        -o contour_tree.pdf
//   dot -Tpdf contour_tree_branch.dot -o contour_tree_branch.pdf
//
// Output fields written to the result DataSet by ContourTreeAugmented:
//   "Supernodes"   (whole-dataset) : mesh vertex ID of each supernode
//   "Superarcs"    (whole-dataset) : masked parent supernode index per supernode;
//                                    NoSuchElement() is true for the root
//   "Superparents" (point field)   : masked superarc index per mesh vertex
//   "WhichBranch"  (whole-dataset) : masked branch index per supernode

#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Initialize.h>
#include <viskores/filter/scalar_topology/ContourTreeUniformAugmented.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ContourTreeAugmentedFieldNames.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ProcessContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace cta = viskores::worklet::contourtree_augmented;

// Write the contour tree as a GraphViz digraph. Edges run from the lower-valued
// endpoint to the higher-valued one; rankdir=BT then places higher values at the top.
// Nodes are labeled "value [mesh vertex ID]".
//
// The two views share this writer:
//  - arcRegularVertices non-null: label each edge with its superarc ID and the regular
//    mesh vertices on that superarc (contour tree view).
//  - whichBranch non-null: color nodes and edges by the branch they belong to
//    (branch decomposition view; WhichBranch is set for all supernodes, including
//    the root).
void WriteDotFile(const std::string& fileName,
                  const cta::IdArrayType& supernodes,
                  const cta::IdArrayType& superarcs,
                  const viskores::Float32* fieldValues,
                  const std::vector<std::vector<viskores::Id>>* arcRegularVertices,
                  const cta::IdArrayType* whichBranch)
{
  auto supernodePortal = supernodes.ReadPortal();
  auto superarcPortal = superarcs.ReadPortal();
  viskores::Id numSupernodes = supernodes.GetNumberOfValues();

  // Assign a color per branch by mapping branch index to a small palette.
  const char* palette[] = { "tomato", "steelblue", "seagreen", "goldenrod",
                            "orchid", "coral",     "teal",     "peru" };
  constexpr size_t paletteSize = sizeof(palette) / sizeof(palette[0]);
  auto branchColor = [&](viskores::Id supernodeIdx) -> const char*
  {
    viskores::Id branchIdx = cta::MaskedIndex(whichBranch->ReadPortal().Get(supernodeIdx));
    return palette[static_cast<size_t>(branchIdx) % paletteSize];
  };

  std::ofstream dotFile(fileName);
  dotFile << "// Render: dot -Tpdf " << fileName << " -o "
          << fileName.substr(0, fileName.rfind('.')) << ".pdf\n"
          << "digraph ContourTree {\n"
          << "  graph [rankdir=BT, splines=true]\n"
          << "  node  [shape=ellipse, style=filled, fontname=Courier]\n"
          << "  edge  [fontname=Courier, fontsize=9]\n\n";

  // Nodes: one per supernode
  for (viskores::Id supernodeIdx = 0; supernodeIdx < numSupernodes; ++supernodeIdx)
  {
    viskores::Id meshVertex = supernodePortal.Get(supernodeIdx);
    dotFile << "  n" << supernodeIdx << " [label=\"" << fieldValues[meshVertex] << " ["
            << meshVertex
            << "]\", fillcolor=" << (whichBranch ? branchColor(supernodeIdx) : "lightblue")
            << "];\n";
  }
  dotFile << "\n";

  // Edges: one per superarc
  for (viskores::Id supernodeIdx = 0; supernodeIdx < numSupernodes; ++supernodeIdx)
  {
    viskores::Id maskedParent = superarcPortal.Get(supernodeIdx);
    if (cta::NoSuchElement(maskedParent))
      continue; // no superarc at the root
    viskores::Id parentIdx = cta::MaskedIndex(maskedParent);

    // Orient the edge from the lower-valued endpoint to the higher-valued one
    viskores::Id lowerEndpoint =
      (fieldValues[supernodePortal.Get(supernodeIdx)] < fieldValues[supernodePortal.Get(parentIdx)])
      ? supernodeIdx
      : parentIdx;
    viskores::Id upperEndpoint = (lowerEndpoint == supernodeIdx) ? parentIdx : supernodeIdx;

    dotFile << "  n" << lowerEndpoint << " -> n" << upperEndpoint << " [";
    if (whichBranch)
    {
      dotFile << "color=" << branchColor(supernodeIdx);
    }
    if (arcRegularVertices)
    {
      dotFile << (whichBranch ? ", " : "") << "label=\"" << supernodeIdx << " (";
      const std::vector<viskores::Id>& arcVerts =
        (*arcRegularVertices)[static_cast<size_t>(supernodeIdx)];
      for (size_t k = 0; k < arcVerts.size(); ++k)
      {
        if (k)
          dotFile << ",";
        dotFile << arcVerts[k];
      }
      dotFile << ")\"";
    }
    dotFile << "];\n";
  }
  dotFile << "}\n";
  std::cout << "Wrote " << fileName << "\n";
}

int main(int argc, char* argv[])
{
  viskores::cont::Initialize(argc, argv);

  // 5x5 scalar field (row-major, vertex 0 at top-left):
  //
  //  idx: 0    1    2    3    4
  //       5    6    7    8    9
  //      10   11   12   13   14
  //      15   16   17   18   19
  //      20   21   22   23   24
  //
  //  val: 100  78   49   17    1
  //        94  71   47   33    6
  //        52  44   50   45   48
  //         8  12   46   91   43
  //         0   5   51   76   83
  constexpr int gridWidth = 5, gridHeight = 5, numMeshVertices = gridWidth * gridHeight;
  const viskores::Float32 fieldValues[numMeshVertices] = { 100, 78, 49, 17, 1,  94, 71, 47, 33,
                                                           6,   52, 44, 50, 45, 48, 8,  12, 46,
                                                           91,  43, 0,  5,  51, 76, 83 };

  viskores::cont::DataSet inputDataSet =
    viskores::cont::DataSetBuilderUniform{}.Create(viskores::Id2(gridWidth, gridHeight));
  inputDataSet.AddPointField("f", fieldValues, numMeshVertices);

  // Compute the augmented contour tree and branch decomposition in one pass.
  viskores::filter::scalar_topology::ContourTreeAugmented filter;
  filter.SetActiveField("f");
  filter.SetComputeBranchDecomposition(true);
  viskores::cont::DataSet result = filter.Execute(inputDataSet);

  // -------------------------------------------------------------------------
  // Print the mesh segmentation: for every vertex the superarc it belongs to.
  // -------------------------------------------------------------------------
  cta::IdArrayType superparents;
  result.GetField(cta::FieldNameSuperparents).GetData().AsArrayHandle(superparents);
  auto superparentPortal = superparents.ReadPortal();

  std::cout << "Superarc IDs (each vertex belongs to this superarc):\n";
  for (int row = 0; row < gridHeight; ++row)
  {
    for (int col = 0; col < gridWidth; ++col)
      std::cout << std::setw(4) << cta::MaskedIndex(superparentPortal.Get(row * gridWidth + col));
    std::cout << "\n";
  }
  std::cout << "\n";

  // -------------------------------------------------------------------------
  // Write GraphViz .dot files for the contour tree (superarcs labeled with
  // their regular vertices) and for the branch decomposition (nodes and
  // superarcs colored by branch).
  // -------------------------------------------------------------------------
  cta::IdArrayType supernodes, superarcs, whichBranch;
  result.GetField(cta::FieldNameSupernodes).GetData().AsArrayHandle(supernodes);
  result.GetField(cta::FieldNameSuperarcs).GetData().AsArrayHandle(superarcs);
  result.GetField(cta::FieldNameWhichBranch).GetData().AsArrayHandle(whichBranch);

  // Regular (non-supernode) mesh vertices on each superarc. The convenience overload returns a
  // vector-of-vectors (arcRegularVertices[i] are the vertices on superarc i); the grouping itself
  // is computed on-device. A flat-array overload is available if a nested vector is not wanted.
  std::vector<std::vector<viskores::Id>> arcRegularVertices =
    cta::ProcessContourTree::CollectRegularVerticesPerSuperarc(result);

  WriteDotFile(
    "contour_tree.dot", supernodes, superarcs, fieldValues, &arcRegularVertices, nullptr);
  WriteDotFile(
    "contour_tree_branch.dot", supernodes, superarcs, fieldValues, nullptr, &whichBranch);

  return 0;
}
