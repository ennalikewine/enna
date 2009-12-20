/*
 * Copyright (C) 2005-2009 The Enna Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software and its Copyright notices. In addition publicly
 * documented acknowledgment must be given that this software has been used if
 * no source code of this software is made available publicly. This includes
 * acknowledgments in either Copyright notices, Manuals, Publicity and
 * Marketing documents or any documentation provided with any product
 * containing this software. This License does not apply to any software that
 * links to the libraries provided by this software (statically or
 * dynamically), but only to the software provided.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef XML_UTILS_H
#define XML_UTILS_H

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

xmlDocPtr get_xml_doc_from_memory (char *buffer);
xmlNode *get_node_xml_tree(xmlNode *root, const char *prop);
xmlChar *get_prop_value_from_xml_tree(xmlNode *root, const char *prop);
xmlNode *get_node_from_xml_tree_by_attr (xmlNode *root, const char *prop,
                                         const char *attr_name,
                                         const char *attr_value);
xmlChar *get_prop_value_from_xml_tree_by_attr (xmlNode *root, const char *prop,
                                               const char *attr_name,
                                               const char *attr_value);
xmlChar *get_attr_value_from_xml_tree (xmlNode *root, const char *prop,
                                       const char *attr_name);
xmlChar *get_attr_value_from_node (xmlNode *node, const char *attr_name);
xmlXPathObjectPtr get_xnodes_from_xml_tree (xmlDocPtr doc, xmlChar *xpath);
int xml_search_str (xmlNode *n, const char *node, char **str);
int xml_search_int (xmlNode *n, const char *node, int *val);
int xml_search_year (xmlNode *n, const char *node, int *year);

#endif /* XML_UTILS_H */
