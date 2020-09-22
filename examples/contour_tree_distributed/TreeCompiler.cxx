#include "TreeCompiler.h"
#include <iomanip>
#include <vtkm/cont/DataSet.h>

#define PRINT_WIDTH 12

// stream output
std::ostream& operator<<(std::ostream& outStream, SupernodeOnSuperarc& node)
{ // stream output
  outStream << node.lowEnd << " " << node.highEnd << " " << node.dataValue << " " << node.globalID
            << std::endl;
  return outStream;
} // stream output

// stream input
std::istream& operator>>(std::istream& inStream, SupernodeOnSuperarc& node)
{ // stream input
  inStream >> node.lowEnd >> node.highEnd >> node.dataValue >> node.globalID;
  return inStream;
} // stream input

// routine to add a known hierarchical tree to it
// note that this DOES NOT finalise - we don't want too many sorts
void TreeCompiler::AddHierarchicalTree(const vtkm::cont::DataSet& addedTree)
{ // TreeCompiler::AddHierarchicalTree()
  // Copy relevant tree content to STL arrays
  vtkm::cont::VariantArrayHandle dataValues_array = addedTree.GetField("DataValues").GetData();
  std::vector<vtkm::FloatDefault> dataValues(dataValues_array.GetNumberOfValues());
  auto dataValues_handle = vtkm::cont::make_ArrayHandle(dataValues, vtkm::CopyFlag::Off);
  vtkm::cont::ArrayCopy(dataValues_array.ResetTypes(vtkm::List<vtkm::FloatDefault>{}),
                        dataValues_handle);
  dataValues_handle.SyncControlArray();

  auto regularNodeGlobalIds_array = addedTree.GetField("RegularNodeGlobalIds").GetData();
  std::vector<vtkm::Id> regularNodeGlobalIds(regularNodeGlobalIds_array.GetNumberOfValues());
  auto regularNodeGlobalIds_handle =
    vtkm::cont::make_ArrayHandle(regularNodeGlobalIds, vtkm::CopyFlag::Off);
  vtkm::cont::ArrayCopy(regularNodeGlobalIds_array.ResetTypes(vtkm::List<vtkm::Id>{}),
                        regularNodeGlobalIds_handle);
  regularNodeGlobalIds_handle
    .SyncControlArray(); //Forces values to get updated if copy happened on GPU

  auto superarcs_array = addedTree.GetField("Superarcs").GetData();
  std::vector<vtkm::Id> added_tree_superarcs(superarcs_array.GetNumberOfValues());
  auto superarcs_handle = vtkm::cont::make_ArrayHandle(added_tree_superarcs, vtkm::CopyFlag::Off);
  vtkm::cont::ArrayCopy(superarcs_array.ResetTypes(vtkm::List<vtkm::Id>{}), superarcs_handle);
  superarcs_handle.SyncControlArray(); //Forces values to get updated if copy happened on GPU

  auto supernodes_array = addedTree.GetField("Supernodes").GetData();
  std::vector<vtkm::Id> added_tree_supernodes(supernodes_array.GetNumberOfValues());
  auto supernodes_handle = vtkm::cont::make_ArrayHandle(added_tree_supernodes, vtkm::CopyFlag::Off);
  vtkm::cont::ArrayCopy(supernodes_array.ResetTypes(vtkm::List<vtkm::Id>{}), supernodes_handle);
  supernodes_handle.SyncControlArray(); //Forces values to get updated if copy happened on GPU

  auto superparents_array = addedTree.GetField("Superparents").GetData();
  std::vector<vtkm::Id> superparents(superparents_array.GetNumberOfValues());
  auto superparents_handle = vtkm::cont::make_ArrayHandle(superparents, vtkm::CopyFlag::Off);
  vtkm::cont::ArrayCopy(superparents_array.ResetTypes(vtkm::List<vtkm::Id>{}), superparents_handle);
  superparents_handle.SyncControlArray(); //Forces values to get updated if copy happened on GPU

  // loop through all of the supernodes in the hierarchical tree
  for (indexType supernode = 0; supernode < added_tree_supernodes.size(); supernode++)
  { // per supernode
    // retrieve the regular ID for the supernode
    indexType regularId = added_tree_supernodes[supernode];
    indexType globalId = regularNodeGlobalIds[regularId];
    dataType dataVal = dataValues[regularId];

    // retrieve the supernode at the far end
    indexType superTo = added_tree_superarcs[supernode];

    // now test - if it is NO_SUCH_ELEMENT, there are two possibilities
    if (vtkm::worklet::contourtree_augmented::NoSuchElement(superTo))
    { // no Superto

      // retrieve the superparent
      indexType superparent = superparents[regularId];


      // the root node will have itself as its superparent
      if (superparent == supernode)
        continue;
      else
      { // not own superparent - an attachment point
        // retrieve the superparent's from & to
        indexType regularFrom = added_tree_supernodes[superparent];
        indexType globalFrom = regularNodeGlobalIds[regularFrom];
        indexType superParentTo = added_tree_superarcs[superparent];
        indexType regularTo =
          added_tree_supernodes[vtkm::worklet::contourtree_augmented::MaskedIndex(superParentTo)];
        indexType globalTo = regularNodeGlobalIds[regularTo];

        // test the superTo to see whether we ascend or descend
        // note that we will never have NO_SUCH_ELEMENT here
        if (vtkm::worklet::contourtree_augmented::IsAscending(superParentTo))
        { // ascending
          this->supernodes.push_back(SupernodeOnSuperarc(globalId, dataVal, globalFrom, globalTo));
        } // ascending
        else
        { // descending
          this->supernodes.push_back(SupernodeOnSuperarc(globalId, dataVal, globalTo, globalFrom));
        } // descending
      }   // not own superparent - an attachment point
    }     // no Superto
    else
    { // there is a valid superarc
      // retrieve the "to" and convert to global
      indexType maskedTo = vtkm::worklet::contourtree_augmented::MaskedIndex(superTo);
      indexType regularTo = added_tree_supernodes[maskedTo];
      indexType globalTo = regularNodeGlobalIds[regularTo];
      dataType dataTo = dataValues[regularTo];

      // test the superTo to see whether we ascend or descend
      // note that we will never have NO_SUCH_ELEMENT here
      // we add both ends
      if (vtkm::worklet::contourtree_augmented::IsAscending(superTo))
      { // ascending
        this->supernodes.push_back(SupernodeOnSuperarc(globalId, dataVal, globalId, globalTo));
        this->supernodes.push_back(SupernodeOnSuperarc(globalTo, dataTo, globalId, globalTo));
      } // ascending
      else
      { // descending
        this->supernodes.push_back(SupernodeOnSuperarc(globalId, dataVal, globalTo, globalId));
        this->supernodes.push_back(SupernodeOnSuperarc(globalTo, dataTo, globalTo, globalId));
      } // descending
    }   // there is a valid superarc
  }     // per supernode

} // TreeCompiler::AddHierarchicalTree()

