##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##=============================================================================

import docutils.nodes

import sphinx_tools

def fullref_role(reftype, name, rawtext, text, lineno, inliner, options, content):
  return sphinx_tools.role_reparse(
      ':numref:`%s {number} ({name})<%s>`' % (reftype, text),
      lineno, inliner)

def partref_role(name, rawtext, text, lineno, inliner, options = {}, content = []):
  return fullref_role(
      'Part', name, rawtext, text, lineno, inliner, options, content)

def chapref_role(name, rawtext, text, lineno, inliner, options = {}, content = []):
  return fullref_role(
      'Chapter', name, rawtext, text, lineno, inliner, options, content)

def secref_role(name, rawtext, text, lineno, inliner, options = {}, content = []):
  return fullref_role(
      'Section', name, rawtext, text, lineno, inliner, options, content)

def setup(app):
  app.add_role('partref', partref_role)
  app.add_role('chapref', chapref_role)
  app.add_role('secref', secref_role)
