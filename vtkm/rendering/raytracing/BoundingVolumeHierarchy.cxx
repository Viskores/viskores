//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <math.h>

#include <vtkm/Math.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/cont/AtomicArray.h>

#include <vtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/MortonCodes.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/rendering/raytracing/Worklets.h>

#include <vtkm/worklet/WorkletMapField.h>

#define AABB_EPSILON 0.00001f
namespace vtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

class LinearBVHBuilder
{
public:
  class CountingIterator;

  class GatherFloat32;

  template <typename Device>
  class GatherVecCast;

  class CreateLeafs;

  class BVHData;

  class PropagateAABBs;

  class TreeBuilder;

  VTKM_CONT
  LinearBVHBuilder() {}

  VTKM_CONT void SortAABBS(BVHData& bvh, bool);

  VTKM_CONT void BuildHierarchy(BVHData& bvh);

  VTKM_CONT void Build(LinearBVH& linearBVH);
}; // class LinearBVHBuilder

class LinearBVHBuilder::CountingIterator : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  CountingIterator() {}
  using ControlSignature = void(FieldOut);
  using ExecutionSignature = void(WorkIndex, _1);
  VTKM_EXEC
  void operator()(const vtkm::Id& index, vtkm::Id& outId) const { outId = index; }
}; //class countingIterator

class LinearBVHBuilder::GatherFloat32 : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  GatherFloat32() {}
  using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayOut);
  using ExecutionSignature = void(WorkIndex, _1, _2, _3);

  template <typename InType, typename OutType>
  VTKM_EXEC void operator()(const vtkm::Id& outIndex,
                            const vtkm::Id& inIndex,
                            const InType& inPortal,
                            OutType& outPortal) const
  {
    outPortal.Set(outIndex, inPortal.Get(inIndex));
  }
}; //class GatherFloat

class LinearBVHBuilder::CreateLeafs : public vtkm::worklet::WorkletMapField
{

public:
  VTKM_CONT
  CreateLeafs() {}

  typedef void ControlSignature(FieldIn, WholeArrayOut);
  typedef void ExecutionSignature(_1, _2, WorkIndex);

  template <typename LeafPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& dataIndex,
                            LeafPortalType& leafs,
                            const vtkm::Id& index) const
  {
    const vtkm::Id offset = index * 2;
    leafs.Set(offset, 1);             // number of primitives
    leafs.Set(offset + 1, dataIndex); // number of primitives
  }
}; //class createLeafs

template <typename DeviceAdapterTag>
class LinearBVHBuilder::GatherVecCast : public vtkm::worklet::WorkletMapField
{
private:
  using Vec4IdArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Id4>;
  using Vec4IntArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec4i_32>;
  using PortalConst = typename Vec4IdArrayHandle::ExecutionTypes<DeviceAdapterTag>::PortalConst;
  using Portal = typename Vec4IntArrayHandle::ExecutionTypes<DeviceAdapterTag>::Portal;

private:
  PortalConst InputPortal;
  Portal OutputPortal;

public:
  VTKM_CONT
  GatherVecCast(const Vec4IdArrayHandle& inputPortal,
                Vec4IntArrayHandle& outputPortal,
                const vtkm::Id& size)
    : InputPortal(inputPortal.PrepareForInput(DeviceAdapterTag()))
  {
    this->OutputPortal = outputPortal.PrepareForOutput(size, DeviceAdapterTag());
  }
  using ControlSignature = void(FieldIn);
  using ExecutionSignature = void(WorkIndex, _1);
  VTKM_EXEC
  void operator()(const vtkm::Id& outIndex, const vtkm::Id& inIndex) const
  {
    OutputPortal.Set(outIndex, InputPortal.Get(inIndex));
  }
}; //class GatherVec3Id

class LinearBVHBuilder::BVHData
{
public:
  vtkm::cont::ArrayHandle<vtkm::UInt32> mortonCodes;
  vtkm::cont::ArrayHandle<vtkm::Id> parent;
  vtkm::cont::ArrayHandle<vtkm::Id> leftChild;
  vtkm::cont::ArrayHandle<vtkm::Id> rightChild;
  vtkm::cont::ArrayHandle<vtkm::Id> leafs;
  vtkm::cont::ArrayHandle<vtkm::Bounds> innerBounds;
  vtkm::cont::ArrayHandleCounting<vtkm::Id> leafOffsets;
  AABBs& AABB;

