
 //
 // XML storage C++ classes version 1.5
 //
 // Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Martin Fuchs <martin-fuchs@gmx.net>
 //

 /// \file xmlstorage.cpp
 /// XMLStorage implementation file


/*

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright
	notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in
	the documentation and/or other materials provided with the
	distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

*/

#include <precomp.h>

#ifndef XS_NO_COMMENT
#define XS_NO_COMMENT	// no #pragma comment(lib, ...) statements in .lib files to enable static linking
#endif

//#include "xmlstorage.h"


namespace XMLStorage {


 // work around GCC's wide string constant bug
#ifdef __GNUC__
const LPCXSSTR XS_EMPTY = XS_EMPTY_STR;
const LPCXSSTR XS_TRUE = XS_TRUE_STR;
const LPCXSSTR XS_FALSE = XS_FALSE_STR;
const LPCXSSTR XS_INTFMT = XS_INTFMT_STR;
const LPCXSSTR XS_INT64FMT = XS_INT64FMT_STR;
const LPCXSSTR XS_FLOATFMT = XS_FLOATFMT_STR;
#endif

const XS_String XS_KEY = XS_KEY_STR;
const XS_String XS_VALUE = XS_VALUE_STR;
const XS_String XS_PROPERTY = XS_PROPERTY_STR;


 /// remove escape characters from zero terminated string
static XS_String unescape(LPCXSSTR s, TCHAR b, TCHAR e)
{
	if (*s == b) {
		LPCXSSTR end = s + _tcslen(s);

		if (end>s && end[-1]==e)
			++s, --end;

		return XS_String(s, end-s);
	} else
		return s;
}

inline XS_String unescape(LPCXSSTR s)
{
	if (*s == _T('\''))
		return unescape(s, _T('\''), _T('\''));
	else
		return unescape(s, _T('"'), _T('"'));
}

 /// remove escape characters from string with specified length
static XS_String unescape(LPCXSSTR s, size_t l, char b, char e)
{
	LPCXSSTR end = s + l;

	if (*s == b)
		if (end>s && end[-1]==e)
			++s, --end;

	return XS_String(s, end-s);
}

inline XS_String unescape(LPCXSSTR s, size_t l)
{
	if (*s == '\'')
		return unescape(s, l, _T('\''), _T('\''));
	else if (*s == '"')
		return unescape(s, l, _T('"'), _T('"'));
	else
		return XS_String(s, l);
}


 /// move to the position defined by xpath in XML tree
bool XMLPos::go(const XPath& xpath)
{
	XMLNode* node = xpath._absolute? _root: _cur;

	node = node->find_relative(xpath);

	if (node) {
		go_to(node);
		return true;
	} else
		return false;
}

