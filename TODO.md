# TODO
Stuff that is still pending to be fixed or implemented.

* Source viewer
  * Navigate include files
* Settings file (libconfig)
  * Sort instances by name or by position in file
  * colors. Need to support white backgrounds I guess
  * Source search: whole file or active scope?
* Polishing
  * Syntax highlighting of macros
* Source window: Set a marker on a source line (press m), press A to add all
  signals between current line and marker.
* Expose nets in functions and tasks
* Expose local variables in always blocks?
* tracing loads does not trace things on the left side, e.g idx isn't traced in `net[idx] = val;`
* Time multiplier for waves (e.g. wave 1 unit is really X us/ns/ps/)
* Wave + source should be able to figure out enums in waves.
* waveform color chooser.
* Reload waves, and possibly live updates.
* Search for value in wave.
* Analog signals.
* Detect arrays in waves, make them expandable in the waveforms
