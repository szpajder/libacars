# ChangeLog

## Version 1.3.0 (2019-08-09):
* JSON output support for all message types. Any protocol tree can be
  serialized into a JSON string with `la_proto_tree_format_json()` function.
  Functions for serializing each individual message type into JSON are provided
  as well.  Refer to `doc/API_REFERENCE.md` for details.
* `decode_acars_apps` application now supports both human readable and JSON
  output. To enable JSON output, set `LA_JSON` environment variable to any
  value. If `LA_JSON` is not set, the program displays human readable output.
* Small bugfixes in ACARS and MIAM decoders.

## Version 1.2.0 (2019-02-26):
* Added support for decoding Media Independent Aircraft Messaging (MIAM - ACARS
  label MA) version 1 and 2. All types of MIAM frames are decoded, provided that
  they fit in a single ACARS message. In case of multi-fragment MIAM messages
  only the first fragment is decoded (partially) due to lack of message
  reassembly support in libacars. This will be addressed in a future release.
  MIAM uses DEFLATE compression, hence libacars now optionally depends on zlib.
  If zlib is not available at the build stage, MIAM decompression code will be
  disabled and many messages will be left undecoded.
* `decode_acars_apps` is a new example application. It decodes all ACARS
  applications supported by libacars. This makes `decode_arinc` application
  obsolete - it will be removed in the next release.
* Minor bugfixes in the build system.

## Version 1.1.0 (2019-01-18):
* Incompatible API change: so far `LA_VERSION` was a preprocessor macro,
  which was expanded during compilation of any program using it. As a result,
  its value contained the version of the C header, while the intention was
  to show the version of the currently running library. `LA_VERSION` has
  therefore been changed to a `char * const * const` variable. As a result,
  it is no longer possible to refer to it in preprocessor constructs
  (like `#ifdef`s or gluing of static strings together).
* Added decoder for Media Advisory messages (ACARS label SA) and an example
  program `media_advisory`. Contributed by Fabrice Crohas.
* Fixed decoding of ADS-C messages containing multiple contract requests.
* A few small fixes.

## Version 1.0.0 (2018-12-10):
* First stable release.
