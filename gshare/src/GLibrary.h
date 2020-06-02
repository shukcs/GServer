#ifndef __GLIBARRY_H__
#define __GLIBARRY_H__

#include "stdconfig.h"
#include <string>

class GLibrary
{
public:
    SHARED_DECL GLibrary(const std::string &nameLib="", const std::string &path="");
    SHARED_DECL ~GLibrary();

    SHARED_DECL const std::string &GetPath()const;
    SHARED_DECL bool Load(const std::string &nameLib, const std::string &path);
    SHARED_DECL void Unload();
    SHARED_DECL void *LoadSymbol(const std::string &symbol);
public:
    SHARED_DECL static std::string CurrentPath();
private:
    std::string     m_strPath;
    void            *m_module;
};

#endif //__GLIBARRY_H__