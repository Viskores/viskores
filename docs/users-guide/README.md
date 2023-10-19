# VTK-m User's Guide Source

This directory contains the source for building the VTK-m User's Guide. The
document is written for the [Sphinx](https://www.sphinx-doc.org/en/master/)
document generator.


## Building the documentation

To build the document, you will need the following installed on your
system.

  * [Doxygen] - Processes source code to pull out documentation.
  * [Sphinx] - Processes [reStructuredText] to build HTML and LaTeX
    formatted documents.
  * [RTD Theme] - We use the Sphinx Read the Docs theme for formatting the
    generated HTML. The build will fail without this installed.
  * [Sphinx CMake Domain] Sphinx does not support documenting CMake
    elements out of the box. This extension provides that support.
  * [Breathe] - Forms a bridge betweein Doxygen and Sphinx to allow the
    Sphinx reStructuredTest to include documentation extracted by Doxygen.

To enable document generation, you first must turn on the CMake option
`VTKm_ENABLE_DOCUMENTATION`, which turns on the Doxygen documentation. With
that on, you can then turn on the `VTKm_ENABLE_USERS_GUIDE` CMake option.

The documentation will be built into HTML format in the
`docs/users-guide/html` directory in the build. It will also build LaTeX
files in `docs/users-guide/latex`. It will come with a `Makefile` that can
be used to generate a pdf form (given the proper LaTeX compiler).


## Features of the documents

The VTK-m User's Guide is built as a standard [Sphinx] project. However,
there are some features that writers should be aware of.

### Writing credit

Kenneth Moreland is the main author of this document. If you have made a
contribution, you can credit yourself in the `Contributors` section of
[acknowledgements.rst].

### Provided substitutions

The Sphinx configuration provides some convenient substitutions that can be
used throughout the document.

  * `|VTKm|` This should be used whenever `VTK-m` is referenced. The
    substitution contains formatting for the word.
    
### Expanded directives

This reStructuredText is build with some extended directives.

#### Info boxes

Two new "admonition" boxes are supported: `didyouknow` and `commonerrors`.
It is encouraged to use these boxes to highlight interesting features or
common gotchas. These are use like other tip boxes.

``` restructuredtext
.. didyouknow::
   In this guide we periodically use these **Did you know?** boxes to
   provide additional information related to the topic at hand.

.. commonerrors::
   **Common Errors** blocks are used to highlight some of the common
   problems or complications you might encounter when dealing with the
   topic of discussion.
```
    
### Section references

It is desired for the VTK-m User's Guide to be available both online as web
pages and as a self-contained document (e.g. pdf). One issue is that
traditional paper documents work best with numbered references to parts,
chapters, and sections whereas html documents prefer descriptive links.

To service both, this document has extensions to automatically provide
references to document parts. Three roles are created: `:partref:`,
`:chapref:`, and `:secref:` to create cross references to parts, chapters,
and sections, respectively. They each take a label, and Sphinx is
configured with numfig and autosection labels. These labels take the form
<file>:<title> where <file> is the name of the file containing the section
(without the `.rst` extension) and <title> is the full name of the section.

Here are examples of cross references.

``` restructuredtext
:partref:`part-getting-started:Getting Started`

:chapref:`introduction:Introduction`

:secref:`introduction:How to Use This Guide`
```
    
### Example code

The VTK-m User's Guide has numerous code examples. These code examples are
pulled from source files that are compiled and run as part of VTK-m's
regression tests. Although these "tests" are not meant to be thorough
regression tests like the others, they ensure that the documentation stays
up to date and correct.

Examples are added to the `examples` directory more or less like any other
unit test in VTK-m (except by convention we start the name with
`GuideExample`). Each of these files can then be scanned to find excerpts
to include as an example in the guide.

#### Marking examples in the code

A simple text scanner goes through the example code looking for lines
containing a comment starting with 4 slashes, `////`. Any such line will
not be included in the example.

An example can be started with `//// BEGIN-EXAMPLE` and ended with
`//// END-EXAMPLE`. Each of these must be given the name of the example.

``` cpp
  ////
  //// BEGIN-EXAMPLE EquilateralTriangle
  ////
  vtkm::Vec<vtkm::Vec2f_32, 3> equilateralTriangle = { { 0.0f, 0.0f },
                                                       { 1.0f, 0.0f },
                                                       { 0.5f, 0.8660254f } };
  ////
  //// END-EXAMPLE EquilateralTriangle
  ////
```

#### Loading examples in the documentation

An example can be loaded into the VTK-m User's Guide using the extended
reStructuredText `load-example` directive. The directive takes the name of
the example as its argument. `load-example` should also be given the
`:file:` and `:caption:` options.

``` restructuredtext
.. load-example:: EquilateralTriangle
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Defining a triangle in the plane.
```

The following options are supported.

  * `:file:` The filename of the file containing the named example. The
    filename is relative to the `examples` directory.
  * `:caption:` The caption used for the example.
  * `:nolinenos:` Turn off line numbering. By default, line numbers are
    shown, but they are suppressed with this option.
    
#### Referencing examples

The example is registered as a `code-block` the named registered as the
example named prepended with `ex:`. This name can then be referenced with
the `:ref:` and `:numref:` roles as with figures, sections, and other cross
references.

The `:numref:` role is particularly useful for referencing each example.
Using `:numref:` with just the name will be replaced with a link titled
"Example #" with "#" being the number of the example.

``` restructuredtext
:numref:`ex:EquilateralTriangle` shows how the :class:`Vec` class can be used to
store several points in the same structure.
```

The `:numref:` role also supports custom text with number substitution as
described in the [Sphinx
documentation](https://www.sphinx-doc.org/en/master/usage/restructuredtext/roles.html#role-numref)

``` restructuredtext
:numref:`Example %s representes an equilateral triangle<ex:EquilateralTriangle>`
using the :class:`Vec` class.
```
    
#### Pausing and resuming capture

Sometimes it is useful to insert code in the compiled test that is not
included as part of the example. This is useful to, for example, insert a
check for values, which can verify that the code is working correctly but
is not necessary for the example.

When a `//// PAUSE-EXAMPLE` is inserted into the code, the following lines
will not be captured until the line `//// RESUME-EXAMPLE` is encountered.

``` cpp
  range.Include(2.0);            // range is now [0.5 .. 2]
  bool b5 = range.Contains(0.5); // b3 is true
  bool b6 = range.Contains(0.6); // b4 is true

  range.Include(vtkm::Range(-1, 1)); // range is now [-1 .. 2]
  //// PAUSE-EXAMPLE
  VTKM_TEST_ASSERT(test_equal(range, vtkm::Range(-1, 2)), "Bad range");
  //// RESUME-EXAMPLE
```

#### Referencing a specific line

You apply a label to a specific line in the code by adding a `//// LABEL`
comment right before it. The `LABEL` needs a text string used to reference
the line.

``` cpp
////
//// BEGIN-EXAMPLE VecCExample
////
//// LABEL index-to-ijk
VTKM_EXEC vtkm::VecCConst<vtkm::IdComponent> HexagonIndexToIJK(vtkm::IdComponent index)
{
  static const vtkm::IdComponent HexagonIndexToIJKTable[8][3] = {
    { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 },
    { 0, 0, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 }
  };

  return vtkm::make_VecC(HexagonIndexToIJKTable[index], 3);
}

//// LABEL ijk-to-index
VTKM_EXEC vtkm::IdComponent HexagonIJKToIndex(vtkm::VecCConst<vtkm::IdComponent> ijk)
{
  static const vtkm::IdComponent HexagonIJKToIndexTable[2][2][2] = {
    {
      // i=0
      { 0, 4 }, // j=0
      { 3, 7 }, // j=1
    },
    {
      // i=1
      { 1, 5 }, // j=0
      { 2, 6 }, // j=1
    }
  };

  return HexagonIJKToIndexTable[ijk[0]][ijk[1]][ijk[2]];
}
////
//// END-EXAMPLE VecCExample
////
```

This line can be referenced in the text using the extended reStructuredText
`:exlineref:` role. The role takes references of the form
"example-name:line-label". If given just this reference, the link text is
"Example #, line #".

``` restructuredtext
A function to convert a 3D index to a flat index starts on
:exlineref:`VecCExample:ijk-to-index`.
```

`:exlineref:` also accepts a formatting string like the `:numref:` builtin
role. `%s` and `{line}` will be replaced with the line number.
`:exlineref:` also follows the [`:numref:`
convention](https://www.sphinx-doc.org/en/master/usage/restructuredtext/roles.html#role-numref)
of replacing `{number}` and `{name}` with the example number and caption,
respectively.

``` restructuredtext
You can convert a flat index to a 3D index (shown starting on
:exlineref:`line {line} in Example {number}<VecCExample:index-to-ijk>`) and
the inverse function (:exlineref:`line %s<VecCExample:ijk-to-index>`).
```

### Ingesting Doxygen

The VTK-m User's Guide is built with [Breathe], which allows it to pull in
Doxygen documentation. Use [Breathe's
directives](https://breathe.readthedocs.io/en/latest/directives.html#directives)
to include the doxygen documentation.

[Sphinx]: https://www.sphinx-doc.org/en/master/
[Doxygen]: https://www.doxygen.nl/
[RTD Theme]: https://sphinx-themes.org/sample-sites/sphinx-rtd-theme/
[Sphinx CMake Domain]: https://github.com/scikit-build/moderncmakedomain
[Breathe]: https://www.breathe-doc.org/
[reStructuredText]: https://docutils.sourceforge.io/rst.html
