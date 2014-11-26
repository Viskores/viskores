//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

/******************************************************************************
 * Copyright (c) 2011, Duane Merrill.  All rights reserved.
 * Copyright (c) 2011-2014, NVIDIA CORPORATION.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the NVIDIA CORPORATION nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#ifndef vtk_m_exec_cuda_internal_ArrayPortalFromTexture_h
#define vtk_m_exec_cuda_internal_ArrayPortalFromTexture_h

#include <vtkm/Types.h>

#include <iterator>

// #include <iostream>

namespace
{

/**
 * \brief Type selection (<tt>IF ? ThenType : ElseType</tt>)
 */
template <bool IF, typename ThenType, typename ElseType>
struct If
{
    /// Conditional type result
    typedef ThenType Type;      // true
};

template <typename ThenType, typename ElseType>
struct If<false, ThenType, ElseType>
{
    typedef ElseType Type;      // false
};

/******************************************************************************
* Size and alignment
******************************************************************************/

/// Structure alignment
template <typename T>
struct AlignBytes
{
    struct Pad
    {
        T       val;
        char    byte;
    };

    enum
    {
        /// The alignment of T in bytes
        ALIGN_BYTES = sizeof(Pad) - sizeof(T)
    };
};

// Specializations where host C++ compilers (e.g., Windows) may disagree with device C++ compilers (EDG)

template <> struct AlignBytes<short4>               { enum { ALIGN_BYTES = 8 }; };
template <> struct AlignBytes<ushort4>              { enum { ALIGN_BYTES = 8 }; };
template <> struct AlignBytes<int2>                 { enum { ALIGN_BYTES = 8 }; };
template <> struct AlignBytes<uint2>                { enum { ALIGN_BYTES = 8 }; };
#ifdef _WIN32
    template <> struct AlignBytes<long2>            { enum { ALIGN_BYTES = 8 }; };
    template <> struct AlignBytes<ulong2>           { enum { ALIGN_BYTES = 8 }; };
#endif
template <> struct AlignBytes<long long>            { enum { ALIGN_BYTES = 8 }; };
template <> struct AlignBytes<unsigned long long>   { enum { ALIGN_BYTES = 8 }; };
template <> struct AlignBytes<float2>               { enum { ALIGN_BYTES = 8 }; };
template <> struct AlignBytes<double>               { enum { ALIGN_BYTES = 8 }; };

template <> struct AlignBytes<int4>                 { enum { ALIGN_BYTES = 16 }; };
template <> struct AlignBytes<uint4>                { enum { ALIGN_BYTES = 16 }; };
template <> struct AlignBytes<float4>               { enum { ALIGN_BYTES = 16 }; };
#ifndef _WIN32
    template <> struct AlignBytes<long2>            { enum { ALIGN_BYTES = 16 }; };
    template <> struct AlignBytes<ulong2>           { enum { ALIGN_BYTES = 16 }; };
#endif
template <> struct AlignBytes<long4>                { enum { ALIGN_BYTES = 16 }; };
template <> struct AlignBytes<ulong4>               { enum { ALIGN_BYTES = 16 }; };
template <> struct AlignBytes<longlong2>            { enum { ALIGN_BYTES = 16 }; };
template <> struct AlignBytes<ulonglong2>           { enum { ALIGN_BYTES = 16 }; };
template <> struct AlignBytes<double2>              { enum { ALIGN_BYTES = 16 }; };
template <> struct AlignBytes<longlong4>            { enum { ALIGN_BYTES = 16 }; };
template <> struct AlignBytes<ulonglong4>           { enum { ALIGN_BYTES = 16 }; };
template <> struct AlignBytes<double4>              { enum { ALIGN_BYTES = 16 }; };


/// Unit-words of data movement
template <typename T>
struct UnitWord
{
    enum {
        ALIGN_BYTES = AlignBytes<T>::ALIGN_BYTES
    };

    template <typename Unit>
    struct IsMultiple
    {
        enum {
            UNIT_ALIGN_BYTES    = AlignBytes<Unit>::ALIGN_BYTES,
            IS_MULTIPLE         = (sizeof(T) % sizeof(Unit) == 0) && (ALIGN_BYTES % UNIT_ALIGN_BYTES == 0)
        };
    };

    /// Biggest shuffle word that T is a whole multiple of and is not larger than the alignment of T
    typedef typename If<IsMultiple<int>::IS_MULTIPLE,
        unsigned int,
        typename If<IsMultiple<short>::IS_MULTIPLE,
            unsigned short,
            unsigned char>::Type>::Type         ShuffleWord;

    /// Biggest volatile word that T is a whole multiple of and is not larger than the alignment of T
    typedef typename If<IsMultiple<long long>::IS_MULTIPLE,
        unsigned long long,
        ShuffleWord>::Type                      VolatileWord;

