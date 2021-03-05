# libacars API Reference

API version: 2.0

Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>

## Basic data types

### la_msg_dir

Indicates the direction in which the message has been sent, ie. whether it's an
uplink or a downlink message. Decoders of bit-oriented applications need this
information to decode the message correctly and unambiguously.


```C
#include <libacars/libacars.h>

typedef enum {
        LA_MSG_DIR_UNKNOWN,
        LA_MSG_DIR_GND2AIR,
        LA_MSG_DIR_AIR2GND
} la_msg_dir;
```

### la_type_descriptor

A structure describing a particular message data type. It provides methods
(pointers to functions) which can perform various tasks on data of this type.

```C
#include <libacars/libacars.h>

typedef void (la_format_text_func)(la_vstring *vstr, void const *data, int indent);
typedef void (la_format_json_func)(la_vstring *vstr, void const *data);
typedef void (la_destroy_type_f)(void *data);

typedef struct {
        la_print_type_f *format_text;
        la_destroy_type_f *destroy;
        la_json_type_f *format_json;
        char *json_key;
// ... (placeholder fields for future use)
} la_type_descriptor;
```

- `la_print_type_f *format_text` - a pointer to a function which serializes the
  message of this type into a human-readable text.
- `la_destroy_type_f *destroy` - a pointer to a function which deallocates the
  memory used by a variable of this type. If the variable is of a simple type
  (eg. a scalar variable or a flat struct), then it can be freed by a simple
  call to `free()`. In this case `destroy` pointer is NULL.
- `la_json_type_f *format_json` - a pointer to a function which serializes the
  message of this type into a JSON string.
- `char *json_key` - JSON key name for this type.

It is not advised to invoke methods from `la_type_descriptor` directly.
`la_proto_tree_format_text()`, `la_proto_tree_format_json()` and
`la_proto_tree_destroy()` wrapper functions shall be used instead.

### la_proto_node

A structure representing a single protocol node.

```C
#include <libacars/libacars.h>

typedef struct la_proto_node la_proto_node;

struct la_proto_node {
        la_type_descriptor const *td;
        void *data;
        la_proto_node *next;
// ... (placeholder fields for future use)
};
```

- `la_type_descriptor const *td` - a pointer to a type descriptor variable
  which describes the protocol carried in this protocol node. Basically, it
  tells what `void *data` points to.
- `void *data` - an opaque pointer to a protocol-specific structure containing
  decoded message data.
- `la_proto_node *next` - a pointer to the next protocol node in this protocol
  tree (or NULL if there are no more nested protocol nodes present).

## Core protocol-agnostic API

### la_proto_node_new()

```C
#include <libacars/libacars.h>

la_proto_node *la_proto_node_new();
```

Allocates a new `la_proto_node` structure.

This function is rarely needed in user programs because protocol decoders
themselves allocate memory for returned protocol node.

### la_proto_tree_format_text()

```C
#include <libacars/libacars.h>

la_vstring *la_proto_tree_format_text(la_vstring *vstr, la_proto_node const *root);
```

