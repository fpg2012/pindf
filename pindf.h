/*
 Copyright 2026 fpg2012 (aka nth233)

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "container/uchar_str.h"
#include "type.h"
#include "core/lexer.h"
#include "core/parser.h"
#include "pdf/obj.h"
#include "pdf/doc.h"
#include "core/serialize.h"
#include "stream/filter.h"

#define PINDF_STREAM_BUF_LEN 1048576

/// @brief Parse a PDF file and create a document object
/// @param parser Parser instance to use
/// @param lexer Lexer instance to use
/// @param fp File pointer to PDF file
/// @param file_len Length of the PDF file in bytes (0 if unknown)
/// @param ret_doc Output pointer to receive the parsed document
/// @return 0 on success, negative error code on failure
int pindf_file_parse(pindf_parser *parser, pindf_lexer *lexer, FILE *fp, uint64 file_len, pindf_doc **ret_doc);

/// @brief Parse a single PDF object from a file
/// @param parser Parser instance to use
/// @param lexer Lexer instance to use
/// @param f File pointer to read from
/// @param ret_obj Output pointer to receive the parsed object
/// @param ret_offset Optional output pointer to receive the file offset of the object
/// @param target_type Expected object type (PINDF_PDF_* constant) or 0 for any type
/// @return 0 on success, negative error code on failure, 1 if object fully parsed and matches target_type
int pindf_parse_one_obj(pindf_parser *parser, pindf_lexer *lexer, FILE *f, pindf_pdf_obj **ret_obj, uint64 *ret_offset, int target_type);

/// @brief Parse a single PDF object from a buffer
/// @param parser Parser instance to use
/// @param lexer Lexer instance to use
/// @param buffer Buffer containing PDF data
/// @param offset Starting offset in the buffer
/// @param ret_obj Output pointer to receive the parsed object
/// @param ret_offset Optional output pointer to receive the buffer offset of the object
/// @param target_type Expected object type (PINDF_PDF_* constant) or 0 for any type
/// @return 0 on success, negative error code on failure, 1 if object fully parsed and matches target_type
int pindf_parse_one_obj_from_buffer(pindf_parser *parser, pindf_lexer *lexer, pindf_uchar_str *buffer, size_t offset, pindf_pdf_obj **ret_obj, uint64 *ret_offset, int target_type);

/// @brief Parse xref table or stream
/// @param parser Parser instance to use
/// @param lexer Lexer instance to use
/// @param fp File pointer to read from
/// @param doc Document to populate with xref entries
/// @param startxref Offset of the xref section in the file
/// @return 0 on success, negative error code on failure
int pindf_parse_xref(pindf_parser *parser, pindf_lexer *lexer, FILE *fp, pindf_doc *doc, uint64 startxref);

/// @brief Quickly find startxref offset by matching "startxref" string
/// @param fp File pointer to read from
/// @param file_len Optional file length (0 to use ftell)
/// @return Offset of "startxref" keyword, or 0 if not found
uint64 pindf_quick_match_startxref(FILE *fp, uint64 file_len);

/// @brief Get a PDF object by its object number
/// @param doc Document containing the object
/// @param parser Parser instance to use for parsing
/// @param lexer Lexer instance to use for parsing
/// @param obj_num Object number to retrieve
/// @return Pointer to the PDF object, or NULL if not found
pindf_pdf_obj *pindf_doc_getobj(pindf_doc *doc, pindf_parser *parser, pindf_lexer *lexer, uint64 obj_num);

/// @brief Decode a stream object
/// @param stream Stream object to decode
/// @param decoded Output buffer to store decoded data
/// @return 0 on success, negative error code on failure
int pindf_stream_decode(pindf_pdf_obj *stream, pindf_uchar_str *decoded);

/// @brief Save modified document to file
/// @param doc Document to save
/// @param fp File pointer to write to
/// @param compress_xref Whether to compress xref table
/// @return 0 on success, negative error code on failure
int pindf_doc_save_modif(pindf_doc *doc, FILE *fp, bool compress_xref);

/// @brief Dereference a PDF object, following indirect references
/// @param doc Document containing the object
/// @param obj Object to dereference
/// @return Dereferenced object, or NULL if not found
pindf_pdf_obj *pindf_deref(pindf_doc *doc, pindf_pdf_obj *obj);