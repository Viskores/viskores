//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef vtkm_worklet_contourtree_augmented_process_contourtree_inc_branch_h
#define vtkm_worklet_contourtree_augmented_process_contourtree_inc_branch_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/worklet/contourtree_augmented/ContourTree.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/PiecewiseLinearFunction.h>

#include <cmath>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{

// TODO The pointered list structure and use of std::vector don't seem to fit well with using Branch with VTKM
template <typename T>
class Branch
{
public:
  vtkm::Id extremum;                // Index of the extremum in the mesh
  T extremumVal;                    // Value at the extremum
  vtkm::Id saddle;                  // Index of the saddle in the mesh (or minimum for root branch)
  T saddleVal;                      // Corresponding value
  vtkm::Id volume;                  // Volume
  Branch<T>* parent;                // Pointer to parent, or nullptr if no parent
  std::vector<Branch<T>*> children; // List of pointers to children

  // Create branch decomposition from contour tree
  template <typename StorageType>
  static Branch<T>* ComputeBranchDecomposition(
    const IdArrayType& contourTreeSuperparents,
    const IdArrayType& contourTreeSupernodes,
    const IdArrayType& whichBranch,
    const IdArrayType& branchMinimum,
    const IdArrayType& branchMaximum,
    const IdArrayType& branchSaddle,
    const IdArrayType& branchParent,
    const IdArrayType& sortOrder,
    const vtkm::cont::ArrayHandle<T, StorageType>& dataField,
    bool dataFieldIsSorted);

  // Simplify branch composition down to target size (i.e., consisting of targetSize branches)
  void simplifyToSize(vtkm::Id targetSize, bool usePersistenceSorter = true);

  // Print the branch decomposition
  void print(std::ostream& os, std::string::size_type indent = 0) const;

  // Persistence of branch
  T persistence() { return std::fabs(extremumVal - saddleVal); }

  // Save branch decomposition (with current branch as root) to file
  // void save(const char *filename) const;

  // Load branch decomposition from file (static function that returns pointer to root branch)
  static Branch<T>* load(const char* filename);

  // Destroy branch (deleting children and propagating volume to parent)
  ~Branch();

  // Compute list of relevant/interesting isovalues
  void getRelevantValues(int type, T eps, std::vector<T>& values) const;

  void accumulateIntervals(int type, T eps, PiecewiseLinearFunction<T>& plf) const;

private:
  // Private default constructore to ensure that branch decomposition can only be created from a contour tree or loaded from storate (via static methods)
  Branch()
    : extremum((vtkm::Id)NO_SUCH_ELEMENT)
    , extremumVal(0)
    , saddle((vtkm::Id)NO_SUCH_ELEMENT)
    , saddleVal(0)
    , volume(0)
    , parent(nullptr)
    , children()
  {
  }

  // Remove symbolic perturbation, i.e., branches with zero persistence
  void removeSymbolicPerturbation();

  // Internal functions to save/load branches to/from a stream
  // void save(std::ostream& os) const;
  // static Branch<T>* load(std::istream& is);
}; // class Branch


/*
template <typename T>
static void writeBinary(std::ostream& os, const T& val)
  { // writeBinary()
    os.write(reinterpret_cast<const char*>(&val), sizeof(T));
  } // writeBinary()


template <typename T>
static void readBinary(std::istream& is, T& val)
  { // readBinary()
    is.read(reinterpret_cast<char*>(&val), sizeof(T));
  } // readBinary()
*/

template <typename T>
struct PersistenceSorter
{ // PersistenceSorter()
  inline bool operator()(Branch<T>* a, Branch<T>* b) { return a->persistence() < b->persistence(); }
}; // PersistenceSorter()


template <typename T>
struct VolumeSorter
{ // VolumeSorter()
  inline bool operator()(Branch<T>* a, Branch<T>* b) { return a->volume < b->volume; }
}; // VolumeSorter()


