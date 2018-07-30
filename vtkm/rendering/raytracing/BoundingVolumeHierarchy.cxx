//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <math.h>

#include <vtkm/Math.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/cont/AtomicArray.h>

#include <vtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/MortonCodes.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/rendering/raytracing/Worklets.h>

#include <vtkm/worklet/DispatcherMapField.h>
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
  class FindAABBs;

  template <typename Device>
  class GatherFloat32;

  template <typename Device>
  class GatherVecCast;

  class BVHData;

  template <typename Device>
  class PropagateAABBs;

  template <typename Device>
  class TreeBuilder;

  VTKM_CONT
  LinearBVHBuilder() {}

  template <typename Device>
  VTKM_CONT void SortAABBS(
    BVHData& bvh,
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>& triangleIndices,
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32, 4>>& outputTriangleIndices,
    Device vtkmNotUsed(device));

  template <typename Device>
  VTKM_CONT void RunOnDevice(LinearBVH& linearBVH, Device device);
}; // class LinearBVHBuilder

class LinearBVHBuilder::CountingIterator : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  CountingIterator() {}
  using ControlSignature = void(FieldOut<>);
  using ExecutionSignature = void(WorkIndex, _1);
  VTKM_EXEC
  void operator()(const vtkm::Id& index, vtkm::Id& outId) const { outId = index; }
}; //class countingIterator

class LinearBVHBuilder::FindAABBs : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  FindAABBs() {}
  using ControlSignature = void(FieldIn<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                WholeArrayIn<Vec3RenderingTypes>);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8);
  template <typename PointPortalType>
  VTKM_EXEC void operator()(const vtkm::Vec<vtkm::Id, 4> indices,
                            vtkm::Float32& xmin,
                            vtkm::Float32& ymin,
                            vtkm::Float32& zmin,
                            vtkm::Float32& xmax,
                            vtkm::Float32& ymax,
                            vtkm::Float32& zmax,
                            const PointPortalType& points) const
  {
    // cast to Float32
    vtkm::Vec<vtkm::Float32, 3> point;
    point = static_cast<vtkm::Vec<vtkm::Float32, 3>>(points.Get(indices[1]));
    xmin = point[0];
    ymin = point[1];
    zmin = point[2];
    xmax = xmin;
    ymax = ymin;
    zmax = zmin;
    point = static_cast<vtkm::Vec<vtkm::Float32, 3>>(points.Get(indices[2]));
    xmin = vtkm::Min(xmin, point[0]);
    ymin = vtkm::Min(ymin, point[1]);
    zmin = vtkm::Min(zmin, point[2]);
    xmax = vtkm::Max(xmax, point[0]);
    ymax = vtkm::Max(ymax, point[1]);
    zmax = vtkm::Max(zmax, point[2]);
    point = static_cast<vtkm::Vec<vtkm::Float32, 3>>(points.Get(indices[3]));
    xmin = vtkm::Min(xmin, point[0]);
    ymin = vtkm::Min(ymin, point[1]);
    zmin = vtkm::Min(zmin, point[2]);
    xmax = vtkm::Max(xmax, point[0]);
    ymax = vtkm::Max(ymax, point[1]);
    zmax = vtkm::Max(zmax, point[2]);


    vtkm::Float32 xEpsilon, yEpsilon, zEpsilon;
    const vtkm::Float32 minEpsilon = 1e-6f;
    xEpsilon = vtkm::Max(minEpsilon, AABB_EPSILON * (xmax - xmin));
    yEpsilon = vtkm::Max(minEpsilon, AABB_EPSILON * (ymax - ymin));
    zEpsilon = vtkm::Max(minEpsilon, AABB_EPSILON * (zmax - zmin));

    xmin -= xEpsilon;
    ymin -= yEpsilon;
    zmin -= zEpsilon;
    xmax += xEpsilon;
    ymax += yEpsilon;
    zmax += zEpsilon;
  }
}; //class FindAABBs

