#ifndef _TREECOMPILER_H_
#define _TREECOMPILER_H_

#include <iostream>
#include <vtkm/Types.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

// FIXME/HACK: Define here for compatibility with PPP TreeCompiler
typedef double dataType;
typedef unsigned long long indexType;

// small class for storing the contour arcs
class Edge
{ // Edge
public:
  indexType low, high;

  // constructor - defaults to -1
  Edge(vtkm::Id Low = -1, vtkm::Id High = -1)
    : low(Low)
    , high(High)
  {
  }
}; // Edge

// comparison operator <
inline bool operator<(const Edge LHS, const Edge RHS)
{ // operator <
  if (LHS.low < RHS.low)
    return true;
  if (LHS.low > RHS.low)
    return false;
  if (LHS.high < RHS.high)
    return true;
  if (LHS.high > RHS.high)
    return false;
  return false;
} // operator <

// a helper class which stores a single supernode inserted onto a superarc
class SupernodeOnSuperarc
{ // class SupernodeOnSuperarc
public:
  // the global ID of the supernode
  indexType globalID;
  // the data value stored at the supernode
  dataType dataValue;

  // the low and high ends of the superarc it is on (may be itself)
  indexType lowEnd, highEnd;

  // constructor
  SupernodeOnSuperarc(indexType GlobalID = vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
                      dataType DataValue = vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
                      indexType LowEnd = vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
                      indexType HighEnd = vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT)
    : globalID(GlobalID)
    , dataValue(DataValue)
    , lowEnd(LowEnd)
    , highEnd(HighEnd)
  { // constructor
  } // constructor
};  // class SupernodeOnSuperarc

// overloaded comparison operator
// primary sort is by superarc (low, high),
// then secondary sort on datavalue
// tertiary on globalID to implement simulated simplicity
inline bool operator<(const SupernodeOnSuperarc& left, const SupernodeOnSuperarc& right)
{ // < operator
  // simple lexicographic sort
  if (left.lowEnd < right.lowEnd)
    return true;
  if (left.lowEnd > right.lowEnd)
    return false;
  if (left.highEnd < right.highEnd)
    return true;
  if (left.highEnd > right.highEnd)
    return false;
  if (left.dataValue < right.dataValue)
    return true;
  if (left.dataValue > right.dataValue)
    return false;
  if (left.globalID < right.globalID)
    return true;
  if (left.globalID > right.globalID)
    return false;

  // fall-through (shouldn't happen, but)
  // if they're the same, it's false
  return false;
} // < operator

// stream output
std::ostream& operator<<(std::ostream& outStream, SupernodeOnSuperarc& node);

// stream input
std::istream& operator>>(std::istream& inStream, SupernodeOnSuperarc& node);

// the class that compiles the contour tree
class TreeCompiler
{ // class TreeCompiler
public:
  // we want a vector of supernodes on superarcs
  std::vector<SupernodeOnSuperarc> supernodes;

  // and a vector of Edges (the output)
  std::vector<Edge> superarcs;

  // default constructor sets it to empty
  TreeCompiler()
  { // constructor
    // clear out the supernode array
    supernodes.resize(0);
    // and the superarc array
    superarcs.resize(0);
  } // constructor

  // routine to add a known hierarchical tree to it
  // note that this DOES NOT finalise - we don't want too many sorts
  void AddHierarchicalTree(const vtkm::cont::DataSet& addedTree);

  // routine to compute the actual superarcs
  void ComputeSuperarcs();

  // routine to print the superarcs
  void PrintSuperarcs();

  // routine to write out binary file
  void WriteBinary(FILE* outFile);

  // routine to read in binary file & append to contents
  void ReadBinary(FILE* inFile);

}; // class TreeCompiler

#endif
