Limitations
===========

This section lists PDF features that are **not implemented** in the pindf library.
The library focuses on low-level PDF parsing and basic manipulation functionality.

Unsupported Features
-----------------

The following PDF features are **not supported** in the current version:

1. **Stream Filters**

   - **Supported**: 

     - ``/FlateDecode`` (zlib/deflate compression)
     - No filter (raw data)

   - **Not supported**: 
     ``/ASCIIHexDecode``, ``/ASCII85Decode``, ``/LZWDecode``,
     ``/RunLengthDecode``, ``/CCITTFaxDecode``, ``/DCTDecode``, ``/JPXDecode``,
     ``/JBIG2Decode``
   - **Limited predictor support**: Only basic PNG predictors are supported;
     TIFF predictors and some PNG predictors are not implemented

2. **Encryption and Security**
   - No support for encrypted PDF files (password-protected documents)
   - No support for digital signatures
   - No support for PDF permissions or access controls

3. **Free Object Management**
   - Free objects (marked as free in xref tables) are detected and ignored
   - Limited support for free object linked-list traversal and management
   - The library does not optimize or compact free object space

See Also
--------

- :doc:`getting_started` for installation and basic usage
- :doc:`examples` for code examples showing supported functionality
- :doc:`pdf_structure` for understanding PDF internals
- :doc:`api/modules` for API reference