template <typename Device>
class LinearBVHBuilder::GatherFloat32 : public vtkm::worklet::WorkletMapField
{
private:
  using FloatArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Float32>;
  using PortalConst = typename FloatArrayHandle::ExecutionTypes<Device>::PortalConst;
  using Portal = typename FloatArrayHandle::ExecutionTypes<Device>::Portal;
  PortalConst InputPortal;
  Portal OutputPortal;

public:
  VTKM_CONT
  GatherFloat32(const FloatArrayHandle& inputPortal,
                FloatArrayHandle& outputPortal,
                const vtkm::Id& size)
    : InputPortal(inputPortal.PrepareForInput(Device()))
  {
    this->OutputPortal = outputPortal.PrepareForOutput(size, Device());
  }
  using ControlSignature = void(FieldIn<>);
  using ExecutionSignature = void(WorkIndex, _1);
  VTKM_EXEC
  void operator()(const vtkm::Id& outIndex, const vtkm::Id& inIndex) const
  {
    OutputPortal.Set(outIndex, InputPortal.Get(inIndex));
  }
}; //class GatherFloat

template <typename Device>
class LinearBVHBuilder::GatherVecCast : public vtkm::worklet::WorkletMapField
{
private:
  using Vec4IdArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>;
  using Vec4IntArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32, 4>>;
  using PortalConst = typename Vec4IdArrayHandle::ExecutionTypes<Device>::PortalConst;
  using Portal = typename Vec4IntArrayHandle::ExecutionTypes<Device>::Portal;

private:
  PortalConst InputPortal;
  Portal OutputPortal;

public:
  VTKM_CONT
  GatherVecCast(const Vec4IdArrayHandle& inputPortal,
                Vec4IntArrayHandle& outputPortal,
                const vtkm::Id& size)
    : InputPortal(inputPortal.PrepareForInput(Device()))
  {
    this->OutputPortal = outputPortal.PrepareForOutput(size, Device());
  }
  using ControlSignature = void(FieldIn<>);
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
  //TODO: make private
  vtkm::cont::ArrayHandle<vtkm::Float32>* xmins;
  vtkm::cont::ArrayHandle<vtkm::Float32>* ymins;
  vtkm::cont::ArrayHandle<vtkm::Float32>* zmins;
  vtkm::cont::ArrayHandle<vtkm::Float32>* xmaxs;
  vtkm::cont::ArrayHandle<vtkm::Float32>* ymaxs;
  vtkm::cont::ArrayHandle<vtkm::Float32>* zmaxs;

  vtkm::cont::ArrayHandle<vtkm::UInt32> mortonCodes;
  vtkm::cont::ArrayHandle<vtkm::Id> parent;
  vtkm::cont::ArrayHandle<vtkm::Id> leftChild;
  vtkm::cont::ArrayHandle<vtkm::Id> rightChild;

  template <typename Device>
  VTKM_CONT BVHData(vtkm::Id numPrimitives, Device vtkmNotUsed(device))
    : NumPrimitives(numPrimitives)
  {
    InnerNodeCount = NumPrimitives - 1;
    vtkm::Id size = NumPrimitives + InnerNodeCount;
    xmins = new vtkm::cont::ArrayHandle<vtkm::Float32>();
    ymins = new vtkm::cont::ArrayHandle<vtkm::Float32>();
    zmins = new vtkm::cont::ArrayHandle<vtkm::Float32>();
    xmaxs = new vtkm::cont::ArrayHandle<vtkm::Float32>();
    ymaxs = new vtkm::cont::ArrayHandle<vtkm::Float32>();
    zmaxs = new vtkm::cont::ArrayHandle<vtkm::Float32>();

    parent.PrepareForOutput(size, Device());
    leftChild.PrepareForOutput(InnerNodeCount, Device());
    rightChild.PrepareForOutput(InnerNodeCount, Device());
    mortonCodes.PrepareForOutput(NumPrimitives, Device());
  }

  VTKM_CONT
  ~BVHData()
  {
    //
    delete xmins;
    delete ymins;
    delete zmins;
    delete xmaxs;
    delete ymaxs;
    delete zmaxs;
  }
  VTKM_CONT
  vtkm::Id GetNumberOfPrimitives() const { return NumPrimitives; }
  VTKM_CONT
  vtkm::Id GetNumberOfInnerNodes() const { return InnerNodeCount; }

private:
  vtkm::Id NumPrimitives;
  vtkm::Id InnerNodeCount;

}; // class BVH

