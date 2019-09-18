//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_zfp_structs_h
#define vtk_m_worklet_zfp_structs_h

#define ZFP_MIN_BITS 0    /* minimum number of bits per block */
#define ZFP_MAX_BITS 4171 /* maximum number of bits per block */
#define ZFP_MAX_PREC 64   /* maximum precision supported */
#define ZFP_MIN_EXP -1074 /* minimum floating-point base-2 exponent */

#include <vtkm/worklet/zfp/ZFPFunctions.h>
#include <vtkm/worklet/zfp/ZFPTypeInfo.h>

namespace vtkm
{
namespace worklet
{
namespace zfp
{

struct ZFPStream
{
  vtkm::UInt32 minbits;
  vtkm::UInt32 maxbits;
  vtkm::UInt32 maxprec;
  vtkm::Int32 minexp;

  template <typename T>
  vtkm::Float64 SetRate(const vtkm::Float64 rate, const vtkm::Int32 dims, T vtkmNotUsed(valueType))
  {
    vtkm::UInt32 n = 1u << (2 * dims);
    vtkm::UInt32 bits = (unsigned int)floor(n * rate + 0.5);
    bits = zfp::MinBits<T>(bits);
    //if (wra) {
    //  /* for write random access, round up to next multiple of stream word size */
    //  bits += (uint)stream_word_bits - 1;
    //  bits &= ~(stream_word_bits - 1);
    //}
    minbits = bits;
    maxbits = bits;
    maxprec = ZFP_MAX_PREC;
    minexp = ZFP_MIN_EXP;
    return (double)bits / n;
  }
};
}
}
} // namespace vtkm::worklet::zfp
#endif
