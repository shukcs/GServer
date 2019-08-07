#ifndef _SOCKETS_Parse_H
#define _SOCKETS_Parse_H

#include "stdconfig.h"
#ifdef _MSC_VER
#pragma warning(disable:4514)
#endif

#include <string>

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif


/***************************************************/
/* interface of class Parse */

/** Splits a string whatever way you want.
	\ingroup util */
class Parse
{
public:
	Parse();
	Parse(const std::string&);
	Parse(const std::string&,const std::string&);
	Parse(const std::string&,const std::string&,short);
	~Parse();
	short issplit(const char);
	void getsplit();
	void getsplit(std::string&);
	std::string getword();
	void getword(std::string&);
	void getword(std::string&,std::string&,int);
	std::string getrest();
	void getrest(std::string&);
	long getvalue();
	void setbreak(const char);
	int getwordlen();
	int getrestlen();
	void enablebreak(const char c) {
		pa_enable = c;
	}
	void disablebreak(const char c) {
		pa_disable = c;
	}
	void getline();
	void getline(std::string&);
	size_t getptr() { return pa_the_ptr; }
	void EnableQuote(bool b) { pa_quote = b; }

private:
	std::string pa_the_str;
	std::string pa_splits;
	std::string pa_ord;
	size_t   pa_the_ptr;
	char  pa_breakchar;
	char  pa_enable;
	char  pa_disable;
	short pa_nospace;
	bool  pa_quote;
};


#ifdef SOCKETS_NAMESPACE
}
#endif

#endif // _SOCKETS_Parse_H