template <typename Device>
class LinearBVHBuilder::PropagateAABBs : public vtkm::worklet::WorkletMapField
{
private:
  using IdArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Id>;
  using Int8Handle = typename vtkm::cont::ArrayHandle<vtkm::Int8>;
  using Float2ArrayHandle = typename vtkm::cont::ArrayHandle<Vec<vtkm::Float32, 2>>;
  using VecInt2Handle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32, 2>>;
  using Float4ArrayHandle = typename vtkm::cont::ArrayHandle<Vec<vtkm::Float32, 4>>;

  using IdConstPortal = typename IdArrayHandle::ExecutionTypes<Device>::PortalConst;
  using Float2ArrayPortal = typename Float2ArrayHandle::ExecutionTypes<Device>::Portal;
  using Int2ArrayPortal = typename VecInt2Handle::ExecutionTypes<Device>::Portal;
  using Int8ArrayPortal = typename Int8Handle::ExecutionTypes<Device>::Portal;
  using Float4ArrayPortal = typename Float4ArrayHandle::ExecutionTypes<Device>::Portal;

  Float4ArrayPortal FlatBVH;
  IdConstPortal Parents;
  IdConstPortal LeftChildren;
  IdConstPortal RightChildren;
  vtkm::Int32 LeafCount;
  //Int8Handle Counters;
  //Int8ArrayPortal CountersPortal;
  vtkm::exec::AtomicArrayExecutionObject<vtkm::Int32, Device> Counters;

public:
  VTKM_CONT
  PropagateAABBs(IdArrayHandle& parents,
                 IdArrayHandle& leftChildren,
                 IdArrayHandle& rightChildren,
                 vtkm::Int32 leafCount,
                 Float4ArrayHandle flatBVH,
                 const vtkm::cont::AtomicArray<vtkm::Int32>& counters)
    : Parents(parents.PrepareForInput(Device()))
    , LeftChildren(leftChildren.PrepareForInput(Device()))
    , RightChildren(rightChildren.PrepareForInput(Device()))
    , LeafCount(leafCount)
    , Counters(counters.PrepareForExecution(Device()))

