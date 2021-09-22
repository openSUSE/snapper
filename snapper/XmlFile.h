/*
 * Copyright (c) [2010-2012] Novell, Inc.
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


#ifndef SNAPPER_XML_FILE_H
#define SNAPPER_XML_FILE_H


#include <libxml/tree.h>
#include <string>
#include <list>
#include <sstream>
#include <boost/noncopyable.hpp>

#include "snapper/AppUtil.h"


namespace snapper
{
    using std::string;
    using std::list;


    class XmlFile : private boost::noncopyable
    {

    public:

	XmlFile();
	XmlFile(int fd, const string& url);
	XmlFile(const string& filename);

	~XmlFile();

	void save(int fd);
	void save(const string& filename);

	void setRootElement(xmlNode* node)
	    { xmlDocSetRootElement(doc, node); }

	const xmlNode* getRootElement() const
	    { return xmlDocGetRootElement(doc); }

    private:

	xmlDoc* doc = nullptr;

    };


    xmlNode* xmlNewNode(const char* name);
    xmlNode* xmlNewChild(xmlNode* node, const char* name);


    const xmlNode* getChildNode(const xmlNode* node, const char* name);
    list<const xmlNode*> getChildNodes(const xmlNode* node, const char* name);


    bool getValue(const xmlNode* node, string& value);

    bool getChildValue(const xmlNode* node, const char* name, string& value);
    bool getChildValue(const xmlNode* node, const char* name, bool& value);

    template<typename Type>
    bool getChildValue(const xmlNode* node, const char* name, Type& value)
    {
	string tmp;
	if (!getChildValue(node, name, tmp))
	    return false;

	std::istringstream istr(tmp);
	classic(istr);
	istr >> value;
	return true;
    }

    bool getAttributeValue(const xmlNode* node, const char* name, string& value);
    bool getAttributeValue(const xmlNode* node, const char* name, bool& value);

    void setChildValue(xmlNode* node, const char* name, const char* value);
    void setChildValue(xmlNode* node, const char* name, const string& value);
    void setChildValue(xmlNode* node, const char* name, bool value);

    template<typename Type>
    void setChildValue(xmlNode* node, const char* name, const Type& value)
    {
	std::ostringstream ostr;
	classic(ostr);
	ostr << value;
	setChildValue(node, name, ostr.str());
    }

    template<typename Type>
    void setChildValue(xmlNode* node, const char* name, const list<Type>& values)
    {
	for (typename list<Type>::const_iterator it = values.begin(); it != values.end(); ++it)
	    setChildValue(node, name, *it);
    }

}


#endif