    /// Biggest memory-access word that T is a whole multiple of and is not larger than the alignment of T
    typedef typename If<IsMultiple<longlong2>::IS_MULTIPLE,
        ulonglong2,
        VolatileWord>::Type                     DeviceWord;

    /// Biggest texture reference word that T is a whole multiple of and is not larger than the alignment of T
    typedef typename If<IsMultiple<int4>::IS_MULTIPLE,
        uint4,
        typename If<IsMultiple<int2>::IS_MULTIPLE,
            uint2,
            ShuffleWord>::Type>::Type           TextureWord;
};

}


namespace vtkm {
namespace exec {
namespace cuda {
namespace internal {

template <
    typename    T,
    typename    Offset = ptrdiff_t>
class DaxTexObjInputIterator
{
public:

    // Required iterator traits
    typedef DaxTexObjInputIterator              self_type;              ///< My own type
    typedef Offset                              difference_type;        ///< Type to express the result of subtracting one iterator from another
    typedef T                                   value_type;             ///< The type of the element the iterator can point to
    typedef T*                                  pointer;                ///< The type of a pointer to an element the iterator can point to
    typedef T                                   reference;              ///< The type of a reference to an element the iterator can point to

    // Use Thrust's iterator categories so we can use these iterators in Thrust 1.7 (or newer) methods
    typedef typename ::thrust::detail::iterator_facade_category<
        ::thrust::device_system_tag,
        ::thrust::random_access_traversal_tag,
        value_type,
        reference
      >::type iterator_category;                                        ///< The iterator category

private:

    // Largest texture word we can use in device
    typedef typename UnitWord<T>::TextureWord TextureWord;

    // Number of texture words per T
    enum { TEXTURE_MULTIPLE = sizeof(T) / sizeof(TextureWord) };

private:

    const T*                  ptr;
    difference_type     tex_offset;
    cudaTextureObject_t tex_obj;

public:

    /// Constructor
    __host__ __device__ __forceinline__ DaxTexObjInputIterator()
    :
        ptr(NULL),
        tex_offset(0),
        tex_obj(0)
    {}

    /// Use this iterator to bind \p ptr with a texture reference
    cudaError_t BindTexture(
        const ::thrust::system::cuda::pointer<T>  ptr,               ///< Native pointer to wrap that is aligned to cudaDeviceProp::textureAlignment
        size_t          numElements,        ///< Number of elements in the range
        size_t          tex_offset = 0)     ///< Offset (in items) from \p ptr denoting the position of the iterator
    {
        this->ptr = ptr.get();
        this->tex_offset = tex_offset;

        cudaChannelFormatDesc   channel_desc = cudaCreateChannelDesc<TextureWord>();
        cudaResourceDesc        res_desc;
        cudaTextureDesc         tex_desc;
        memset(&res_desc, 0, sizeof(cudaResourceDesc));
        memset(&tex_desc, 0, sizeof(cudaTextureDesc));
        res_desc.resType                = cudaResourceTypeLinear;
        res_desc.res.linear.devPtr      = (void*)ptr.get();
        res_desc.res.linear.desc        = channel_desc;
        res_desc.res.linear.sizeInBytes = numElements * sizeof(T);
        tex_desc.readMode               = cudaReadModeElementType;

        return cudaCreateTextureObject(&tex_obj, &res_desc, &tex_desc, NULL);
    }

    /// Unbind this iterator from its texture reference
    cudaError_t UnbindTexture()
    {
      return cudaDestroyTextureObject(tex_obj);
    }

    /// Postfix increment
    __host__ __device__ __forceinline__ self_type operator++(int)
    {
        self_type retval = *this;
        tex_offset++;
        return retval;
    }

    /// Prefix increment
    __host__ __device__ __forceinline__ self_type operator++()
    {
        tex_offset++;
        return *this;
    }

    /// Postfix decrement
    __host__ __device__ __forceinline__ self_type operator--(int)
    {
        self_type retval = *this;
        tex_offset--;
        return retval;
    }

    /// Prefix decrement
    __host__ __device__ __forceinline__ self_type operator--()
    {
        tex_offset--;
        return *this;
    }

    /// Indirection
    __host__ __device__ __forceinline__ reference operator*() const
    {
#ifndef DAX_CUDA_COMPILATION
        // Simply dereference the pointer on the host
        return ptr[tex_offset];
#else
        // Move array of uninitialized words, then alias and assign to return value
        TextureWord words[TEXTURE_MULTIPLE];

        #pragma unroll
        for (int i = 0; i < TEXTURE_MULTIPLE; ++i)
        {
            words[i] = tex1Dfetch<TextureWord>(
                tex_obj,
                (tex_offset * TEXTURE_MULTIPLE) + i);
        }

        // Load from words
        return *reinterpret_cast<T*>(words);
#endif
    }