  {
    this->FlatBVH = flatBVH.PrepareForOutput((LeafCount - 1) * 4, Device());
  }
  using ControlSignature = void(WholeArrayIn<Scalar>,
                                WholeArrayIn<Scalar>,
                                WholeArrayIn<Scalar>,
                                WholeArrayIn<Scalar>,
                                WholeArrayIn<Scalar>,
                                WholeArrayIn<Scalar>);
  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4, _5, _6);

  template <typename InputPortalType>
  VTKM_EXEC_CONT void operator()(const vtkm::Id workIndex,
                                 const InputPortalType& xmin,
                                 const InputPortalType& ymin,
                                 const InputPortalType& zmin,
                                 const InputPortalType& xmax,
                                 const InputPortalType& ymax,
                                 const InputPortalType& zmax) const
  {
    //move up into the inner nodes
    vtkm::Id currentNode = LeafCount - 1 + workIndex;
    vtkm::Vec<vtkm::Id, 2> childVector;
    while (currentNode != 0)
    {
      currentNode = Parents.Get(currentNode);

      vtkm::Int32 oldCount = Counters.Add(currentNode, 1);
      if (oldCount == 0)
        return;
      vtkm::Id currentNodeOffset = currentNode * 4;
      childVector[0] = LeftChildren.Get(currentNode);
      childVector[1] = RightChildren.Get(currentNode);
      if (childVector[0] > (LeafCount - 2))
      {
        childVector[0] = childVector[0] - LeafCount + 1;

        vtkm::Vec<vtkm::Float32, 4>
          first4Vec; // = FlatBVH.Get(currentNode); only this one needs effects this

        first4Vec[0] = xmin.Get(childVector[0]);
        first4Vec[1] = ymin.Get(childVector[0]);
        first4Vec[2] = zmin.Get(childVector[0]);
        first4Vec[3] = xmax.Get(childVector[0]);
        FlatBVH.Set(currentNodeOffset, first4Vec);

        vtkm::Vec<vtkm::Float32, 4> second4Vec = FlatBVH.Get(currentNodeOffset + 1);
        second4Vec[0] = ymax.Get(childVector[0]);
        second4Vec[1] = zmax.Get(childVector[0]);
        FlatBVH.Set(currentNodeOffset + 1, second4Vec);

        childVector[0] = -(childVector[0] + 1);
      }
      else
      {
        vtkm::Id child = childVector[0] * 4;

        vtkm::Vec<vtkm::Float32, 4> cFirst4Vec = FlatBVH.Get(child);
        vtkm::Vec<vtkm::Float32, 4> cSecond4Vec = FlatBVH.Get(child + 1);
        vtkm::Vec<vtkm::Float32, 4> cThird4Vec = FlatBVH.Get(child + 2);

        cFirst4Vec[0] = vtkm::Min(cFirst4Vec[0], cSecond4Vec[2]);
        cFirst4Vec[1] = vtkm::Min(cFirst4Vec[1], cSecond4Vec[3]);
        cFirst4Vec[2] = vtkm::Min(cFirst4Vec[2], cThird4Vec[0]);
        cFirst4Vec[3] = vtkm::Max(cFirst4Vec[3], cThird4Vec[1]);
        FlatBVH.Set(currentNodeOffset, cFirst4Vec);

        vtkm::Vec<vtkm::Float32, 4> second4Vec = FlatBVH.Get(currentNodeOffset + 1);
        second4Vec[0] = vtkm::Max(cSecond4Vec[0], cThird4Vec[2]);
        second4Vec[1] = vtkm::Max(cSecond4Vec[1], cThird4Vec[3]);

        FlatBVH.Set(currentNodeOffset + 1, second4Vec);
      }

      if (childVector[1] > (LeafCount - 2))
      {
        childVector[1] = childVector[1] - LeafCount + 1;


        vtkm::Vec<vtkm::Float32, 4> second4Vec = FlatBVH.Get(currentNodeOffset + 1);

        second4Vec[2] = xmin.Get(childVector[1]);
        second4Vec[3] = ymin.Get(childVector[1]);
        FlatBVH.Set(currentNodeOffset + 1, second4Vec);

        vtkm::Vec<vtkm::Float32, 4> third4Vec;
        third4Vec[0] = zmin.Get(childVector[1]);
        third4Vec[1] = xmax.Get(childVector[1]);
        third4Vec[2] = ymax.Get(childVector[1]);
        third4Vec[3] = zmax.Get(childVector[1]);
        FlatBVH.Set(currentNodeOffset + 2, third4Vec);
        childVector[1] = -(childVector[1] + 1);
      }
      else
      {

        vtkm::Id child = childVector[1] * 4;

        vtkm::Vec<vtkm::Float32, 4> cFirst4Vec = FlatBVH.Get(child);
        vtkm::Vec<vtkm::Float32, 4> cSecond4Vec = FlatBVH.Get(child + 1);
        vtkm::Vec<vtkm::Float32, 4> cThird4Vec = FlatBVH.Get(child + 2);

        vtkm::Vec<vtkm::Float32, 4> second4Vec = FlatBVH.Get(currentNodeOffset + 1);
        second4Vec[2] = vtkm::Min(cFirst4Vec[0], cSecond4Vec[2]);
        second4Vec[3] = vtkm::Min(cFirst4Vec[1], cSecond4Vec[3]);
        FlatBVH.Set(currentNodeOffset + 1, second4Vec);

        cThird4Vec[0] = vtkm::Min(cFirst4Vec[2], cThird4Vec[0]);
        cThird4Vec[1] = vtkm::Max(cFirst4Vec[3], cThird4Vec[1]);
        cThird4Vec[2] = vtkm::Max(cSecond4Vec[0], cThird4Vec[2]);
        cThird4Vec[3] = vtkm::Max(cSecond4Vec[1], cThird4Vec[3]);
        FlatBVH.Set(currentNodeOffset + 2, cThird4Vec);
      }
      vtkm::Vec<vtkm::Float32, 4> fourth4Vec(0.f, 0.f, 0.f, 0.f);
      vtkm::Int32 leftChild =
        static_cast<vtkm::Int32>((childVector[0] >= 0) ? childVector[0] * 4 : childVector[0]);
      memcpy(&fourth4Vec[0], &leftChild, 4);
      vtkm::Int32 rightChild =
        static_cast<vtkm::Int32>((childVector[1] >= 0) ? childVector[1] * 4 : childVector[1]);
      memcpy(&fourth4Vec[1], &rightChild, 4);
      FlatBVH.Set(currentNodeOffset + 3, fourth4Vec);
    }
  }
}; //class PropagateAABBs