Walks the whole protocol tree pointed to by `root`, serializing each node into
human-readable text and appending the result to the variable-length string
pointed to by `vstr`. If `vstr` is NULL, the function stores the result in a
newly allocated variable-length string (which should be later freed by the
caller using `la_proto_tree_destroy()`.

### la_proto_tree_format_json()

```C
#include <libacars/libacars.h>

la_vstring *la_proto_tree_format_json(la_vstring *vstr, la_proto_node const *root);
```

Walks the whole protocol tree pointed to by `root`, serializing each node into a
JSON string and appending the result to the variable-length string pointed to by
`vstr`. If `vstr` is NULL, the function stores the result in a newly allocated
variable-length string (which should be later freed by the caller using
`la_proto_tree_destroy()`.

### la_proto_tree_destroy()

```C
#include <libacars/libacars.h>

void la_proto_tree_destroy(la_proto_node *root);
```

Walks the whole protocol tree pointed to by `root`, freeing memory allocated
for each node. If `root` is NULL, the function does nothing.

### la_proto_tree_find_protocol()

```C
#include <libacars/libacars.h>

la_proto_node *la_proto_tree_find_protocol(la_proto_node *root, la_type_descriptor const *td);
```

Walks the whole protocol tree pointed to by `root`, and searches for a node of a
type described by the type descriptor `*td`, Returns a pointer to the first
found protocol node which matches the given type.  Returns NULL if no matching
node has been found.

## ACARS API

### la_acars_msg

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

typedef struct {
	bool crc_ok;
	bool err;
	bool final_block;
	char mode;
	char reg[8];
	char ack;
	char label[3];
	char sublabel[3];
	char mfi[3];
	char block_id;
	char msg_num[4];
	char msg_num_seq;
	char flight_id[7];
	la_reasm_status reasm_status;
	char *txt;
// ... (placeholder fields for future use)
} la_acars_msg;
```

A structure representing a decoded ACARS message.

- `crc_ok` - `true` if ACARS CRC verification succeeded, `false` otherwise.
- `err` - `true` if the decoder failed to decode the message. Values of other
  fields are left uninitialized.
- `final_block` - `true` if the message text is terminated with ETX character
  (meaning it's the final block of a multiblock ACARS message), `false` if
  it has been terminated with ETB character (meaning there are more blocks
  left in this message)
- `mode` - mode character
- `reg` - aircraft registration number (NULL-terminated)
- `ack` - acknowledgement character
- `label` - message label (NULL-terminated)
- `sublabel` - message sublabel (NULL-terminated, empty string, if absent)
- `mfi` - message function identifier (NULL-terminated, empty string, if absent)
- `block_id` - block ID character
- `msg_num` - first three characters of downlink message number (NULL-terminated)
- `msg_num_seq` - fourth character of downlink message number (sequence number
  indicator)
- `flight_id` - flight number (NULL-terminated)
- `txt` - message text (NULL-terminated)
- `reasm_status` - reassembly status, returned by the reassembly engine after
  it has processed this message

### la_acars_parse_and_reassemble()

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

la_proto_node *la_acars_parse_and_reassemble(uint8_t const *buf, int len,
		la_msg_dir msg_dir, la_reasm_ctx *rtables, struct timeval rx_time);

```

Takes a buffer of `len` bytes pointed to by `buf` and attempts to decode it as
an ACARS message. If there are any supported nested protocols in the ACARS
message text, they will be decoded as well.

The buffer shall start with the first byte **after** the initial SOH byte
(`'\x01'`) and should end with DEL byte (`'\x7f'`). `msg_dir` shall indicate the
direction of the transmission.  If it's set to `LA_MSG_DIR_UNKNOWN`, the
function will determine it using block ID field in the ACARS header.

The function returns a pointer to a newly allocated `la_proto_node` structure
which is the root of the decoded protocol tree. The `data` pointer of the top
`la_proto_node` will point to a `la_acars_msg` structure. The function performs
ACARS CRC verification and stores the result in the `crc_ok` field. If the
message could not be decoded, the `err` flag will be set to true.

If `rtables` points to a valid reassembly engine context, the function also
performs message reassembly in case the message is fragmented. In this case,
`rx_time` must be set to the time when the message has been received (this is
required for proper handling of reassembly timeouts). If `reasm_ctx` is NULL,
then no reassembly is done.

### la_acars_parse()

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

la_proto_node *la_acars_parse(uint8_t const *buf, int len, la_msg_dir msg_dir);
```

This function is provided for backward compatbility with libacars version 1. It
is equivalent to `la_acars_parse_and_reassemble()` with a NULL `reasm_ctx`,
ie. it parses the given buffer as an ACARS message, without reassembly.

### la_acars_extract_sublabel_and_mfi()

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

int la_acars_extract_sublabel_and_mfi(char const *label, la_msg_dir msg_dir,
		char const *txt, int len, char *sublabel, char *mfi);
```

Extracts sublabel and MFI (Message Function Identifier) fields from the given
ACARS message text `txt` and stores (copies) their values into buffers pointed
to by `sublabel` and `mfi`. Returns the offset (number of bytes) from the
beginning of the message text right after these fields. Returns 0 if sublabel
and MFI are not present. If supplied parameters are incorrect, the function
returns -1.

- `label` - ACARS label of the message in question. The function searches for
  label and MFI only if the label is set to `H1`.

- `msg_dir` - message direction of the message in question. Must be set either
  to `LA_MSG_DIR_GND2AIR` or `LA_MSG_DIR_AIR2GND`.

- `txt` - ACARS message text.

- `len` - length of the ACARS message text. This allows `txt` not to be
  NULL-terminated.

- `sublabel` - must be either set to NULL or point to a preallocated character
  buffer of at least 3 characters in length. If sublabel field is present, its
  value is copied into this buffer. If `sublabel` is NULL, the field is just
  skipped, without copying.

- `mfi` - must be either set to NULL or point to a preallocated character buffer
  of at least 3 characters in length. If MFI field is present, its value is
  copied into this buffer. If `mfi` is NULL, the field is skipped, without
  copying.

**Example 1**: the following call:

```C
char sublabel[3], mfi[3];
char *txt = "#T2BT-3![[mS0L8ZeIK0?J|EDDF";
int ret = la_acars_extract_sublabel_and_mfi("H1", LA_MSG_DIR_AIR2GND,
	txt, strlen(txt), sublabel, mfi);
```

sets `sublabel` to "T2", sets `mfi` to an empty string (because MFI is not
present), and returns 4, which is the position of the first character after the
sublabel field, ie 'T' (because the sublabel field is "#T2B" in this case).

**Example 2**: the following call:

```C
char sublabel[3], mfi[3];
char *txt = "- #MD/AA ATLTWXA.CR1.N856DN203A3AA8E5C1A9323EDD";
int ret = la_acars_extract_sublabel_and_mfi("H1", LA_MSG_DIR_GND2AIR,
	txt, strlen(txt), sublabel, mfi);
```

sets `sublabel` to "MD", sets `mfi` to "AA" and returns 9, which is the position
of the initial character 'A' in "ATLTWXA".

The offset returned by `la_acars_extract_sublabel_and_mfi()` might be used to
skip sublabel and MFI and make the message text acceptable for processing by
functions expecting the sublabel and MFI to be stripped. Refer to an example
program in the [Programmer's Guide](PROGRAMMING_GUIDE.md).

### la_acars_format_text()

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

void la_acars_format_text(la_vstring *vstr, void const *data, int indent);
```

Serializes a decoded ACARS message pointed to by `data` into a human-readable
text indented by `indent` spaces and appends the result to `vstr` (which must be
non-NULL).

If libacars has been built with libxml2 support and `prettify_xml` configuration
variable is set to `true`, then the function attempts to parse the message text
as an XML document. If parsing succeeds (meaning the message indeed contains
XML), the text is pretty-printed (reformatted into multi-line output with proper
indentation).

Use this function if you want to serialize only the ACARS protocol node and not
its child nodes. In most cases `la_proto_tree_format_text()` should be used
instead.

### la_acars_format_json()

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

void la_acars_format_json(la_vstring *vstr, void const *data);
```

Serializes a decoded ACARS message pointed to by `data` into a JSON string and
appends the result to `vstr` (which must be non-NULL).

Use this function if you want to serialize only the ACARS protocol node and not
its child nodes. In most cases `la_proto_tree_format_json()` should be used
instead.

### la_acars_apps_parse_and_reassemble()

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

la_proto_node *la_acars_apps_parse_and_reassemble(char const *reg,
		char const *label, char const *txt, la_msg_dir msg_dir,
		la_reasm_ctx *rtables, struct timeval rx_time);
```

Tries to determine the ACARS application using message label stored in `label`.
If the label corresponds to any supported applications, respective application
decoders are executed in sequence to decode the message text `txt`. `msg_dir`
must be set to a correct transmission direction for the decoding to succeed.

The function returns a pointer to a newly allocated `la_proto_node` structure
which is the root of the decoded protocol tree. If none of the application
decoders recognized the message, the function returns NULL.

If `rtables` points to a valid reassembly engine context, it is passed to the
decoder of each application which may use message fragmentation at the
application layer (currently the only such application is MIAM). In this case,
`reg` must point to the registration of the aircraft which transmitted or
received the message (this is used as a hash key) and `rx_time` must be set to
the time when the message has been received (required for proper handling of
reassembly timeouts). If `reasm_ctx` is NULL, then no reassembly is done.

### la_acars_decode_apps()

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

la_proto_node *la_acars_decode_apps(char const *label,
		char const *txt, la_msg_dir msg_dir);
```

This function is provided for backward compatbility with libacars version 1. It
is equivalent to `la_acars_apps_parse_and_reassemble()` with a NULL
`reasm_ctx`, ie. it decodes ACARS application message `txt` without reassembly.


### la_proto_tree_find_acars()

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

la_proto_node *la_proto_tree_find_acars(la_proto_node *root);
```

Walks the protocol tree pointed to by `root` and returns a pointer to the first
encountered node containing an ACARS message (ie. having a type descriptor of
`la_DEF_acars_message`). If `root` is NULL or no matching protocol node has been
found in the tree, the function returns NULL.

## ARINC-622 API

ARINC-622 describes a generic format for carrying Air Traffic Services (ATS)
applications in ACARS message text. The API is defined in `<libacars/arinc.h>`.
It might be used in a program which already performs basic ACARS decoding.

### la_arinc_msg

```C
#include <libacars/libacars.h>
#include <libacars/arinc.h>

typedef struct {
        char gs_addr[8];
        char air_reg[8];
        la_arinc_imi imi;
        bool crc_ok;
// ... (placeholder fields for future use)
} la_arinc_msg;
```

A structure representing a decoded ARINC-622 message.

- `gs_addr` - ground facility address (NULL-terminated)
- `air_reg` - aircraft address (NULL-terminated)
- `imi` - an enumerated value mapped from the Imbedded Message Identifier (IMI);
  refer to `<libacars/arinc.h>` for a definition of `la_arinc_imi` type
- `crc_ok` - `true` if ARINC-622 CRC verification succeeded, `false` otherwise.

### la_arinc_parse()

```C
#include <libacars/libacars.h>
#include <libacars/arinc.h>

la_proto_node *la_arinc_parse(char const *txt, la_msg_dir msg_dir);
```

Attempts to parse the message text `txt` sent in direction `msg_dir` as an
ARINC-622 message. Performs the following tasks:

- splits the message text into a set of fields:

  - ground facility address
  - Imbedded Message Identifier (IMI)
  - aircraft address (registration number)

- if the IMI indicates a supported bit-oriented application:

  - verifies the CRC of the message
  - converts the hex string embedded in the message text into an octet string
  - parses the octet string with an application-specific parser

The function returns a pointer to a newly allocated `la_proto_node` structure
which is the root of the decoded protocol tree. The `data` pointer of the top
`la_proto_node` will point to a `la_arinc_msg` structure. The result of ARINC
CRC verification is stored in the `crc_ok` boolean field. If the message text
does not represent an ARINC-622 application message or if the IMI field value is
unknown, the function returns NULL.

libacars currently supports the following IMIs: CR1, CC1, DR1, AT1, ADS, DIS.

### la_arinc_format_text()

```C
#include <libacars/libacars.h>
#include <libacars/arinc.h>

void la_arinc_format_text(la_vstring *vstr, void const *data, int indent);
```

Serializes a decoded ARINC-622 message pointed to by `data` into a human-readable
text indented by `indent` spaces and appends the result to `vstr` (which must be
non-NULL).

Use this function if you want to serialize only the ARINC protocol node and not
its child nodes. In most cases `la_proto_tree_format_text()` should be used
instead.

### la_arinc_format_json()

```C
#include <libacars/libacars.h>
#include <libacars/arinc.h>

void la_arinc_format_json(la_vstring *vstr, void const *data);
```

Serializes a decoded ARINC-622 message pointed to by `data` into a JSON string
and appends the result to `vstr` (which must be non-NULL).

Use this function if you want to serialize only the ARINC protocol node and not
its child nodes. In most cases `la_proto_tree_format_json()` should be used
instead.

### la_proto_tree_find_arinc()

```C
#include <libacars/libacars.h>
#include <libacars/arinc.h>

la_proto_node *la_proto_tree_find_arinc(la_proto_node *root);
```

Walks the protocol tree pointed to by `root` and returns a pointer to the first
encountered node containing an ARINC-622 message (ie. having a type descriptor of
`la_DEF_arinc_message`). If `root` is NULL or no matching protocol node has been
found in the tree, the function returns NULL.

## ADS-C API

### la_adsc_msg_t

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>

typedef struct {
        bool err;
        la_list *tag_list;
// ... (placeholder fields for future use)
} la_adsc_msg_t;
```

A structure representing a decoded ADS-C message.

- `err` - `true` if the decoder failed to decode the message, `false` otherwise.
- `tag_list` - a pointer to a `la_list` of decoded ADS-C tags (structures of
  type `la_adsc_tag_t`).

### la_adsc_tag_t

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>

typedef struct {
        uint8_t tag;
        la_adsc_type_descriptor_t *type;
        void *data;
} la_adsc_tag_t;
```

A generic ADS-C tag structure.

- `tag` - tag ID
- `type` - a pointer to an ADS-C tag descriptor type which provides a textual
  label for the tag and basic methods for manipulating the tag. This is mostly
  for libacars internal use.
- `data` - an opaque pointer to a structure representing a decoded tag. Refer to
  `<libacars/adsc.h>` for definitions of tag structures (`la_adsc_basic_report_t`,
  `la_adsc_flight_id_t` and so forth).

### la_adsc_parse()

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>

la_proto_node *la_adsc_parse(uint8_t const *buf, int len, la_msg_dir msg_dir,
		la_arinc_imi imi);
```

Parses the octet string pointed to by `buf` having a length of `len` octets as
an ADS-C message sent in the direction indicated by `msg_dir`. The value of
`imi` indicates the value of the IMI field in the ARINC-622 message and must
have a value of either `ARINC_MSG_ADS` or `ARINC_MSG_DIS`.

The function returns a pointer to a newly allocated `la_proto_node` structure
which is the root of the decoded protocol tree. The `data` pointer of the top
`la_proto_node` will point to a `la_adsc_msg_t` structure.  If the message could
not be decoded, the `err` flag will be set to true.

### la_adsc_format_text()

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>

void la_adsc_format_text(la_vstring *vstr, void const *data, int indent);
```

Serializes a decoded ADS-C message pointed to by `data` into a human-readable
text indented by `indent` spaces and appends the result to `vstr` (which must be
non-NULL). As the ADS-C protocol node is always the leaf (terminating) node in
the protocol tree, this function will have the same effect as
`la_proto_tree_format_text()` which should be used instead in most cases.

### la_adsc_format_json()

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>

void la_adsc_format_json(la_vstring *vstr, void const *data);
```

Serializes a decoded ADS-C message pointed to by `data` into a JSON string
and appends the result to `vstr` (which must be non-NULL). As the ADS-C protocol
node is always the leaf (terminating) node in the protocol tree, this function
will have the same effect as `la_proto_tree_format_text()` which should be used
instead in most cases.

### la_adsc_destroy()

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>

void la_adsc_destroy(void *data);
```

Deallocates memory used by the protocol node `data` and all the child
structures. `data` must be a pointer to a `la_adsc_msg_t` structure. Rather than
this function, you should use `la_proto_tree_destroy()` instead.

### la_proto_tree_find_adsc()

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>

la_proto_node *la_proto_tree_find_adsc(la_proto_node *root);
```

Walks the protocol tree pointed to by `root` and returns a pointer to the first
encountered node containing an ADS-C message (ie. having a type descriptor of
`la_DEF_adsc_message`). If `root` is NULL or no matching protocol node has been
found in the tree, the function returns NULL.

## CPDLC API

Basic CPDLC API is defined in `<libacars/cpdlc.h>`. This is enough to perform
typical operations, like decoding and serializing decoded CPDLC messages into
human-readable text. Advanced API is also available in `<libacars/asn1/*.h>`. It
is used when there is a need to access individual fields of decoded CPDLC
messages.

### la_cpdlc_msg

```C
#include <libacars/libacars.h>
#include <libacars/cpdlc.h>

typedef struct {
        asn_TYPE_descriptor_t *asn_type;
        void *data;
        bool err;
// ... (placeholder fields for future use)
} la_cpdlc_msg;
```

A top-level structure representing a decoded CPDLC message.

- `asn_type` - a descriptor of a top-level ASN.1 data type contained in `data`.
- `data` - an opaque pointer to a decoded ASN.1 structure of the message
- `err` - `true` if the decoder failed to decode the message, `false` otherwise.

### la_cpdlc_parse()

```C
#include <libacars/libacars.h>
#include <libacars/cpdlc.h>

la_proto_node *la_cpdlc_parse(uint8_t const *buf, int len, la_msg_dir msg_dir);
```

Parses the octet string pointed to by `buf` having a length of `len` octets as
a FANS-1/A CPDLC message sent in the direction indicated by `msg_dir`.

The function returns a pointer to a newly allocated `la_proto_node` structure
which is the root of the decoded protocol tree. The `data` pointer of the top
`la_proto_node` will point to a `la_cpdlc_msg` structure.  If the message could
not be decoded, the `err` flag will be set to true.

It is imperative to specify correct value for `msg_dir`. This is because CPDLC
messages are encoded using ASN.1 Packed Encoding Rules (PER). PER does not
encode ASN.1 tags, so the message type cannot be determined based on its
contents. Supplying a wrong message direction will (at best) cause the message
decoding to fail (`err` flag in `la_cpdlc_msg` structure will be set to `true`),
but it may also happen that the decoder will produce a perfectly valid result,
which is in fact different to what has been transmitted by the sender due to
encoding ambiguity (some encoded messages can be successfully decoded both as
CPDLC uplink and downlink message).

`la_cpdlc_parse()` sets the fields in the resulting `la_cpdlc_msg` structure as
follows:

- for air-to-ground messages:
  - `asn_type` is set to `&asn_DEF_FANSATCDownlinkMessage`
  - `data` pointer type is `FANSATCDownlinkMessage_t *`
- for ground-to-air messages:
  - `asn_type` is set to `&asn_DEF_FANSATCUplinkMessage`
  - `data` pointer type is `FANSATCUplinkMessage_t *`

These types are defined in the respective header files in `<libacars/asn1/...>`
include directory.

### la_cpdlc_format_text()

```C
#include <libacars/libacars.h>
#include <libacars/cpdlc.h>

void la_cpdlc_format_text(la_vstring *vstr, void const *data, int indent);
```

Serializes a decoded CPDLC message pointed to by `data` into a human-readable
text indented by `indent` spaces and appends the result to `vstr` (which must be
non-NULL). As the CPDLC protocol node is always the leaf (terminating) node in
the protocol tree, this function will have the same effect as
`la_proto_tree_format_text()` which should be used instead of this function in
most cases.

### la_cpdlc_format_json()

```C
#include <libacars/libacars.h>
#include <libacars/cpdlc.h>

void la_cpdlc_format_json(la_vstring *vstr, void const *data);
```

Serializes a decoded CPDLC message pointed to by `data` into a JSON string and
appends the result to `vstr` (which must be non-NULL). As the CPDLC protocol
node is always the leaf (terminating) node in the protocol tree, this function
will have the same effect as `la_proto_tree_format_json()` which should be used
instead of this function in most cases.

If `dump_asn1` config variable is set to `true`, a raw ASN.1 structure dump will
be prepended to the human-readable output.

### la_cpdlc_destroy()

```C
#include <libacars/libacars.h>
#include <libacars/cpdlc.h>

void la_cpdlc_destroy(void *data);
```

Deallocates memory used by the protocol node `data` and all the child
structures. `data` must be a pointer to a `la_cpdlc_msg` structure. Rather than
this function, you should use `la_proto_tree_destroy()` instead.

### la_proto_tree_find_cpdlc()

```C
#include <libacars/libacars.h>
#include <libacars/cpdlc.h>

la_proto_node *la_proto_tree_find_cpdlc(la_proto_node *root);
```

Walks the protocol tree pointed to by `root` and returns a pointer to the first
encountered node containing a CPDLC message (ie. having a type descriptor of
`la_DEF_cpdlc_message`). If `root` is NULL or no matching protocol node has been
found in the tree, the function returns NULL.

## Media Advisory API

### la_media_adv_msg

```C
#include <libacars/libacars.h>
#include <libacars/media-adv.h>

typedef struct {
	bool err;
	uint8_t version;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	char state;
	char current_link[2];
	char available_links[10];
	char text[255];
// ... (placeholder fields for future use)
} la_media_adv_msg;
```

- `err` - `true` if the decoder failed to decode the message, `false` otherwise.
- `version` - message version number
  Currently only version 0 is supported (which is probably the only version
  in existence).
- `hour`, `minute`, `second` - UTC time of the event
- `state` - describes the event which triggered this Media Advisory message.
  Possible values: 'E' (link established), 'L' (link lost).
- `current_link` - indicates the type of the link which state change has
  triggered this Media Advisory message.  Refer to the structure `link_type_map`
  in `src/libacars/media-adv.c` for a list of all types and their textual
  descriptions.
- `available_links` - a NULL-terminated string with concatenated letter codes
  of links which are currently available.

### la_media_adv_parse()

```C
#include <libacars/libacars.h>
#include <libacars/media-adv.h>

la_proto_node *la_media_adv_parse(char const *txt);
```

Parses the NULL-terminated string pointed to by `txt` as a Media Advisory
message.

The function returns a pointer to a newly allocated `la_proto_node` structure
which is the root of the decoded protocol tree. The `data` pointer of the top
`la_proto_node` will point to a `la_media_adv_msg` structure.  If the message
could not be decoded, the `err` flag will be set to true.

### la_media_adv_format_text()

```C
#include <libacars/libacars.h>
#include <libacars/media-adv.h>

void la_media_adv_format_text(la_vstring *vstr, void const *data, int indent);
```

Serializes a decoded Media Advisory message pointed to by `data` into a
human-readable text indented by `indent` spaces and appends the result to `vstr`
(which must be non-NULL). As the Media Advisory protocol node is always the leaf
(terminating) node in the protocol tree, this function will have the same effect
as `la_proto_tree_format_text()` which should be used instead in most cases.

### la_media_adv_format_json()

```C
#include <libacars/libacars.h>
#include <libacars/media-adv.h>

void la_media_adv_format_json(la_vstring *vstr, void const *data);
```

Serializes a decoded Media Advisory message pointed to by `data` into a JSON
string and appends the result to `vstr` (which must be non-NULL). As the Media
Advisory protocol node is always the leaf (terminating) node in the protocol
tree, this function will have the same effect as `la_proto_tree_format_json()`
which should be used instead in most cases.

### la_proto_tree_find_media_adv()

```C
#include <libacars/libacars.h>
#include <libacars/media-adv.h>

la_proto_node *la_proto_tree_find_media_adv(la_proto_node *root);
```

Walks the protocol tree pointed to by `root` and returns a pointer to the first
encountered node containing a Media Advisory message (ie. having a type
descriptor of `la_DEF_media_adv_message`). If `root` is NULL or no matching
protocol node has been found in the tree, the function returns NULL.

## MIAM

Media Independent Aircraft Messaging (MIAM) is a protocol that provides a
standardized interface for the exchange of data between aircraft and ground
systems. It supports the transfer of messages much larger than possible in the
ACARS subnetworks through message segmentation and reassembly.  To reduce the
impact of MIAM transfers on the ACARS networks, the protocol includes data
compression, segment temporization, and flow regulation controls. The standard
ACARS label for MIAM messages is MA, however some carriers use H1 for this
purpose.

The protocol uses two layers:

- The outer layer is a network convergence function, which provides transport of
  MIAM messages over a specific medium. libacars implements ACARS convergence
  function. The API for it is defined in `<libacars/miam.h>`.

- The inner layer is called MIAM CORE and it's the actual messaging service.
  libacars API for it is defined in `<libacars/miam-core.h>`.

Decoding a MIAM message from the top level will produce a `proto_tree` which is
up to four levels deep. For example, a MIAM File Segment message containing a
MIAM CORE version 1 Data PDU will produce a tree with the following node
descriptors and data types:

```
la_DEF_miam_message (la_miam_msg)
|
\-> la_DEF_miam_file_segment_message (la_miam_file_segment_msg)
    |
    \-> la_DEF_miam_core_pdu (la_miam_core_pdu)
        |
        \-> la_DEF_miam_core_v1_data_pdu (la_miam_core_v1_data_pdu)
```

## MIAM ACARS Convergence Function API

### la_miam_parse_and_reassemble()

```C
#include <libacars/libacars.h>
#include <libacars/miam.h>

la_proto_node *la_miam_parse_and_reassemble(char const *reg, char const *txt,
	la_reasm_ctx *rtables, struct timeval const rx_time);

```
Attempts to parse NULL-terminated string pointed to by `txt` as a MIAM ACARS CF
message. Optionally performs reassembly of the inner payload (this currently
applies to MIAM File Segments only).

- `reg` - registration of the aircraft which sent or received this message. Used
  as a hash key during reassembly process. If reassembly is not desired (ie.
  `rtables` argument is set to NULL), this parameter is not used and might be set
  to NULL.
- `txt` - ACARS message text (sublabel and MFI must be stripped beforehand)
- `rtables` - if reassembly is desired, this should point to a valid reassembly
  context. Setting it to NULL disables reassembly.
- `rx_time` - if reassembly is desired, this should indicate the reception
  timestamp of the message, which is required for proper handling of reassembly
  timeouts. If reassembly is disabled, this parameter is not used and might be
  set to any value, for example `{ 0, 0 }`.

The parser returns a pointer to a newly allocated `la_proto_node` structure
which is the root of the decoded protocol tree. The `data` pointer of the top
`la_proto_node` will point to a `la_miam_msg` structure. `td` will point to
`la_DEF_miam_message` type descriptor. If the message has not been identified as
MIAM, the parser returns NULL. This informs the caller that the message possibly
contains another ACARS application (not MIAM), hence the return value of NULL
should not be treated as fatal.

The MIAM ACARS CF API provides eight type descriptors:

```C
#include <libacars/libacars.h>
#include <libacars/miam.h>

extern la_type_descriptor const la_DEF_miam_message;
```

`la_DEF_miam_message` is the top-level type descriptor for any MIAM message. The
`data` pointer of `la_proto_node` of this type points to a `la_miam_msg`
structure. The only purpose of this structure is to identify the MIAM ACARS CF
frame type contained at the next level of the `proto_tree`.

```C
#include <libacars/libacars.h>
#include <libacars/miam.h>

typedef enum {
        LA_MIAM_FID_UNKNOWN = 0,
        LA_MIAM_FID_SINGLE_TRANSFER,
        LA_MIAM_FID_FILE_TRANSFER_REQ,
        LA_MIAM_FID_FILE_TRANSFER_ACCEPT,
        LA_MIAM_FID_FILE_SEGMENT,
        LA_MIAM_FID_FILE_TRANSFER_ABORT,
        LA_MIAM_FID_XOFF_IND,
        LA_MIAM_FID_XON_IND
} la_miam_frame_id;
#define LA_MIAM_FRAME_ID_CNT 8

// MIAM ACARS CF frame
typedef struct {
        la_miam_frame_id frame_id;
// ... (placeholder fields for future use)
} la_miam_msg;
```

- `frame_id` - identifies the MIAM ACARS CF frame type.

The next node of the tree may be of any of the following types:

```C
#include <libacars/libacars.h>
#include <libacars/miam.h>

extern la_type_descriptor const la_DEF_miam_single_transfer_message;
extern la_type_descriptor const la_DEF_miam_file_transfer_request_message;
extern la_type_descriptor const la_DEF_miam_file_transfer_accept_message;
extern la_type_descriptor const la_DEF_miam_file_segment_message;
extern la_type_descriptor const la_DEF_miam_file_transfer_abort_message;
extern la_type_descriptor const la_DEF_miam_xoff_ind_message;
extern la_type_descriptor const la_DEF_miam_xon_ind_message;
```

Each type descriptor describes a particular MIAM ACARS CF frame type - Single
Transfer, File Transfer Request, File Transfer Accept, File Segment, File
Transfer Abort, XOFF Indication and XON Indication, respectively. Relevant
structures for each frame type are shown below. They contain values of various
header fields of the ACARS CF frame.

```C
#include <libacars/libacars.h>
#include <libacars/miam.h>

// MIAM File Transfer Request
typedef struct {
	size_t file_size;
	uint16_t file_id;
	struct tm validity_time;
	la_reasm_status reasm_status;
// ... (placeholder fields for future use)
} la_miam_file_transfer_request_msg;

// MIAM File Transfer Accept
typedef struct {
	uint16_t file_id;
	uint16_t segment_size;
	uint16_t onground_segment_tempo;
	uint16_t inflight_segment_tempo;
// ... (placeholder fields for future use)
} la_miam_file_transfer_accept_msg;

// MIAM File Segment
typedef struct {
	uint16_t file_id;
	uint16_t segment_id;
// ... (placeholder fields for future use)
} la_miam_file_segment_msg;

// MIAM File Transfer Abort
typedef struct {
	uint16_t file_id;
	uint16_t reason;
// ... (placeholder fields for future use)
} la_miam_file_transfer_abort_msg;

// MIAM XOFF IND
typedef struct {
	uint16_t file_id;	// 0-127 or 0xFFF = pause all transfers
// ... (placeholder fields for future use)
} la_miam_xoff_ind_msg;

// MIAM XON IND
typedef struct {
	uint16_t file_id;	// 0-127 or 0xFFF = resume all transfers
	uint16_t onground_segment_tempo;
	uint16_t inflight_segment_tempo;
// ... (placeholder fields for future use)
} la_miam_xon_ind_msg;
```

**Note:** You might notice there is no `la_miam_single_transfer_msg` type
present. This is because MIAM Single Transfer message contains only MIAM CORE
PDU, hence the data type for Single Transfer message is set to
`la_miam_core_pdu` (refer to the MIAM CORE API section).

### la_miam_parse()

```C
#include <libacars/libacars.h>
#include <libacars/miam.h>

la_proto_node *la_miam_parse(char const *txt);

```

This function is provided for backward compatbility with libacars version 1. It
is equivalent to `la_miam_parse_and_reassemble()` with a NULL `reasm_ctx`,
ie. it decodes MIAM message in the given buffer without reassembly.

### la_miam_format_text()

```C
#include <libacars/libacars.h>
#include <libacars/miam.h>

void la_miam_format_text(la_vstring *vstr, void const *data, int indent);
```

Serializes a decoded MIAM ACARS CF frame pointed to by `data` into a
human-readable text indented by `indent` spaces and appends the result to `vstr`
(which must be non-NULL).

### la_miam_format_json()

```C
#include <libacars/libacars.h>
#include <libacars/miam.h>

void la_miam_format_json(la_vstring *vstr, void const *data);
```

Serializes a decoded MIAM ACARS CF frame pointed to by `data` into a JSON string
and appends the result to `vstr` (which must be non-NULL).

### la_proto_tree_find_miam()

```C
#include <libacars/libacars.h>
#include <libacars/miam.h>

la_proto_node *la_proto_tree_find_miam(la_proto_node *root);
```

Walks the protocol tree pointed to by `root` and returns a pointer to the first
encountered node containing MIAM ACARS CF frame  (ie. having a type descriptor
of `la_DEF_miam_message`). If `root` is NULL or no matching protocol node has
been found in the tree, the function returns NULL.

No `la_proto_tree_find_*()` routines are provided for individual CF frame types.
If you need to locate a node containing a particular frame type, use
`la_proto_tree_find_protocol()` function with a pointer to the type descriptor
of interest, for example:

```C
#include <libacars/libacars.h>
#include <libacars/miam.h>

la_proto_node *file_seg_node = la_proto_tree_find_protocol(tree_ptr, &la_DEF_miam_file_segment_message);
```

## MIAM CORE API

### la_miam_core_pdu_parse()

```C
#include <libacars/libacars.h>
#include <libacars/miam-core.h>

la_proto_node *la_miam_core_pdu_parse(char const *txt);
```

Attempts to parse the NULL-terminated string `txt` as a MIAM CORE protocol data
unit. Returns a pointer to a newly allocated `la_proto_node` structure which is
the root of the decoded protocol tree. The `data` pointer of the top
`la_proto_node` will point to a `la_miam_core_pdu` structure. `td` will point to
`la_DEF_miam_core_pdu` type descriptor. If the message has not been identified
as MIAM CORE PDU, the parser returns NULL. If this routine has been called from
`la_miam_parse()`, then the NULL return value will propagate upwards to the
caller to indicate that the supplied message possibly contains a different ACARS
application (not MIAM).

The MIAM CORE API provides the following type descriptors:


```C
#include <libacars/libacars.h>
#include <libacars/miam-core.h>

extern la_type_descriptor const la_DEF_miam_core_pdu;
```

`la_DEF_miam_core_pdu` is the top-level type descriptor for any MIAM CORE PDU.
The `data` pointer of `la_proto_node` of this type points to a
`la_miam_core_pdu` structure:

```C
typedef enum {
	LA_MIAM_CORE_PDU_DATA = 0,
	LA_MIAM_CORE_PDU_ACK = 1,
	LA_MIAM_CORE_PDU_ALO = 2,
	LA_MIAM_CORE_PDU_ALR = 3,
	LA_MIAM_CORE_PDU_UNKNOWN = 4
} la_miam_core_pdu_type;
#define LA_MIAM_CORE_PDU_TYPE_MAX 4

typedef struct {
	uint32_t err;			// PDU decoding error code
	uint8_t version;		// MIAM CORE PDU version
	la_miam_core_pdu_type pdu_type;	// MIAM CORE PDU type
// ... (placeholder fields for future use)
} la_miam_core_pdu;
```

- `err` - a bit field with information about parsing errors (see "Error
  handling" section below)
- `version` - MIAM CORE protocol version (1 or 2)
- `pdu_type` - MIAM CORE PDU type

The next node of the tree may be of any of the following types:

```C
#include <libacars/libacars.h>
#include <libacars/miam-core.h>

extern la_type_descriptor const la_DEF_miam_core_v1v2_alo_pdu;
extern la_type_descriptor const la_DEF_miam_core_v1v2_alr_pdu;
extern la_type_descriptor const la_DEF_miam_core_v1_data_pdu;
extern la_type_descriptor const la_DEF_miam_core_v1_ack_pdu;
extern la_type_descriptor const la_DEF_miam_core_v2_data_pdu;
extern la_type_descriptor const la_DEF_miam_core_v2_ack_pdu;
```

Each type descriptor describes a particular MIAM CORE PDU type:

- ALO - Aloha (same format for MIAM CORE v1 and v2)
- ALR - Aloha Response (same format for MIAM CORE v1 and v2)
- DATA - Data Transfer (v1 or v2)
- ACK - Acknowledgement (v1 or v2)

Relevant structures for each PDU type contain various header field values. Refer
to `<libacars/miam-core.h>` for details.

### la_miam_core_format_text()

```C
#include <libacars/libacars.h>
#include <libacars/miam-core.h>

void la_miam_core_format_text(la_vstring *vstr, void const *data, int indent);
```

Serializes a decoded MIAM CORE PDU pointed to by `data` into a human-readable
text indented by `indent` spaces and appends the result to `vstr` (which must be
non-NULL).

If libacars has been built with libxml2 support and `prettify_xml` configuration
variable is set to `true`, then the function attempts to parse the message text
as an XML document. If parsing succeeds (meaning the message indeed contains
XML), the text is pretty-printed (reformatted into multi-line output with proper
indentation).

### la_miam_core_format_json()

```C
#include <libacars/libacars.h>
#include <libacars/miam-core.h>

void la_miam_core_format_json(la_vstring *vstr, void const *data);
```

Serializes a decoded MIAM CORE PDU pointed to by `data` into a JSON string and
appends the result to `vstr` (which must be non-NULL).

### la_proto_tree_find_miam_core()

```C
#include <libacars/libacars.h>
#include <libacars/miam-core.h>

la_proto_node *la_proto_tree_find_miam_core(la_proto_node *root);
```

Walks the protocol tree pointed to by `root` and returns a pointer to the first
encountered node containing MIAM CORE PDU (ie. having a type descriptor of
`la_DEF_miam_core_pdu`). If `root` is NULL or no matching protocol node has been
found in the tree, the function returns NULL.

No `la_proto_tree_find_*()` routines are provided for individual PDU types.
If you need to locate a node containing a particular PDU type, use
`la_proto_tree_find_protocol()` function with a pointer to the type descriptor
of interest, for example:

```C
#include <libacars/libacars.h>
#include <libacars/miam.h>

la_proto_node *v2_data_pdu = la_proto_tree_find_protocol(tree_ptr, &la_DEF_miam_core_v2_data_pdu);
```

### Error handling

Each MIAM CORE PDU structure contains an `err` field:

```C
uint32_t err;			// PDU decoding error code
```

The upper half of the value is a bit field describing header decoding errors,
while the lower half describes message body decoding errors. Refer to
`<libacars/miam-core.h>` for possible values, which are defined as
`LA_MIAM_ERR_*` macros.

Serialization routines inspect the error field and print diagnostic output for
each encountered error. Header errors are fatal, ie. the actual PDU is not
printed because its content is dubious. Body errors are not fatal - the header
will still be printed and the body as well (at least partially). Diagnostic
messages are printed after the message body.

## Message reassembly API

The purpose of reassembly API in libacars is to provide a generic engine for
reassembling arbitrary portions of data, including various kinds of protocol
data units (packets, messages, etc). In order for the API to be applicable, the
protocol must satisfy certain prerequisites, in particular:

- All fragments of a given message must be identifiable by a certain invariant
  key to distinguish between fragments belonging to different messages.

- All fragments must have sequence numbers to determine their order.

- Sequence numbers of subsequent fragments must differ by one.

- There must exist an end-of-message indication. This might be accomplished
  either by a boolean flag indicating whether a particular fragment is the final
  fragment of the message, or by specifying the total size of the reassembled
  message in bytes (the reassembly process is deemed complete after receiving
  that amount of data).

- Fragments of each message must be passed to the reassembly engine in correct
  order.  Duplicates (consecutive repetitions of a fragment with the same
  sequence number) are allowed (they are skipped), but sequence number gaps and
  reversals are not allowed (however see below for an exception).

- Non-zero reassembly timeout must be specified for each message. This is
  particularly important when sequence numbers wrap often. The engine must
  expire old incomplete reassembly entries in a timely manner to prevent merging
  fragments belonging to different messages and to limit memory consumption.

- Receive timestamp of each fragment must be known, so that reassembly timeouts
  could be handled properly.

Additionally:

- Sequence numbers may optionally wrap at a certain constant value.

- If the sequence numbers of the fragments of each message start from a fixed
  known value, the engine may ensure completeness of reassembled messages,
  ie. that each message begins with a fragment with a sequence number of that
  fixed value. This is also the only case when reversals of sequence numbers are
  allowed - fragments with sequence numbers lower than the previously received
  fragment are treated as duplicates (skipped).

- Sequencing fragments using fragment offset (like in IP protocol) is not
  supported.

The engine is used internally by libacars to reassemble ACARS messages and MIAM
file transfers. In addition, dumpvdl2 uses the engine to reassemble X.25.

### la_reasm_ctx_new()

```C
#include <libacars/reassembly.h>

la_reasm_ctx *la_reasm_ctx_new();
```

Initializes reassembly engine context which stores the reassembly state table
for each protocol. A pointer to this context should then be passed to the
decoding routine of each protocol which may contain fragmented messages - in
particular, `la_acars_parse_and_reassemble()`,
`la_acars_apps_parse_and_reassemble()` and `la_miam_parse_and_reassemble()`.

### la_reasm_ctx_destroy()

```C
#include <libacars/reassembly.h>

void la_reasm_ctx_destroy(void *ctx);
```

Deallocates memory used by the reassembly context and all protocol state tables.

### la_reasm_table_new()

```C
#include <libacars/reassembly.h>

la_reasm_table *la_reasm_table_new(la_reasm_ctx *rctx, void const *table_id,
		la_reasm_table_funcs funcs, int cleanup_interval);

```
Creates a reassembly table for the given protocol and returns a pointer to it.

- `rctx` - reassembly context (initialized with `la_reasm_ctx_new()`)

- `table_id` - an arbitrary pointer identifying the new table. For example
  libacars uses pointers to la_type_descriptors to identify reassembly tables.

- `funcs` - a set of callbacks implementing protocol-specific operations
  performed during reassembly process.

- `cleanup_interval` - a positive number indicating how often stale entries
  (incomplete, timed out reassemblies) are removed from the reassembly table.
  Cleanup is performed every `cleanup_interval` processed fragments of this
  protocol (ie. every `cleanup_interval` executions of `la_reasm_fragment_add()`
  function with the same value of `table_id`).

`la_reasm_table_funcs` structure contains a set of pointers to user-supplied
protocol-specific callbacks and is defined as follows:

```C
#include <libacars/reassembly.h>

typedef void *(la_reasm_get_key_func)(void const *msg);
typedef la_hash_func la_reasm_hash_func;
typedef la_hash_compare_func la_reasm_compare_func;
typedef la_hash_key_destroy_func la_reasm_key_destroy_func;

typedef struct {
	la_reasm_get_key_func *get_key;
	la_reasm_get_key_func *get_tmp_key;
	la_reasm_hash_func *hash_key;
	la_reasm_compare_func *compare_keys;
	la_reasm_key_destroy_func *destroy_key;
} la_reasm_table_funcs;
```

- `get_key` - a callback which takes a pointer to the message metadata structure
  (which might be the packet header, for example) as argument and returns a
  pointer to a newly allocated value which will be used as the hash key.
- `get_tmp_key` - same as `get_key`, but this callback should return a temporary
  key, which will be used for hash lookups only and could be freed with a single
  call to `free()`.
- `hash_key`, `compare_keys`, `destroy_key` - callbacks used for hash
  operations. Refer to the chapter about hash API for more information on how
  these callbacks work.

### la_reasm_table_lookup()

```C
#include <libacars/reassembly.h>

la_reasm_table *la_reasm_table_lookup(la_reasm_ctx *rctx, void const *table_id);
```

Searches for the reassembly table `table_id` in the context pointed to by `rctx`
and returns a pointer to it. Returns NULL if not found.

### la_reasm_fragment_add()

```C
#include <libacars/reassembly.h>

la_reasm_status la_reasm_fragment_add(la_reasm_table *rtable, la_reasm_fragment_info const *finfo);
```

Adds the fragment `finfo` to the reassembly table `rtable`. Fragment data
is supplied in the `la_reasm_fragment_info` structure, defined as follows:

```C
#include <libacars/reassembly.h>

typedef struct {
	void const *msg_info;
	uint8_t *msg_data;
	int msg_data_len;
	int total_pdu_len;
	struct timeval rx_time;
	struct timeval reasm_timeout;
	int seq_num;
	int seq_num_first;
	int seq_num_wrap;
	bool is_final_fragment;
} la_reasm_fragment_info;
```

- `msg_info` - an opaque pointer to message metadata (eg. packet header). This
  pointer is passed as an argument to `get_key` and `get_tmp_key` callbacks
  supplied when creating the reassembly table with `la_reasm_table_new()`.
- `msg_data`, `msg_data_len` - the data part of the fragment (ie. its payload).
  The reassembly engine concatenates all these parts together.
- `total_pdu_len` - total length of the reassembled message (if known). Setting
  this field to a positive value indicates that the reassembly should be deemed
  complete when the amount of data collected from fragments equals or exceeds
  this value. If the total length is unknown, this field should be set to zero.
  In this case, the final fragment of the message must be indicated by setting
  `is_final_fragment` flag to true. Note that only `total_pdu_len` value from
  the first fragment of the message is taken into account In subsequent fragments
  of the same message this field may be set to any value (eq. zero).
- `rx_time` - time when this fragment has been received. Used for handling
  reassembly timeouts and expiration of old incomplete entries.
- `reasm_timeout` - reassembly timeout. All fragments must be received before it
  expires, otherwise the reassembly of the message will be aborted.
- `seq_num` - sequence number of this fragment. Fragments of every message must
  be numbered consecutively.
- `seq_num_first` - if fragment sequence numbering of each message starts with a
  known constant value, it should be given here. Reassembly will only succeed
  when the first fragment of the message has this sequence number. If the sequence
  number of first fragment is variable, this field must be set to a special value
  `SEQ_FIRST_NONE`.
- `seq_num_wrap` - if fragment sequence numbers wrap at a certain fixed value,
  this value must be supplied here. For example, sequence numbers in X.25
  protocols go from 0 to 7 and then back to 0. In this case `seq_num_wrap` must
  be set to 8. If sequence numbers don't wrap, this field must be set to a
  special value `SEQ_WRAP_NONE`.
- `is_final_fragment` - if total length of the reassembled message is not known
  (ie.`total_pdu_len` has been set to zero in the initial fragment of this
  message) then this field must be set to `true` in the final fragment of the
  message and to `false` in all fragments preceding it. This allows the algorithm
  to determine whether the reassembly of the message in question has already
  completed (ie. all fragments have been received).

`la_reasm_fragment_add()` returns the message reassembly status an enumerated
value defined as follows:

```C
#include <libacars/reassembly.h>

typedef enum {
	LA_REASM_UNKNOWN,
	LA_REASM_COMPLETE,
	LA_REASM_IN_PROGRESS,
	LA_REASM_SKIPPED,
	LA_REASM_DUPLICATE,
	LA_REASM_FRAG_OUT_OF_SEQUENCE,
	LA_REASM_ARGS_INVALID
} la_reasm_status;
```

- `LA_REASM_UNKNOWN` - default value, never returned by the function. Might be
  used as variable initializer.
- `LA_REASM_COMPLETE` - submitted fragment has been successfully added to the
  reassembly table. The algorithm considers the reassembly of this message as
  complete. Reassembled payload is ready for retrieval with
  `la_reasm_payload_get()`.
- `LA_REASM_IN_PROGRESS` - submitted fragment has been successfully added to the
  reassembly table. Reassembly of this message is not yet complete, the
  algorithm expects more fragments to be submitted.
- `LA_REASM_SKIPPED` - reassembly of this message has not been and will not be
  performed. This value is returned when a non-fragmented message has been
  submitted (ie. it does not exist in the hash table and it's the final
  fragment).
- `LA_REASM_DUPLICATE` - submitted fragment has not been added to the table
  because it is a duplicate of a fragment submitted earlier. This condition is
  not fatal. The reassembly will continue when subsequent fragments are submitted.
- `LA_REASM_FRAG_OUT_OF_SEQUENCE` - submitted fragment has an incorrect sequence
  number (ie. there was a gap or reversal). Reassembly of this message has been
  aborted and its entry removed from the hash table. If any other fragment of this
  messages is submitted, it will be treated as a new message, ie. the reassembly
  will start from scratch.
- `LA_REASM_ARGS_INVALID` - failure due to incorrect arguments. Either
  `msg_info` is NULL or a zero timeout has been submitted.

### la_reasm_payload_get()

Retrieves the reasembled payload of the given message.

```C
#include <libacars/reassembly.h>

int la_reasm_payload_get(la_reasm_table *rtable, void const *msg_info, uint8_t **result);
```

- `rtable` - pointer to the reassembly table.
- `msg_info` - an opaque pointer identifying the requested message. Will be
  passed to `get_tmp_key` to retrieve the hash key for reassembly table lookup.
- `result` - the reassembled payload will be stored in a newly-allocated buffer
  and the pointer to this buffer will be stored here.

The function returns the length of the reassembled buffer. If the given message
could not be found in the reassembly table, the function returns a negative
value.

The size of the allocated result buffer is in fact one byte larger than the
value returned by the function. The final byte is set to 0. This allows the
caller to cast the result to `char *` and treat is as a string, should the
message contents be textual.

### la_reasm_status_name_get()

Returns a short textual description of the given reassembly status value.

```C
#include <libacars/reassembly.h>

char const *la_reasm_status_name_get(la_reasm_status status);
```

## la_vstring API

libacars uses `la_vstring` data type for storing results of message
serialization. It is essentially a variable-length, appendable, auto-growing
NULL-terminated string. It is generic and can therefore be used for any purpose
not necessarily related to ACARS processing.

### la_vstring

```C
#include <libacars/vstring.h>

typedef struct {
        char *str;
        size_t len;
        size_t allocated_size;
} la_vstring;
```

- `str` - the pointer to the string buffer. You may access it directly for
  reading (eg. to print it) but do not write it directly, use provided functions
  instead.
- `len` - current length of the string (not including trailing '\0')
- `allocated_size` - current size of the allocated buffer

### la_vstring_new()

```C
#include <libacars/vstring.h>

la_vstring *la_vstring_new();
```

Allocates a new `la_vstring` and returns a pointer to it.

### la_vstring_destroy()

```C
#include <libacars/vstring.h>

void la_vstring_destroy(la_vstring *vstr, bool destroy_buffer);
```

Frees the memory used by the `la_vstring` pointed to by `vstr`. If
`destroy_buffer` is set to true, frees the character buffer `str` as well.
Otherwise the character buffer and its contents are preserved and may be used
later (but remember to copy the pointer to it before executing `la_vstring_destroy()`
and free it later with `free()`.

### la_vstring_append_sprintf()

```C
#include <libacars/vstring.h>

void la_vstring_append_sprintf(la_vstring *vstr, char const *fmt, ...);
```

Appends formatted string to the end of `vstr`.  Automatically extends `vstr` if
it's too short to fit the result.

### la_vstring_append_buffer()

```C
#include <libacars/vstring.h>

void la_vstring_append_buffer(la_vstring *vstr, void const *buffer, size_t size);
```

Appends the contents of `buffer` of length `size` to the end of `vstr`.
Automatically extends `vstr` if it's too short to fit the result. The bufer does
not need to be NULL-terminated. It may also contain non-printable characters,
including NULL byte ('\0'). In the latter case it is not safe to print the
resulting la_vstring with `printf`-like functions - the output will get
truncated at the first NULL character. `write()` or `fwrite()` should be used
instead.

### la_isprintf_multiline_text()

```C
#include <libacars/vstring.h>

void la_isprintf_multiline_text(la_vstring *vstr, int indent, char const *txt);
```

Appends the contents of `txt` to the end of `vstr`. If `txt` contains multiple
lines of text (separated by `'\n'` characters), then each line is separately
indented by `indent` spaces.

## la_list API

`la_list` is a single-linked list.

### la_list

```C
#include <libacars/list.h>

typedef struct la_list la_list;

struct la_list {
        void *data;
        la_list *next;
};
```

- `data` - an opaque pointer to data element stored in the list item
- `next` - a pointer to the next item in the list

### la_list_append()

```C
#include <libacars/list.h>

la_list *la_list_append(la_list *l, void *data);
```

Allocates a new list item, stores `data` in it and appends it at the end of the
list `l`. Returns a pointer to the top element of the list, ie. when `l` is not
NULL, it returns `l`, otherwise returns a pointer to the newly allocated list
item.

### la_list_next()

```C
#include <libacars/list.h>

la_list *la_list_next(la_list const *l);
```

A convenience function which returns a pointer to the list element occuring
after `l`. Essentially, it returns `l->next`. Returns NULL, if `l` is the last
item in the list.  Useful when iterating over list elements in a `for()` or
`while()` loop (but see also `la_list_foreach()`).

### la_list_length()

```C
#include <libacars/list.h>

size_t la_list_length(la_list const *l);
```

Returns the number of items in the list `l`. If `l` is NULL, returns 0.

### la_list_foreach()

```C
#include <libacars/list.h>

void la_list_foreach(la_list *l, void (*cb)(), void *ctx);
```

Iterates over items of the list `l` and executes a callback function `cb` for
each item:

```C
cb(l->data, ctx);
```

- `l->data` - a pointer to the data chunk stored in the current list item
- `ctx` - an opaque pointer to an arbitrary context data (might be NULL)

### la_list_free()

```C
#include <libacars/list.h>

void la_list_free(la_list *l);
```

Deallocates memory used by all items in the list `l`. All data chunks stored in
the list are freed with `free(3)`. This is appropriate when the data chunks are
simple data structures allocated with a single `malloc(3)` call.

### la_list_free_full()

```C
#include <libacars/list.h>

void la_list_free_full(la_list *l, void (*node_free)());
```

Deallocates memory used by all items in the list `l`. All data chunks stored in
the list are freed by executing callback function `node_free`:

```C
node_free(l->data);
```

- `l->data` - a pointer to the data chunk stored in the current list item

This function should be used when data chunks are complex structures composed of
multiple allocated memory chunks.

### la_list_free_full_with_ctx()

```C
#include <libacars/list.h>

void la_list_free_full_with_ctx(la_list *l, void (*node_free)(), void *ctx);
```

Deallocates memory used by all items in the list `l`. All data chunks stored in
the list are freed by executing callback function `node_free`:

```C
node_free(l->data, ctx);
```

- `l->data` - a pointer to the data chunk stored in the current list item
- `ctx` - a caller-provided context pointer

This function should be used when data chunks are complex structures composed of
multiple allocated memory chunks and the callback function requires some context
to do its job.

## la_hash API

`la_hash` is a simple hash table implementation. Number of hash buckets is fixed
at 173. libacars provides a function for hashing based on character strings,
however basically any data type might be used as a hash key. It's just a matter
of implementing appropriate callback functions - hash bucket calculator, key
comparator, key destructor and value destructor.

### la_hash_new()

```C
#include <libacars/hash.h>

typedef uint32_t (la_hash_func)(void const *key);
typedef bool (la_hash_compare_func)(void const *key1, void const *key2);
typedef void (la_hash_key_destroy_func)(void *key);
typedef void (la_hash_value_destroy_func)(void *value);

la_hash *la_hash_new(la_hash_func *compute_hash, la_hash_compare_func *compare_keys,
	la_hash_key_destroy_func *destroy_key, la_hash_value_destroy_func *destroy_value);
```

Creates a new hash table and returns a pointer to it.

- `compute_hash` - a pointer to a user-supplied callback function which performs
  the hashing, ie computes the hash bucket for the given key. Must be non-NULL.
- `compare_keys` - a pointer to a user-supplied callback function which tests
  two hash keys for equality. Must be non-NULL.
- `destroy_key` - a pointer to a user-supplied callback function which
  deallocates memory used by the given hash key. Might be NULL - in this case
  keys are not freed when removing/replacing hash entries. The caller is then
  responsible for deallocating the keys.
- `destroy_value` - a pointer to a user-supplied callback function which
  deallocates memory used by the given hash value. Might be NULL - in this case
  values are not freed when removing/replacing hash entries. The caller is then
  responsible for deallocating the values.

libcars provides the following callbacks:

```C
uint32_t la_hash_key_str(void const *k);
```

Computes and returns hash bucket number for storing the given string key. `k`
must be of type `char *`. To be used as `compute_hash`.

```C
bool la_hash_compare_keys_str(void const *key1, void const *key2);
```

Tests two string keys for equality by running `strcmp` on them. `k1` and `k2`
must be of type `char *`. Returns true if given strings are equal, false
otherwise. To be used as `compare_keys`.

```C
void la_simple_free(void *data);
```

Frees the memory area pointed to by `data`. To be used as `destroy_key` and/or
`destroy_value` in case keys and/or values are simple scalar data types that can
be freed by a single call to `free()`.

### la_hash_string()

A helper function to be used in custom `compute_hash` callbacks.

```C
uint32_t la_hash_string(char const *str, uint32_t h);
```

Computes the hash bucket for the given string `str` taking `h` as the initial
hash bucket for computation. This allows hashing over several strings without
the need of concatenating them beforehand. It's just a matter of calling
`la_hash_string` several times, for each string in turn.  During the first call
`h` should be set to `LA_HASH_INIT`. Subsequent calls should use `h` value
returned by the previous call. Example:

```C
typedef struct {
	char *str1, str2, str3;
} three_strings_key;

uint32_t hash_three_strings(void const *k) {
	three_strings_key *key = (three_strings_key *)k;
	uint32_t result = la_hash_string(key->str1, LA_HASH_INIT);
	result = la_hash_string(key->str2, result);
	result = la_hash_string(key->str3, result);
	return result;
}
```

### la_hash_insert()

```C
#include <libacars/hash.h>

bool la_hash_insert(la_hash *h, void *key, void *value);
```

Inserts the key `key` into hash `h` and sets its value to `value`. `key` and
`value` pointers are copied directly into the hash, which means that they must
exist during the entire lifetime of the hash entry (no automatic / static
variables allowed). If the key already exists in the hash, the old value is
freed, and the key value is replaced with the new one. The key in the hash is
not replaced, but the key given as argument is freed in this case. Returns
`true` if the given key already existed in the hash, `false` otherwise.

### la_hash_remove()

```C
#include <libacars/hash.h>

bool la_hash_remove(la_hash *h, void *key);
```

Removes the key `key` from the hash `h`. Frees the key and the value stored in
the hash. The key given as an argument is not freed, so a static value is
allowed here. Returns `true` if the given key existed in the hash, `false`
otherwise.

### la_hash_lookup()

```C
#include <libacars/hash.h>

void *la_hash_lookup(la_hash const *h, void const *key);
```

Searches for the key `key` in the hash `h`. Returns the pointer to its value if
found, otherwise returns NULL. As `key` is used in read-only mode, it may point
to a static or automatic variable.

### la_hash_foreach_remove()

```C
#include <libacars/hash.h>

typedef bool (la_hash_if_func)(void const *key, void const *value, void *ctx);

int la_hash_foreach_remove(la_hash *h, la_hash_if_func *if_func, void *if_func_ctx);
```

Removes all entries from hash `h` which satisfy a given condition. The condition
is implemented as a user-provided `if_func` callback. `la_hash_foreach_remove`
iterates over hash entries, and runs the callback for each entry. Callback
arguments are:

- `key` - the key of the currently evaluated hash entry
- `value` - the value of the currently evaluated hash entry
- `ctx` - a pointer to an arbitrary user data. `if_func_ctx` is passed here.

All entries for which the `if_func` callback returns `true` are removed from the
hash. The function returns number of removed entries.

### la_hash_destroy()

```C
#include <libacars/hash.h>

void la_hash_destroy(la_hash *h);
```

Deallocates memory used by the hash `h`.  If `destroy_key` callback has been
provided, then all keys are freed using this callback.  The same applies to hash
values, if `destroy_value` callback has been provided. If `h` is NULL, the call
to the function is a harmless no-op.

## JSON API

`<libacars/json.h>` provides a simple set of routines to construct a JSON
string. The API is completely stateless. It supports escaping of characters in
string values only. No escaping is performed on key names.

### la_json_start()

```C
#include <libacars/vstring.h>
#include <libacars/json.h>

void la_json_start(la_vstring *vstr);
```

Start a JSON string by appending an initial `{` character to the string `vstr`.

### la_json_end()

```C
void la_json_end(la_vstring *vstr);
```

Terminates the JSON string by emitting `}` character (and trimming preceding
comma, if present).

### la_json_append_bool()

```C
void la_json_append_bool(la_vstring *vstr, char const *key, bool val);
```

Emits a boolean value `val` as a JSON key named `key`.

### la_json_append_double()

```C
void la_json_append_double(la_vstring *vstr, char const *key, double val);
```

Emits a double precision floating point value `val` as a JSON key named `key`.

### la_json_append_int64

```C
void la_json_append_int64(la_vstring *vstr, char const *key, int64_t val);
```

Emits a 64-bit signed integer value `val` as a JSON key named `key`.

### la_json_append_long()

```C
void la_json_append_long(la_vstring *vstr, char const *key, long val)
```

Emits a long integer value `val` as a JSON key named `key`.

**Note:** This function is deprecated, since the `long` type is not portable.
la_json_append_int64 should be used instead.

### la_json_append_char()

```C
void la_json_append_char(la_vstring *vstr, char const *key, char val);
```

Emits a char value `val` as a JSON key named `key`.

### la_json_append_string()

```C
void la_json_append_string(la_vstring *vstr, char const *key, char const *val);
```

Emits a string value `val` as a JSON key named `key`. Escapes non-printable
octets with `\u00XX` sequences.

**Note:** this function does not handle NULL characters inside the string.  If
this might be an issue, `la_json_append_octet_string_as_string` shall be used
instead.

### la_json_append_octet_string()

```C
void la_json_append_octet_string(la_vstring *vstr, char const *key,
		uint8_t const *buf, size_t len);
```

Emits a JSON array named `key` containing a string of `len` octets stored in `buf`.
Octet values are serialized as decimal integers.

### la_json_append_octet_string_as_string()

```C
void la_json_append_octet_string_as_string(la_vstring *vstr, char const *key,
		uint8_t const *buf, size_t len);
```
Emits a string value `val` as a JSON key named `key`. The string consists of
characters represented by octets in the given `buf` of length `len` Escapes
non-printable octets with `\u00XX` sequences.

### la_json_object_start()

```C
void la_json_object_start(la_vstring *vstr, char const *key);
```

Begins a JSON object named `key` by emitting a key name and an opening `{`
character. Every invocation of this function requires using
`la_json_object_end()` later on - otherwise a malformed JSON string will be
produced.

### la_json_object_end()

```C
void la_json_object_end(la_vstring *vstr);
```

Ends a previously opened JSON object by emitting a terminating `}` character
and a comma.

### la_json_array_start()

```C
void la_json_array_start(la_vstring *vstr, char const *key);
```

Begins a JSON array named `key` by emitting a key name and an opening `[`
character. Every invocation of this function requires using
`la_json_array_end()` later on - otherwise a malformed JSON string will be
produced.

### la_json_array_end()

```C
void la_json_array_end(la_vstring *vstr);
```

Ends a previously opened JSON array by emitting a terminating `]` character
and a comma.

## Miscellaneous functions and variables

### LA_VERSION

```C
#include <libacars/version.h>

extern char const * const LA_VERSION;
```

A variable containing the version string of the libacars library currently
running.

## libacars configuration parameters

libacars has several configuration variables affecting the operation of library
components. It provides an API for reading and setting their values.

Refer to `libacars/config_defaults.h` file in libacars source directory for a
full list of all supported configuration variables, their types and default
values.

### la_config_init()

```C
#include <libacars/libacars.h>

void la_config_init();
```

Creates a new config and initializes it with default values. If the config was
already initialized, it is destroyed first.

Usually there is no need to call this function explicitly, because the config
will be initialized automatically on first read or modification.

### la_config_destroy()

```C
#include <libacars/libacars.h>

void la_config_destroy();
```

Destroys the current config and frees memory allocated for it.

### la_config_get_*()

```C
#include <libacars/libacars.h>

bool la_config_get_bool(char const *name, bool *result);
bool la_config_get_int(char const *name, long int *result);
bool la_config_get_double(char const *name, double *result);
bool la_config_get_str(char const *name, char **result);
```

These functions retrieve the value of the config variable `name` and store its
value in `*result`. They return `true` if the given variable existed in the
config, `false` otherwise. If the return value is `false`, the value of
`*result` remains unchanged. If the given variable exists in the config, but is
of a different type, it is treated as non-existent.

In case of string variables, `la_config_get_str` returns a pointer to the value
stored in the config, ie. the value is not copied and it should not be modified
in place.

### la_config_set_*()

```C
#include <libacars/libacars.h>

bool la_config_set_bool(char const *name, bool value);
bool la_config_set_int(char const *name, long int value);
bool la_config_set_double(char const *name, double value);
bool la_config_set_str(char const *name, char const *value);
```

These functions set the value of the config variable `name` to the given value.
If the variable does not exist in the config, it is first created. If the value
exists in the config, but is of a different type, it is discarded and recreated
with a new type. If `name` is NULL, `false` is returned, otherwise the result is
always `true`.

### la_config_unset()

```C
#include <libacars/libacars.h>

bool la_config_unset(char *name);
```

Deletes config variable `name` from the current config. Returns `true` if the
variable existed, `false` otherwise.

## Debugging

If libacars has been build with `-DCMAKE_BUILD_TYPE=Debug` option submitted to
cmake, debugging output might be enabled by setting `LA_DEBUG` environment
variable to a numer corresponding to the desired verbosity level:

- 0 - disabled
- 1 - errors
- 2 - info
- 3 - verbose

Additionally, ASN.1 decoders (used to decode CPDLC) can produce their own
debugging messages. To enable ASN.1 debugging, rebuild libacars with options:
`-DCMAKE_BUILD_TYPE=Debug -DEMIT_ASN_DEBUG=ON`.

Debug messages are printed to stderr.

// vim: textwidth=80
