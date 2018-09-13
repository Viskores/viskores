//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_zfp_functions_h
#define vtk_m_worklet_zfp_functions_h

#include <vtkm/Math.h>
#include <vtkm/worklet/zfp/ZFPBlockWriter.h>
#include <vtkm/worklet/zfp/ZFPCodec.h>
#include <vtkm/worklet/zfp/ZFPTypeInfo.h>

namespace vtkm
{
namespace worklet
{
namespace zfp
{

template <typename T>
vtkm::UInt32 MinBits(const vtkm::UInt32 bits)
{
  return bits;
}

template <>
vtkm::UInt32 MinBits<vtkm::Float32>(const vtkm::UInt32 bits)
{
  return vtkm::Max(bits, 1 + 8u);
}

template <>
vtkm::UInt32 MinBits<vtkm::Float64>(const vtkm::UInt32 bits)
{
  return vtkm::Max(bits, 1 + 11u);
}


template <vtkm::Int32 N, typename FloatType>
inline VTKM_EXEC vtkm::Int32 MaxExponent(const FloatType* vals)
{
  FloatType maxVal = 0;
  for (vtkm::Int32 i = 0; i < N; ++i)
  {
    maxVal = vtkm::Max(maxVal, vtkm::Abs(vals[i]));
  }

  if (maxVal > 0)
  {
    vtkm::Int32 exponent;
    vtkm::Frexp(maxVal, &exponent);
    /* clamp exponent in case x is denormal */
    return vtkm::Max(exponent, 1 - get_ebias<FloatType>());
  }
  return -get_ebias<FloatType>();
}

// maximum number of bit planes to encode
inline VTKM_EXEC vtkm::Int32 precision(vtkm::Int32 maxexp, vtkm::Int32 maxprec, vtkm::Int32 minexp)
{
  return vtkm::Min(maxprec, vtkm::Max(0, maxexp - minexp + 8));
}

template <typename Scalar>
inline VTKM_EXEC Scalar quantize(Scalar x, vtkm::Int32 e)
{
  return vtkm::Ldexp(x, (CHAR_BIT * (vtkm::Int32)sizeof(Scalar) - 2) - e);
}

template <typename Int, typename Scalar, vtkm::Int32 BlockSize>
inline VTKM_EXEC void fwd_cast(Int* iblock, const Scalar* fblock, vtkm::Int32 emax)
{
  Scalar s = quantize<Scalar>(1, emax);
  //std::cout<<"EMAX "<<emax<<" q "<<s<<"\n";
  for (vtkm::Int32 i = 0; i < BlockSize; ++i)
  {
    iblock[i] = static_cast<Int>(s * fblock[i]);
    //std::cout<<i<<" f = "<<fblock[i]<<" i = "<<(vtkm::UInt64)iblock[i]<<"\n";
  }
}

template <typename Int, vtkm::Int32 S>
inline VTKM_EXEC void fwd_lift(Int* p)
{
  Int x, y, z, w;
  x = *p;
  p += S;
  y = *p;
  p += S;
  z = *p;
  p += S;
  w = *p;
  p += S;

  /*
    ** non-orthogonal transform
    **        ( 4  4  4  4) (x)
    ** 1/16 * ( 5  1 -1 -5) (y)
    **        (-4  4  4 -4) (z)
    **        (-2  6 -6  2) (w)
    */
  x += w;
  x >>= 1;
  w -= x;
  z += y;
  z >>= 1;
  y -= z;
  x += z;
  x >>= 1;
  z -= x;
  w += y;
  w >>= 1;
  y -= w;
  w += y >> 1;
  y -= w >> 1;

  p -= S;
  *p = w;
  p -= S;
  *p = z;
  p -= S;
  *p = y;
  p -= S;
  *p = x;
}

template <typename Int, typename UInt>
inline VTKM_EXEC UInt int2uint(const Int x);

template <>
inline VTKM_EXEC vtkm::UInt64 int2uint<vtkm::Int64, vtkm::UInt64>(const vtkm::Int64 x)
{
  return (x + (vtkm::UInt64)0xaaaaaaaaaaaaaaaaull) ^ (vtkm::UInt64)0xaaaaaaaaaaaaaaaaull;
}

template <>
inline VTKM_EXEC vtkm::UInt32 int2uint<vtkm::Int32, vtkm::UInt32>(const vtkm::Int32 x)
{
  return (x + (vtkm::UInt32)0xaaaaaaaau) ^ (vtkm::UInt32)0xaaaaaaaau;
}



template <typename UInt, typename Int, vtkm::Int32 BlockSize>
inline VTKM_EXEC void fwd_order(UInt* ublock, const Int* iblock)
{
  const zfp::ZFPCodec codec;
  for (vtkm::Int32 i = 0; i < BlockSize; ++i)
  {
    ublock[i] = int2uint<Int, UInt>(iblock[codec.CodecLookup(i)]);
  }
}

template <typename Int, vtkm::Int32 BlockSize>
inline VTKM_EXEC void fwd_xform(Int* p);

template <>
inline VTKM_EXEC void fwd_xform<vtkm::Int64, 64>(vtkm::Int64* p)
{
  vtkm::UInt32 x, y, z;
  /* transform along x */
  for (z = 0; z < 4; z++)
    for (y = 0; y < 4; y++)
      fwd_lift<vtkm::Int64, 1>(p + 4 * y + 16 * z);
  /* transform along y */
  for (x = 0; x < 4; x++)
    for (z = 0; z < 4; z++)
      fwd_lift<vtkm::Int64, 4>(p + 16 * z + 1 * x);
  /* transform along z */
  for (y = 0; y < 4; y++)
    for (x = 0; x < 4; x++)
      fwd_lift<vtkm::Int64, 16>(p + 1 * x + 4 * y);
}

template <>
inline VTKM_EXEC void fwd_xform<vtkm::Int32, 64>(vtkm::Int32* p)
{
  vtkm::UInt32 x, y, z;
  /* transform along x */
  for (z = 0; z < 4; z++)
    for (y = 0; y < 4; y++)
      fwd_lift<vtkm::Int32, 1>(p + 4 * y + 16 * z);
  /* transform along y */
  for (x = 0; x < 4; x++)
    for (z = 0; z < 4; z++)
      fwd_lift<vtkm::Int32, 4>(p + 16 * z + 1 * x);
  /* transform along z */
  for (y = 0; y < 4; y++)
    for (x = 0; x < 4; x++)
      fwd_lift<vtkm::Int32, 16>(p + 1 * x + 4 * y);
}

template <typename T>
void print_bits(T bits)
{
  const int bit_size = sizeof(T) * 8;
  for (int i = bit_size - 1; i >= 0; --i)
  {
    T one = 1;
    T mask = one << i;
    int val = (bits & mask) >> i;
    printf("%d", val);
  }
  printf("\n");
}

template <typename PortalType, typename Scalar, typename Int, vtkm::Int32 BlockSize>
inline VTKM_EXEC void encode_block(BlockWriter<BlockSize, PortalType>& stream,
                                   Int* iblock,
                                   vtkm::Int32 maxprec,
                                   vtkm::Int32 maxbits)
{
  //std::cout<<"Incoming stream \n";
  //stream.print();
  using UInt = typename zfp::zfp_traits<Scalar>::UInt;

  fwd_xform<Int, BlockSize>(iblock);

  UInt ublock[BlockSize];
  fwd_order<UInt, Int, BlockSize>(ublock, iblock);
  //for(int i = 0; i < 64; ++i)
  //{
  //  std::cout<<"iblock "<<i<<" "<<(vtkm::UInt64)iblock[i]<<"\n";
  //}
  //for(int i = 0; i < 64; ++i)
  //{
  //  std::cout<<"ublock "<<i<<" "<<ublock[i]<<"\n";
  //}

  //bitstream s = *stream;
  vtkm::UInt32 intprec = CHAR_BIT * (vtkm::UInt32)sizeof(UInt);
  vtkm::UInt32 kmin = intprec > (vtkm::UInt32)maxprec ? intprec - maxprec : 0;
  vtkm::UInt32 bits = maxbits;
  vtkm::UInt32 i, m;
  vtkm::UInt32 n = 0;
  vtkm::UInt64 x;
  //std::cout<<"Kmin "<<kmin<<"\n";
  /* encode one bit plane at a time from MSB to LSB */
  for (vtkm::UInt32 k = intprec; bits && k-- > kmin;)
  {
    /* step 1: extract bit plane #k to x */
    x = 0;
    for (i = 0; i < BlockSize; i++)
    {
      x += (vtkm::UInt64)((ublock[i] >> k) & 1u) << i;
    }
    //std::cout<<"Bit plane "<<x<<"\n";
    /* step 2: encode first n bits of bit plane */
    m = vtkm::Min(n, bits);
    bits -= m;
    //std::cout<<"Bits left "<<bits<<" m "<<m<<"\n";
    x = stream.write_bits(x, m);
    //std::cout<<"Wrote m "<<m<<" bits\n";
    /* step 3: unary run-length encode remainder of bit plane */
    for (; n < BlockSize && bits && (bits--, stream.write_bit(!!x)); x >>= 1, n++)
    {
      //std::cout<<"outer n "<<n<<" bits "<<bits<<"\n";
      for (; n < BlockSize - 1 && bits && (bits--, !stream.write_bit(x & 1u)); x >>= 1, n++)
      {
        //std::cout<<"n "<<n<<" bits "<<bits<<"\n";
      }
    }
    //stream.print();
  }

  //*stream = s;
  //return maxbits - bits;
}



} // namespace zfp
} // namespace worklet
} // namespace vtkm
#endif //  vtk_m_worklet_zfp_type_info_h