template <typename Device>
class LinearBVHBuilder::TreeBuilder : public vtkm::worklet::WorkletMapField
{
public:
  using UIntArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::UInt32>;
  using IdArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Id>;
  using UIntPortalType = typename UIntArrayHandle::ExecutionTypes<Device>::PortalConst;
  using IdPortalType = typename IdArrayHandle::ExecutionTypes<Device>::Portal;

private:
  UIntPortalType MortonCodePortal;
  IdPortalType ParentPortal;
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

  VTKM_EXEC
  inline vtkm::Int32 delta(const vtkm::Int32& a, const vtkm::Int32& b) const
  {
    bool tie = false;
    bool outOfRange = (b < 0 || b > LeafCount - 1);
    //still make the call but with a valid adderss
    vtkm::Int32 bb = (outOfRange) ? 0 : b;
    vtkm::UInt32 aCode = MortonCodePortal.Get(a);
    vtkm::UInt32 bCode = MortonCodePortal.Get(bb);
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
  TreeBuilder(const UIntArrayHandle& mortonCodesHandle,
              IdArrayHandle& parentHandle,
              const vtkm::Id& leafCount)
    : MortonCodePortal(mortonCodesHandle.PrepareForInput(Device()))
    , LeafCount(leafCount)
  {
    InnerCount = LeafCount - 1;
    this->ParentPortal = parentHandle.PrepareForOutput(InnerCount + LeafCount, Device());
  }
  using ControlSignature = void(FieldOut<>, FieldOut<>);
  using ExecutionSignature = void(WorkIndex, _1, _2);
  VTKM_EXEC
  void operator()(const vtkm::Id& index, vtkm::Id& leftChild, vtkm::Id& rightChild) const
  {
    vtkm::Int32 idx = vtkm::Int32(index);
    //something = MortonCodePortal.Get(index) + 1;
    //determine range direction
    vtkm::Int32 d = 0 > (delta(idx, idx + 1) - delta(idx, idx - 1)) ? -1 : 1;

    //find upper bound for the length of the range
    vtkm::Int32 minDelta = delta(idx, idx - d);
    vtkm::Int32 lMax = 2;
    while (delta(idx, idx + lMax * d) > minDelta)
      lMax *= 2;

    //binary search to find the lower bound
    vtkm::Int32 l = 0;
    for (int t = lMax / 2; t >= 1; t /= 2)
    {
      if (delta(idx, idx + (l + t) * d) > minDelta)
        l += t;
    }

    vtkm::Int32 j = idx + l * d;
    vtkm::Int32 deltaNode = delta(idx, j);
    vtkm::Int32 s = 0;
    vtkm::Float32 divFactor = 2.f;
    //find the split position using a binary search
    for (vtkm::Int32 t = (vtkm::Int32)ceil(vtkm::Float32(l) / divFactor);;
         divFactor *= 2, t = (vtkm::Int32)ceil(vtkm::Float32(l) / divFactor))
    {
      if (delta(idx, idx + (s + t) * d) > deltaNode)
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
      ParentPortal.Set(split + InnerCount, idx);
      leftChild = split + InnerCount;
    }
    else
    {
      //inner node
      ParentPortal.Set(split, idx);
      leftChild = split;
    }


    if (vtkm::Max(idx, j) == split + 1)
    {
      //leaf
      ParentPortal.Set(split + InnerCount + 1, idx);
      rightChild = split + InnerCount + 1;
    }
    else
    {
      ParentPortal.Set(split + 1, idx);
      rightChild = split + 1;
    }
  }
}; // class TreeBuilder

template <typename Device>
VTKM_CONT void LinearBVHBuilder::SortAABBS(
  BVHData& bvh,
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>& triangleIndices,
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32, 4>>& outputTriangleIndices,
  Device vtkmNotUsed(device))
{
  //create array of indexes to be sorted with morton codes
  vtkm::cont::ArrayHandle<vtkm::Id> iterator;
  iterator.PrepareForOutput(bvh.GetNumberOfPrimitives(), Device());
  vtkm::worklet::DispatcherMapField<CountingIterator, Device> iteratorDispatcher;
  iteratorDispatcher.Invoke(iterator);

  //std::cout<<"\n\n\n";
  //sort the morton codes

  vtkm::cont::DeviceAdapterAlgorithm<Device>::SortByKey(bvh.mortonCodes, iterator);

  vtkm::Id arraySize = bvh.GetNumberOfPrimitives();
  vtkm::cont::ArrayHandle<vtkm::Float32>* tempStorage;
  vtkm::cont::ArrayHandle<vtkm::Float32>* tempPtr;


  tempStorage = new vtkm::cont::ArrayHandle<vtkm::Float32>();
  //xmins
  vtkm::worklet::DispatcherMapField<GatherFloat32<Device>, Device>(
    GatherFloat32<Device>(*bvh.xmins, *tempStorage, arraySize))
    .Invoke(iterator);

  tempPtr = bvh.xmins;
  bvh.xmins = tempStorage;
  tempStorage = tempPtr;

  vtkm::worklet::DispatcherMapField<GatherFloat32<Device>, Device>(
    GatherFloat32<Device>(*bvh.ymins, *tempStorage, arraySize))
    .Invoke(iterator);

  tempPtr = bvh.ymins;
  bvh.ymins = tempStorage;
  tempStorage = tempPtr;
  //zmins
  vtkm::worklet::DispatcherMapField<GatherFloat32<Device>, Device>(
    GatherFloat32<Device>(*bvh.zmins, *tempStorage, arraySize))
    .Invoke(iterator);
  tempPtr = bvh.zmins;
  bvh.zmins = tempStorage;
  tempStorage = tempPtr;
  //xmaxs
  vtkm::worklet::DispatcherMapField<GatherFloat32<Device>, Device>(
    GatherFloat32<Device>(*bvh.xmaxs, *tempStorage, arraySize))
    .Invoke(iterator);

  tempPtr = bvh.xmaxs;
  bvh.xmaxs = tempStorage;
  tempStorage = tempPtr;
  //ymaxs
  vtkm::worklet::DispatcherMapField<GatherFloat32<Device>, Device>(
    GatherFloat32<Device>(*bvh.ymaxs, *tempStorage, arraySize))
    .Invoke(iterator);

  tempPtr = bvh.ymaxs;
  bvh.ymaxs = tempStorage;
  tempStorage = tempPtr;
  //zmaxs
  vtkm::worklet::DispatcherMapField<GatherFloat32<Device>, Device>(
    GatherFloat32<Device>(*bvh.zmaxs, *tempStorage, arraySize))
    .Invoke(iterator);

  tempPtr = bvh.zmaxs;
  bvh.zmaxs = tempStorage;
  tempStorage = tempPtr;

  vtkm::worklet::DispatcherMapField<GatherVecCast<Device>, Device>(
    GatherVecCast<Device>(triangleIndices, outputTriangleIndices, arraySize))
    .Invoke(iterator);
  delete tempStorage;

} // method SortAABBs

// Adding this as a template parameter to allow restricted types and
// storage for dynamic coordinate system to limit crazy code bloat and
// compile times.
//
template <typename Device>
VTKM_CONT void LinearBVHBuilder::RunOnDevice(LinearBVH& linearBVH, Device device)
{
  Logger* logger = Logger::GetInstance();
  logger->OpenLogEntry("bvh_constuct");
  logger->AddLogData("device", GetDeviceString(Device()));
  vtkm::cont::Timer<Device> constructTimer;

  auto coordsHandle = linearBVH.GetCoordsHandle();
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> triangleIndices = linearBVH.GetTriangles();
  vtkm::Id numberOfTriangles = linearBVH.GetNumberOfTriangles();

  logger->AddLogData("bvh_num_triangles ", numberOfTriangles);

  const vtkm::Id numBBoxes = numberOfTriangles;
  BVHData bvh(numBBoxes, device);

  vtkm::cont::Timer<Device> timer;
  vtkm::worklet::DispatcherMapField<FindAABBs, Device>(FindAABBs())
    .Invoke(triangleIndices,
            *bvh.xmins,
            *bvh.ymins,
            *bvh.zmins,
            *bvh.xmaxs,
            *bvh.ymaxs,
            *bvh.zmaxs,
            coordsHandle);

  vtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("find_aabb", time);
  timer.Reset();

  // Find the extent of all bounding boxes to generate normalization for morton codes
  vtkm::Vec<vtkm::Float32, 3> minExtent(vtkm::Infinity32(), vtkm::Infinity32(), vtkm::Infinity32());
  vtkm::Vec<vtkm::Float32, 3> maxExtent(
    vtkm::NegativeInfinity32(), vtkm::NegativeInfinity32(), vtkm::NegativeInfinity32());
  maxExtent[0] =
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Reduce(*bvh.xmaxs, maxExtent[0], MaxValue());
  maxExtent[1] =
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Reduce(*bvh.ymaxs, maxExtent[1], MaxValue());
  maxExtent[2] =
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Reduce(*bvh.zmaxs, maxExtent[2], MaxValue());
  minExtent[0] =
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Reduce(*bvh.xmins, minExtent[0], MinValue());
  minExtent[1] =
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Reduce(*bvh.ymins, minExtent[1], MinValue());
  minExtent[2] =
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Reduce(*bvh.zmins, minExtent[2], MinValue());

  time = timer.GetElapsedTime();
  logger->AddLogData("calc_extents", time);
  timer.Reset();

  vtkm::Vec<vtkm::Float32, 3> deltaExtent = maxExtent - minExtent;
  vtkm::Vec<vtkm::Float32, 3> inverseExtent;
  for (int i = 0; i < 3; ++i)
  {
    inverseExtent[i] = (deltaExtent[i] == 0.f) ? 0 : 1.f / deltaExtent[i];
  }

  //Generate the morton codes
  vtkm::worklet::DispatcherMapField<MortonCodeAABB, Device>(
    MortonCodeAABB(inverseExtent, minExtent))
    .Invoke(
      *bvh.xmins, *bvh.ymins, *bvh.zmins, *bvh.xmaxs, *bvh.ymaxs, *bvh.zmaxs, bvh.mortonCodes);

  time = timer.GetElapsedTime();
  logger->AddLogData("morton_codes", time);
  timer.Reset();

  linearBVH.Allocate(bvh.GetNumberOfPrimitives(), Device());

  SortAABBS(bvh, triangleIndices, linearBVH.LeafNodes, Device());

  time = timer.GetElapsedTime();
  logger->AddLogData("sort_aabbs", time);
  timer.Reset();

  vtkm::worklet::DispatcherMapField<TreeBuilder<Device>, Device>(
    TreeBuilder<Device>(bvh.mortonCodes, bvh.parent, bvh.GetNumberOfPrimitives()))
    .Invoke(bvh.leftChild, bvh.rightChild);

  time = timer.GetElapsedTime();
  logger->AddLogData("build_tree", time);
  timer.Reset();

  const vtkm::Int32 primitiveCount = vtkm::Int32(bvh.GetNumberOfPrimitives());

  vtkm::cont::ArrayHandle<vtkm::Int32> counters;
  counters.PrepareForOutput(bvh.GetNumberOfPrimitives() - 1, Device());
  vtkm::Int32 zero = 0;
  vtkm::worklet::DispatcherMapField<MemSet<vtkm::Int32>, Device>(MemSet<vtkm::Int32>(zero))
    .Invoke(counters);
  vtkm::cont::AtomicArray<vtkm::Int32> atomicCounters(counters);


  vtkm::worklet::DispatcherMapField<PropagateAABBs<Device>, Device>(
    PropagateAABBs<Device>(
      bvh.parent, bvh.leftChild, bvh.rightChild, primitiveCount, linearBVH.FlatBVH, atomicCounters))
    .Invoke(*bvh.xmins, *bvh.ymins, *bvh.zmins, *bvh.xmaxs, *bvh.ymaxs, *bvh.zmaxs);

  time = timer.GetElapsedTime();
  logger->AddLogData("propagate_aabbs", time);

  time = constructTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
}
} //namespace detail

