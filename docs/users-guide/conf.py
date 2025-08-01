##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "The Viskores User's Guide"
copyright = 'Kitware Inc., National Technology & Engineering Solutions of Sandia LLC, UT-Battelle LLC, Los Alamos National Security LLC'
author = 'Kenneth Moreland'
version = '@Viskores_VERSION_FULL@'
release = '@Viskores_VERSION_MAJOR@.@Viskores_VERSION_MINOR@'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

# We provide some custom extensions in the _ext directory
import sys
sys.path.append('@CMAKE_CURRENT_SOURCE_DIR@/_ext')

extensions = [
  'sphinx.ext.autosectionlabel',
  'sphinx.ext.mathjax',
  'sphinx.ext.todo',

  # Extension available from https://breathe.readthedocs.io/en/stable/
  'breathe',

  # Extension available from https://sphinx-themes.org/sample-sites/sphinx-rtd-theme/
  # Can be installed with `pip install sphinx-rtd-theme`
  'sphinx_rtd_theme',

  # Extension available from https://github.com/scikit-build/moderncmakedomain
  # Can be installed with `pip install sphinxcontrib-moderncmakedomain`
  'sphinxcontrib.moderncmakedomain',

  # Extensions included in the _ext directory.
  'extract_examples',
  'fullref',
  'infoboxes',
]
# Note: there are some custom extensions at the bottom of this file.

todo_include_todos = @include_todos@

numfig = True
autosectionlabel_prefix_document = True

#templates_path = ['_templates']
exclude_patterns = ['CMakeFiles', '*.cmake', '.DS_Store']

primary_domain = 'cpp'
highlight_language = 'cpp'

numfig = True
numfig_format = {
  'figure': 'Figure %s',
  'table': 'Table %s',
  'code-block': 'Example %s',
  'section': '%s',
}

today_fmt = '%B %d, %Y'

rst_prolog = '''
.. |Viskores| replace:: Viskores
.. |Veclike| replace:: ``Vec``-like
.. |report-year| replace:: 2025
.. |report-number| replace:: ORNL/TM-2025/4019
'''

breathe_projects = { 'viskores': '@doxygen_xml_output_dir@' }
breathe_default_project = 'viskores'

example_directory = '@example_directory@'
example_command_comment = '////'
example_language = 'cpp'

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

#html_theme = 'alabaster'
#html_theme = 'sphinxdoc'
#html_theme = 'bizstyle'
#html_theme = 'classic'
html_theme = 'sphinx_rtd_theme'
#html_static_path = ['_static']


# -- Options for LaTeX output -------------------------------------------------

latex_toplevel_sectioning = 'part'
