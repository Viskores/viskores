==============================
Math
==============================

.. index:: math

|VTKm| comes with several math functions that tend to be useful for visualization algorithms.
The implementation of basic math operations can vary subtly on different accelerators, and these functions provide cross platform support.

All math functions are located in the ``vtkm`` package.
The functions are most useful in the execution environment, but they can also be used in the control environment when needed.

------------------------------
Basic Math
------------------------------

The :file:`vtkm/Math.h` header file contains several math functions that replicate the behavior of the basic POSIX math functions as well as related functionality.

.. didyouknow::
   When writing worklets, you should favor using these math functions provided by |VTKm| over the standard math functions in :file:`vtkm/Math.h`.
   |VTKm|'s implementation manages several compiling and efficiency issues when porting.

Exponentials
==============================

.. doxygenfunction:: vtkm::Exp(vtkm::Float32)
.. doxygenfunction:: vtkm::Exp(vtkm::Float64)
.. doxygenfunction:: vtkm::Exp(const T&)
.. doxygenfunction:: vtkm::Exp(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::Exp10(vtkm::Float32)
.. doxygenfunction:: vtkm::Exp10(vtkm::Float64)
.. doxygenfunction:: vtkm::Exp10(T)
.. doxygenfunction:: vtkm::Exp10(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::Exp2(vtkm::Float32)
.. doxygenfunction:: vtkm::Exp2(vtkm::Float64)
.. doxygenfunction:: vtkm::Exp2(const T&)
.. doxygenfunction:: vtkm::Exp2(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::ExpM1(vtkm::Float32)
.. doxygenfunction:: vtkm::ExpM1(vtkm::Float64)
.. doxygenfunction:: vtkm::ExpM1(const T&)
.. doxygenfunction:: vtkm::ExpM1(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::Log(vtkm::Float32)
.. doxygenfunction:: vtkm::Log(vtkm::Float64)
.. doxygenfunction:: vtkm::Log(const T&)
.. doxygenfunction:: vtkm::Log(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::Log10(vtkm::Float32)
.. doxygenfunction:: vtkm::Log10(vtkm::Float64)
.. doxygenfunction:: vtkm::Log10(const T&)
.. doxygenfunction:: vtkm::Log10(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::Log1P(vtkm::Float32)
.. doxygenfunction:: vtkm::Log1P(vtkm::Float64)
.. doxygenfunction:: vtkm::Log1P(const T&)
.. doxygenfunction:: vtkm::Log1P(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::Log2(vtkm::Float32)
.. doxygenfunction:: vtkm::Log2(vtkm::Float64)
.. doxygenfunction:: vtkm::Log2(const T&)
.. doxygenfunction:: vtkm::Log2(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::Pow(vtkm::Float32, vtkm::Float32)
.. doxygenfunction:: vtkm::Pow(vtkm::Float64, vtkm::Float64)

Non-finites
==============================

.. doxygenfunction:: Infinity
.. doxygenfunction:: Infinity32
.. doxygenfunction:: Infinity64

.. doxygenfunction:: IsFinite
.. doxygenfunction:: IsInf
.. doxygenfunction:: IsNan
.. doxygenfunction:: IsNegative(vtkm::Float32)
.. doxygenfunction:: IsNegative(vtkm::Float64)

.. doxygenfunction:: Nan
.. doxygenfunction:: Nan32
.. doxygenfunction:: Nan64

.. doxygenfunction:: NegativeInfinity
.. doxygenfunction:: NegativeInfinity32
.. doxygenfunction:: NegativeInfinity64

Polynomials
==============================

.. doxygenfunction:: vtkm::Cbrt(vtkm::Float32)
.. doxygenfunction:: vtkm::Cbrt(vtkm::Float64)
.. doxygenfunction:: vtkm::Cbrt(const T&)
.. doxygenfunction:: vtkm::Cbrt(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::QuadraticRoots

.. doxygenfunction:: vtkm::RCbrt(vtkm::Float32)
.. doxygenfunction:: vtkm::RCbrt(vtkm::Float64)
.. doxygenfunction:: vtkm::RCbrt(T)
.. doxygenfunction:: vtkm::RCbrt(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::RSqrt(vtkm::Float32)
.. doxygenfunction:: vtkm::RSqrt(vtkm::Float64)
.. doxygenfunction:: vtkm::RSqrt(T)
.. doxygenfunction:: vtkm::RSqrt(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::Sqrt(vtkm::Float32)
.. doxygenfunction:: vtkm::Sqrt(vtkm::Float64)
.. doxygenfunction:: vtkm::Sqrt(const T&)
.. doxygenfunction:: vtkm::Sqrt(const vtkm::Vec<T, N>&)

Remainders and Quotient
==============================

.. doxygenfunction:: vtkm::ModF(vtkm::Float32, vtkm::Float32&)
.. doxygenfunction:: vtkm::ModF(vtkm::Float64, vtkm::Float64&)

.. doxygenfunction:: vtkm::Remainder(vtkm::Float32, vtkm::Float32)
.. doxygenfunction:: vtkm::Remainder(vtkm::Float64, vtkm::Float64)

.. doxygenfunction:: RemainderQuotient(vtkm::Float32, vtkm::Float32, QType&)
.. doxygenfunction:: RemainderQuotient(vtkm::Float64, vtkm::Float64, QType&)

Rounding and Precision
==============================

.. doxygenfunction:: vtkm::Ceil(vtkm::Float32)
.. doxygenfunction:: vtkm::Ceil(vtkm::Float64)
.. doxygenfunction:: vtkm::Ceil(const T&)
.. doxygenfunction:: vtkm::Ceil(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::CopySign(vtkm::Float32, vtkm::Float32)
.. doxygenfunction:: vtkm::CopySign(vtkm::Float64, vtkm::Float64)
.. doxygenfunction:: vtkm::CopySign(const vtkm::Vec<T, N>&, const vtkm::Vec<T, N>&)

.. doxygenfunction:: Epsilon
.. doxygenfunction:: Epsilon32
.. doxygenfunction:: Epsilon64

.. doxygenfunction:: vtkm::FMod(vtkm::Float32, vtkm::Float32)
.. doxygenfunction:: vtkm::FMod(vtkm::Float64, vtkm::Float64)

.. doxygenfunction:: vtkm::Round(vtkm::Float32)
.. doxygenfunction:: vtkm::Round(vtkm::Float64)
.. doxygenfunction:: vtkm::Round(const T&)
.. doxygenfunction:: vtkm::Round(const vtkm::Vec<T, N>&)

Sign
==============================

.. doxygenfunction:: vtkm::Abs(vtkm::Int32)
.. doxygenfunction:: vtkm::Abs(vtkm::Int64)
.. doxygenfunction:: vtkm::Abs(vtkm::Float32)
.. doxygenfunction:: vtkm::Abs(vtkm::Float64)
.. doxygenfunction:: vtkm::Abs(T)
.. doxygenfunction:: vtkm::Abs(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::Floor(vtkm::Float32)
.. doxygenfunction:: vtkm::Floor(vtkm::Float64)
.. doxygenfunction:: vtkm::Floor(const T&)
.. doxygenfunction:: vtkm::Floor(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::SignBit(vtkm::Float32)
.. doxygenfunction:: vtkm::SignBit(vtkm::Float64)

Trigonometry
==============================

.. doxygenfunction:: vtkm::ACos(vtkm::Float32)
.. doxygenfunction:: vtkm::ACos(vtkm::Float64)
.. doxygenfunction:: vtkm::ACos(const T&)
.. doxygenfunction:: vtkm::ACos(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::ACosH(vtkm::Float32)
.. doxygenfunction:: vtkm::ACosH(vtkm::Float64)
.. doxygenfunction:: vtkm::ACosH(const T&)
.. doxygenfunction:: vtkm::ACosH(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::ASin(vtkm::Float32)
.. doxygenfunction:: vtkm::ASin(vtkm::Float64)
.. doxygenfunction:: vtkm::ASin(const T&)
.. doxygenfunction:: vtkm::ASin(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::ASinH(vtkm::Float32)
.. doxygenfunction:: vtkm::ASinH(vtkm::Float64)
.. doxygenfunction:: vtkm::ASinH(const T&)
.. doxygenfunction:: vtkm::ASinH(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::ATan(vtkm::Float32)
.. doxygenfunction:: vtkm::ATan(vtkm::Float64)
.. doxygenfunction:: vtkm::ATan(const T&)
.. doxygenfunction:: vtkm::ATan(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::ATan2(vtkm::Float32, vtkm::Float32)
.. doxygenfunction:: vtkm::ATan2(vtkm::Float64, vtkm::Float64)

.. doxygenfunction:: vtkm::ATanH(vtkm::Float32)
.. doxygenfunction:: vtkm::ATanH(vtkm::Float64)
.. doxygenfunction:: vtkm::ATanH(const T&)
.. doxygenfunction:: vtkm::ATanH(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::Cos(vtkm::Float32)
.. doxygenfunction:: vtkm::Cos(vtkm::Float64)
.. doxygenfunction:: vtkm::Cos(const T&)
.. doxygenfunction:: vtkm::Cos(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::CosH(vtkm::Float32)
.. doxygenfunction:: vtkm::CosH(vtkm::Float64)
.. doxygenfunction:: vtkm::CosH(const T&)
.. doxygenfunction:: vtkm::CosH(const vtkm::Vec<T, N>&)

.. doxygenfunction:: Pi
.. doxygenfunction:: Pi_2
.. doxygenfunction:: Pi_3
.. doxygenfunction:: Pi_4
.. doxygenfunction:: Pi_180

.. doxygenfunction:: vtkm::Sin(vtkm::Float32)
.. doxygenfunction:: vtkm::Sin(vtkm::Float64)
.. doxygenfunction:: vtkm::Sin(const T&)
.. doxygenfunction:: vtkm::Sin(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::SinH(vtkm::Float32)
.. doxygenfunction:: vtkm::SinH(vtkm::Float64)
.. doxygenfunction:: vtkm::SinH(const T&)
.. doxygenfunction:: vtkm::SinH(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::Tan(vtkm::Float32)
.. doxygenfunction:: vtkm::Tan(vtkm::Float64)
.. doxygenfunction:: vtkm::Tan(const T&)
.. doxygenfunction:: vtkm::Tan(const vtkm::Vec<T, N>&)

.. doxygenfunction:: vtkm::TanH(vtkm::Float32)
.. doxygenfunction:: vtkm::TanH(vtkm::Float64)
.. doxygenfunction:: vtkm::TanH(const T&)
.. doxygenfunction:: vtkm::TanH(const vtkm::Vec<T, N>&)

.. doxygenfunction:: TwoPi

Miscellaneous
==============================

.. doxygenfunction:: FloatDistance(vtkm::Float64, vtkm::Float64)
.. doxygenfunction:: FloatDistance(vtkm::Float32, vtkm::Float32)

.. doxygenfunction:: Max(const T&, const T&)
.. doxygenfunction:: Min(const T&, const T&)


------------------------------
Vector Analysis
------------------------------

.. index:: vector analysis

Visualization and computational geometry algorithms often perform vector analysis operations.
The :file:`vtkm/VectorAnalysis.h` header file provides functions that perform the basic common vector analysis operations.

.. doxygenfunction:: vtkm::Cross
.. doxygenfunction:: vtkm::Lerp(const ValueType&, const ValueType&, const WeightType&)
.. doxygenfunction:: vtkm::Magnitude
.. doxygenfunction:: vtkm::MagnitudeSquared
.. doxygenfunction:: vtkm::Normal
.. doxygenfunction:: vtkm::Normalize
.. doxygenfunction:: vtkm::Orthonormalize
.. doxygenfunction:: vtkm::Project
.. doxygenfunction:: vtkm::ProjectedDistance
.. doxygenfunction:: vtkm::RMagnitude
.. doxygenfunction:: vtkm::TriangleNormal


------------------------------
Matrices
------------------------------

.. index:: matrix

Linear algebra operations on small matrices that are done on a single thread are located in :file:`vtkm/Matrix.h`.

This header defines the :class:`vtkm::Matrix` templated class.
The template parameters are first the type of component, then the number of rows, then the number of columns.
The overloaded parentheses operator can be used to retrieve values based on row and column indices.
Likewise, the bracket operators can be used to reference the :class:`vtkm::Matrix` as a 2D array (indexed by row first).

.. doxygenclass:: vtkm::Matrix
   :members:

The following example builds a :class:`vtkm::Matrix` that contains the values

.. math::
   \left|
   \begin{array}{ccc}
     0 & 1 & 2 \\
     10 & 11 & 12
   \end{array}
   \right|

.. load-example:: BuildMatrix
   :file: GuideExampleMatrix.cxx
   :caption: Creating a :class:`vtkm::Matrix`.

The :file:`vtkm/Matrix.h` header also defines the following functions
that operate on matrices.

.. index::
   single: matrix; determinant
   single: determinant

.. doxygenfunction:: vtkm::MatrixDeterminant(const vtkm::Matrix<T, Size, Size>&)

.. doxygenfunction:: vtkm::MatrixGetColumn
.. doxygenfunction:: vtkm::MatrixGetRow

.. index::
   double: identity; matrix

.. doxygenfunction:: vtkm::MatrixIdentity()
.. doxygenfunction:: vtkm::MatrixIdentity(vtkm::Matrix<T, Size, Size>&)

.. index::
   double: inverse; matrix

.. doxygenfunction:: vtkm::MatrixInverse

.. doxygenfunction:: vtkm::MatrixMultiply(const vtkm::Matrix<T, NumRow, NumInternal>&, const vtkm::Matrix<T, NumInternal, NumCol>&)
.. doxygenfunction:: vtkm::MatrixMultiply(const vtkm::Matrix<T, NumRow, NumCol>&, const vtkm::Vec<T, NumCol>&)
.. doxygenfunction:: vtkm::MatrixMultiply(const vtkm::Vec<T, NumRow>&, const vtkm::Matrix<T, NumRow, NumCol>&)

.. doxygenfunction:: vtkm::MatrixSetColumn
.. doxygenfunction:: vtkm::MatrixSetRow

.. index::
   double: transpose; matrix

.. doxygenfunction:: vtkm::MatrixTranspose

.. index:: linear system

.. doxygenfunction:: vtkm::SolveLinearSystem


------------------------------
Newton's Method
------------------------------

.. index:: Newton's method

|VTKm|'s matrix methods (documented in :secref:`math:Matrices`)
provide a method to solve a small linear system of equations. However,
sometimes it is necessary to solve a small nonlinear system of equations.
This can be done with the :func:`vtkm::NewtonsMethod` function defined in the
:file:`vtkm/NewtonsMethod.h` header.

The :func:`vtkm::NewtonsMethod` function assumes that the number of
variables equals the number of equations. Newton's method operates on an
iterative evaluate and search. Evaluations are performed using the functors
passed into the :func:`vtkm::NewtonsMethod`.

.. doxygenfunction:: vtkm::NewtonsMethod

The :func:`vtkm::NewtonsMethod` function returns a \vtkm{NewtonsMethodResult} object.
\textidentifier{NewtonsMethodResult} is a \textcode{struct} templated on the type and number of input values of the nonlinear system.
\textidentifier{NewtonsMethodResult} contains the following items.

.. doxygenstruct:: vtkm::NewtonsMethodResult
   :members:

.. load-example:: NewtonsMethod
   :file: GuideExampleNewtonsMethod.cxx
   :caption: Using :func:`vtkm::NewtonsMethod` to solve a small system of nonlinear equations.