struct LinearBVH::ConstructFunctor
{
  LinearBVH* Self;
  VTKM_CONT
  ConstructFunctor(LinearBVH* self)
    : Self(self)
  {
  }
  template <typename Device>
  bool operator()(Device)
  {
    Self->ConstructOnDevice(Device());
    return true;
  }
};

LinearBVH::LinearBVH()
  : IsConstructed(false)
  , CanConstruct(false){};

VTKM_CONT
LinearBVH::LinearBVH(vtkm::cont::ArrayHandleVirtualCoordinates coordsHandle,
                     vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> triangles,
                     vtkm::Bounds coordBounds)
  : CoordBounds(coordBounds)
  , CoordsHandle(coordsHandle)
  , Triangles(triangles)
  , IsConstructed(false)
  , CanConstruct(true)
{
}

VTKM_CONT
LinearBVH::LinearBVH(const LinearBVH& other)
  : FlatBVH(other.FlatBVH)
  , LeafNodes(other.LeafNodes)
  , LeafCount(other.LeafCount)
  , CoordBounds(other.CoordBounds)
  , CoordsHandle(other.CoordsHandle)
  , Triangles(other.Triangles)
  , IsConstructed(other.IsConstructed)
  , CanConstruct(other.CanConstruct)
{
}
template <typename Device>
VTKM_CONT void LinearBVH::Allocate(const vtkm::Id& leafCount, Device deviceAdapter)
{
  LeafCount = leafCount;
  LeafNodes.PrepareForOutput(leafCount, deviceAdapter);
  FlatBVH.PrepareForOutput((leafCount - 1) * 4, deviceAdapter);
}

