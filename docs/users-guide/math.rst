==============================
Math
==============================

.. index:: math

|Viskores| comes with several math functions that tend to be useful for visualization algorithms.
The implementation of basic math operations can vary subtly on different accelerators, and these functions provide cross platform support.

All math functions are located in the ``viskores`` package.
The functions are most useful in the execution environment, but they can also be used in the control environment when needed.

------------------------------
Basic Math
------------------------------

The :file:`viskores/Math.h` header file contains several math functions that replicate the behavior of the basic POSIX math functions as well as related functionality.

.. didyouknow::
   When writing worklets, you should favor using these math functions provided by |Viskores| over the standard math functions in :file:`viskores/Math.h`.
   |Viskores|'s implementation manages several compiling and efficiency issues when porting.

Comparison and Distance
==============================

.. doxygenfunction:: Clamp

.. doxygenfunction:: FloatDistance(viskores::Float64, viskores::Float64)
.. doxygenfunction:: FloatDistance(viskores::Float32, viskores::Float32)

.. doxygenfunction:: Max(const T&, const T&)
.. doxygenfunction:: Min(const T&, const T&)

Exponentials
==============================

.. doxygenfunction:: viskores::Exp(viskores::Float32)
.. doxygenfunction:: viskores::Exp(viskores::Float64)
.. doxygenfunction:: viskores::Exp(const T&)
.. doxygenfunction:: viskores::Exp(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::Exp10(viskores::Float32)
.. doxygenfunction:: viskores::Exp10(viskores::Float64)
.. doxygenfunction:: viskores::Exp10(T)
.. doxygenfunction:: viskores::Exp10(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::Exp2(viskores::Float32)
.. doxygenfunction:: viskores::Exp2(viskores::Float64)
.. doxygenfunction:: viskores::Exp2(const T&)
.. doxygenfunction:: viskores::Exp2(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::ExpM1(viskores::Float32)
.. doxygenfunction:: viskores::ExpM1(viskores::Float64)
.. doxygenfunction:: viskores::ExpM1(const T&)
.. doxygenfunction:: viskores::ExpM1(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::Log(viskores::Float32)
.. doxygenfunction:: viskores::Log(viskores::Float64)
.. doxygenfunction:: viskores::Log(const T&)
.. doxygenfunction:: viskores::Log(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::Log10(viskores::Float32)
.. doxygenfunction:: viskores::Log10(viskores::Float64)
.. doxygenfunction:: viskores::Log10(const T&)
.. doxygenfunction:: viskores::Log10(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::Log1P(viskores::Float32)
.. doxygenfunction:: viskores::Log1P(viskores::Float64)
.. doxygenfunction:: viskores::Log1P(const T&)
.. doxygenfunction:: viskores::Log1P(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::Log2(viskores::Float32)
.. doxygenfunction:: viskores::Log2(viskores::Float64)
.. doxygenfunction:: viskores::Log2(const T&)
.. doxygenfunction:: viskores::Log2(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::Pow(viskores::Float32, viskores::Float32)
.. doxygenfunction:: viskores::Pow(viskores::Float64, viskores::Float64)

Non-finites
==============================

.. doxygenfunction:: Infinity
.. doxygenfunction:: Infinity32
.. doxygenfunction:: Infinity64

.. doxygenfunction:: IsFinite
.. doxygenfunction:: IsInf
.. doxygenfunction:: IsNan
.. doxygenfunction:: IsNegative(viskores::Float32)
.. doxygenfunction:: IsNegative(viskores::Float64)

.. doxygenfunction:: Nan
.. doxygenfunction:: Nan32
.. doxygenfunction:: Nan64

.. doxygenfunction:: NegativeInfinity
.. doxygenfunction:: NegativeInfinity32
.. doxygenfunction:: NegativeInfinity64

Polynomials
==============================

.. doxygenfunction:: viskores::Cbrt(viskores::Float32)
.. doxygenfunction:: viskores::Cbrt(viskores::Float64)
.. doxygenfunction:: viskores::Cbrt(const T&)
.. doxygenfunction:: viskores::Cbrt(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::QuadraticRoots

.. doxygenfunction:: viskores::RCbrt(viskores::Float32)
.. doxygenfunction:: viskores::RCbrt(viskores::Float64)
.. doxygenfunction:: viskores::RCbrt(T)
.. doxygenfunction:: viskores::RCbrt(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::RSqrt(viskores::Float32)
.. doxygenfunction:: viskores::RSqrt(viskores::Float64)
.. doxygenfunction:: viskores::RSqrt(T)
.. doxygenfunction:: viskores::RSqrt(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::Sqrt(viskores::Float32)
.. doxygenfunction:: viskores::Sqrt(viskores::Float64)
.. doxygenfunction:: viskores::Sqrt(const T&)
.. doxygenfunction:: viskores::Sqrt(const viskores::Vec<T, N>&)

Remainders and Quotient
==============================

.. doxygenfunction:: viskores::ModF(viskores::Float32, viskores::Float32&)
.. doxygenfunction:: viskores::ModF(viskores::Float64, viskores::Float64&)

.. doxygenfunction:: viskores::Remainder(viskores::Float32, viskores::Float32)
.. doxygenfunction:: viskores::Remainder(viskores::Float64, viskores::Float64)

.. doxygenfunction:: RemainderQuotient(viskores::Float32, viskores::Float32, QType&)
.. doxygenfunction:: RemainderQuotient(viskores::Float64, viskores::Float64, QType&)

Rounding and Precision
==============================

.. doxygenfunction:: viskores::Ceil(viskores::Float32)
.. doxygenfunction:: viskores::Ceil(viskores::Float64)
.. doxygenfunction:: viskores::Ceil(const T&)
.. doxygenfunction:: viskores::Ceil(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::CopySign(viskores::Float32, viskores::Float32)
.. doxygenfunction:: viskores::CopySign(viskores::Float64, viskores::Float64)
.. doxygenfunction:: viskores::CopySign(const viskores::Vec<T, N>&, const viskores::Vec<T, N>&)

.. doxygenfunction:: Epsilon
.. doxygenfunction:: Epsilon32
.. doxygenfunction:: Epsilon64

.. doxygenfunction:: viskores::FMod(viskores::Float32, viskores::Float32)
.. doxygenfunction:: viskores::FMod(viskores::Float64, viskores::Float64)

.. doxygenfunction:: viskores::Round(viskores::Float32)
.. doxygenfunction:: viskores::Round(viskores::Float64)
.. doxygenfunction:: viskores::Round(const T&)
.. doxygenfunction:: viskores::Round(const viskores::Vec<T, N>&)

Sign
==============================

.. doxygenfunction:: viskores::Abs(viskores::Int32)
.. doxygenfunction:: viskores::Abs(viskores::Int64)
.. doxygenfunction:: viskores::Abs(viskores::Float32)
.. doxygenfunction:: viskores::Abs(viskores::Float64)
.. doxygenfunction:: viskores::Abs(T)
.. doxygenfunction:: viskores::Abs(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::Floor(viskores::Float32)
.. doxygenfunction:: viskores::Floor(viskores::Float64)
.. doxygenfunction:: viskores::Floor(const T&)
.. doxygenfunction:: viskores::Floor(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::SignBit(viskores::Float32)
.. doxygenfunction:: viskores::SignBit(viskores::Float64)

Trigonometry
==============================

.. doxygenfunction:: viskores::ACos(viskores::Float32)
.. doxygenfunction:: viskores::ACos(viskores::Float64)
.. doxygenfunction:: viskores::ACos(const T&)
.. doxygenfunction:: viskores::ACos(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::ACosH(viskores::Float32)
.. doxygenfunction:: viskores::ACosH(viskores::Float64)
.. doxygenfunction:: viskores::ACosH(const T&)
.. doxygenfunction:: viskores::ACosH(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::ASin(viskores::Float32)
.. doxygenfunction:: viskores::ASin(viskores::Float64)
.. doxygenfunction:: viskores::ASin(const T&)
.. doxygenfunction:: viskores::ASin(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::ASinH(viskores::Float32)
.. doxygenfunction:: viskores::ASinH(viskores::Float64)
.. doxygenfunction:: viskores::ASinH(const T&)
.. doxygenfunction:: viskores::ASinH(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::ATan(viskores::Float32)
.. doxygenfunction:: viskores::ATan(viskores::Float64)
.. doxygenfunction:: viskores::ATan(const T&)
.. doxygenfunction:: viskores::ATan(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::ATan2(viskores::Float32, viskores::Float32)
.. doxygenfunction:: viskores::ATan2(viskores::Float64, viskores::Float64)

.. doxygenfunction:: viskores::ATanH(viskores::Float32)
.. doxygenfunction:: viskores::ATanH(viskores::Float64)
.. doxygenfunction:: viskores::ATanH(const T&)
.. doxygenfunction:: viskores::ATanH(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::Cos(viskores::Float32)
.. doxygenfunction:: viskores::Cos(viskores::Float64)
.. doxygenfunction:: viskores::Cos(const T&)
.. doxygenfunction:: viskores::Cos(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::CosH(viskores::Float32)
.. doxygenfunction:: viskores::CosH(viskores::Float64)
.. doxygenfunction:: viskores::CosH(const T&)
.. doxygenfunction:: viskores::CosH(const viskores::Vec<T, N>&)

.. doxygenfunction:: Pi
.. doxygenfunction:: Pi_2
.. doxygenfunction:: Pi_3
.. doxygenfunction:: Pi_4
.. doxygenfunction:: Pi_180

.. doxygenfunction:: viskores::Sin(viskores::Float32)
.. doxygenfunction:: viskores::Sin(viskores::Float64)
.. doxygenfunction:: viskores::Sin(const T&)
.. doxygenfunction:: viskores::Sin(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::SinH(viskores::Float32)
.. doxygenfunction:: viskores::SinH(viskores::Float64)
.. doxygenfunction:: viskores::SinH(const T&)
.. doxygenfunction:: viskores::SinH(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::Tan(viskores::Float32)
.. doxygenfunction:: viskores::Tan(viskores::Float64)
.. doxygenfunction:: viskores::Tan(const T&)
.. doxygenfunction:: viskores::Tan(const viskores::Vec<T, N>&)

.. doxygenfunction:: viskores::TanH(viskores::Float32)
.. doxygenfunction:: viskores::TanH(viskores::Float64)
.. doxygenfunction:: viskores::TanH(const T&)
.. doxygenfunction:: viskores::TanH(const viskores::Vec<T, N>&)

.. doxygenfunction:: TwoPi


------------------------------
Vector Analysis
------------------------------

.. index:: vector analysis

Visualization and computational geometry algorithms often perform vector analysis operations.
The :file:`viskores/VectorAnalysis.h` header file provides functions that perform the basic common vector analysis operations.

.. doxygenfunction:: viskores::Cross
.. doxygenfunction:: viskores::Lerp(const ValueType&, const ValueType&, const WeightType&)
.. doxygenfunction:: viskores::Magnitude
.. doxygenfunction:: viskores::MagnitudeSquared
.. doxygenfunction:: viskores::Normal
.. doxygenfunction:: viskores::Normalize
.. doxygenfunction:: viskores::Orthonormalize
.. doxygenfunction:: viskores::Project
.. doxygenfunction:: viskores::ProjectedDistance
.. doxygenfunction:: viskores::RMagnitude
.. doxygenfunction:: viskores::TriangleNormal


------------------------------
Matrices
------------------------------

.. index:: matrix

Linear algebra operations on small matrices that are done on a single thread are located in :file:`viskores/Matrix.h`.

This header defines the :class:`viskores::Matrix` templated class.
The template parameters are first the type of component, then the number of rows, then the number of columns.
The overloaded parentheses operator can be used to retrieve values based on row and column indices.
Likewise, the bracket operators can be used to reference the :class:`viskores::Matrix` as a 2D array (indexed by row first).

.. doxygenclass:: viskores::Matrix
   :members:

The following example builds a :class:`viskores::Matrix` that contains the values

.. math::
   \left|
   \begin{array}{ccc}
     0 & 1 & 2 \\
     10 & 11 & 12
   \end{array}
   \right|

.. load-example:: BuildMatrix
   :file: GuideExampleMatrix.cxx
   :caption: Creating a :class:`viskores::Matrix`.

The :file:`viskores/Matrix.h` header also defines the following functions
that operate on matrices.

.. index::
   single: matrix; determinant
   single: determinant

.. doxygenfunction:: viskores::MatrixDeterminant(const viskores::Matrix<T, Size, Size>&)

.. doxygenfunction:: viskores::MatrixGetColumn
.. doxygenfunction:: viskores::MatrixGetRow

.. index::
   double: identity; matrix

.. doxygenfunction:: viskores::MatrixIdentity()
.. doxygenfunction:: viskores::MatrixIdentity(viskores::Matrix<T, Size, Size>&)

.. index::
   double: inverse; matrix

.. doxygenfunction:: viskores::MatrixInverse

.. doxygenfunction:: viskores::MatrixMultiply(const viskores::Matrix<T, NumRow, NumInternal>&, const viskores::Matrix<T, NumInternal, NumCol>&)
.. doxygenfunction:: viskores::MatrixMultiply(const viskores::Matrix<T, NumRow, NumCol>&, const viskores::Vec<T, NumCol>&)
.. doxygenfunction:: viskores::MatrixMultiply(const viskores::Vec<T, NumRow>&, const viskores::Matrix<T, NumRow, NumCol>&)

.. doxygenfunction:: viskores::MatrixSetColumn
.. doxygenfunction:: viskores::MatrixSetRow

.. index::
   double: transpose; matrix

.. doxygenfunction:: viskores::MatrixTranspose

.. index:: linear system

.. doxygenfunction:: viskores::SolveLinearSystem


------------------------------
Newton's Method
------------------------------

.. index:: Newton's method

|Viskores|'s matrix methods (documented in :secref:`math:Matrices`)
provide a method to solve a small linear system of equations. However,
sometimes it is necessary to solve a small nonlinear system of equations.
This can be done with the :func:`viskores::NewtonsMethod` function defined in the
:file:`viskores/NewtonsMethod.h` header.

The :func:`viskores::NewtonsMethod` function assumes that the number of
variables equals the number of equations. Newton's method operates on an
iterative evaluate and search. Evaluations are performed using the functors
passed into the :func:`viskores::NewtonsMethod`.

.. doxygenfunction:: viskores::NewtonsMethod

The :func:`viskores::NewtonsMethod` function returns a \viskores{NewtonsMethodResult} object.
\textidentifier{NewtonsMethodResult} is a \textcode{struct} templated on the type and number of input values of the nonlinear system.
\textidentifier{NewtonsMethodResult} contains the following items.

.. doxygenstruct:: viskores::NewtonsMethodResult
   :members:

.. load-example:: NewtonsMethod
   :file: GuideExampleNewtonsMethod.cxx
   :caption: Using :func:`viskores::NewtonsMethod` to solve a small system of nonlinear equations.
