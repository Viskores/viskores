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

def role_reparse(rawtext, lineno, inliner):
  '''Reparses inline code for a role.

  It is often the case that a role for reStructuredText can be defined using
  other existing functionality. Rather than try to implement the node building
  for such extensions from scratch, it is much easier to substitute the text
  with something and instruct the parser to parse that.

  Unfortunately, Sphinx currently does not provide the ability to do such a
  "reparse" while implementing a role. This method is a hacked version of the
  `parse` method of `docutils.parsers.rst.states.Inliner`. (This code might
  break if the internals of that class change.)

  To use this method, create a role implementation function as normal, modify
  the text/rawText to the form that implements the functionality, and call
  this method with the new text as well as the passed in line number and
  inliner object.

  Note that if the parsing of the text breaks, the error messages may be
  confusing because they could refer to items that are not being directly
  used in the reStructuredText document.
  '''
  remaining = rawtext
  processed = []
  unprocessed = []
  messages = []
  while remaining:
    match = inliner.patterns.initial.search(remaining)
    if match:
      groups = match.groupdict()
      method = inliner.dispatch[groups['start'] or groups['backquote']
                                or groups['refend'] or groups['fnend']]
      before, inlines, remaining, sysmessages = method(inliner, match, lineno)
      unprocessed.append(before)
      messages += sysmessages
      if inlines:
        processed += inliner.implicit_inline(''.join(unprocessed), lineno)
        processed += inlines
        unprocessed = []
      else:
        break
    remaining = ''.join(unprocessed) + remaining
    if remaining:
      processed += inliner.implicit_inline(remaining, lineno)
    return processed, messages

import docutils.nodes
import docutils.statemachine
import sphinx.util.nodes

class ReparseNodes:
  '''This class is used within directive classes to implement a directive by
  creating new reStructuredText code. This new code is fed back to the parser
  and the resulting nodes can be returned as the implementation of the
  directive.

  To use this class, construct an object. Then use the ``add_line`` method to
  add lines one at a time. When finished, use the ``get_nodes`` function to
  get the nodes that should be returned from the directives ``run`` method.
  Here is a simple example::

    import sphinx.util.docutils

    class Foo(sphinx.util.docutils.SphinxDirective):
      def run(self):
        reparse = ReparseNodes()
        reparse.add_line('.. note::', 'fakefile.rst', 1)
        reparse.add_line('   This box added in directive.', 'fakefile.rst', 2)
        reparse.add_line('', 'fakefile.rst', 3)
        reparse.add_line('This text also added in directive.', 'fakefile.rst', 3)
        return reparse.get_nodes(self)

    def setup(app):
      app.add_directive('foo', Foo)

  Note that if you want the reparse to base its file location on the location
  of your directive (which is a good idea), you can get the name of the file
  containing the directive call with ``self.state.document.current_source``
  and get the line to directive starts at with ``self.lineno``.

  The implementation of this class suggested from
  https://stackoverflow.com/questions/34350844/how-to-add-rst-format-in-nodes-for-directive
  '''
  def __init__(self):
    self.source = docutils.statemachine.ViewList()

  def add_line(self, sourceline, filename, linenumber):
    self.source.append(sourceline, filename, linenumber)

  def get_nodes(self, directive):
    node = docutils.nodes.section()
    node.document = directive.state.document

    sphinx.util.nodes.nested_parse_with_titles(directive.state,
                                               self.source,
                                               node)
    return node.children