    /// Addition
    template <typename Distance>
    __host__ __device__ __forceinline__ self_type operator+(Distance n) const
    {
        self_type retval;
        retval.ptr          = ptr;
        retval.tex_obj      = tex_obj;
        retval.tex_offset   = tex_offset + n;
        return retval;
    }

    /// Addition assignment
    template <typename Distance>
    __host__ __device__ __forceinline__ self_type& operator+=(Distance n)
    {
        tex_offset += n;
        return *this;
    }

    /// Subtraction
    template <typename Distance>
    __host__ __device__ __forceinline__ self_type operator-(Distance n) const
    {
        self_type retval;
        retval.ptr          = ptr;
        retval.tex_obj      = tex_obj;
        retval.tex_offset   = tex_offset - n;
        return retval;
    }

    /// Subtraction assignment
    template <typename Distance>
    __host__ __device__ __forceinline__ self_type& operator-=(Distance n)
    {
        tex_offset -= n;
        return *this;
    }

    /// Distance
    __host__ __device__ __forceinline__ difference_type operator-(self_type other) const
    {
        return tex_offset - other.tex_offset;
    }

    /// Array subscript
    template <typename Distance>
    __host__ __device__ __forceinline__ reference operator[](Distance n) const
    {
        return *(*this + n);
    }

    /// Structure dereference
    __host__ __device__ __forceinline__ pointer operator->()
    {
        return &(*(*this));
    }

    /// Equal to
    __host__ __device__ __forceinline__ bool operator==(const self_type& rhs) const
    {
        return ((ptr == rhs.ptr) && (tex_offset == rhs.tex_offset) && (tex_obj == rhs.tex_obj));
    }

    /// Not equal to
    __host__ __device__ __forceinline__ bool operator!=(const self_type& rhs) const
    {
        return ((ptr != rhs.ptr) || (tex_offset != rhs.tex_offset) || (tex_obj != rhs.tex_obj));
    }

    /// less than
    __host__ __device__ __forceinline__ bool operator<(const self_type& rhs)
    {
        return (tex_offset < rhs.tex_offset);
    }

    /// ostream operator
    friend std::ostream& operator<<(std::ostream& os, const self_type& itr)
    {
        return os;
    }

};


template<class TextureIterator>
class ConstArrayPortalFromTexture
{
public:

  typedef typename TextureIterator::value_type ValueType;
  typedef TextureIterator IteratorType;

  VTKM_EXEC_CONT_EXPORT ConstArrayPortalFromTexture() {  }

  VTKM_CONT_EXPORT
  ConstArrayPortalFromTexture(IteratorType begin, ptrdiff_t size)
    : Length(size),
      BeginIterator(begin),
      EndIterator(begin+size)
  {  }

  /// Copy constructor for any other ConstArrayPortalFromTexture with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<typename OtherIteratorT>
  VTKM_EXEC_CONT_EXPORT
  ConstArrayPortalFromTexture(const ConstArrayPortalFromTexture<OtherIteratorT> &src)
    : Length(src.Length),
      BeginIterator(src.BeginIterator),
      EndIterator(src.EndIterator)
  {  }

  template<typename OtherIteratorT>
  VTKM_EXEC_CONT_EXPORT
  ConstArrayPortalFromTexture<IteratorType> &operator=(
      const ConstArrayPortalFromTexture<OtherIteratorT> &src)
  {
    this->Length = src.Length;
    this->BeginIterator = src.BeginIterator;
    this->EndIterator = src.EndIterator;
    return *this;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    return static_cast<vtkm::Id>(this->Length);
  }

  VTKM_EXEC_EXPORT
  ValueType Get(vtkm::Id index) const {
    return *this->IteratorAt(index);
  }

  VTKM_EXEC_EXPORT
  void Set(vtkm::Id index, ValueType value) const {
    *this->IteratorAt(index) = value;
  }

  VTKM_CONT_EXPORT
  IteratorType GetIteratorBegin() const { return this->BeginIterator; }

  VTKM_CONT_EXPORT
  IteratorType GetIteratorEnd() const { return this->EndIterator; }

private:
  ptrdiff_t Length;
  IteratorType BeginIterator;
  IteratorType EndIterator;

  VTKM_EXEC_EXPORT
  IteratorType IteratorAt(vtkm::Id index) const {
    // Not using std::advance because on CUDA it cannot be used on a device.
    return (this->BeginIterator + index);
  }
};

}
}
}
} // namespace vtkm::exec::cuda::internal


#endif //vtk_m_exec_cuda_internal_ArrayPortalFromTexture_h