  VTKM_CONT BVHData(vtkm::Id numPrimitives, AABBs& aabbs)
    : leafOffsets(0, 2, numPrimitives)
    , AABB(aabbs)
    , NumPrimitives(numPrimitives)
  {
    InnerNodeCount = NumPrimitives - 1;
    vtkm::Id size = NumPrimitives + InnerNodeCount;

    parent.Allocate(size);
    leftChild.Allocate(InnerNodeCount);
    rightChild.Allocate(InnerNodeCount);
    innerBounds.Allocate(InnerNodeCount);
    mortonCodes.Allocate(NumPrimitives);
  }

  VTKM_CONT
  ~BVHData() {}

  VTKM_CONT
  vtkm::Id GetNumberOfPrimitives() const { return NumPrimitives; }
  VTKM_CONT
  vtkm::Id GetNumberOfInnerNodes() const { return InnerNodeCount; }

private:
  vtkm::Id NumPrimitives;
  vtkm::Id InnerNodeCount;

}; // class BVH

class LinearBVHBuilder::PropagateAABBs : public vtkm::worklet::WorkletMapField
{
private:
  vtkm::Int32 LeafCount;

public:
  VTKM_CONT
  PropagateAABBs(vtkm::Int32 leafCount)
    : LeafCount(leafCount)

