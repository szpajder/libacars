# libacars

libacars is a library for decoding various ACARS message payloads.

### Supported message types

- [X] FANS-1/A ADS-C (Automatic Dependent Surveillance - Contract)
- [X] FANS-1/A CPDLC (Controller-Pilot Data Link Communications)

### Installation

You need a C compiler and cmake.

Clone the repository and execute:

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

TODO

### Licenses

libacars, Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


Contains code from the following software projects:


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

