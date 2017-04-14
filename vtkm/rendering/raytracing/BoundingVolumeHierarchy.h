//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_BoundingVolumeHierachy_h
#define vtk_m_worklet_BoundingVolumeHierachy_h
#include <math.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/Math.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/exec/AtomicArray.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/raytracing/Worklets.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>

#include <limits>
#include <cstring>

namespace vtkm {
namespace rendering {
namespace raytracing {
//
// This is the data structure that is passed to the ray tracer.
//
//template<typename DeviceAdapter>
class LinearBVH
{

public:
  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32, 4> > FlatBVH;
  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Int32, 4> > LeafNodes;
  vtkm::Vec<Float32, 3> ExtentMin;
  vtkm::Vec<Float32, 3> ExtentMax;
  vtkm::Id LeafCount;
  VTKM_CONT
  LinearBVH()
  {}
  template<typename DeviceAdapter>
  VTKM_CONT
  void Allocate(const vtkm::Id &leafCount,
                DeviceAdapter deviceAdapter)
  {
    LeafCount = leafCount;
    LeafNodes.PrepareForOutput(leafCount, deviceAdapter);
    FlatBVH.PrepareForOutput((leafCount-1)*4, deviceAdapter);
  }


}; // class LinearBVH

template<typename DeviceAdapter>
class LinearBVHBuilder
{
private:
  typedef typename vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float64,3> > Vec3DoubleArrayHandle;
  typedef typename vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> > Vec3FloatArrayHandle;
  typedef typename Vec3DoubleArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst Point64PortalConst;
  typedef typename Vec3FloatArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst Point32PortalConst;
public:
  class CountingIterator : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT
    CountingIterator(){}
    typedef void ControlSignature(FieldOut<>);
    typedef void ExecutionSignature(WorkIndex, _1);
    VTKM_EXEC
    void operator()(const vtkm::Id &index, vtkm::Id &outId) const
    {
      outId = index;
    }
  }; //class countingIterator

  class FindAABBs : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT
    FindAABBs() {}
    typedef void ControlSignature(FieldIn<>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  WholeArrayIn<Vec3RenderingTypes>);
    typedef void ExecutionSignature(_1,
                                    _2,
                                    _3,
                                    _4,
                                    _5,
                                    _6,
                                    _7,
                                    _8);
    template<typename PointPortalType>
    VTKM_EXEC
    void operator()(const vtkm::Vec<vtkm::Id,4> indices,
                    vtkm::Float32 &xmin,
                    vtkm::Float32 &ymin,
                    vtkm::Float32 &zmin,
                    vtkm::Float32 &xmax,
                    vtkm::Float32 &ymax,
                    vtkm::Float32 &zmax,
                    const PointPortalType &points) const
    {
     // cast to Float32
      vtkm::Vec<vtkm::Float32,3> point;
      point = static_cast< vtkm::Vec<vtkm::Float32,3> >(points.Get(indices[1]));
      xmin = point[0];
      ymin = point[1];
      zmin = point[2];
      xmax = xmin;
      ymax = ymin;
      zmax = zmin;
      point = static_cast< vtkm::Vec<vtkm::Float32,3> >(points.Get(indices[2]));
      xmin = vtkm::Min(xmin,point[0]);
      ymin = vtkm::Min(ymin,point[1]);
      zmin = vtkm::Min(zmin,point[2]);
      xmax = vtkm::Max(xmax,point[0]);
      ymax = vtkm::Max(ymax,point[1]);
      zmax = vtkm::Max(zmax,point[2]);
      point = static_cast< vtkm::Vec<vtkm::Float32,3> >(points.Get(indices[3]));
      xmin = vtkm::Min(xmin,point[0]);
      ymin = vtkm::Min(ymin,point[1]);
      zmin = vtkm::Min(zmin,point[2]);
      xmax = vtkm::Max(xmax,point[0]);
      ymax = vtkm::Max(ymax,point[1]);
      zmax = vtkm::Max(zmax,point[2]);
    }
  }; //class FindAABBs