template <typename T>
template <typename StorageType>
Branch<T>* Branch<T>::ComputeBranchDecomposition(
  const IdArrayType& contourTreeSuperparents,
  const IdArrayType& contourTreeSupernodes,
  const IdArrayType& whichBranch,
  const IdArrayType& branchMinimum,
  const IdArrayType& branchMaximum,
  const IdArrayType& branchSaddle,
  const IdArrayType& branchParent,
  const IdArrayType& sortOrder,
  const vtkm::cont::ArrayHandle<T, StorageType>& dataField,
  bool dataFieldIsSorted)
{ // ComputeBranchDecomposition()
  auto branchMinimumPortal = branchMinimum.GetPortalConstControl();
  auto branchMaximumPortal = branchMaximum.GetPortalConstControl();
  auto branchSaddlePortal = branchSaddle.GetPortalConstControl();
  auto branchParentPortal = branchParent.GetPortalConstControl();
  auto sortOrderPortal = sortOrder.GetPortalConstControl();
  auto supernodesPortal = contourTreeSupernodes.GetPortalConstControl();
  auto dataFieldPortal = dataField.GetPortalConstControl();
  vtkm::Id nBranches = branchSaddle.GetNumberOfValues();
  std::vector<Branch<T>*> branches;
  Branch<T>* root = nullptr;
  branches.reserve(static_cast<std::size_t>(nBranches));

  for (int branchID = 0; branchID < nBranches; ++branchID)
    branches.push_back(new Branch<T>);

  // Reconstruct explicit branch decomposition from array representation
  for (std::size_t branchID = 0; branchID < static_cast<std::size_t>(nBranches); ++branchID)
  {
    if (!noSuchElement(branchSaddlePortal.Get(static_cast<vtkm::Id>(branchID))))
    {
      branches[branchID]->saddle = maskedIndex(
        supernodesPortal.Get(maskedIndex(branchSaddlePortal.Get(static_cast<vtkm::Id>(branchID)))));
      vtkm::Id branchMin = maskedIndex(supernodesPortal.Get(
        maskedIndex(branchMinimumPortal.Get(static_cast<vtkm::Id>(branchID)))));
      vtkm::Id branchMax = maskedIndex(supernodesPortal.Get(
        maskedIndex(branchMaximumPortal.Get(static_cast<vtkm::Id>(branchID)))));
      if (branchMin < branches[branchID]->saddle)
        branches[branchID]->extremum = branchMin;
      else if (branchMax > branches[branchID]->saddle)
        branches[branchID]->extremum = branchMax;
      else
      {
        std::cerr << "Internal error";
        return 0;
      }
    }
    else
    {
      branches[branchID]->saddle =
        supernodesPortal.Get(maskedIndex(branchMinimumPortal.Get(static_cast<vtkm::Id>(branchID))));
      branches[branchID]->extremum =
        supernodesPortal.Get(maskedIndex(branchMaximumPortal.Get(static_cast<vtkm::Id>(branchID))));
    }

    if (dataFieldIsSorted)
    {
      branches[branchID]->saddleVal = dataFieldPortal.Get(branches[branchID]->saddle);
      branches[branchID]->extremumVal = dataFieldPortal.Get(branches[branchID]->extremum);
    }
    else
    {
      branches[branchID]->saddleVal =
        dataFieldPortal.Get(sortOrderPortal.Get(branches[branchID]->saddle));
      branches[branchID]->extremumVal =
        dataFieldPortal.Get(sortOrderPortal.Get(branches[branchID]->extremum));
    }

    branches[branchID]->saddle = sortOrderPortal.Get(branches[branchID]->saddle);
    branches[branchID]->extremum = sortOrderPortal.Get(branches[branchID]->extremum);

    if (noSuchElement(branchParentPortal.Get(static_cast<vtkm::Id>(branchID))))
    {
      root = branches[branchID]; // No parent -> this is the root branch
    }
    else
    {
      branches[branchID]->parent = branches[static_cast<size_t>(
        maskedIndex(branchParentPortal.Get(static_cast<vtkm::Id>(branchID))))];
      branches[branchID]->parent->children.push_back(branches[branchID]);
    }
  }

  // FIXME: This is a somewhat hackish way to compute the volume, but it works
  // It would probably be better to compute this from the already computed volume information
  auto whichBranchPortal = whichBranch.GetPortalConstControl();
  auto superparentsPortal = contourTreeSuperparents.GetPortalConstControl();
  for (vtkm::Id i = 0; i < contourTreeSuperparents.GetNumberOfValues(); i++)
  {
    branches[static_cast<size_t>(
               maskedIndex(whichBranchPortal.Get(maskedIndex(superparentsPortal.Get(i)))))]
      ->volume++; // Increment volume
  }
  if (root)
  {
    root->removeSymbolicPerturbation();
  }

  return root;
} // ComputeBranchDecomposition()


template <typename T>
void Branch<T>::simplifyToSize(vtkm::Id targetSize, bool usePersistenceSorter)
{ // simplifyToSize()
  if (targetSize <= 1)
    return;

  // Top-down simplification, starting from one branch and adding in the rest on a biggest-first basis
  std::vector<Branch<T>*> q;
  q.push_back(this);

  std::vector<Branch<T>*> active;
  while (active.size() < static_cast<std::size_t>(targetSize) && !q.empty())
  {
    if (usePersistenceSorter)
    {
      std::pop_heap(
        q.begin(),
        q.end(),
        PersistenceSorter<
          T>()); // FIXME: This should be volume, but we were doing this wrong for the demo, so let's start with doing this wrong here, too
    }
    else
    {
      std::pop_heap(
        q.begin(),
        q.end(),
        VolumeSorter<
          T>()); // FIXME: This should be volume, but we were doing this wrong for the demo, so let's start with doing this wrong here, too
    }
    Branch<T>* b = q.back();
    q.pop_back();

    active.push_back(b);

    for (Branch<T>* c : b->children)
    {
      q.push_back(c);
      if (usePersistenceSorter)
      {
        std::push_heap(q.begin(), q.end(), PersistenceSorter<T>());
      }
      else
      {
        std::push_heap(q.begin(), q.end(), VolumeSorter<T>());
      }
    }
  }

  // Rest are inactive
  for (Branch<T>* b : q)
  {
    // Hackish, remove c from its parents child list
    if (b->parent)
      b->parent->children.erase(
        std::remove(b->parent->children.begin(), b->parent->children.end(), b));

    delete b;
  }
} // simplifyToSize()


