﻿#include "GLibrary.h"
#include "net/socket_include.h"
#include "Utility.h"
#if !defined _WIN32 && !defined _WIN64
#include <dlfcn.h>
#endif //!defined _WIN32 && !defined _WIN64

using namespace std;

GLibrary::GLibrary(const string &nameLib, const string &path)
: m_module(NULL)
{
    Load(nameLib, path);
}

GLibrary::~GLibrary()
{
    Unload();
}

const string & GLibrary::GetPath() const
{
    return m_strPath;
}

bool GLibrary::Load(const string &nameLib, const string &path)
{
    m_strPath.clear();
    if (nameLib.size() <= 0)
        return false;

    string file = path;
    char last = 0;
    if (path.size() > 0)
        last = *(--path.end());
#if defined _WIN32 || defined _WIN64
    if (last != '\\' && last != '/')
        file += "/";

    file += nameLib + ".dll";
    m_module = LoadLibraryA(file.c_str());
#else
    if (last != '/')
        file += "/";

    file += "lib" + nameLib + ".so";
    m_module = dlopen(file.c_str(), RTLD_NOW);
#endif
    if (NULL == m_module)
        fprintf(stderr, "Load library fail!\n");
    else
        m_strPath = file;
    return m_module != NULL;
}

void GLibrary::Unload()
{
    if (!m_module)
        return;

#if defined _WIN32 || defined _WIN64
    FreeLibrary((HMODULE)m_module);
#else
    dlclose(m_module);
#endif
    m_module = NULL;
}

void *GLibrary::LoadSymbol(const std::string &symbol)
{
    if (!m_module)
        return NULL;

#if defined _WIN32 || defined _WIN64
    return GetProcAddress((HMODULE)m_module, symbol.c_str());
#else
    return dlsym(m_module, symbol.c_str());
#endif
}

string GLibrary::CurrentPath()
{
    return Utility::ModuleDirectory();
}
