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
// scalar field, writes GraphViz .dot files, and prints a grid-shaped ASCII
// map of superarc IDs.
//
// The scalar field is the same as MakeTestDataSet().Make2DUniformDataSet1(),
// field "pointvar", from viskores/cont/testing/MakeTestDataSet.h.
//
// Render the .dot files:
//   dot -Tpdf contour_tree.dot       -o contour_tree.pdf
//   dot -Tpdf contour_tree_branch.dot -o contour_tree_branch.pdf
//
// Output fields written to the result DataSet by ContourTreeAugmented:
//   "Supernodes"   (whole-dataset) : mesh vertex ID of each supernode
//   "Superarcs"    (whole-dataset) : masked parent supernode index per supernode;
//                                    NoSuchElement() is true for the root
//   "Superparents" (point field)   : masked superarc index per mesh vertex

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
#include <sstream>
#include <utility>
#include <vector>

namespace cta = viskores::worklet::contourtree_augmented;

// GraphViz node label: the field value at meshVertex, followed by its index.
std::string NodeLabel(viskores::Id meshVertex, const viskores::Float32* fieldValues)
{
  std::ostringstream label;
  label << fieldValues[meshVertex] << " [" << meshVertex << "]";
  return label.str();
}

// Orient a superarc for rendering: edges run from the lower-valued endpoint to the
// higher-valued one (rankdir=BT then places higher values at the top).
std::pair<viskores::Id, viskores::Id> ArcEndpoints(
  viskores::Id supernodeIdx,
  viskores::Id parentIdx,
  const cta::IdArrayType::ReadPortalType& supernodePortal,
  const viskores::Float32* fieldValues)
{
  viskores::Id lowerEndpoint =
    (fieldValues[supernodePortal.Get(supernodeIdx)] < fieldValues[supernodePortal.Get(parentIdx)])
    ? supernodeIdx
    : parentIdx;
  viskores::Id upperEndpoint = (lowerEndpoint == supernodeIdx) ? parentIdx : supernodeIdx;
  return { lowerEndpoint, upperEndpoint };
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
  viskores::filter::scalar_topology::ContourTreeAugmented cta_filter;
  cta_filter.SetActiveField("f");
  cta_filter.SetComputeBranchDecomposition(true);
  viskores::cont::DataSet result = cta_filter.Execute(inputDataSet);

  // -------------------------------------------------------------------------
  // Print the superarc map as three aligned ASCII grids:
  //   vertex indices, function values, superarc IDs.
  // -------------------------------------------------------------------------
  cta::IdArrayType superparents;
  result.GetField(cta::FieldNameSuperparents).GetData().AsArrayHandle(superparents);
  auto superparentPortal = superparents.ReadPortal();

  std::cout << "Vertex indices:\n";
  for (int row = 0; row < gridHeight; ++row)
  {
    for (int col = 0; col < gridWidth; ++col)
      std::cout << std::setw(4) << row * gridWidth + col;
    std::cout << "\n";
  }
  std::cout << "\nFunction values:\n";
  for (int row = 0; row < gridHeight; ++row)
  {
    for (int col = 0; col < gridWidth; ++col)
      std::cout << std::setw(4) << static_cast<int>(fieldValues[row * gridWidth + col]);
    std::cout << "\n";
  }
  std::cout << "\nSuperarc IDs (each vertex belongs to this superarc):\n";
  for (int row = 0; row < gridHeight; ++row)
  {
    for (int col = 0; col < gridWidth; ++col)
      std::cout << std::setw(4) << cta::MaskedIndex(superparentPortal.Get(row * gridWidth + col));
    std::cout << "\n";
  }
  std::cout << "\n";

  // -------------------------------------------------------------------------
  // Write GraphViz .dot file for the contour tree.
  // Edges run from lower to higher value; rankdir=BT places higher values at top.
  // -------------------------------------------------------------------------
  cta::IdArrayType supernodes, superarcs;
  result.GetField(cta::FieldNameSupernodes).GetData().AsArrayHandle(supernodes);
  result.GetField(cta::FieldNameSuperarcs).GetData().AsArrayHandle(superarcs);
  auto supernodePortal = supernodes.ReadPortal();
  auto superarcPortal = superarcs.ReadPortal();
  viskores::Id numSupernodes = supernodes.GetNumberOfValues();

  // Regular (non-supernode) mesh vertices on each superarc. The convenience overload returns a
  // vector-of-vectors (arcRegularVertices[i] are the vertices on superarc i); the grouping itself
  // is computed on-device. A flat-array overload is available if a nested vector is not wanted.
  std::vector<std::vector<viskores::Id>> arcRegularVertices =
    cta::ProcessContourTree::CollectRegularVerticesPerSuperarc(result);

  std::ofstream dotContourTree("contour_tree.dot");
  dotContourTree << "// Render: dot -Tpdf contour_tree.dot -o contour_tree.pdf\n"
                 << "digraph ContourTree {\n"
                 << "  graph [rankdir=BT, splines=true]\n"
                 << "  node  [shape=ellipse, style=filled, fillcolor=lightblue, fontname=Courier]\n"
                 << "  edge  [fontname=Courier, fontsize=9]\n\n";

  for (viskores::Id supernodeIdx = 0; supernodeIdx < numSupernodes; ++supernodeIdx)
  {
    viskores::Id meshVertex = supernodePortal.Get(supernodeIdx);
    dotContourTree << "  n" << supernodeIdx << " [label=\"" << NodeLabel(meshVertex, fieldValues)
                   << "\"];\n";
  }
  dotContourTree << "\n";

  for (viskores::Id supernodeIdx = 0; supernodeIdx < numSupernodes; ++supernodeIdx)
  {
    viskores::Id maskedParent = superarcPortal.Get(supernodeIdx);
    if (cta::NoSuchElement(maskedParent))
      continue;
    viskores::Id parentIdx = cta::MaskedIndex(maskedParent);

    viskores::Id lowerEndpoint, upperEndpoint;
    std::tie(lowerEndpoint, upperEndpoint) =
      ArcEndpoints(supernodeIdx, parentIdx, supernodePortal, fieldValues);

    dotContourTree << "  n" << lowerEndpoint << " -> n" << upperEndpoint << " [label=\""
                   << supernodeIdx << " (";
    const std::vector<viskores::Id>& arcVerts =
      arcRegularVertices[static_cast<size_t>(supernodeIdx)];
    for (size_t k = 0; k < arcVerts.size(); ++k)
    {
      if (k)
        dotContourTree << ",";
      dotContourTree << arcVerts[k];
    }
    dotContourTree << ")\"];\n";
  }
  dotContourTree << "}\n";
  std::cout << "Wrote contour_tree.dot\n";

  // -------------------------------------------------------------------------
  // Write GraphViz .dot file for the branch decomposition.
  // -------------------------------------------------------------------------
  cta::IdArrayType whichBranch;
  result.GetField(cta::FieldNameWhichBranch).GetData().AsArrayHandle(whichBranch);
  auto whichBranchPortal = whichBranch.ReadPortal();

  // Assign a color per branch by mapping branch index to a small palette.
  const char* palette[] = { "tomato", "steelblue", "seagreen", "goldenrod",
                            "orchid", "coral",     "teal",     "peru" };
  constexpr size_t paletteSize = sizeof(palette) / sizeof(palette[0]);

  std::ofstream dotBranch("contour_tree_branch.dot");
  dotBranch << "// Render: dot -Tpdf contour_tree_branch.dot -o contour_tree_branch.pdf\n"
            << "digraph BranchDecomposition {\n"
            << "  graph [rankdir=BT, splines=true]\n"
            << "  node  [shape=ellipse, fontname=Courier]\n"
            << "  edge  [fontname=Courier, fontsize=9]\n\n";

  // Color each node by its branch (WhichBranch is set for all supernodes, including the root).
  for (viskores::Id supernodeIdx = 0; supernodeIdx < numSupernodes; ++supernodeIdx)
  {
    viskores::Id meshVertex = supernodePortal.Get(supernodeIdx);
    viskores::Id branchIdx = cta::MaskedIndex(whichBranchPortal.Get(supernodeIdx));
    const char* color = palette[static_cast<size_t>(branchIdx) % paletteSize];
    dotBranch << "  n" << supernodeIdx << " [label=\"" << NodeLabel(meshVertex, fieldValues)
              << "\", style=filled, fillcolor=" << color << "];\n";
  }
  dotBranch << "\n";

  // Edges: color by the branch of the arc.
  for (viskores::Id supernodeIdx = 0; supernodeIdx < numSupernodes; ++supernodeIdx)
  {
    viskores::Id maskedParent = superarcPortal.Get(supernodeIdx);
    if (cta::NoSuchElement(maskedParent))
      continue;
    viskores::Id parentIdx = cta::MaskedIndex(maskedParent);

    viskores::Id lowerEndpoint, upperEndpoint;
    std::tie(lowerEndpoint, upperEndpoint) =
      ArcEndpoints(supernodeIdx, parentIdx, supernodePortal, fieldValues);

    viskores::Id branchIdx = cta::MaskedIndex(whichBranchPortal.Get(supernodeIdx));
    const char* color = palette[static_cast<size_t>(branchIdx) % paletteSize];
    dotBranch << "  n" << lowerEndpoint << " -> n" << upperEndpoint << " [color=" << color
              << "];\n";
  }
  dotBranch << "}\n";
  std::cout << "Wrote contour_tree_branch.dot\n";

  return 0;
}