  {
  }
  using ControlSignature = void(WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,     //Parents
                                WholeArrayIn,     //lchild
                                WholeArrayIn,     //rchild
                                AtomicArrayInOut, //counters
                                WholeArrayInOut   // flatbvh
                                );
  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12);

  template <typename InputPortalType,
            typename OffsetPortalType,
            typename IdPortalType,
            typename AtomicType,
            typename BVHType>
  VTKM_EXEC_CONT void operator()(const vtkm::Id workIndex,
                                 const InputPortalType& xmin,
                                 const InputPortalType& ymin,
                                 const InputPortalType& zmin,
                                 const InputPortalType& xmax,
                                 const InputPortalType& ymax,
                                 const InputPortalType& zmax,
                                 const OffsetPortalType& leafOffsets,
                                 const IdPortalType& parents,
                                 const IdPortalType& leftChildren,
                                 const IdPortalType& rightChildren,
                                 AtomicType& counters,
                                 BVHType& flatBVH) const

  {
    //move up into the inner nodes
    vtkm::Id currentNode = LeafCount - 1 + workIndex;
    vtkm::Id2 childVector;
    while (currentNode != 0)
    {
      currentNode = parents.Get(currentNode);
      vtkm::Int32 oldCount = counters.Add(currentNode, 1);
      if (oldCount == 0)
      {
        return;
      }
      vtkm::Id currentNodeOffset = currentNode * 4;
      childVector[0] = leftChildren.Get(currentNode);
      childVector[1] = rightChildren.Get(currentNode);
      if (childVector[0] > (LeafCount - 2))
      {
        //our left child is a leaf, so just grab the AABB
        //and set it in the current node
        childVector[0] = childVector[0] - LeafCount + 1;

        vtkm::Vec4f_32 first4Vec; // = FlatBVH.Get(currentNode); only this one needs effects this

        first4Vec[0] = xmin.Get(childVector[0]);
        first4Vec[1] = ymin.Get(childVector[0]);
        first4Vec[2] = zmin.Get(childVector[0]);
        first4Vec[3] = xmax.Get(childVector[0]);
        flatBVH.Set(currentNodeOffset, first4Vec);

        vtkm::Vec4f_32 second4Vec = flatBVH.Get(currentNodeOffset + 1);
        second4Vec[0] = ymax.Get(childVector[0]);
        second4Vec[1] = zmax.Get(childVector[0]);
        flatBVH.Set(currentNodeOffset + 1, second4Vec);
        // set index to leaf
        vtkm::Id leafIndex = leafOffsets.Get(childVector[0]);
        childVector[0] = -(leafIndex + 1);
      }
      else
      {
        //our left child is an inner node, so gather
        //both AABBs in the child and join them for
        //the current node left AABB.
        vtkm::Id child = childVector[0] * 4;

        vtkm::Vec4f_32 cFirst4Vec = flatBVH.Get(child);
        vtkm::Vec4f_32 cSecond4Vec = flatBVH.Get(child + 1);
        vtkm::Vec4f_32 cThird4Vec = flatBVH.Get(child + 2);

        cFirst4Vec[0] = vtkm::Min(cFirst4Vec[0], cSecond4Vec[2]);
        cFirst4Vec[1] = vtkm::Min(cFirst4Vec[1], cSecond4Vec[3]);
        cFirst4Vec[2] = vtkm::Min(cFirst4Vec[2], cThird4Vec[0]);
        cFirst4Vec[3] = vtkm::Max(cFirst4Vec[3], cThird4Vec[1]);
        flatBVH.Set(currentNodeOffset, cFirst4Vec);

        vtkm::Vec4f_32 second4Vec = flatBVH.Get(currentNodeOffset + 1);
        second4Vec[0] = vtkm::Max(cSecond4Vec[0], cThird4Vec[2]);
        second4Vec[1] = vtkm::Max(cSecond4Vec[1], cThird4Vec[3]);

        flatBVH.Set(currentNodeOffset + 1, second4Vec);
      }

      if (childVector[1] > (LeafCount - 2))
      {
        //our right child is a leaf, so just grab the AABB
        //and set it in the current node
        childVector[1] = childVector[1] - LeafCount + 1;


        vtkm::Vec4f_32 second4Vec = flatBVH.Get(currentNodeOffset + 1);

        second4Vec[2] = xmin.Get(childVector[1]);
        second4Vec[3] = ymin.Get(childVector[1]);
        flatBVH.Set(currentNodeOffset + 1, second4Vec);

        vtkm::Vec4f_32 third4Vec;
        third4Vec[0] = zmin.Get(childVector[1]);
        third4Vec[1] = xmax.Get(childVector[1]);
        third4Vec[2] = ymax.Get(childVector[1]);
        third4Vec[3] = zmax.Get(childVector[1]);
        flatBVH.Set(currentNodeOffset + 2, third4Vec);

        // set index to leaf
        vtkm::Id leafIndex = leafOffsets.Get(childVector[1]);
        childVector[1] = -(leafIndex + 1);
      }
      else
      {
        //our left child is an inner node, so gather
        //both AABBs in the child and join them for
        //the current node left AABB.
        vtkm::Id child = childVector[1] * 4;

        vtkm::Vec4f_32 cFirst4Vec = flatBVH.Get(child);
        vtkm::Vec4f_32 cSecond4Vec = flatBVH.Get(child + 1);
        vtkm::Vec4f_32 cThird4Vec = flatBVH.Get(child + 2);

        vtkm::Vec4f_32 second4Vec = flatBVH.Get(currentNodeOffset + 1);
        second4Vec[2] = vtkm::Min(cFirst4Vec[0], cSecond4Vec[2]);
        second4Vec[3] = vtkm::Min(cFirst4Vec[1], cSecond4Vec[3]);
        flatBVH.Set(currentNodeOffset + 1, second4Vec);

        cThird4Vec[0] = vtkm::Min(cFirst4Vec[2], cThird4Vec[0]);
        cThird4Vec[1] = vtkm::Max(cFirst4Vec[3], cThird4Vec[1]);
        cThird4Vec[2] = vtkm::Max(cSecond4Vec[0], cThird4Vec[2]);
        cThird4Vec[3] = vtkm::Max(cSecond4Vec[1], cThird4Vec[3]);
        flatBVH.Set(currentNodeOffset + 2, cThird4Vec);
      }
      vtkm::Vec4f_32 fourth4Vec;
      vtkm::Int32 leftChild =
        static_cast<vtkm::Int32>((childVector[0] >= 0) ? childVector[0] * 4 : childVector[0]);
      memcpy(&fourth4Vec[0], &leftChild, 4);
      vtkm::Int32 rightChild =
        static_cast<vtkm::Int32>((childVector[1] >= 0) ? childVector[1] * 4 : childVector[1]);
      memcpy(&fourth4Vec[1], &rightChild, 4);
      flatBVH.Set(currentNodeOffset + 3, fourth4Vec);
    }
  }
}; //class PropagateAABBs