 /// move to the position defined by xpath in XML tree
bool const_XMLPos::go(const XPath& xpath)
{
	const XMLNode* node = xpath._absolute? _root: _cur;

	node = node->find_relative(xpath);

	if (node) {
		go_to(node);
		return true;
	} else
		return false;
}


LPCXSSTR XPathElement::parse(LPCXSSTR path)
{
	LPCXSSTR slash = _tcschr(path, '/');
	if (slash == path)
		return NULL;

	 // parse relative path
	size_t l = slash? slash-path: _tcslen(path);
	XS_String comp(path, l);
	path += l;

	 // look for [n] and [@attr_name="attr_value"] expressions in path components
	LPCXSSTR bracket = _tcschr(comp.c_str(), _T('['));
	l = bracket? bracket-comp.c_str(): comp.length();
	_child_name.assign(comp.c_str(), l);

	if (bracket) {
		XS_String expr = unescape(bracket, _T('['), _T(']'));
		LPCXSSTR p = expr.c_str();

		while(_istspace((_TUCHAR)*p)) ++p;

		if (isdigit(*p)) {
			int n = _ttoi(p);	// read index number

			if (n)
				_child_idx = n - 1;	// convert into zero based index
		} else {
			while(*p == '@') {
				LPCXSSTR equal = _tcschr(++p, '=');

				if (equal) {
					 // look for delimiters
					LPCXSSTR del1 = equal+1;
					LPCXSSTR del2 = *del1==_T('\'')||*del1==_T('"')? _tcschr(del1+1, *del1): NULL;

					 // read attribute name and value
					const XS_String& attrName = unescape(p, equal-p);
					const XS_String& attrValue = del2? unescape(del1, ++del2-del1): unescape(del1);

					_mAttrAndAttr[attrName] = attrValue;

					if (!del2)
						break;

					p = del2 + 1;
					while(isspace((unsigned char)*p)) ++p;

					 // handle [@attr1="value1" and @attr2="value2"] expressions
					 // handle expressions like [@name="1.2" and @profil="IPE" and @date="2006_07"]
					if (!_tcsnicmp(p, _T("and "), 4))
						p += 4;
				}

				while(_istspace((_TUCHAR)*p)) ++p;
			}
		}
	}

	return path;
}

XMLNode* XPathElement::find(XMLNode* node) const
{
	int n = 0;

	for(XMLNode::Children::const_iterator it=node->_children.begin(); it!=node->_children.end(); ++it)
		if (matches(**it, n))
			return *it;

	return NULL;
}

const XMLNode* XPathElement::const_find(const XMLNode* node) const
{
	int n = 0;

	for(XMLNode::Children::const_iterator it=node->_children.begin(); it!=node->_children.end(); ++it)
		if (matches(**it, n))
			return *it;

	return NULL;
}

bool XPathElement::matches(const XMLNode& node, int& n) const
{
	if (node != _child_name)
		if (_child_name != XS_TEXT("*"))	// use asterisk as wildcard
			return false;

	for(MAPATTR::const_iterator b=_mAttrAndAttr.begin(), e=_mAttrAndAttr.end(); b!=e; ++b)
		if (node.get(b->first) != b->second)
			return false;

	if (_child_idx == -1)
		return true;
	else if (n++ == _child_idx)
		return true;
	else
		return false;
}


void XPath::init(LPCXSSTR path)
{
	 // Is this an absolute path?
	if (*path == '/') {
		_absolute = true;
		++path;
	} else
		_absolute = false;

	 // parse path
	while(*path) {
		XPathElement elem;

		path = elem.parse(path);

		if (!path)
			break;

		push_back(elem);

		if (*path == '/')
			++path;
	}
}


const XMLNode* XMLNode::find_relative(const XPath& xpath) const
{
	const XMLNode* node = this;

	for(XPath::const_iterator it=xpath.begin(); it!=xpath.end(); ++it) {
		node = it->const_find(node);

		if (!node)
			break;
	}

	return node;
}

XMLNode* XMLNode::find_relative(const XPath& xpath)
{
	XMLNode* node = this;

	for(XPath::const_iterator it=xpath.begin(); it!=xpath.end(); ++it) {
		node = it->find(node);

		if (!node)
			break;
	}

	return node;
}

XMLNode* XMLNode::create_relative(const XPath& xpath)
{
	XMLNode* node = this;

	for(XPath::const_iterator it=xpath.begin(); it!=xpath.end(); ++it) {
		XMLNode* child = it->find(node);

		if (!child) {
			child = new XMLNode(it->_child_name);
			node->add_child(child);

			for(XPathElement::MAPATTR::const_iterator b=it->_mAttrAndAttr.begin(), e=it->_mAttrAndAttr.end(); b!=e; ++b)
				(*this)[b->first] = b->second;
		}

		node = child;
	}

	return node;
}

