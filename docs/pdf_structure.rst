PDF File Structure
===================

PDF (Portable Document Format) files have a well-defined structure that pindf is designed to parse and manipulate. Understanding this structure helps when working with the library.

PDF File Components
-------------------

A PDF file consists of four main parts:

**Header**
  The first line of a PDF file specifies the version, e.g., ``%PDF-1.4``. This indicates the PDF specification version the file conforms to.

**Body**
  Contains the document's objects organized as an object tree. These objects include:

  - Dictionaries (key-value pairs)
  - Arrays (ordered lists)
  - Strings (literal and hexadecimal)
  - Numbers (integers and reals)
  - Boolean values
  - Names (starting with ``/``)
  - Streams (dictionary + binary data)
  - Null objects
  - References (``n g R`` format)
  - Indirect objects

**Cross-Reference Table (xref)**
  A table containing byte offsets for each indirect object in the file. This enables random access to objects without reading the entire file sequentially. The xref table is essential for PDF's incremental update feature.

**Trailer**
  Contains metadata and points to the root catalog dictionary. The trailer includes:

  - The ``/Root`` entry pointing to the catalog dictionary
  - The ``/Size`` entry indicating the number of entries in the xref table
  - The ``/Info`` entry containing document metadata
  - The ``/ID`` entry with file identifiers

How pindf Handles PDF Structure
-------------------------------

pindf provides components to work with each part of the PDF structure:

**Lexical Analysis**
  The lexer tokenizes PDF syntax elements such as:

  - Numbers (integers and reals)
  - Strings (literal and hexadecimal)
  - Names
  - Keywords (``obj``, ``endobj``, ``stream``, ``endstream``, etc.)
  - Operators and delimiters

**Parser**
  Converts tokens into structured PDF objects. The parser handles:

  - Object parsing (direct and indirect)
  - Dictionary and array construction
  - Stream detection and parsing
  - Reference resolution

**Document Structure**
  Manages the complete PDF document representation:

  - ``pindf_doc`` structure containing xref table and trailer
  - Object lookup via ``pindf_doc_getobj()``
  - Cross-reference table parsing with ``pindf_parse_xref()``

**Modification Tracking**
  Supports incremental updates to PDF files:

  - ``pindf_modif`` structure tracks added/modified objects
  - ``pindf_doc_save_modif()`` saves changes while preserving original content
  - Allows adding new objects without rewriting the entire file

Cross-Reference Table Details
-----------------------------

The xref table is critical for PDF performance. It contains entries in the format::

  nnnnnnnnnn ggggg n

Where:

- ``nnnnnnnnnn`` is the 10-digit byte offset
- ``ggggg`` is the 5-digit generation number
- ``n`` is the entry type (``f`` for free, ``n`` for in-use)

pindf's ``pindf_xref`` structure stores these entries for efficient object lookup.

Object System
-------------

pindf supports all standard PDF object types through the ``pindf_pdf_obj`` structure:

+-------------------+------------------------+------------------------------------------+
| Type              | Enum constant          | Description                              |
+===================+========================+==========================================+
| Integer           | ``PINDF_PDF_INT``      | Integer numbers                          |
+-------------------+------------------------+------------------------------------------+
| Real              | ``PINDF_PDF_REAL``     | Floating point numbers                   |
+-------------------+------------------------+------------------------------------------+
| Dictionary        | ``PINDF_PDF_DICT``     | Key-value pairs with name keys          |
+-------------------+------------------------+------------------------------------------+
| Array             | ``PINDF_PDF_ARRAY``    | Ordered list of objects                 |
+-------------------+------------------------+------------------------------------------+
| Literal String    | ``PINDF_PDF_LTR_STR``  | Strings in parentheses                  |
+-------------------+------------------------+------------------------------------------+
| Hexadecimal String| ``PINDF_PDF_HEX_STR``  | Strings in angle brackets               |
+-------------------+------------------------+------------------------------------------+
| Boolean           | ``PINDF_PDF_BOOL``     | ``true`` or ``false``                   |
+-------------------+------------------------+------------------------------------------+
| Stream            | ``PINDF_PDF_STREAM``  | Dictionary + binary data                |
+-------------------+------------------------+------------------------------------------+
| Name              | ``PINDF_PDF_NAME``     | Names starting with ``/``               |
+-------------------+------------------------+------------------------------------------+
| Null              | ``PINDF_PDF_NULL``     | Null object                             |
+-------------------+------------------------+------------------------------------------+
| Reference         | ``PINDF_PDF_REF``      | Object reference ``n g R``              |
+-------------------+------------------------+------------------------------------------+
| Indirect Object   | ``PINDF_PDF_IND_OBJ``  | Object with generation number           |
+-------------------+------------------------+------------------------------------------+

Modification and Saving
-----------------------

pindf supports incremental PDF updates, which is the standard way PDF editors modify files:

1. **Original content remains unchanged** - The library copies the original file content
2. **New objects appended** - Modified or new objects are added at the end
3. **Updated xref table** - A new cross-reference table includes original and new objects
4. **Updated trailer** - The trailer points to the new xref table

This approach:

- Preserves digital signatures on unchanged portions
- Allows undo operations (original objects remain)
- Is more efficient than rewriting the entire file

Example modification flow::

  // Parse original PDF
  pindf_doc *doc = pindf_file_parse(parser, lexer, f, file_len, &doc);

  // Make modifications
  doc->modif = pindf_modif_new(doc->xref->size);
  // ... add/modify objects ...

  // Save with modifications
  pindf_doc_save_modif(doc, output_fp, true);

Stream Objects
--------------

PDF streams contain binary data (images, compressed content, etc.) and are represented as:

1. **Stream dictionary** - Contains metadata (length, filters, etc.)
2. **Stream data** - The actual binary content

pindf can decode only one type of stream filters: **FlateDecode** (zlib/deflate compression)

Use ``pindf_stream_decode()`` to decode compressed streams.

See Also
--------

- :doc:`getting_started` for installation and basic usage
- :doc:`examples` for code examples
- :doc:`api/modules` for API reference