class LinearBVHBuilder::TreeBuilder : public vtkm::worklet::WorkletMapField
{
private:
  vtkm::Id LeafCount;
  vtkm::Id InnerCount;
  //TODO: get intrinsic support
  VTKM_EXEC
  inline vtkm::Int32 CountLeadingZeros(vtkm::UInt32& x) const
  {
    vtkm::UInt32 y;
    vtkm::UInt32 n = 32;
    y = x >> 16;
    if (y != 0)
    {
      n = n - 16;
      x = y;
    }
    y = x >> 8;
    if (y != 0)
    {
      n = n - 8;
      x = y;
    }
    y = x >> 4;
    if (y != 0)
    {
      n = n - 4;
      x = y;
    }
    y = x >> 2;
    if (y != 0)
    {
      n = n - 2;
      x = y;
    }
    y = x >> 1;
    if (y != 0)
      return vtkm::Int32(n - 2);
    return vtkm::Int32(n - x);
  }

  // returns the count of largest shared prefix between
  // two morton codes. Ties are broken by the indexes
  // a and b.
  //
  // returns count of the largest binary prefix

  template <typename MortonType>
  VTKM_EXEC inline vtkm::Int32 delta(const vtkm::Int32& a,
                                     const vtkm::Int32& b,
                                     const MortonType& mortonCodePortal) const
  {
    bool tie = false;
    bool outOfRange = (b < 0 || b > LeafCount - 1);
    //still make the call but with a valid adderss
    vtkm::Int32 bb = (outOfRange) ? 0 : b;
    vtkm::UInt32 aCode = mortonCodePortal.Get(a);
    vtkm::UInt32 bCode = mortonCodePortal.Get(bb);
    //use xor to find where they differ
    vtkm::UInt32 exOr = aCode ^ bCode;
    tie = (exOr == 0);
    //break the tie, a and b must always differ
    exOr = tie ? vtkm::UInt32(a) ^ vtkm::UInt32(bb) : exOr;
    vtkm::Int32 count = CountLeadingZeros(exOr);
    if (tie)
      count += 32;
    count = (outOfRange) ? -1 : count;
    return count;
  }

public:
  VTKM_CONT
  TreeBuilder(const vtkm::Id& leafCount)
    : LeafCount(leafCount)
    , InnerCount(leafCount - 1)
  {
  }
  using ControlSignature = void(FieldOut, FieldOut, WholeArrayIn, WholeArrayOut);
  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4);

  template <typename MortonType, typename ParentType>
  VTKM_EXEC void operator()(const vtkm::Id& index,
                            vtkm::Id& leftChild,
                            vtkm::Id& rightChild,
                            const MortonType& mortonCodePortal,
                            ParentType& parentPortal) const
  {
    vtkm::Int32 idx = vtkm::Int32(index);
    //determine range direction
    vtkm::Int32 d =
      0 > (delta(idx, idx + 1, mortonCodePortal) - delta(idx, idx - 1, mortonCodePortal)) ? -1 : 1;

    //find upper bound for the length of the range
    vtkm::Int32 minDelta = delta(idx, idx - d, mortonCodePortal);
    vtkm::Int32 lMax = 2;
    while (delta(idx, idx + lMax * d, mortonCodePortal) > minDelta)
      lMax *= 2;

    //binary search to find the lower bound
    vtkm::Int32 l = 0;
    for (int t = lMax / 2; t >= 1; t /= 2)
    {
      if (delta(idx, idx + (l + t) * d, mortonCodePortal) > minDelta)
        l += t;
    }

    vtkm::Int32 j = idx + l * d;
    vtkm::Int32 deltaNode = delta(idx, j, mortonCodePortal);
    vtkm::Int32 s = 0;
    vtkm::Float32 divFactor = 2.f;
    //find the split position using a binary search
    for (vtkm::Int32 t = (vtkm::Int32)ceil(vtkm::Float32(l) / divFactor);;
         divFactor *= 2, t = (vtkm::Int32)ceil(vtkm::Float32(l) / divFactor))
    {
      if (delta(idx, idx + (s + t) * d, mortonCodePortal) > deltaNode)
      {
        s += t;
      }

      if (t == 1)
        break;
    }

    vtkm::Int32 split = idx + s * d + vtkm::Min(d, 0);
    //assign parent/child pointers
    if (vtkm::Min(idx, j) == split)
    {
      //leaf
      parentPortal.Set(split + InnerCount, idx);
      leftChild = split + InnerCount;
    }
    else
    {
      //inner node
      parentPortal.Set(split, idx);
      leftChild = split;
    }


    if (vtkm::Max(idx, j) == split + 1)
    {
      //leaf
      parentPortal.Set(split + InnerCount + 1, idx);
      rightChild = split + InnerCount + 1;
    }
    else
    {
      parentPortal.Set(split + 1, idx);
      rightChild = split + 1;
    }
  }
}; // class TreeBuilder

