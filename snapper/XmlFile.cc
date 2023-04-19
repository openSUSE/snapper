/*
 * Copyright (c) [2010-2012] Novell, Inc.
 * Copyright (c) [2020-2023] SUSE LLC
 *
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may
 * find current contact information at www.novell.com.
 */


#include <cstring>
#include <unistd.h>

#include "snapper/Exception.h"
#include "snapper/XmlFile.h"


namespace snapper
{

    XmlFile::XmlFile()
	: doc(xmlNewDoc((const xmlChar*) "1.0"))
    {
	if (!doc)
	    SN_THROW(BadAllocException());
    }


    XmlFile::XmlFile(int fd, const string& url)
	: doc(xmlReadFd(fd, url.c_str(), NULL, XML_PARSE_NOBLANKS | XML_PARSE_NONET))
    {
	close(fd);

	if (!doc)
	    SN_THROW(IOErrorException("xmlReadFd failed"));
    }


    XmlFile::XmlFile(const string& filename)
	: doc(xmlReadFile(filename.c_str(), NULL, XML_PARSE_NOBLANKS | XML_PARSE_NONET))
    {
	if (!doc)
	    SN_THROW(IOErrorException("xmlReadFile failed"));
    }


    XmlFile::~XmlFile()
    {
	xmlFreeDoc(doc);
	doc = nullptr;
    }


    void
    XmlFile::save(int fd)
    {
	FILE* f = fdopen(fd, "w");
	if (!f)
	{
	    close(fd);
	    SN_THROW(IOErrorException("fdopen"));
	}

	if (xmlDocFormatDump(f, doc, 1) == -1)
	{
	    fclose(f);
	    SN_THROW(IOErrorException("xmlDocFormatDump failed"));
	}

	fflush(f);
	fsync(fileno(f));

	if (fclose(f) != 0)
	    SN_THROW(IOErrorException("fclose failed"));
    }


    void
    XmlFile::save(const string& filename)
    {
	if (xmlSaveFormatFile(filename.c_str(), doc, 1) == -1)
	    SN_THROW(IOErrorException("xmlSaveFormatFile failed"));
    }


    xmlNode*
    xmlNewNode(const char* name)
    {
	return ::xmlNewNode(NULL, (const xmlChar*) name);
    }


    xmlNode*
    xmlNewChild(xmlNode* node, const char* name)
    {
	return ::xmlNewChild(node, NULL, (const xmlChar*) name, NULL);
    }


    const xmlNode*
    getChildNode(const xmlNode* node, const char* name)
    {
	if (node != NULL)
	    node = node->children;

	for (const xmlNode* cur_node = node; cur_node; cur_node = cur_node->next)
	{
	    if (strcmp(name, (const char*) cur_node->name) == 0)
	    {
		return cur_node;
	    }
	}

	return NULL;
    }


    vector<const xmlNode*>
    getChildNodes(const xmlNode* node, const char* name)
    {
	vector<const xmlNode*> ret;

	if (node != NULL)
	    node = node->children;

	for (const xmlNode* cur_node = node; cur_node; cur_node = cur_node->next)
	{
	    if (cur_node->type == XML_ELEMENT_NODE &&
		strcmp(name, (const char*) cur_node->name) == 0)
	    {
		ret.push_back(cur_node);
	    }
	}

	return ret;
    }


    bool
    getValue(const xmlNode* node, string& value)
    {
	xmlChar* tmp = xmlNodeGetContent(node);
	if (tmp == nullptr)
	    return false;

	value = (const char*) tmp;
	xmlFree(tmp);
	return true;
    }


    bool
    getChildValue(const xmlNode* node, const char* name, string& value)
    {
	if (node != NULL)
	    node = node->children;

	for (const xmlNode* cur_node = node; cur_node; cur_node = cur_node->next)
	{
	    if (cur_node->type == XML_ELEMENT_NODE &&
		strcmp(name, (const char*) cur_node->name) == 0)
	    {
		if (cur_node->children && cur_node->children->content)
		    value = (const char*) cur_node->children->content;
		else
		    value = "";

		return true;
	    }
	}

	return false;
    }


    bool
    getChildValue(const xmlNode* node, const char* name, bool& value)
    {
	string tmp;
	if (!getChildValue(node, name, tmp))
	    return false;

	value = tmp == "true";
	return true;
    }


    bool
    getAttributeValue(const xmlNode* node, const char* name, string& value)
    {
	xmlChar* tmp = xmlGetNoNsProp(node, (const xmlChar*) name);
	if (tmp == nullptr)
	    return false;

	value = (const char*) tmp;
	xmlFree(tmp);
	return true;
    }


    bool
    getAttributeValue(const xmlNode* node, const char* name, bool& value)
    {
	string tmp;
	if (!getAttributeValue(node, name, tmp))
	    return false;

	value = tmp == "true";
	return true;
    }


    void
    setChildValue(xmlNode* node, const char* name, const char* value)
    {
	xmlNewTextChild(node, NULL, (const xmlChar*) name, (const xmlChar*) value);
    }


    void
    setChildValue(xmlNode* node, const char* name, const string& value)
    {
	xmlNewTextChild(node, NULL, (const xmlChar*) name, (const xmlChar*) value.c_str());
    }


    void
    setChildValue(xmlNode* node, const char* name, bool value)
    {
	xmlNewTextChild(node, NULL, (const xmlChar*) name, (const xmlChar*)(value ? "true" : "false"));
    }

}
