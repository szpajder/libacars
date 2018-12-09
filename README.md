# libacars

libacars is a library for decoding various ACARS message payloads.

### Supported message types

- [X] FANS-1/A ADS-C (Automatic Dependent Surveillance - Contract)
- [X] FANS-1/A CPDLC (Controller-Pilot Data Link Communications)

### Installation

You need a C compiler and cmake.

Clone the repository:

	git clone https://github.com/szpajder/libacars

Compile and install:

	cd libacars
	mkdir build
	cd build
	cmake ../
	make
	sudo make install
	sudo ldconfig

By default the library will be installed to `/usr/local/lib`.
Header files will land in `/usr/local/include/libacars`.

### Example programs

A few example programs are provided in `src/examples` subdirectory:

- `decode_arinc.c` - decodes ARINC 622 messages supplied at the
  command line or from a file.

- `adsc_get_position` - illustrates how to extract position-related
  fields from decoded ADS-C message.

- `cpdlc_get_position` - illustrates how to extract position-related
  fields from CPDLC position reports.

These programs will be compiled together with the library.
`make install` won't install them - you can find binaries in
`build/src/examples` subdirectory. Run each program with `-h` option
for instructions.

### API documentation

Refer to the following documents:

- `doc/PROG_GUIDE.md` - libacars Programmer's Guide
- `doc/API_REFERENCE.md` - libacars API Reference

### Licenses

libacars, Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>

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