void LinearBVH::Construct()
{
  if (IsConstructed)
    return;
  if (!CanConstruct)
    throw vtkm::cont::ErrorBadValue(
      "Linear BVH: coordinates and triangles must be set before calling construct!");

  ConstructFunctor functor(this);
  vtkm::cont::TryExecute(functor);
  IsConstructed = true;
}

VTKM_CONT
void LinearBVH::SetData(vtkm::cont::ArrayHandleVirtualCoordinates coordsHandle,
                        vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> triangles,
                        vtkm::Bounds coordBounds)
{
  CoordBounds = coordBounds;
  CoordsHandle = coordsHandle;
  Triangles = triangles;
  IsConstructed = false;
  CanConstruct = true;
}

template <typename Device>
void LinearBVH::ConstructOnDevice(Device device)
{
  Logger* logger = Logger::GetInstance();
  vtkm::cont::Timer<Device> timer;
  logger->OpenLogEntry("bvh");
  if (!CanConstruct)
    throw vtkm::cont::ErrorBadValue(
      "Linear BVH: coordinates and triangles must be set before calling construct!");
  if (!IsConstructed)
  {
    //
    // This algorithm needs at least 2 triangles
    //
    vtkm::Id numTriangles = this->GetNumberOfTriangles();
    if (numTriangles == 1)
    {
      vtkm::Vec<vtkm::Id, 4> triangle = Triangles.GetPortalControl().Get(0);
      Triangles.Allocate(2);
      Triangles.GetPortalControl().Set(0, triangle);
      Triangles.GetPortalControl().Set(1, triangle);
    }
    detail::LinearBVHBuilder builder;
    builder.RunOnDevice(*this, device);
    IsConstructed = true;
  }

  vtkm::Float64 time = timer.GetElapsedTime();
  logger->CloseLogEntry(time);
}

