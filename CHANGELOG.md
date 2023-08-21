# ChangeLog

## Version 2.2.0 (2023-08-21)

* Support for decoding OHMA messages. These are diagnostic data exchanged with
  Boeing 737MAX aircraft. Decoding requires libacars to be built with ZLIB
  support.  The data is encoded in JSON which libacars might optionally
  pretty-print, when serializing the message as text. Pretty-printing feature
  requires Jansson library.
* Support for compilation with MSVC (thx John Beniston).

## Version 2.1.4 (2022-03-06)

* Fix compile error under MS Visual C++ caused by `struct timeval`
  being defined in `winsock.h` rather than `sys/time.h` (thx Jon Beniston)
* Fix compile error under GCC >= 10 (multiple definitions of `asn_debug_indent`)
* Fix for cmake >=3.17 not finding zlib1.dll when building with MinGW.

## Version 2.1.3 (2021-03-05)

* Fixed incorrect calculation of reporting interval in ADS-C Periodic
  Contract Requests. Reported by Roman Tordia.

## Version 2.1.2 (2020-12-02)

* Fixed a build failure when compiling without libxml2 support.

## Version 2.1.1 (2020-11-18)

* Media Advisory: fixed a bug where a message with 10 or more available link
  indicators could overflow the static buffer and produce garbled result (thx
  mmiszewski).
* Media Advisory: fixed a bug where a message with an exceptionally long text
  field could cause buffer overrun and crash the program (thx mmiszewski).
* Media Advisory: replaced message parsing routine with a simpler and more
  robust one.

## Version 2.1.0 (2020-11-07)

* ADS-C: fixed incorrect computation of vertical speed threshold in contract
  request messages.
* JSON: fixed a bug where an ASN.1 string containing a \0 character caused
  truncation of JSON output. NULLs in strings are now replaced with \u0000
  sequence.
* JSON: ASN.1 SEQUENCEs are now printed as JSON objects rather than arrays of
  objects. It produces a simpler and more readable JSON structure. The change
  applies to all ASN.1 types formatted with
  `la_asn1_format_SEQUENCE_cpdlc_as_json` function. Refer to
  `asn1-format-cpdlc-json.c` file for a full list.
* JSON: added `json_append_octet_string_as_string` function which prints the
  contents of the given byte buffer as a JSON string rather than as a list of
  numeric values.
* JSON: added a bunch of new functions for formatting basic ASN.1 types. Now we
  have them all in libacars. Before that they were scattered between libacars
  and dumpvdl2.
* JSON: deprecate `la_json_append_long` function. A more portable version named
  `la_json_append_int64` shall be used instead.
* Small bugfixes, code cleanups.

## Version 2.0.1 (2020-08-24)

* Fixed a bug causing FANS1/A ErrorInformation message elements to be decoded
  incorrectly

## Version 2.0.0 (2020-01-16)

* New major release 2. API and ABI are not backwards compatible with version 1,
  hence the new soname - libacars-2.so.2. Header files are now installed to
  /usr/local/include/libacars-2. New pkg-config script libacars-2.pc provides
  updated compiler and linker flags. Version 2 can be installed in parallel with
  version 1.
* New feature: generic engine for reassembly of fragmented messages. Supports
  automatic tracking of fragments' sequence numbers (with possible wraparounds),
  handles reassembly timeouts and fragment deduplication. Messages can be keyed
  with arbitrary data (eg. custom header fields).
* New feature: automatic reassembly of ACARS messages. New function
  `la_acars_parse_and_reassemble` performs both parsing and reassembly, while
  existing `la_acars_parse` function performs parsing only and works as before.
* New feature: automatic reassembly of MIAM file transfers. New function
  `la_miam_parse_and_reassemble` performs both parsing and reassembly, while
  existing `la_miam_parse` function performs parsing only and works as before.
* New feature: pretty-printing of XML text in ACARS and MIAM CORE payloads.
  If the message text is identified as XML (ie. it parses without errors),
  `la_acars_format_text` and `la_miam_core_format_text` routines will print
  it with proper indentation for better readability. The feature is optional,
  depends on libxml2 and must be turned on by setting `prettify_xml`
  configuration variable to `true`.
* New feature: simple hash table implementation. Mostly used for libacars internal
  purposes, but it's exposed in the API and can be used for any purpose.
* New feature: when compiled with debugging support, debug output to stderr is
  now configurable with `LA_DEBUG` environment variable. Set it to the desired
  verbosity level - from 0 (none) to 3 (verbose). The default is 0 (debug output
  suppressed).
* Incompatible change: new library configuration API. `la_config` structure
  with static fields has been removed. Configuration variables are now
  stored in a hash table, read with `la_config_get_*` and set with
  `la_config_set_*` functions. Refer to `doc/API_REFERENCE.md` for details.
  Refer to `libacars/config_defaults.h` for the most current list of
  configuration options and their default values.
* Incompatible change: ACARS parser now strips sublabel and MFI (Message
  Function Identifier) fields from the message text, if present. Their values are
  stored in `la_acars_msg` structure in `sublabel` and `mfi` fields. In text
  and JSON output they are printed as separate fields.
* Incompatible change: MIAM parser and ARINC-622 ATS message parser now expect
  sublabel and MFI fields to be stripped by the caller, otherwise the parser
  will ignore the message. This operation can be performed with minimal fuss
  using `la_acars_extract_sublabel_and_mfi` function.
* Incompatible change: `la_miam_parse` function prototype has changed. All
  parameters except `txt` (message text) have been removed.
* Incompatible change: `no` field of `la_acars_msg` structure has been removed.
  MSN is now stored in two fields: `msg_num` (first three characters)
  and `msg_num_seq` (last character, ie. sequence indicator).
* Incompatible change: Media Advisory message timestamp and version are now
  stored as numbers, not characters. Type of `hour`, `minute`, `second` and
  `version` fields in `la_media_adv_msg` structure has been changed from `char[]`
  to `uint8_t`. Relevant JSON keys have changed types as well.
* Incompatible change: `state` and `current_link` fields in `la_media_adv_msg`
  structure changed from `char[2]` to `char`, hence their values are no longer
  NULL-terminated.
* examples: media_advisory app has been removed. Use decode_acars_apps instead.
* Bugfixes, code cleanups.

## Version 1.3.1 (2019-09-20)

* Binary releases for Windows were mistakenly built with AVX instruction set
  enabled which caused the lib to fail on older CPUs (https://github.com/szpajder/libacars/issues/3).
  Thanks to G7GQW for reporting and to Jonti Olds for analysis.
* Downgraded optimization level for release builds from -O3 to -O2 to reduce
  chances of similar problems for cross-platform builds.

## Version 1.3.0 (2019-08-09)

* JSON output support for all message types. Any protocol tree can be
  serialized into a JSON string with `la_proto_tree_format_json()` function.
  Functions for serializing each individual message type into JSON are provided
  as well.  Refer to `doc/API_REFERENCE.md` for details.
* `decode_acars_apps` application now supports both human readable and JSON
  output. To enable JSON output, set `LA_JSON` environment variable to any
  value. If `LA_JSON` is not set, the program displays human readable output.
* Small bugfixes in ACARS and MIAM decoders.

## Version 1.2.0 (2019-02-26)

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

## Version 1.1.0 (2019-01-18)

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

## Version 1.0.0 (2018-12-10)

* First stable release.
