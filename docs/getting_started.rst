Getting Started
===============

This guide covers how to build, install, and start using the pindf library.

Quick Start
-----------

Here's a minimal example to parse a PDF file:

.. code-block:: c

    #include "pindf.h"
    #include <stdio.h>
    #include <stdlib.h>

    int main(int argc, char **argv) {
        if (argc != 2) {
            fprintf(stderr, "Usage: %s <pdf_file>\n", argv[0]);
            return 1;
        }

        pindf_set_log_level(PINDF_LOG_LEVEL_INFO);
        pindf_lexer *lexer = pindf_lexer_new();
        pindf_parser *parser = pindf_parser_new();

        FILE *f = fopen(argv[1], "rb");
        fseek(f, 0, SEEK_END);
        uint64 file_len = ftell(f);
        fseek(f, 0, SEEK_SET);

        pindf_doc *doc = NULL;
        int ret = pindf_file_parse(parser, lexer, f, file_len, &doc);
        if (ret < 0) {
            fprintf(stderr, "Failed to parse PDF\n");
            return 1;
        }

        printf("PDF parsed successfully\n");
        printf("Version: %s\n", doc->pdf_version);
        printf("Xref offset: %d\n", doc->xref_offset);

        // Get the root catalog
        pindf_pdf_obj *root_ref = pindf_dict_getvalue2(&doc->trailer, "/Root");
        if (root_ref) {
            printf("Root object reference found\n");
        }

        // Cleanup
        pindf_parser_clear(parser);
        pindf_lexer_clear(lexer);
        fclose(f);

        return 0;
    }

For more comprehensive examples, see the :doc:`examples` page.

Building the Library
--------------------

The library is built using a simple Makefile. The build system detects your platform and generates the appropriate shared library format:

- **Linux**: ``libpindf.so``
- **macOS**: ``libpindf.dylib``
- **Windows**: ``libpindf.dll``

Prerequisites
^^^^^^^^^^^^^

- C compiler (gcc, clang, or MSVC)
- zlib development libraries (for stream decompression)

On Ubuntu/Debian::

    sudo apt-get install zlib1g-dev

On macOS with Homebrew::

    brew install zlib

On Windows, download zlib from: https://zlib.net/

Basic Build
^^^^^^^^^^^

Clone the repository and build::

    git clone https://github.com/nth233/pindf.git
    cd pindf
    make

This will create the shared library file (``libpindf.so``, ``libpindf.dylib``, or ``libpindf.dll``) in the project root.

Build Types
^^^^^^^^^^^

The library supports two build configurations:

**Debug build** (default)::

    make BUILD_TYPE=debug
    # or simply: make

**Release build**::

    make BUILD_TYPE=release

Building Tests
--------------

The project includes several test programs that demonstrate library usage::

    make test

This builds all test executables:

- ``lexer_test`` - Shows lexical analysis of PDF files
- ``parser_test`` - Parses PDF files and outputs structure as JSON
- ``doc_save_modif_test`` - Demonstrates PDF modification
- ``vec_test`` - Tests the vector container
- ``dict_test`` - Tests PDF dictionary operations
- ``modif_test`` - Tests modification tracking
- ``compress_test`` - Tests stream compression/decompression

Running Tests
^^^^^^^^^^^^^

**Lexer test** (prints tokens from a PDF file)::

    ./test/lexer_test <pdf_file>

**Parser test** (converts PDF structure to JSON)::

    ./test/parser_test <pdf_file>
    # Outputs json_dump.json and xref.txt

**PDF modification test** (adds outline to a PDF)::

    ./test/doc_save_modif_test <input_pdf> <output_pdf>

Generating Documentation
------------------------

The project uses Doxygen for API documentation and Sphinx for user documentation::

    make doc

This will:

1. Generate Doxygen XML documentation in ``docs/doxygen/``
2. Build HTML documentation in ``docs/_build/html/``

Open ``docs/_build/html/index.html`` to view the complete documentation.

Platform-Specific Notes
-----------------------

Linux
^^^^^

The default build creates ``libpindf.so``. To install system-wide, copy `libpindf.so ` to `/usr/local/lib/`, and copy the project directory to `/usr/local/include`.

macOS
^^^^^

The build creates ``libpindf.dylib``. To install system-wide, copy `libpindf.dylib` to `/usr/local/lib/`, and copy the project directory to `/usr/local/include`.

Windows
^^^^^^^

The build creates ``libpindf.dll``. You'll need:

1. Visual Studio or MinGW compiler
2. zlib DLLs (``zlib1.dll``)

For development, copy headers to your include path and link against the library.

Using in Your Project
---------------------

Include the main header::

    #include "pindf.h"

Link against the library::

    # Linux/macOS
    gcc -o myapp myapp.c -L. -lpindf

    # Windows (MinGW)
    gcc -o myapp.exe myapp.c -L. -lpindf.dll

    # Windows (MSVC)
    cl myapp.c libpindf.lib

If you've installed the library system-wide, you can use::

    gcc -o myapp myapp.c -lpindf

Cleaning Up
-----------

To remove built files::

    make clean

For documentation only::

    make clean_doc