// routine to compute the actual superarcs
void TreeCompiler::ComputeSuperarcs()
{ // TreeCompiler::ComputeSuperarcs()
  // first we sort the vector
  std::sort(supernodes.begin(), supernodes.end());

  // we could do a unique test here, but it's easier just to suppress it inside the loop

  // now we loop through it: note the -1
  // this is because we know a priori that the last one is the last supernode on a superarc
  // and would fail the test inside the loop. By putting it in the loop test, we avoid having
  // to have an explicit if statement inside the loop
  for (indexType supernode = 0; supernode < supernodes.size() - 1; supernode++)
  { // loop through supernodes
    // this is actually painfully simple: if the (lowEnd, highEnd) don't match the next one,
    // then we're at the end of the group and do nothing.  Otherwise, we link to the next one
    if ((supernodes[supernode].lowEnd != supernodes[supernode + 1].lowEnd) ||
        (supernodes[supernode].highEnd != supernodes[supernode + 1].highEnd))
      continue;

    // if the supernode matches, then we have a repeat, and can suppress
    if (supernodes[supernode].globalID == supernodes[supernode + 1].globalID)
      continue;

    // otherwise, add a superarc to the list
    superarcs.push_back(Edge(supernodes[supernode].globalID, supernodes[supernode + 1].globalID));
  } // loop through supernodes

  // now sort them
  std::sort(superarcs.begin(), superarcs.end());
} // TreeCompiler::ComputeSuperarcs()

// routine to print the superarcs
void TreeCompiler::PrintSuperarcs()
{ // TreeCompiler::PrintSuperarcs()
  std::cout << "============" << std::endl;
  std::cout << "Contour Tree" << std::endl;

  for (indexType superarc = 0; superarc < superarcs.size(); superarc++)
  { // per superarc
    if (superarcs[superarc].low < superarcs[superarc].high)
    { // order by ID not value
      std::cout << std::setw(PRINT_WIDTH) << superarcs[superarc].low << " ";
      std::cout << std::setw(PRINT_WIDTH) << superarcs[superarc].high << std::endl;
    } // order by ID not value
    else
    { // order by ID not value
      std::cout << std::setw(PRINT_WIDTH) << superarcs[superarc].high << " ";
      std::cout << std::setw(PRINT_WIDTH) << superarcs[superarc].low << std::endl;
    } // order by ID not value

  } // per superarc

} // TreeCompiler::PrintSuperarcs()

// routine to write out binary file
void TreeCompiler::WriteBinary(FILE* outFile)
{ // WriteBinary()
  // do a bulk write of the entire contents
  // no error checking, no type checking, no nothing
  fwrite(&(supernodes[0]), sizeof(SupernodeOnSuperarc), supernodes.size(), outFile);
} // WriteBinary()

// routine to read in binary file and append
void TreeCompiler::ReadBinary(FILE* inFile)
{ // ReadBinary()
  // use fseek to jump to the end
  fseek(inFile, 0, SEEK_END);

  // use fTell to retrieve the size of the file
  long long nBytes = ftell(inFile);
  // now rewind
  rewind(inFile);

  // compute how many elements are to be read
  long long nSupernodes = nBytes / sizeof(SupernodeOnSuperarc);

  // retrieve the current size
  long long currentSize = supernodes.size();

  // resize to add the right number
  supernodes.resize(currentSize + nSupernodes);

  // now read directly into the right chunk
  fread(&(supernodes[currentSize]), sizeof(SupernodeOnSuperarc), nSupernodes, inFile);

} // ReadBinary()

// stream output - just dumps the supernodeonsuperarcs
std::ostream& operator<<(std::ostream& outStream, TreeCompiler& tree)
{ // stream output
  for (indexType supernode = 0; supernode < tree.supernodes.size(); supernode++)
    outStream << tree.supernodes[supernode];
  return outStream;
} // stream output

// stream input - reads in the supernodeonsuperarcs & appends them
std::istream& operator>>(std::istream& inStream, TreeCompiler& tree)
{ // stream input
  while (!inStream.eof())
  {
    SupernodeOnSuperarc tempNode;
    inStream >> tempNode;
    tree.supernodes.push_back(tempNode);
  }
  // we will overshoot, so subtract one
  tree.supernodes.resize(tree.supernodes.size() - 1);
  return inStream;
} // stream input
