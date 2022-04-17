# libacars [![Build Status](https://travis-ci.com/szpajder/libacars.svg?branch=master)](https://travis-ci.com/szpajder/libacars) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/szpajder/libacars.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/szpajder/libacars/context:cpp)

libacars is a library for decoding ACARS message contents.

Current stable version: **2.1.4** (released March 6, 2022)

## Supported message types

- [X] FANS-1/A ADS-C (Automatic Dependent Surveillance - Contract)
- [X] FANS-1/A CPDLC (Controller-Pilot Data Link Communications)
- [X] MIAM (Media Independent Aircraft Messaging)
- [X] Media Advisory (Status of data links: VDL2, HF, Satcom, VHF ACARS)

## Installation

### Binary packages

64-bit Windows binary packages of stable releases are provided in the
[Releases](https://github.com/szpajder/libacars/releases) section.

Unzip the archive into any directory. You should be able to run example programs
directly from `bin` subdirectory.

### Building from source

Requirements:

- c11-capable C compiler
- cmake 3.1 or later
- zlib 1.2 (optional)
- libxml2 (optional)

The project should build and run correctly on the following platforms:

- Linux i686 and x86_64 (gcc)
- MacOS (clang)
- Windows (mingw or msvc)

#### Installing dependencies

- libacars needs zlib for MIAM message decompression. If zlib is not present,
  decompression code will be disabled and many MIAM messages will be left
  undecoded. Therefore it is recommended to install zlib development package
  first. On Debian/Raspbian distros it is named `zlib1g-dev`:

```
apt-get install zlib1g-dev
```

- ACARS and MIAM CORE messages sometimes contain XML documents.  libacars may
  optionally pretty-print these documents, ie. add  proper formatting and
  indentation. To enable this feature, you need libxml2 library:

```
apt-get install libxml2-dev
```

#### Compiling libacars (Linux)

- **Option 1:** To run stable and tested code, download the latest stable
  release tarball from [Releases](https://github.com/szpajder/libacars/releases)
  section and unpack it:

```
unzip libacars-x.y.z.zip
cd libacars-x.y.z
```

- **Option 2:** To run the latest development code which has not yet made it
  into a stable release, clone the source repository and checkout the `unstable`
  branch:

```
git clone https://github.com/szpajder/libacars
cd libacars
git checkout unstable
```

`master` branch is always in sync with the latest stable release. `unstable`
branch is where the latest cutting-edge code goes first.

- Configure the build:

```
mkdir build
cd build
cmake ../
```

- Inspect the configuration summary in the cmake output and verify if all the
  dependencies have been detected properly:

```
-- libacars configuration summary:
-- - zlib:              requested: ON, enabled: TRUE
-- - libxml2:           requested: ON, enabled: TRUE
```

- Compile and install:

```
make
sudo make install
sudo ldconfig
```

The library will be installed to `/usr/local/lib` (or
`/usr/local/lib64`). Header files will land in
`/usr/local/include/libacars-2/libacars`.

#### Compiling libacars (Mac)

Install dependencies and tools with `brew`:

```
brew install cmake zlib libxml2
```

Then follow the above instructions for Linux. Just skip the final `sudo
ldconfig` step.

### Advanced compilation options

The following options may be used when invoking cmake:

- `-DCMAKE_BUILD_TYPE=Debug` - enables debugging support.

- `-DCMAKE_BUILD_TYPE=Release` - disables debugging support (the default).

- `-DEMIT_ASN_DEBUG=ON` - enables debugging output of ASN.1 decoders (very
  verbose). This option requires `-DCMAKE_BUILD_TYPE=Debug`, otherwise it will
  do nothing.

- `-DZLIB=FALSE` - forcefully disables zlib support. It will not be used even
  if zlib is available.

- `-DLIBXML2=FALSE` - disables libxml2 support.

## Example applications

Example apps are provided in `examples` subdirectory:

- `decode_acars_apps` - reads messages from command line or from a file and
  decodes all ACARS applications supported by the library.

- `adsc_get_position` - illustrates how to extract position-related
  fields from decoded ADS-C message.

- `cpdlc_get_position` - illustrates how to extract position-related
  fields from CPDLC position reports.

Apps will be compiled together with the library. `make install` installs them
to `/usr/local/bin`.  Run each program with `-h` option for usage instructions.

## API documentation

Refer to the following documents:

- `doc/PROG_GUIDE.md` - libacars Programmer's Guide
- `doc/API_REFERENCE.md` - libacars API Reference

## Applications using libacars

- [dumpvdl2](https://github.com/szpajder/dumpvdl2), a VDL-2 decoder
- [vdlm2dec](https://github.com/TLeconte/vdlm2dec), a VDL-2 decoder
- [acarsdec](https://github.com/TLeconte/acarsdec/), an ACARS decoder
- [JAERO](https://github.com/jontio/JAERO/), a Satcom ACARS decoder

## Credits and thanks

I hereby express my gratitude to everybody who helped with the development and
testing of libacars. Special thanks go to:

- Fabrice Crohas
- Dick van Noort
- acarslogger
- Michał Miszewski

## Licenses

libacars, Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Contains code from the following software projects:

- Rocksoft^tm Model CRC Algorithm Table Generation Program V1.0
  by Ross Williams

- asn1c, Copyright (c) 2003-2017 Lev Walkin <vlm@lionet.info>
  and contributors. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

Packaged releases for Windows include zlib library in binary DLL form.
zlib data compression library, (C) 1995-2017 Jean-loup Gailly and Mark Adler.

// vim: textwidth=80
