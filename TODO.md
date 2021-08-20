# TODO
Stuff that is still pending to be fixed or implemented.

* Source viewer
  * Navigate include files
* Settings file (libconfig)
  * Sort instances by name or by position in file
  * colors. Need to support white backgrounds I guess
  * Source search: whole file or active scope?
* Polishing
  * Hierarchy "more" expansion shouldn't show up if the "..more.." just hides 1 element.
  * Syntax highlighting of macros
* Nets should get selected out of the hierarchy panel (no verdi style get without design)
* Different layout for loading waves without source, different hierarchy panel probably.
* Different layout for loading source without waves.
* Auto-match wave hierarchy against design hierarchy.
* Source window: Set a marker on a source line (press m), press A to add all
  signals between current line and marker.
* Expose nets in functions and tasks
* Expose local variables in always blocks?
* tracing loads does not trace things on the left side, e.g idx isn't traced in `net[idx] = val;`