 /// count the nodes matching the given relative XPath expression
int XMLNode::count(XPath::const_iterator from, const XPath::const_iterator& to) const
{
	const XPathElement& elem = *from++;
	int cnt = 0;
	int n = 0;

	for(XMLNode::Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
		if (elem.matches(**it, n)) {
			if (from != to)
				 // iterate deeper
				cnt += (*it)->count(from, to);
			else
				 // increment match counter
				++cnt;
		}

	return cnt;
}

 /// copy matching tree nodes using the given XPath filter expression
bool XMLNode::filter(const XPath& xpath, XMLNode& target) const
{
	XMLNode* ret = filter(xpath.begin(), xpath.end());
	if (!ret)
		return false;

	 // move returned nodes to target node
	target._children.move(ret->_children);
	target._attributes = ret->_attributes;

	delete ret;

	return true;
}

 /// create a new node tree using the given XPath filter expression
XMLNode* XMLNode::filter(XPath::const_iterator from, const XPath::const_iterator& to) const
{
	XMLNode* copy = NULL;

	const XPathElement& elem = *from++;
	int cnt = 0;
	int n = 0;

	for(XMLNode::Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
		if (elem.matches(**it, n)) {
			if (!copy)
				copy = new XMLNode(*this, XMLNode::COPY_NOCHILDREN);

			if (from != to) {
				XMLNode* ret = (*it)->filter(from, to);

				if (ret) {
					copy->add_child(ret);
					++cnt;
				}
			} else {
				copy->add_child(new XMLNode(**it, XMLNode::COPY_NOCHILDREN));
				++cnt;
			}
		}

	if (!cnt) {
		delete copy;
		copy = NULL;
	}

	return copy;
}


 /// encode XML string literals
std::string EncodeXMLString(const XS_String& str, bool cdata)
{
	LPCXSSTR s = str.c_str();
	size_t l = XS_len(s);

	if (cdata) {
		 // encode the whole string in a CDATA section
		std::string ret = CDATA_START;

#ifdef XS_STRING_UTF8
		ret += str;
#else
		ret += get_utf8(str);
#endif

#ifdef XS_STRICT_XML_1_0
		for(char*p=&ret.at(9); *p; ++p) // strlen(CDATA_START) == 9
			if ((unsigned)*p<0x20 && *p!='\t' && *p!='\r' && *p!='\n')
				*p = '?';
#endif

		ret += CDATA_END;

		return ret;
	} else if (l <= BUFFER_LEN) {
		LPXSSTR buffer = (LPXSSTR)alloca(6*sizeof(XS_CHAR)*XS_len(s));	// worst case "&quot;" / "&apos;"
		LPXSSTR o = buffer;

		for(LPCXSSTR p=s; *p; ++p)
			switch(*p) {
			  case '&':
				*o++ = '&';	*o++ = 'a';	*o++ = 'm';	*o++ = 'p';	*o++ = ';';				// "&amp;"
				break;

			  case '<':
				*o++ = '&';	*o++ = 'l'; *o++ = 't';	*o++ = ';';							// "&lt;"
				break;

			  case '>':
				*o++ = '&';	*o++ = 'g'; *o++ = 't';	*o++ = ';';							// "&gt;"
				break;

			  case '"':
				*o++ = '&';	*o++ = 'q'; *o++ = 'u'; *o++ = 'o'; *o++ = 't';	*o++ = ';';	// "&quot;"
				break;

			  case '\'':
				*o++ = '&';	*o++ = 'a'; *o++ = 'p'; *o++ = 'o'; *o++ = 's';	*o++ = ';';	// "&apos;"
				break;

			  default:
				if ((unsigned)*p>=0x20 || *p=='\t' || *p=='\r' || *p=='\n')
					*o++ = *p;
				else {
#ifdef XS_STRICT_XML_1_0
					*o++ = '?';
#else
					char b[16];
					sprintf(b, "&#%d;", (unsigned)*p);
					for(const char*q=b; *q; )
						*o++ = *q++;
#endif
				}
			}

#ifdef XS_STRING_UTF8
		return XS_String(buffer, o-buffer);
#else
		return get_utf8(buffer, o-buffer);
#endif
	} else { // l > BUFFER_LEN
		 // alternative code for larger strings using ostringstream
		 // and avoiding to use alloca() for preallocated memory
		fast_ostringstream out;

#ifdef XS_STRING_UTF8
		LPCXSSTR s = str.c_str();
#else
		std::string utf8_str = get_utf8(str);
		const char* s = utf8_str.c_str();
#endif

		for(const char* p=s; *p; ++p)
			switch(*p) {
			  case '&':
				out << "&amp;";
				break;

			  case '<':
				out << "&lt;";
				break;

			  case '>':
				out << "&gt;";
				break;

			  case '"':
				out << "&quot;";
				break;

			  case '\'':
				out << "&apos;";
				break;

			  default:
				if ((unsigned)*p>=0x20 || *p=='\t' || *p=='\r' || *p=='\n')
					out << (unsigned char)*p;
				else {
#ifdef XS_STRICT_XML_1_0
					out << '?';
#else
					out << "&#" << (unsigned)*p << ";";
#endif
				}
			}

#ifdef XS_STRING_UTF8
		return XS_String(out.str());
#else
		return out.str();
#endif
	}
}

 /// decode XML string literals
XS_String DecodeXMLString(const std::string& str_utf8)
{
#ifdef XS_STRING_UTF8
	const XS_String& str = str_utf8;
#else
	XS_String str;
	assign_utf8(str, str_utf8.c_str(), str_utf8.length());
#endif

	LPCXSSTR s = str.c_str();
	size_t l = XS_len(s);

	TMP_ALLOC(XS_CHAR, buffer, heap, l);

	LPXSSTR o = buffer;
	for(LPCXSSTR p=s; *p; ++p)
		if (*p == '&') {
			if (!XS_nicmp(p+1, XS_TEXT("lt;"), 3)) {
				*o++ = '<';
				p += 3;
			} else if (!XS_nicmp(p+1, XS_TEXT("gt;"), 3)) {
				*o++ = '>';
				p += 3;
			} else if (!XS_nicmp(p+1, XS_TEXT("amp;"), 4)) {
				*o++ = '&';
				p += 4;
			} else if (!XS_nicmp(p+1, XS_TEXT("quot;"), 5)) {
				*o++ = '"';
				p += 5;
			} else if (!XS_nicmp(p+1, XS_TEXT("apos;"), 5)) {
				*o++ = '\'';
				p += 5;
			} else	//@@ maybe decode "&#xx;" special characters
				*o++ = *p;
		} else if (*p=='<' && !XS_nicmp(p+1,XS_TEXT("![CDATA["),8)) {
			LPCXSSTR e = XS_strstr(p+9, XS_TEXT(CDATA_END));
			if (e) {
				p += 9;
				size_t l = e - p;
				memcpy(o, p, l);
				o += l;
				p = e + 2;
			} else
				*o++ = *p;
		} else
			*o++ = *p;

	return XS_String(buffer, o-buffer);
}


 /// write node with children tree to output stream using original white space
void XMLNode::original_write_worker(std::ostream& out) const
{
	out << _leading << '<' << EncodeXMLString(*this);

	for(AttributeMap::const_iterator it=_attributes.begin(); it!=_attributes.end(); ++it)
		out << ' ' << EncodeXMLString(it->first) << "=\"" << EncodeXMLString(it->second) << "\"";

	if (!_children.empty() || !_content.empty()) {
		out << '>';

		if (_cdata_content)
			out << CDATA_START << _content << CDATA_END;
		else
			out << _content;

		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
			(*it)->original_write_worker(out);

		out << _end_leading << "</" << EncodeXMLString(*this) << '>';
	} else
		out << "/>";

	out << _trailing;
}


 /// print node without any white space
void XMLNode::plain_write_worker(std::ostream& out) const
{
	out << '<' << EncodeXMLString(*this);

	for(AttributeMap::const_iterator it=_attributes.begin(); it!=_attributes.end(); ++it)
		out << ' ' << EncodeXMLString(it->first) << "=\"" << EncodeXMLString(it->second) << "\"";

	 // strip leading white space from content
	const char* content = _content.c_str();
	while(isspace((unsigned char)*content)) ++content;

	if (!_children.empty() || *content) {
		out << ">" << content;

		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
			(*it)->plain_write_worker(out);

		out << "</" << EncodeXMLString(*this) << ">";
	} else
		out << "/>";
}


 /// pretty print node with children tree to output stream
void XMLNode::pretty_write_worker(std::ostream& out, const XMLFormat& format, int indent) const
{
	for(int i=indent; i--; )
		out << XML_INDENT_SPACE;

	out << '<' << EncodeXMLString(*this);

	for(AttributeMap::const_iterator it=_attributes.begin(); it!=_attributes.end(); ++it)
		out << ' ' << EncodeXMLString(it->first) << "=\"" << EncodeXMLString(it->second) << "\"";

	 // strip leading white space from content
	const char* content = _content.c_str();
	while(isspace((unsigned char)*content)) ++content;

	if (!_children.empty() || *content) {
		out << '>' << content;

		if (!_children.empty())
			out << format._endl;

		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
			(*it)->pretty_write_worker(out, format, indent+1);

		for(int i=indent; i--; )
			out << XML_INDENT_SPACE;

		out << "</" << EncodeXMLString(*this) << '>' << format._endl;
	} else
		out << "/>" << format._endl;
}


 /// write node with children tree to output stream using smart formating
void XMLNode::smart_write_worker(std::ostream& out, const XMLFormat& format, int indent) const
{
	 // strip the first line feed from _leading
	const char* leading = _leading.c_str();
	if (*leading == '\n') ++leading;

	if (!*leading)
		for(int i=indent; i--; )
			out << XML_INDENT_SPACE;
	else
		out << leading;

	out << '<' << EncodeXMLString(*this);

	for(AttributeMap::const_iterator it=_attributes.begin(); it!=_attributes.end(); ++it)
		out << ' ' << EncodeXMLString(it->first) << "=\"" << EncodeXMLString(it->second) << "\"";

	 // strip leading white space from content
	const char* content = _content.c_str();
	while(isspace((unsigned char)*content)) ++content;

	if (_children.empty() && !*content)
		out << "/>";
	else {
		out << '>';

		if (_cdata_content)
			out << CDATA_START << _content << CDATA_END;
		else if (!*content)
			out << format._endl;
		else
			out << content;

		Children::const_iterator it = _children.begin();

		if (it != _children.end()) {
			for(; it!=_children.end(); ++it)
				(*it)->smart_write_worker(out, format, indent+1);

			 // strip the first line feed from _end_leading
			const char* end_leading = _end_leading.c_str();
			if (*end_leading == '\n') ++end_leading;

			if (!*end_leading)
				for(int i=indent; i--; )
					out << XML_INDENT_SPACE;
			else
				out << end_leading;
		} else
			out << _end_leading;

		out << "</" << EncodeXMLString(*this) << '>';
	}

	if (_trailing.empty())
		out << format._endl;
	else
		out << _trailing;
}


std::ostream& operator<<(std::ostream& out, const XMLError& err)
{
	out << err._systemId << "(" << err._line << ")";

	if (err._column > 1)
		out << " [column " << err._column << "]";

	out << " : " << err._message;

	return out;
}


#if defined(XS_NATIVE) || defined(XMLNODE_LOCATION)

std::string XMLLocation::str() const
{
	std::ostringstream out;

	if (_pdisplay_path)
		out << _pdisplay_path;

	out << "(" << _line << ")";

	if (_column > 1)
		out << " [column " << _column << "]";

	out << " :";

	return out.str();
}

#endif


const char* get_xmlsym_end_utf8(const char* p)
{
	for(; *p; ++p) {
		char c = *p;

		// NameChar ::= Letter | Digit | '.' | '-' | '_' | ':' | CombiningChar | Extender
		if (c == '\xC3')	// UTF-8 escape character
			++p;	//TODO only continue on umlaut characters
		else if (!isalnum(c) && c!='.' && c!='-' && c!='_' && c!=':')
			break;
	}

	return p;
}


void DocType::parse(const char* p)
{
	while(isspace((unsigned char)*p)) ++p;

	const char* start = p;
	p = get_xmlsym_end_utf8(p);
	_name.assign(start, p-start);

	while(isspace((unsigned char)*p)) ++p;

	start = p;
	p = get_xmlsym_end_utf8(p);
	std::string keyword(p, p-start);	// "PUBLIC" or "SYSTEM"

	while(isspace((unsigned char)*p)) ++p;

	if (*p=='"' || *p=='\'') {
		char delim = *p;

		start = ++p;
		while(*p && *p!=delim) ++p;

		if (*p == delim)
			_public.assign(start, p++-start);
	} else
		_public.erase();

	while(isspace((unsigned char)*p)) ++p;

	if (*p=='"' || *p=='\'') {
		char delim = *p;

		start = ++p;
		while(*p && *p!=delim) ++p;

		if (*p == delim)
			_system.assign(start, p++-start);
	} else
		_system.erase();
}


void XMLFormat::print_header(std::ostream& out, bool lf) const
{
	if (_inhibit_header)
		return;

	if (_utf8_bom) {
		 // write UTF-8 signature at file start
		out.put('\xEF');
		out.put('\xBB');
		out.put('\xBF');
		assert(!_stricmp(_encoding.c_str(), "utf-8"));
	}

	out << "<?xml version=\"" << _version << "\" encoding=\"" << _encoding << "\"";

	if (_standalone != -1)
		out << " standalone=\"yes\"";

	out << "?>";

	if (lf)
		out << _endl;

	if (!_doctype.empty()) {
		out << "<!DOCTYPE " << _doctype._name;

		if (!_doctype._public.empty()) {
			out << " PUBLIC \"" << _doctype._public << '"';

			if (lf)
				out << _endl;

			out << " \"" << _doctype._system << '"';
		} else if (!_doctype._system.empty())
			out << " SYSTEM \"" << _doctype._system << '"';

		out << "?>";

		if (lf)
			out << _endl;
	}

	for(StyleSheetList::const_iterator it=_stylesheets.begin(); it!=_stylesheets.end(); ++it) {
		it->print(out);

		if (lf)
			out << _endl;
	}

/*	if (!_additional.empty()) {
		out << _additional;

		if (lf)
			out << _endl;
	} */
}

void StyleSheet::print(std::ostream& out) const
{
	out << "<?xml-stylesheet"
			" href=\"" << _href << "\""
			" type=\"" << _type << "\"";

	if (!_title.empty())
		out << " title=\"" << _title << "\"";

	if (!_media.empty())
		out << " media=\"" << _media << "\"";

	if (!_charset.empty())
		out << " charset=\"" << _charset << "\"";

	if (_alternate)
		out << " alternate=\"yes\"";

	out << "?>";
}


 /// return formated error message
std::string XMLError::str() const
{
	std::ostringstream out;

	out << *this;

	return out.str();
}


 /// return merged error strings
XS_String XMLErrorList::str(const char* del) const
{
	std::ostringstream out;

	for(const_iterator it=begin(); it!=end(); ++it)
		out << *it << del;

	return out.str();
}


void XMLReaderBase::finish_read()
{
	if (_pos != NULL) {
		if (_pos->_children.empty())
			_pos->_trailing.append(_content);
		else
			_pos->_children.back()->_trailing.append(_content);
	}

	_content.erase();
}


 /// store XML version and encoding into XML reader
void XMLReaderBase::XmlDeclHandler(const char* version, const char* encoding, int standalone)
{
	if (version)
		_format._version = version;

	if (encoding)
		_format._encoding = encoding;

	_format._standalone = standalone;
}


 /// notifications about XML start tag
void XMLReaderBase::StartElementHandler(const XS_String& name, const XMLNode::AttributeMap& attributes)
{
	const char* s = _content.c_str();
	const char* e = s + _content.length();
	const char* p = s;

	 // search for content end leaving only white space for leading
	for(p=e; p>s; --p)
		if (!isspace((unsigned char)p[-1]))
			break;

	if (p != s) {
		if (_pos->_children.empty()) {	// no children in last node?
			if (_last_tag == TAG_START)
				_pos->_content.append(s, p-s);
			else if (_last_tag == TAG_END)
				_pos->_trailing.append(s, p-s);
			else // TAG_NONE at root node
				p = s;
		} else
			_pos->_children.back()->_trailing.append(s, p-s);
	}

	std::string leading;

	if (p != e)
		leading.assign(p, e-p);

	XMLNode* node = new XMLNode(name, leading);

	_pos.add_down(node);

#ifdef XMLNODE_LOCATION
	node->_location = get_location();
#endif

	node->_attributes = attributes;

	_last_tag = TAG_START;
	_content.erase();
}

 /// notifications about XML end tag
void XMLReaderBase::EndElementHandler()
{
	const char* s = _content.c_str();
	const char* e = s + _content.length();
	const char* p;

	if (!strncmp(s,CDATA_START,9) && !strncmp(e-3,CDATA_END,3)) {
		s += 9;
		p = (e-=3);

		_pos->_cdata_content = true;
	} else {
		 // search for content end leaving only white space for _end_leading
		for(p=e; p>s; --p)
			if (!isspace((unsigned char)p[-1]))
				break;

		_pos->_cdata_content = false;
	}

	if (p != s) {
		if (_pos->_children.empty())	// no children in current node?
			_pos->_content.append(s, p-s);
		else if (_last_tag == TAG_START)
			_pos->_content.append(s, p-s);
		else
			_pos->_children.back()->_trailing.append(s, p-s);
	}

	if (p != e)
		_pos->_end_leading.assign(p, e-p);

	_pos.back();

	_last_tag = TAG_END;
	_content.erase();
}

#if defined(XS_USE_XERCES) || defined(XS_USE_EXPAT)
 /// store content, white space and comments
void XMLReaderBase::DefaultHandler(const XML_Char* s, int len)
{
#if defined(XML_UNICODE) || defined(XS_USE_XERCES)
	_content.append(String_from_XML_Char(s, len));
#else
	_content.append(s, len);
#endif
}
#endif


XS_String XMLWriter::s_empty_attr;

void XMLWriter::create(const XS_String& name)
{
	if (!_stack.empty()) {
		StackEntry& last = _stack.top();

		if (last._state < PRE_CLOSED) {
			write_attributes(last);
			close_pre(last);
		}

		++last._children;
	}

	StackEntry entry;
	entry._node_name = name;
	_stack.push(entry);

	write_pre(entry);
}

bool XMLWriter::back()
{
	if (!_stack.empty()) {
		write_post(_stack.top());

		_stack.pop();
		return true;
	} else {
		assert(!_stack.empty());
		return false;
	}
}

void XMLWriter::close_pre(StackEntry& entry)
{
	_out << '>';

	entry._state = PRE_CLOSED;
}

void XMLWriter::write_pre(StackEntry& entry)
{
	if (_format._pretty >= PRETTY_LINEFEED)
		_out << _format._endl;

	if (_format._pretty == PRETTY_INDENT) {
		for(size_t i=_stack.size(); --i>0; )
			_out << XML_INDENT_SPACE;
	}

	_out << '<' << EncodeXMLString(entry._node_name);
	//entry._state = PRE;
}

void XMLWriter::write_attributes(StackEntry& entry)
{
	for(AttrMap::const_iterator it=entry._attributes.begin(); it!=entry._attributes.end(); ++it)
		_out << ' ' << EncodeXMLString(it->first) << "=\"" << EncodeXMLString(it->second) << "\"";

	entry._state = ATTRIBUTES;
}

void XMLWriter::write_post(StackEntry& entry)
{
	if (entry._state < ATTRIBUTES)
		write_attributes(entry);

	if (entry._children || !entry._content.empty()) {
		if (entry._state < PRE_CLOSED)
			close_pre(entry);

		_out << entry._content;
		//entry._state = CONTENT;

		if (_format._pretty>=PRETTY_LINEFEED && entry._content.empty())
			_out << _format._endl;

		if (_format._pretty==PRETTY_INDENT && entry._content.empty()) {
			for(size_t i=_stack.size(); --i>0; )
				_out << XML_INDENT_SPACE;
		}

		_out << "</" << EncodeXMLString(entry._node_name) << ">";
	} else {
		_out << "/>";
	}

	entry._state = POST;
}

void XMLWriter::write(const std::string& s)
{
	if (!_stack.empty()) {
		StackEntry& last = _stack.top();

		if (last._state < PRE_CLOSED) {
			write_attributes(last);
			close_pre(last);
		}
	}

	_out.write(s.c_str(), s.length());
}


 /// XPath find function
bool XMLStreamReader::find_relative(const XPath& xpath)
{
	for(XPath::const_iterator it=xpath.begin(); it!=xpath.end(); ++it) {
		const XPathElement& xelem = *it;

		if (!find(xelem))
			return false;
	}

	return true;
}

bool XMLStreamReader::find_node(LPCXSSTR target_tag, int target_level, int min_level, bool stop_on_others)
{
	bool found = false;

	for(; _state_rdr.has_next(); _state_rdr.next()) {
		switch(_state_rdr._state) {
			case XMLStateReader::XSS_START_ELEMENT: {
				 // stop parsing if we found the target node and its content is already processed.
				if (found)
					goto break_parsingLoop;

				 // check for match in target level and eventually node name
				if (_level+1 == target_level) {
					if (target_tag==NULL || _state_rdr._node_name==target_tag)
						found = true;
					else if (stop_on_others)
						return false;
				}

				StartElementHandler(_state_rdr._node_name, _state_rdr._attrs);
				break;}

			case XMLStateReader::XSS_ELEMENT:
				 // stop parsing if we found the target node and its content is already processed.
				if (found)
					goto break_parsingLoop;

				 // check for match in target level and eventually node name
				if (_level+1 == target_level) {
					if (target_tag==NULL || _state_rdr._node_name==target_tag)
						found = true;
					else if (stop_on_others)
						return false;
				}

				StartElementHandler(_state_rdr._node_name, _state_rdr._attrs);
				// fall through...

			case XMLStateReader::XSS_END_ELEMENT: {
				 // stop parsing if we found the target node and its content is already processed.
				if (found)
					goto break_parsingLoop;

				 // stop before leaving the targeted level range; used by next_stop_on_others()
				if (_level == min_level)
					return false;

				EndElementHandler();

				 // stop searching if the current level leaves the target range; used by back()
				if (_level < min_level) {
					_state_rdr.next();
					return false;
				}
				break;}

			case XMLStateReader::XSS_CHARACTERS:
//			case XMLStateReader::XSS_COMMENT:
//			case XMLStateReader::XSS_SPACE:
//			case XMLStateReader::XSS_CDATA:
				DefaultHandler(_state_rdr.get_encoded_content());
				break;

//			case XMLStateReader::XSS_PROCESSING_INSTRUCTION:
//				XmlDeclHandler();
//				break;
		}
	} break_parsingLoop:

	if (found) {
		_node.set_encoded_content(_content);
		_content.erase();
		return true;
	} else
		return false;
}

void XMLStreamReader::StartElementHandler(const XS_String& name, const XMLNode::AttributeMap& attr)
{
	_node.assign(name);
	_node.get_attributes() = attr;
	_content.erase();

	++_level;
}

void XMLStreamReader::EndElementHandler()
{
	_node.clear();
	_node.get_attributes().clear();
	_content.erase();

	--_level;
}

void XMLStreamReader::DefaultHandler(const std::string& s)
{
	_content.append(EncodeXMLString(s));
}


bool XMLStateReader::has_next()
{
	while(_state == XSS_NONE)
		if (!_parse_ctx.proceed())
			break;

	return _state != XSS_NONE;
}

void XMLStateReader::next()
{
	_state = XSS_NONE;

	_parse_ctx.proceed();
}

void XMLStateReader::StartElementHandler(const XS_String& name, const XMLNode::AttributeMap& attr)
{
	_node_name = name;
	_attrs = attr;
	_content.erase();

	assert(_state==XSS_NONE);
	_state = XSS_START_ELEMENT;
}

void XMLStateReader::EndElementHandler()
{
//	_node_name.erase();
//	_attrs.clear();
	_content.erase();

	if (_state == XSS_START_ELEMENT)
		_state = XSS_ELEMENT;
	else {
		assert(_state==XSS_NONE);
		_state = XSS_END_ELEMENT;
	}
}

void XMLStateReader::DefaultHandler(const std::string& s)
{
	_content.append(EncodeXMLString(s));

	assert(_state==XSS_NONE);
	_state = XSS_CHARACTERS;
}

/*@@
void XMLStateReader::XmlDeclHandler(const char* version, const char* encoding, int standalone)
{
	_instructions.append("<?").append(_state_rdr.getPITarget())
		.append(" ").append(_state_rdr.getPIData()).append("?>");
}
*/


}	// namespace XMLStorage
