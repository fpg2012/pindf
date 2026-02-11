pindf
=====

**pindf** is a lightweight C library for parsing and manipulating PDF files.

Features
--------

- **PDF parsing**: Parse PDF files with lexical analysis and object parsing
- **Object manipulation**: Access and modify PDF objects (dictionaries, arrays, streams, etc.)
- **Cross-reference table handling**: Read and write PDF xref tables
- **Modification saving**: Save modifications to PDF files while preserving original content
- **Stream decoding**: Support for compressed stream objects (FlateDecode, ASCIIHex, etc.)
- **Minimal dependencies**: Only requires zlib for decompression
- **Cross-platform**: Works on Linux, macOS, and Windows

Overview
--------

pindf is designed to be a simple, efficient PDF parsing library written in C. It provides a clean API for working with PDF files at a low level, allowing you to:

- Parse PDF files and extract their structure
- Access and modify individual PDF objects
- Add new content to existing PDFs
- Save modifications incrementally (preserving original content)
- Decode compressed stream data

Documentation
-------------

- :doc:`getting_started` - Installation and quick start guide
- :doc:`contents` - Table of contents
- :doc:`pdf_structure` - Detailed explanation of PDF file format
- :doc:`examples` - Complete code examples and usage patterns
- :doc:`api/modules` - Full API reference (generated from source code)
- :doc:`limitations` - Known limitations and constraints

Contents
--------

.. toctree::
   :maxdepth: 1

   getting_started
   contents
   pdf_structure
   examples
   api/modules
   limitations

Getting Help
------------

- Check the :doc:`examples` page for practical usage examples
- Review the :doc:`pdf_structure` guide to understand PDF internals
- See the API reference for detailed function documentation