VTKM_CONT void LinearBVHBuilder::SortAABBS(BVHData& bvh, bool singleAABB)
{
  //create array of indexes to be sorted with morton codes
  vtkm::cont::ArrayHandle<vtkm::Id> iterator;
  iterator.Allocate(bvh.GetNumberOfPrimitives());

  vtkm::worklet::DispatcherMapField<CountingIterator> iterDispatcher;
  iterDispatcher.Invoke(iterator);

  //sort the morton codes

  vtkm::cont::Algorithm::SortByKey(bvh.mortonCodes, iterator);

  vtkm::Id arraySize = bvh.GetNumberOfPrimitives();
  vtkm::cont::ArrayHandle<vtkm::Float32> temp1;
  vtkm::cont::ArrayHandle<vtkm::Float32> temp2;
  temp1.Allocate(arraySize);

  vtkm::worklet::DispatcherMapField<GatherFloat32> gatherDispatcher;

  //xmins
  gatherDispatcher.Invoke(iterator, bvh.AABB.xmins, temp1);

  temp2 = bvh.AABB.xmins;
  bvh.AABB.xmins = temp1;
  temp1 = temp2;
  //ymins
  gatherDispatcher.Invoke(iterator, bvh.AABB.ymins, temp1);

  temp2 = bvh.AABB.ymins;
  bvh.AABB.ymins = temp1;
  temp1 = temp2;
  //zmins
  gatherDispatcher.Invoke(iterator, bvh.AABB.zmins, temp1);

  temp2 = bvh.AABB.zmins;
  bvh.AABB.zmins = temp1;
  temp1 = temp2;
  //xmaxs
  gatherDispatcher.Invoke(iterator, bvh.AABB.xmaxs, temp1);

  temp2 = bvh.AABB.xmaxs;
  bvh.AABB.xmaxs = temp1;
  temp1 = temp2;
  //ymaxs
  gatherDispatcher.Invoke(iterator, bvh.AABB.ymaxs, temp1);

  temp2 = bvh.AABB.ymaxs;
  bvh.AABB.ymaxs = temp1;
  temp1 = temp2;
  //zmaxs
  gatherDispatcher.Invoke(iterator, bvh.AABB.zmaxs, temp1);

  temp2 = bvh.AABB.zmaxs;
  bvh.AABB.zmaxs = temp1;
  temp1 = temp2;

  // Create the leaf references
  bvh.leafs.Allocate(arraySize * 2);
  // we only actually have a single primitive, but the algorithm
  // requires 2. Make sure they both point to the original
  // primitive
  if (singleAABB)
  {
    auto iterPortal = iterator.GetPortalControl();
    for (int i = 0; i < 2; ++i)
    {
      iterPortal.Set(i, 0);
    }
  }

  vtkm::worklet::DispatcherMapField<CreateLeafs> leafDispatcher;
  leafDispatcher.Invoke(iterator, bvh.leafs);

} // method SortAABB

