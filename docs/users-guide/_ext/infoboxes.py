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
import docutils.parsers.rst

import sphinx.util.docutils

class didyouknownode(docutils.nodes.Admonition, docutils.nodes.Element):
  pass

class commonerrorsnode(docutils.nodes.Admonition, docutils.nodes.Element):
  pass

def visit_didyouknow_node(self, node):
  self.visit_admonition(node)

def visit_commonerrors_node(self, node):
  self.visit_admonition(node)

def depart_didyouknow_node(self, node):
  self.depart_admonition(node)

def depart_commonerrors_node(self, node):
  self.depart_admonition(node)

class didyouknowdirective(sphinx.util.docutils.SphinxDirective):
  has_content = True
  def run(self):
    admonitionnode = didyouknownode('\n'.join(self.content))
    admonitionnode += docutils.nodes.title('Did You Know?', 'Did You Know?')
    admonitionnode['classes'] += ['tip']
    self.state.nested_parse(self.content, self.content_offset, admonitionnode)

    return [admonitionnode]

class commonerrorsdirective(sphinx.util.docutils.SphinxDirective):
  has_content = True
  def run(self):
    admonitionnode = commonerrorsnode('\n'.join(self.content))
    admonitionnode += docutils.nodes.title('Common Errors', 'Common Errors')
    admonitionnode['classes'] += ['error']
    self.state.nested_parse(self.content, self.content_offset, admonitionnode)

    return [admonitionnode]

def setup(app):
  app.add_node(didyouknownode,
               html=(visit_didyouknow_node, depart_didyouknow_node),
               latex=(visit_didyouknow_node, depart_didyouknow_node),
               text=(visit_didyouknow_node, depart_didyouknow_node),
               )
  app.add_directive('didyouknow', didyouknowdirective)

  app.add_node(commonerrorsnode,
               html=(visit_commonerrors_node, depart_commonerrors_node),
               latex=(visit_commonerrors_node, depart_commonerrors_node),
               text=(visit_commonerrors_node, depart_commonerrors_node),
               )
  app.add_directive('commonerrors', commonerrorsdirective)