// explicitly export
template VTKM_RENDERING_EXPORT void LinearBVH::ConstructOnDevice<
  vtkm::cont::DeviceAdapterTagSerial>(vtkm::cont::DeviceAdapterTagSerial);
#ifdef VTKM_ENABLE_TBB
template VTKM_RENDERING_EXPORT void LinearBVH::ConstructOnDevice<vtkm::cont::DeviceAdapterTagTBB>(
  vtkm::cont::DeviceAdapterTagTBB);
#endif
#ifdef VTKM_ENABLE_OPENMP
template VTKM_CONT_EXPORT void LinearBVH::ConstructOnDevice<vtkm::cont::DeviceAdapterTagOpenMP>(
  vtkm::cont::DeviceAdapterTagOpenMP);
#endif
#ifdef VTKM_ENABLE_CUDA
template VTKM_RENDERING_EXPORT void LinearBVH::ConstructOnDevice<vtkm::cont::DeviceAdapterTagCuda>(
  vtkm::cont::DeviceAdapterTagCuda);
#endif

VTKM_CONT
bool LinearBVH::GetIsConstructed() const
{
  return IsConstructed;
}
VTKM_CONT
vtkm::cont::ArrayHandleVirtualCoordinates LinearBVH::GetCoordsHandle() const
{
  return CoordsHandle;
}

VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> LinearBVH::GetTriangles() const
{
  return Triangles;
}

vtkm::Id LinearBVH::GetNumberOfTriangles() const
{
  return Triangles.GetPortalConstControl().GetNumberOfValues();
}
}
}
} // namespace vtkm::rendering::raytracing