VTKM_CONT void LinearBVHBuilder::Build(LinearBVH& linearBVH)
{

  //
  //
  // This algorithm needs at least 2 AABBs
  //
  bool singleAABB = false;
  vtkm::Id numberOfAABBs = linearBVH.GetNumberOfAABBs();
  if (numberOfAABBs == 1)
  {
    numberOfAABBs = 2;
    singleAABB = true;
    vtkm::Float32 xmin = linearBVH.AABB.xmins.GetPortalControl().Get(0);
    vtkm::Float32 ymin = linearBVH.AABB.ymins.GetPortalControl().Get(0);
    vtkm::Float32 zmin = linearBVH.AABB.zmins.GetPortalControl().Get(0);
    vtkm::Float32 xmax = linearBVH.AABB.xmaxs.GetPortalControl().Get(0);
    vtkm::Float32 ymax = linearBVH.AABB.ymaxs.GetPortalControl().Get(0);
    vtkm::Float32 zmax = linearBVH.AABB.zmaxs.GetPortalControl().Get(0);

    linearBVH.AABB.xmins.Allocate(2);
    linearBVH.AABB.ymins.Allocate(2);
    linearBVH.AABB.zmins.Allocate(2);
    linearBVH.AABB.xmaxs.Allocate(2);
    linearBVH.AABB.ymaxs.Allocate(2);
    linearBVH.AABB.zmaxs.Allocate(2);
    for (int i = 0; i < 2; ++i)
    {
      linearBVH.AABB.xmins.GetPortalControl().Set(i, xmin);
      linearBVH.AABB.ymins.GetPortalControl().Set(i, ymin);
      linearBVH.AABB.zmins.GetPortalControl().Set(i, zmin);
      linearBVH.AABB.xmaxs.GetPortalControl().Set(i, xmax);
      linearBVH.AABB.ymaxs.GetPortalControl().Set(i, ymax);
      linearBVH.AABB.zmaxs.GetPortalControl().Set(i, zmax);
    }
  }


  const vtkm::Id numBBoxes = numberOfAABBs;
  BVHData bvh(numBBoxes, linearBVH.GetAABBs());


  // Find the extent of all bounding boxes to generate normalization for morton codes
  vtkm::Vec3f_32 minExtent(vtkm::Infinity32(), vtkm::Infinity32(), vtkm::Infinity32());
  vtkm::Vec3f_32 maxExtent(
    vtkm::NegativeInfinity32(), vtkm::NegativeInfinity32(), vtkm::NegativeInfinity32());
  maxExtent[0] = vtkm::cont::Algorithm::Reduce(bvh.AABB.xmaxs, maxExtent[0], MaxValue());
  maxExtent[1] = vtkm::cont::Algorithm::Reduce(bvh.AABB.ymaxs, maxExtent[1], MaxValue());
  maxExtent[2] = vtkm::cont::Algorithm::Reduce(bvh.AABB.zmaxs, maxExtent[2], MaxValue());
  minExtent[0] = vtkm::cont::Algorithm::Reduce(bvh.AABB.xmins, minExtent[0], MinValue());
  minExtent[1] = vtkm::cont::Algorithm::Reduce(bvh.AABB.ymins, minExtent[1], MinValue());
  minExtent[2] = vtkm::cont::Algorithm::Reduce(bvh.AABB.zmins, minExtent[2], MinValue());

  linearBVH.TotalBounds.X.Min = minExtent[0];
  linearBVH.TotalBounds.X.Max = maxExtent[0];
  linearBVH.TotalBounds.Y.Min = minExtent[1];
  linearBVH.TotalBounds.Y.Max = maxExtent[1];
  linearBVH.TotalBounds.Z.Min = minExtent[2];
  linearBVH.TotalBounds.Z.Max = maxExtent[2];

  vtkm::Vec3f_32 deltaExtent = maxExtent - minExtent;
  vtkm::Vec3f_32 inverseExtent;
  for (int i = 0; i < 3; ++i)
  {
    inverseExtent[i] = (deltaExtent[i] == 0.f) ? 0 : 1.f / deltaExtent[i];
  }

  //Generate the morton codes
  vtkm::worklet::DispatcherMapField<MortonCodeAABB> mortonDispatch(
    MortonCodeAABB(inverseExtent, minExtent));
  mortonDispatch.Invoke(bvh.AABB.xmins,
                        bvh.AABB.ymins,
                        bvh.AABB.zmins,
                        bvh.AABB.xmaxs,
                        bvh.AABB.ymaxs,
                        bvh.AABB.zmaxs,
                        bvh.mortonCodes);
  linearBVH.Allocate(bvh.GetNumberOfPrimitives());

  SortAABBS(bvh, singleAABB);

  vtkm::worklet::DispatcherMapField<TreeBuilder> treeDispatch(
    TreeBuilder(bvh.GetNumberOfPrimitives()));
  treeDispatch.Invoke(bvh.leftChild, bvh.rightChild, bvh.mortonCodes, bvh.parent);

  const vtkm::Int32 primitiveCount = vtkm::Int32(bvh.GetNumberOfPrimitives());

  vtkm::cont::ArrayHandle<vtkm::Int32> counters;
  counters.Allocate(bvh.GetNumberOfPrimitives() - 1);

  vtkm::cont::ArrayHandleConstant<vtkm::Int32> zero(0, bvh.GetNumberOfPrimitives() - 1);
  vtkm::cont::Algorithm::Copy(zero, counters);

  vtkm::worklet::DispatcherMapField<PropagateAABBs> propDispatch(PropagateAABBs{ primitiveCount });

  propDispatch.Invoke(bvh.AABB.xmins,
                      bvh.AABB.ymins,
                      bvh.AABB.zmins,
                      bvh.AABB.xmaxs,
                      bvh.AABB.ymaxs,
                      bvh.AABB.zmaxs,
                      bvh.leafOffsets,
                      bvh.parent,
                      bvh.leftChild,
                      bvh.rightChild,
                      counters,
                      linearBVH.FlatBVH);

  linearBVH.Leafs = bvh.leafs;
}
} //namespace detail