template <typename T>
void Branch<T>::print(std::ostream& os, std::string::size_type indent) const
{ // print()
  os << std::string(indent, ' ') << "{" << std::endl;
  os << std::string(indent, ' ') << "  saddle = " << saddleVal << " (" << saddle << ")"
     << std::endl;
  os << std::string(indent, ' ') << "  extremum = " << extremumVal << " (" << extremum << ")"
     << std::endl;
  os << std::string(indent, ' ') << "  volume = " << volume << std::endl;
  if (!children.empty())
  {
    os << std::string(indent, ' ') << "  children = [" << std::endl;
    for (Branch<T>* c : children)
      c->print(os, indent + 4);
    os << std::string(indent, ' ') << std::string(indent, ' ') << "  ]" << std::endl;
  }
  os << std::string(indent, ' ') << "}" << std::endl;
} // print()


/*template<typename T>
void Branch<T>::save(const char *filename) const
  { // save()
    std::ofstream os(filename);
    save(os);
  } // save()


template<typename T>
Branch<T>* Branch<T>::load(const char* filename)
  { // load()
    std::ifstream is(filename);
    return load(is);
  } // load()
*/

template <typename T>
Branch<T>::~Branch()
{ // ~Branch()
  for (Branch<T>* c : children)
    delete c;
  if (parent)
    parent->volume += volume;
} // ~Branch()


// TODO this recursive accumlation of values does not lend itself well to the use of VTKM data structures
template <typename T>
void Branch<T>::getRelevantValues(int type, T eps, std::vector<T>& values) const
{ // getRelevantValues()
  T val;

  bool isMax = false;
  if (extremumVal > saddleVal)
    isMax = true;

  switch (type)
  {
    default:
    case 0:
      val = saddleVal + (isMax ? +eps : -eps);
      break;
    case 1:
      val = T(0.5f) * (extremumVal + saddleVal);
      break;
    case 2:
      val = extremumVal + (isMax ? -eps : +eps);
      break;
  }
  if (parent)
    values.push_back(val);
  for (Branch* c : children)
    c->getRelevantValues(type, eps, values);
} // getRelevantValues()


template <typename T>
void Branch<T>::accumulateIntervals(int type, T eps, PiecewiseLinearFunction<T>& plf) const
{ //accumulateIntervals()
  bool isMax = (extremumVal > saddleVal);
  T val;

  switch (type)
  {
    default:
    case 0:
      val = saddleVal + (isMax ? +eps : -eps);
      break;
    case 1:
      val = T(0.5f) * (extremumVal + saddleVal);
      break;
    case 2:
      val = extremumVal + (isMax ? -eps : +eps);
      break;
  }

  if (parent)
  {
    PiecewiseLinearFunction<T> addPLF;
    addPLF.addSample(saddleVal, 0.0);
    addPLF.addSample(extremumVal, 0.0);
    addPLF.addSample(val, 1.0);
    plf += addPLF;
  }
  for (Branch<T>* c : children)
    c->accumulateIntervals(type, eps, plf);
} // accumulateIntervals()


template <typename T>
void Branch<T>::removeSymbolicPerturbation()
{                                      // removeSymbolicPerturbation()
  std::vector<Branch<T>*> newChildren; // Temporary list of children that are not flat

  for (Branch<T>* c : children)
  {
    // First recursively remove symbolic perturbation (zero persistence branches) for  all children below the current child
    // Necessary to be able to detect whether we can remove the current child
    c->removeSymbolicPerturbation();

    // Does child have zero persistence (flat region)
    if (c->extremumVal == c->saddleVal && c->children.empty())
    {
      // If yes, then we get its associated volume and delete it
      delete c; // Will add volume to parent, i.e., us
    }
    else
    {
      // Otherwise, keep child
      newChildren.push_back(c);
    }
  }
  // Swap out new list of children
  children.swap(newChildren);
} // removeSymbolicPerturbation()

/*
template<typename T>
void Branch<T>::save(std::ostream& os) const
  { // save()
    writeBinary(os, extremum);
    writeBinary(os, extremumVal);
    writeBinary(os, saddle);
    writeBinary(os, saddleVal);
    writeBinary(os, volume);
    vtkm::Id n_children = children.size();
    writeBinary(os, n_children);
    for (Branch<T> *c : children) c->save(os);
  } // save()


template<typename T>
Branch<T>* Branch<T>::load(std::istream& is)
  { // load()
    Branch<T> *res = new Branch<T>;
    readBinary(is, res->extremum);
    readBinary(is, res->extremumVal);
    readBinary(is, res->saddle);
    readBinary(is, res->saddleVal);
    readBinary(is, res->volume);
    vtkm::Id n_children;
    readBinary(is, n_children);
    for (vtkm::Id cNo = 0; cNo < n_children; ++cNo)
    {
        Branch<T> *c = load(is);
        c->parent = res;
        res->children.push_back(c);
    }
    return res;
  } // load()
*/

} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