  class GatherFloat32 : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Float32>                       FloatArrayHandle;
    typedef typename FloatArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst PortalConst;
    typedef typename FloatArrayHandle::ExecutionTypes<DeviceAdapter>::Portal      Portal;
  private:
    PortalConst InputPortal;
    Portal      OutputPortal;
  public:
    VTKM_CONT
    GatherFloat32(const FloatArrayHandle &inputPortal,
                  FloatArrayHandle &outputPortal,
                  const vtkm::Id &size)
      : InputPortal(inputPortal.PrepareForInput( DeviceAdapter() ))
    {
      this->OutputPortal = outputPortal.PrepareForOutput(size, DeviceAdapter() );
    }
    typedef void ControlSignature(FieldIn<>);
    typedef void ExecutionSignature(WorkIndex, _1);
    VTKM_EXEC
    void operator()(const vtkm::Id &outIndex, const vtkm::Id &inIndex) const
    {
      OutputPortal.Set(outIndex, InputPortal.Get(inIndex));
    }
  }; //class GatherFloat

  class GatherVecCast : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id,4> >  Vec4IdArrayHandle;
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32,4> >  Vec4IntArrayHandle;
    typedef typename Vec4IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst PortalConst;
    typedef typename Vec4IntArrayHandle::ExecutionTypes<DeviceAdapter>::Portal      Portal;
  private:
    PortalConst InputPortal;
    Portal      OutputPortal;
  public:
    VTKM_CONT
    GatherVecCast(const Vec4IdArrayHandle &inputPortal,
                 Vec4IntArrayHandle &outputPortal,
                 const vtkm::Id &size)
      : InputPortal(inputPortal.PrepareForInput( DeviceAdapter() ))
    {
      this->OutputPortal = outputPortal.PrepareForOutput(size, DeviceAdapter() );
    }
    typedef void ControlSignature(FieldIn<>);
    typedef void ExecutionSignature(WorkIndex, _1);
    VTKM_EXEC
    void operator()(const vtkm::Id &outIndex, const vtkm::Id &inIndex) const
    {
      OutputPortal.Set(outIndex, InputPortal.Get(inIndex));
    }
  }; //class GatherVec3Id

  class BVHData
  {
  public:

    //TODO: make private
    vtkm::cont::ArrayHandle<vtkm::Float32> *xmins;
    vtkm::cont::ArrayHandle<vtkm::Float32> *ymins;
    vtkm::cont::ArrayHandle<vtkm::Float32> *zmins;
    vtkm::cont::ArrayHandle<vtkm::Float32> *xmaxs;
    vtkm::cont::ArrayHandle<vtkm::Float32> *ymaxs;
    vtkm::cont::ArrayHandle<vtkm::Float32> *zmaxs;

    vtkm::cont::ArrayHandle<vtkm::UInt32>  mortonCodes;
    vtkm::cont::ArrayHandle<vtkm::Id>      parent;
    vtkm::cont::ArrayHandle<vtkm::Id>      leftChild;
    vtkm::cont::ArrayHandle<vtkm::Id>      rightChild;

    VTKM_CONT
    BVHData(vtkm::Id numPrimitives)
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

      parent.PrepareForOutput(size, DeviceAdapter());
      leftChild.PrepareForOutput(InnerNodeCount, DeviceAdapter());
      rightChild.PrepareForOutput(InnerNodeCount, DeviceAdapter());
      mortonCodes.PrepareForOutput(NumPrimitives, DeviceAdapter());

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
    vtkm::Id GetNumberOfPrimitives() const
    {
      return NumPrimitives;
    }
    VTKM_CONT
    vtkm::Id GetNumberOfInnerNodes() const
    {
      return InnerNodeCount;
    }
    private:
    vtkm::Id NumPrimitives;
    vtkm::Id InnerNodeCount;

  }; // class BVH

  class PropagateAABBs : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
    typedef typename vtkm::cont::ArrayHandle<vtkm::Int8> Int8Handle;
    typedef typename vtkm::cont::ArrayHandle<Vec<vtkm::Float32,2> > Float2ArrayHandle;
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32,2> > VecInt2Handle;
    typedef typename vtkm::cont::ArrayHandle<Vec<vtkm::Float32,4> > Float4ArrayHandle;

    typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IdConstPortal;
    typedef typename Float2ArrayHandle::ExecutionTypes<DeviceAdapter>::Portal Float2ArrayPortal;
    typedef typename VecInt2Handle::ExecutionTypes<DeviceAdapter>::Portal Int2ArrayPortal;
    typedef typename Int8Handle::ExecutionTypes<DeviceAdapter>::Portal Int8ArrayPortal;
    typedef typename Float4ArrayHandle::ExecutionTypes<DeviceAdapter>::Portal Float4ArrayPortal;

    Float4ArrayPortal FlatBVH;
    IdConstPortal Parents;
    IdConstPortal LeftChildren;
    IdConstPortal RightChildren;
    vtkm::Int32 LeafCount;
    //Int8Handle Counters;
    //Int8ArrayPortal CountersPortal;
    vtkm::exec::AtomicArray<vtkm::Int32,DeviceAdapter> Counters;
  public:
    VTKM_CONT
    PropagateAABBs(IdArrayHandle &parents,
                   IdArrayHandle &leftChildren,
                   IdArrayHandle &rightChildren,
                   vtkm::Int32 leafCount,
                   Float4ArrayHandle flatBVH,
                   const vtkm::exec::AtomicArray<vtkm::Int32,DeviceAdapter> &counters)
      : Parents(parents.PrepareForInput( DeviceAdapter() )),
        LeftChildren(leftChildren.PrepareForInput( DeviceAdapter() )),
        RightChildren(rightChildren.PrepareForInput( DeviceAdapter() )),
        LeafCount(leafCount),
        Counters(counters)

    {
      this->FlatBVH = flatBVH.PrepareForOutput((LeafCount - 1) * 4, DeviceAdapter() );
    }
    typedef void ControlSignature(ExecObject,
                                  ExecObject,
                                  ExecObject,
                                  ExecObject,
                                  ExecObject,
                                  ExecObject);
    typedef void ExecutionSignature(WorkIndex,
                                    _1,
                                    _2,
                                    _3,
                                    _4,
                                    _5,
                                    _6);
    template<typename StrorageType>
    VTKM_EXEC_CONT
    void operator()(const vtkm::Id workIndex,
                    const vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32, StrorageType> &xmin,
                    const vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32, StrorageType> &ymin,
                    const vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32, StrorageType> &zmin,
                    const vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32, StrorageType> &xmax,
                    const vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32, StrorageType> &ymax,
                    const vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32, StrorageType> &zmax) const
    {
      //move up into the inner nodes
      vtkm::Id currentNode = LeafCount - 1 + workIndex;
      vtkm::Vec<vtkm::Id,2> childVector;
      while(currentNode != 0)
      {
        currentNode = Parents.Get(currentNode);

        vtkm::Int32 oldCount = Counters.Add(currentNode,1);
        if(oldCount == 0) return;
        vtkm::Id currentNodeOffset = currentNode * 4;
        childVector[0] = LeftChildren.Get(currentNode);
        childVector[1] = RightChildren.Get(currentNode);
        if(childVector[0] > (LeafCount - 2))
        {
          childVector[0] = childVector[0] - LeafCount + 1;

          vtkm::Vec<vtkm::Float32,4> first4Vec;// = FlatBVH.Get(currentNode); only this one needs effects this

          first4Vec[0] = xmin.Get(childVector[0]);
          first4Vec[1] = ymin.Get(childVector[0]);
          first4Vec[2] = zmin.Get(childVector[0]);
          first4Vec[3] = xmax.Get(childVector[0]);
          FlatBVH.Set(currentNodeOffset, first4Vec);

          vtkm::Vec<vtkm::Float32,4> second4Vec = FlatBVH.Get(currentNodeOffset+1);
          second4Vec[0] = ymax.Get(childVector[0]);
          second4Vec[1] = zmax.Get(childVector[0]);
          FlatBVH.Set(currentNodeOffset+1, second4Vec);

          childVector[0] = -(childVector[0] + 1);
        }
        else
        {
          vtkm::Id child = childVector[0] * 4;

          vtkm::Vec<vtkm::Float32,4> cFirst4Vec = FlatBVH.Get(child);
          vtkm::Vec<vtkm::Float32,4> cSecond4Vec = FlatBVH.Get(child+1);
          vtkm::Vec<vtkm::Float32,4> cThird4Vec = FlatBVH.Get(child+2);

          cFirst4Vec[0] = vtkm::Min(cFirst4Vec[0], cSecond4Vec[2]);
          cFirst4Vec[1] = vtkm::Min(cFirst4Vec[1], cSecond4Vec[3]);
          cFirst4Vec[2] = vtkm::Min(cFirst4Vec[2], cThird4Vec[0]);
          cFirst4Vec[3] = vtkm::Max(cFirst4Vec[3], cThird4Vec[1]);
          FlatBVH.Set(currentNodeOffset,cFirst4Vec);

          vtkm::Vec<vtkm::Float32,4> second4Vec = FlatBVH.Get(currentNodeOffset+1);
          second4Vec[0] = vtkm::Max(cSecond4Vec[0], cThird4Vec[2]);
          second4Vec[1] = vtkm::Max(cSecond4Vec[1], cThird4Vec[3]);

          FlatBVH.Set(currentNodeOffset+1, second4Vec);
        }

        if(childVector[1] > (LeafCount - 2))
        {
          childVector[1] = childVector[1] - LeafCount + 1;


          vtkm::Vec<vtkm::Float32,4> second4Vec = FlatBVH.Get(currentNodeOffset+1);

          second4Vec[2] = xmin.Get(childVector[1]);
          second4Vec[3] = ymin.Get(childVector[1]);
          FlatBVH.Set(currentNodeOffset+1, second4Vec);

          vtkm::Vec<vtkm::Float32,4> third4Vec;
          third4Vec[0] = zmin.Get(childVector[1]);
          third4Vec[1] = xmax.Get(childVector[1]);
          third4Vec[2] = ymax.Get(childVector[1]);
          third4Vec[3] = zmax.Get(childVector[1]);
          FlatBVH.Set(currentNodeOffset+2, third4Vec);
          childVector[1] = -(childVector[1] + 1);
        }
        else
        {

          vtkm::Id child = childVector[1] * 4;

          vtkm::Vec<vtkm::Float32,4> cFirst4Vec = FlatBVH.Get(child);
          vtkm::Vec<vtkm::Float32,4> cSecond4Vec = FlatBVH.Get(child+1);
          vtkm::Vec<vtkm::Float32,4> cThird4Vec = FlatBVH.Get(child+2);

          vtkm::Vec<vtkm::Float32,4> second4Vec = FlatBVH.Get(currentNodeOffset+1);
          second4Vec[2] = vtkm::Min(cFirst4Vec[0], cSecond4Vec[2]);
          second4Vec[3] = vtkm::Min(cFirst4Vec[1], cSecond4Vec[3]);
          FlatBVH.Set(currentNodeOffset+1, second4Vec);

          cThird4Vec[0] = vtkm::Min(cFirst4Vec[2], cThird4Vec[0]);
          cThird4Vec[1] = vtkm::Max(cFirst4Vec[3], cThird4Vec[1]);
          cThird4Vec[2] = vtkm::Max(cSecond4Vec[0], cThird4Vec[2]);
          cThird4Vec[3] = vtkm::Max(cSecond4Vec[1], cThird4Vec[3]);
          FlatBVH.Set(currentNodeOffset+2,cThird4Vec);

        }
        vtkm::Vec<vtkm::Float32,4> fourth4Vec;
        vtkm::Int32 leftChild = static_cast<vtkm::Int32>((childVector[0] >= 0) ? childVector[0] * 4 : childVector[0]);
        memcpy(&fourth4Vec[0],&leftChild,4);
        vtkm::Int32 rightChild = static_cast<vtkm::Int32>((childVector[1] >= 0) ? childVector[1] * 4 : childVector[1]);
        memcpy(&fourth4Vec[1],&rightChild,4);
        FlatBVH.Set(currentNodeOffset+3,fourth4Vec);
      }
    }
  }; //class PropagateAABBs

  class MortonCodeAABB : public vtkm::worklet::WorkletMapField
  {
  private:
     // (1.f / dx),(1.f / dy), (1.f, / dz)
    vtkm::Vec<vtkm::Float32,3> InverseExtent;
    vtkm::Vec<vtkm::Float32,3> MinCoordinate;

    //expands 10-bit unsigned int into 30 bits
    VTKM_EXEC
    vtkm::UInt32 ExpandBits(vtkm::UInt32 x) const
    {
      x = (x * 0x00010001u) & 0xFF0000FFu;
      x = (x * 0x00000101u) & 0x0F00F00Fu;
      x = (x * 0x00000011u) & 0xC30C30C3u;
      x = (x * 0x00000005u) & 0x49249249u;
      return x;
    }
    //Returns 30 bit morton code for coordinates for
    //coordinates in the unit cude
    VTKM_EXEC
    vtkm::UInt32 Morton3D(vtkm::Float32 &x,
                          vtkm::Float32 &y,
                          vtkm::Float32 &z) const
    {
      //take the first 10 bits
      x = fminf(fmaxf(x * 1024.0f, 0.0f), 1023.0f);
      y = fminf(fmaxf(y * 1024.0f, 0.0f), 1023.0f);
      z = fminf(fmaxf(z * 1024.0f, 0.0f), 1023.0f);
      //expand the 10 bits to 30
      vtkm::UInt32 xx = ExpandBits((vtkm::UInt32)x);
      vtkm::UInt32 yy = ExpandBits((vtkm::UInt32)y);
      vtkm::UInt32 zz = ExpandBits((vtkm::UInt32)z);
      //interleave coordinates
      return xx * 4 + yy * 2 + zz;
    }

  public:
    VTKM_CONT
    MortonCodeAABB(const vtkm::Vec<vtkm::Float32,3> &inverseExtent,
                   const vtkm::Vec<vtkm::Float32,3> &minCoordinate)
      : InverseExtent(inverseExtent),
        MinCoordinate( minCoordinate) {}

    typedef void ControlSignature(FieldIn<>,
                                  FieldIn<>,
                                  FieldIn<>,
                                  FieldIn<>,
                                  FieldIn<>,
                                  FieldIn<>,
                                  FieldOut<>);
    typedef void ExecutionSignature(_1,
                                    _2,
                                    _3,
                                    _4,
                                    _5,
                                    _6,
                                    _7);
    typedef _7 InputDomain;

    VTKM_EXEC
    void operator()(const vtkm::Float32 &xmin,
                    const vtkm::Float32 &ymin,
                    const vtkm::Float32 &zmin,
                    const vtkm::Float32 &xmax,
                    const vtkm::Float32 &ymax,
                    const vtkm::Float32 &zmax,
                    vtkm::UInt32 &mortonCode) const
    {
      vtkm::Vec<vtkm::Float32,3> direction(xmax - xmin,
                                           ymax - ymin,
                                           zmax - zmin);
      vtkm::Float32 halfDistance = sqrtf(vtkm::dot(direction,direction)) * 0.5f;
      vtkm::Normalize(direction);
      vtkm::Float32 centroidx = xmin + halfDistance * direction[0] - MinCoordinate[0];
      vtkm::Float32 centroidy = ymin + halfDistance * direction[1] - MinCoordinate[1];
      vtkm::Float32 centroidz = zmin + halfDistance * direction[2] - MinCoordinate[2];
      //normalize the centroid tp 10 bits
      centroidx *= InverseExtent[0];
      centroidy *= InverseExtent[1];
      centroidz *= InverseExtent[2];
      mortonCode = Morton3D(centroidx, centroidy, centroidz);
    }
  }; // class MortonCodeAABB

  class TreeBuilder : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef typename vtkm::cont::ArrayHandle<vtkm::UInt32>  UIntArrayHandle;
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id>      IdArrayHandle;
    typedef typename UIntArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst UIntPortalType;
    typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::Portal       IdPortalType;
  private:
    UIntPortalType MortonCodePortal;
    IdPortalType   ParentPortal;
    vtkm::Id LeafCount;
    vtkm::Id InnerCount;
    //TODO: get instrinsic support
    VTKM_EXEC
    vtkm::Int32 CountLeadingZeros(vtkm::UInt32 &x) const
    {
      vtkm::UInt32 y;
      vtkm::UInt32 n = 32;
      y = x >>16; if (y != 0) { n = n -16; x = y; }
      y = x >> 8; if (y != 0) { n = n - 8; x = y; }
      y = x >> 4; if (y != 0) { n = n - 4; x = y; }
      y = x >> 2; if (y != 0) { n = n - 2; x = y; }
      y = x >> 1; if (y != 0) return vtkm::Int32(n - 2);
      return vtkm::Int32(n - x);
    }

    // returns the count of largest shared prefix between
    // two morton codes. Ties are broken by the indexes
    // a and b.
    //
    // returns count of the largest binary prefix

    VTKM_EXEC
    vtkm::Int32 delta(const vtkm::Int32 &a,
                      const vtkm::Int32 &b) const
    {
      bool tie = false;
      bool outOfRange = (b < 0 || b > LeafCount -1);
      //still make the call but with a valid adderss
      vtkm::Int32 bb = (outOfRange) ? 0 : b;
      vtkm::UInt32 aCode =  MortonCodePortal.Get(a);
      vtkm::UInt32 bCode =  MortonCodePortal.Get(bb);
      //use xor to find where they differ
      vtkm::UInt32 exOr = aCode ^ bCode;
      tie = (exOr == 0);
      //break the tie, a and b must always differ
      exOr = tie ? vtkm::UInt32(a) ^  vtkm::UInt32(bb) : exOr;
      vtkm::Int32 count = CountLeadingZeros(exOr);
      if(tie) count += 32;
      count = (outOfRange) ? -1 : count;
      return count;
    }
  public:
    VTKM_CONT
    TreeBuilder(const UIntArrayHandle &mortonCodesHandle,
                IdArrayHandle &parentHandle,
                const vtkm::Id &leafCount)
      : MortonCodePortal(mortonCodesHandle.PrepareForInput(DeviceAdapter())),
        LeafCount(leafCount)
    {
      InnerCount = LeafCount - 1;
      this->ParentPortal = parentHandle.PrepareForOutput(InnerCount + LeafCount, DeviceAdapter() );
    }
    typedef void ControlSignature(FieldOut<>,
                                  FieldOut<>);
    typedef void ExecutionSignature(WorkIndex,
                                    _1,
                                    _2);
    VTKM_EXEC
    void operator()(const vtkm::Id &index,
                    vtkm::Id &leftChild,
                    vtkm::Id &rightChild) const
    {
      vtkm::Int32 idx = vtkm::Int32(index);
      //something = MortonCodePortal.Get(index) + 1;
      //determine range direction
      vtkm::Int32 d = 0 > (delta(idx, idx + 1) - delta(idx, idx - 1)) ?  -1 : 1;

      //find upper bound for the length of the range
      vtkm::Int32 minDelta = delta(idx, idx - d);
      vtkm::Int32 lMax = 2;
      while( delta(idx, idx + lMax * d) > minDelta ) lMax *= 2;

      //binary search to find the lower bound
      vtkm::Int32 l = 0;
      for(int t = lMax / 2; t >= 1; t/=2)
      {
        if(delta(idx, idx + (l + t)*d ) > minDelta) l += t;
      }

      vtkm::Int32 j = idx + l * d;
      vtkm::Int32 deltaNode = delta(idx,j);
      vtkm::Int32 s = 0;
      vtkm::Float32 divFactor = 2.f;
      //find the split postition using a binary search
      for(vtkm::Int32 t = (vtkm::Int32) ceil(vtkm::Float32(l) / divFactor);; divFactor*=2, t = (vtkm::Int32) ceil(vtkm::Float32(l) / divFactor) )
      {
        if(delta(idx, idx + (s + t) * d) > deltaNode)
        {
          s += t;
        }

        if(t == 1) break;
      }

      vtkm::Int32 split = idx + s * d + vtkm::Min(d,0);
      //assign parent/child pointers
      if(vtkm::Min(idx, j) == split)
      {
        //leaf
        ParentPortal.Set(split + InnerCount,idx);
        leftChild = split + InnerCount;
      }
      else
      {
        //inner node
        ParentPortal.Set(split, idx);
        leftChild = split;
      }


      if(vtkm::Max(idx, j) == split + 1)
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


public:
  VTKM_CONT
  LinearBVHBuilder() {}

  VTKM_CONT
  void SortAABBS(BVHData &bvh,
                 vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id,4> > &triangleIndices,
                 vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Int32,4> > &outputTriangleIndices)
  {
     //create array of indexes to be sorted with morton codes
    vtkm::cont::ArrayHandle<vtkm::Id>     iterator;
    iterator.PrepareForOutput( bvh.GetNumberOfPrimitives(), DeviceAdapter() );
    vtkm::worklet::DispatcherMapField<CountingIterator> iteratorDispatcher;
    iteratorDispatcher.Invoke(iterator);

    /*
    for(int i = 0; i < bvh.GetNumberOfPrimitives(); i++)
    {
      //std::cout<<iterator.GetPortalControl().Get(i)
               <<" "<<bvh.mortonCodes.GetPortalControl().Get(i)
               <<" "<<bvh.xmins->GetPortalControl().Get(i)
               <<" "<<bvh.ymins->GetPortalControl().Get(i)
               <<" "<<bvh.zmins->GetPortalControl().Get(i)
               <<" "<<bvh.xmaxs->GetPortalControl().Get(i)
               <<" "<<bvh.ymaxs->GetPortalControl().Get(i)
               <<" "<<bvh.zmaxs->GetPortalControl().Get(i)
               <<" "<<triangleIndices->GetPortalControl().Get(i)
               <<" \n";
    }
    */
    //std::cout<<"\n\n\n";
    //sort the morton codes

    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::SortByKey(bvh.mortonCodes,
                                                                  iterator);

    vtkm::Id arraySize = bvh.GetNumberOfPrimitives();
    vtkm::cont::ArrayHandle<vtkm::Float32> *tempStorage;
    vtkm::cont::ArrayHandle<vtkm::Float32> *tempPtr;


    //outputTriangleIndices.Allocate( bvh.GetNumberOfPrimitives() );

    tempStorage = new vtkm::cont::ArrayHandle<vtkm::Float32>();
    //tempStorage->Allocate( arraySize );
    // for (int i = 0; i < 12; ++i)
    // {
    //   //std::cout<<bvh.xmins->GetPortalControl().Get(i)<<" ";
    // }//std::cout<<"\n";
    //xmins
    vtkm::worklet::DispatcherMapField<GatherFloat32>( GatherFloat32(*bvh.xmins,
                                                                    *tempStorage,
                                                                    arraySize) )
      .Invoke(iterator);
    tempPtr     = bvh.xmins;
    bvh.xmins   = tempStorage;
    tempStorage = tempPtr;
    vtkm::worklet::DispatcherMapField<GatherFloat32>( GatherFloat32(*bvh.ymins,
                                                                    *tempStorage,
                                                                    arraySize) )
      .Invoke(iterator);
    tempPtr     = bvh.ymins;
    bvh.ymins   = tempStorage;
    tempStorage = tempPtr;
    //zmins
    vtkm::worklet::DispatcherMapField<GatherFloat32>( GatherFloat32(*bvh.zmins,
                                                                    *tempStorage,
                                                                    arraySize) )
      .Invoke(iterator);
    tempPtr     = bvh.zmins;
    bvh.zmins   = tempStorage;
    tempStorage = tempPtr;
    //xmaxs
    vtkm::worklet::DispatcherMapField<GatherFloat32>( GatherFloat32(*bvh.xmaxs,
                                                                    *tempStorage,
                                                                    arraySize) )
      .Invoke(iterator);
    tempPtr     = bvh.xmaxs;
    bvh.xmaxs   = tempStorage;
    tempStorage = tempPtr;
    //ymaxs
    vtkm::worklet::DispatcherMapField<GatherFloat32>( GatherFloat32(*bvh.ymaxs,
                                                                    *tempStorage,
                                                                    arraySize) )
      .Invoke(iterator);
    tempPtr     = bvh.ymaxs;
    bvh.ymaxs   = tempStorage;
    tempStorage = tempPtr;
    //zmaxs
    vtkm::worklet::DispatcherMapField<GatherFloat32>( GatherFloat32(*bvh.zmaxs,
                                                                    *tempStorage,
                                                                    arraySize) )
      .Invoke(iterator);

    tempPtr     = bvh.zmaxs;
    bvh.zmaxs   = tempStorage;
    tempStorage = tempPtr;
    vtkm::worklet::DispatcherMapField<GatherVecCast>( GatherVecCast(triangleIndices,
                                                                    outputTriangleIndices,
                                                                    arraySize) )
      .Invoke(iterator);
    delete tempStorage;

  } // method SortAABBs


  VTKM_CONT
  void run(vtkm::cont::DynamicArrayHandleCoordinateSystem &coordsHandle,
           vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> >  &triangleIndices,
           const vtkm::Id &numberOfTriangles,
           LinearBVH &linearBVH)
  {
    const vtkm::Id numBBoxes = numberOfTriangles;
    BVHData bvh(numBBoxes);

    vtkm::worklet::DispatcherMapField<FindAABBs>( FindAABBs() )
      .Invoke(triangleIndices,
            *bvh.xmins,
            *bvh.ymins,
            *bvh.zmins,
            *bvh.xmaxs,
            *bvh.ymaxs,
            *bvh.zmaxs,
            coordsHandle);
    // Find the extent of all bounding boxes to generate normalization for morton codes
    vtkm::Vec<vtkm::Float32,3> minExtent(vtkm::Infinity32(),
                                         vtkm::Infinity32(),
                                         vtkm::Infinity32());
    vtkm::Vec<vtkm::Float32,3> maxExtent(vtkm::NegativeInfinity32(),
                                         vtkm::NegativeInfinity32(),
                                         vtkm::NegativeInfinity32());
    maxExtent[0] = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Reduce(*bvh.xmaxs,
                                                                             maxExtent[0],
                                                                             MaxValue());
    maxExtent[1] = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Reduce(*bvh.ymaxs,
                                                                             maxExtent[1],
                                                                             MaxValue());
    maxExtent[2] = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Reduce(*bvh.zmaxs,
                                                                             maxExtent[2],
                                                                             MaxValue());
    minExtent[0] = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Reduce(*bvh.xmins,
                                                                             minExtent[0],
                                                                             MinValue());
    minExtent[1] = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Reduce(*bvh.ymins,
                                                                             minExtent[1],
                                                                             MinValue());
    minExtent[2] = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Reduce(*bvh.zmins,
                                                                             minExtent[2],
                                                                             MinValue());

    vtkm::Vec<vtkm::Float32,3> deltaExtent = maxExtent - minExtent;
    vtkm::Vec<vtkm::Float32,3> inverseExtent;
    for (int i = 0; i < 3; ++i)
    {
      inverseExtent[i] = (deltaExtent[i] == 0.f) ? 0 : 1.f / deltaExtent[i];
    }

    //Generate the morton codes
    vtkm::worklet::DispatcherMapField<MortonCodeAABB>( MortonCodeAABB(inverseExtent,minExtent) )
      .Invoke(*bvh.xmins,
              *bvh.ymins,
              *bvh.zmins,
              *bvh.xmaxs,
              *bvh.ymaxs,
              *bvh.zmaxs,
              bvh.mortonCodes);
    linearBVH.Allocate( bvh.GetNumberOfPrimitives(), DeviceAdapter() );
    SortAABBS(bvh, triangleIndices, linearBVH.LeafNodes);


    vtkm::worklet::DispatcherMapField<TreeBuilder>( TreeBuilder(bvh.mortonCodes,
                                                                bvh.parent,
                                                                bvh.GetNumberOfPrimitives()) )
      .Invoke(bvh.leftChild,
              bvh.rightChild);

    const vtkm::Int32 primitiveCount = vtkm::Int32(bvh.GetNumberOfPrimitives());

    vtkm::cont::ArrayHandle<vtkm::Int32> counters;
    counters.PrepareForOutput(bvh.GetNumberOfPrimitives() - 1, DeviceAdapter());
    vtkm::Int32 zero= 0;
    vtkm::worklet::DispatcherMapField< MemSet<vtkm::Int32> >( MemSet<vtkm::Int32>(zero) )
      .Invoke(counters);
    vtkm::exec::AtomicArray<vtkm::Int32, DeviceAdapter> atomicCounters(counters);

    vtkm::worklet::DispatcherMapField<PropagateAABBs>( PropagateAABBs(bvh.parent,
                                                                      bvh.leftChild,
                                                                      bvh.rightChild,
                                                                      primitiveCount,
                                                                      linearBVH.FlatBVH,
                                                                      atomicCounters ))
      .Invoke(vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32>(*bvh.xmins),
              vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32>(*bvh.ymins),
              vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32>(*bvh.zmins),
              vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32>(*bvh.xmaxs),
              vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32>(*bvh.ymaxs),
              vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32>(*bvh.zmaxs));

  }
};// class LinearBVHBuilder
}}}// namespace vtkm::rendering::raytracing
#endif //vtk_m_worklet_BoundingVolumeHierachy_h