LinearBVH::LinearBVH()
  : IsConstructed(false)
  , CanConstruct(false){};

VTKM_CONT
LinearBVH::LinearBVH(AABBs& aabbs)
  : AABB(aabbs)
  , IsConstructed(false)
  , CanConstruct(true)
{
}

VTKM_CONT
LinearBVH::LinearBVH(const LinearBVH& other)
  : AABB(other.AABB)
  , FlatBVH(other.FlatBVH)
  , Leafs(other.Leafs)
  , LeafCount(other.LeafCount)
  , IsConstructed(other.IsConstructed)
  , CanConstruct(other.CanConstruct)
{
}

VTKM_CONT void LinearBVH::Allocate(const vtkm::Id& leafCount)
{
  LeafCount = leafCount;
  FlatBVH.Allocate((leafCount - 1) * 4);
}

void LinearBVH::Construct()
{
  if (IsConstructed)
    return;
  if (!CanConstruct)
    throw vtkm::cont::ErrorBadValue(
      "Linear BVH: coordinates and triangles must be set before calling construct!");

  detail::LinearBVHBuilder builder;
  builder.Build(*this);
}

VTKM_CONT
void LinearBVH::SetData(AABBs& aabbs)
{
  AABB = aabbs;
  IsConstructed = false;
  CanConstruct = true;
}

// explicitly export
//template VTKM_RENDERING_EXPORT void LinearBVH::ConstructOnDevice<
//  vtkm::cont::DeviceAdapterTagSerial>(vtkm::cont::DeviceAdapterTagSerial);
//#ifdef VTKM_ENABLE_TBB
//template VTKM_RENDERING_EXPORT void LinearBVH::ConstructOnDevice<vtkm::cont::DeviceAdapterTagTBB>(
//  vtkm::cont::DeviceAdapterTagTBB);
//#endif
//#ifdef VTKM_ENABLE_OPENMP
//template VTKM_CONT_EXPORT void LinearBVH::ConstructOnDevice<vtkm::cont::DeviceAdapterTagOpenMP>(
//  vtkm::cont::DeviceAdapterTagOpenMP);
//#endif
//#ifdef VTKM_ENABLE_CUDA
//template VTKM_RENDERING_EXPORT void LinearBVH::ConstructOnDevice<vtkm::cont::DeviceAdapterTagCuda>(
//  vtkm::cont::DeviceAdapterTagCuda);
//#endif
//
VTKM_CONT
bool LinearBVH::GetIsConstructed() const
{
  return IsConstructed;
}

vtkm::Id LinearBVH::GetNumberOfAABBs() const
{
  return AABB.xmins.GetNumberOfValues();
}

AABBs& LinearBVH::GetAABBs()
{
  return AABB;
}
}
}
} // namespace vtkm::rendering::raytracing
