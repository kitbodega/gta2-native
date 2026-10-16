// wincompat_registry.cpp - emulacion del registro de Windows (arte, no emulador):
// guarda el arbol SOFTWARE\DMA Design Ltd\GTA2\* en fichero persistente bajo storageDir.
#include "windows.h"
#include <types.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>
#include <pthread.h>

const char* G2Platform_GetStorageDir(void);
void G2Platform_SetStorageDir(const char* dir);

namespace
{

struct RegValue
{
    DWORD type = REG_DWORD;
    std::vector<unsigned char> data;
};

typedef std::map<std::string, RegValue> ValMap;
typedef std::map<std::string, ValMap> KeyMap;

pthread_mutex_t g_regLock = PTHREAD_MUTEX_INITIALIZER;
bool g_regLoaded = false;
KeyMap g_tree; // "SOFTWARE\DMA Design Ltd\GTA2\Sound" -> valores

std::string normPath(const char* p)
{
    std::string s = p ? p : "";
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '\\') s[i] = '/';
    }
    if (!s.empty() && s.back() == '/')
        s.pop_back();
    return s;
}

void regLoadOnce()
{
    if (g_regLoaded) return;
    g_regLoaded = true;
    char path[600];
    snprintf(path, sizeof(path), "%s/gta2_registry.bin", G2Platform_GetStorageDir());
    FILE* f = fopen(path, "rb");
    if (!f) return;
    unsigned short pathLen = 0, nameLen = 0;
    while (fread(&pathLen, 2, 1, f) == 1 &&
           fread(&nameLen, 2, 1, f) == 1)
    {
        std::string k(pathLen, '\0');
        std::string n(nameLen, '\0');
        if (fread(&k[0], 1, pathLen, f) != pathLen) break;
        if (fread(&n[0], 1, nameLen, f) != nameLen) break;
        unsigned int type = 0, len = 0;
        if (fread(&type, 4, 1, f) != 1) break;
        if (fread(&len, 4, 1, f) != 1) break;
        RegValue rv;
        rv.type = type;
        rv.data.resize(len);
        if (len && fread(&rv.data[0], 1, len, f) != len) break;
        g_tree[normPath(k.c_str())][n] = rv;
    }
    fclose(f);
}

void regSave()
{
    char path[600];
    snprintf(path, sizeof(path), "%s/gta2_registry.bin", G2Platform_GetStorageDir());
    char tmp[620];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE* f = fopen(tmp, "wb");
    if (!f) return;
    for (KeyMap::const_iterator it = g_tree.begin(); it != g_tree.end(); ++it)
    {
        for (ValMap::const_iterator v = it->second.begin(); v != it->second.end(); ++v)
        {
            unsigned short pl = (unsigned short)it->first.size();
            unsigned short nl = (unsigned short)v->first.size();
            unsigned int type = v->second.type;
            unsigned int len = (unsigned int)v->second.data.size();
            fwrite(&pl, 2, 1, f);
            fwrite(&nl, 2, 1, f);
            fwrite(it->first.c_str(), 1, pl, f);
            fwrite(v->first.c_str(), 1, nl, f);
            fwrite(&type, 4, 1, f);
            fwrite(&len, 4, 1, f);
            if (len) fwrite(&v->second.data[0], 1, len, f);
        }
    }
    fclose(f);
    remove(path);
    rename(tmp, path);
}

} // namespace

extern "C" {

long RegOpenKeyExA(HKEY hKey, const char* lpSubKey, DWORD ulOptions,
                   DWORD samDesired, PHKEY phkResult)
{
    (void)hKey; (void)ulOptions; (void)samDesired;
    if (!lpSubKey || !phkResult) return ERROR_INVALID_PARAMETER;
    pthread_mutex_lock(&g_regLock);
    regLoadOnce();
    KeyMap::const_iterator it = g_tree.find(normPath(lpSubKey));
    if (it == g_tree.end())
    {
        pthread_mutex_unlock(&g_regLock);
        return ERROR_FILE_NOT_FOUND;
    }
    *phkResult = (HKEY)(uintptr_t)(it->first.c_str());
    (void)phkResult;
    pthread_mutex_unlock(&g_regLock);
    return ERROR_SUCCESS;
}

long RegOpenKeyA(HKEY hKey, const char* lpSubKey, PHKEY phkResult)
{
    return RegOpenKeyExA(hKey, lpSubKey, 0, 0x20019u, phkResult);
}

long RegCreateKeyExA(HKEY hKey, const char* lpSubKey, DWORD Reserved, char* lpClass,
                     DWORD dwOptions, DWORD samDesired, void* lpSecurityAttributes,
                     PHKEY phkResult, DWORD* lpdwDisposition)
{
    (void)hKey; (void)Reserved; (void)lpClass; (void)dwOptions; (void)samDesired;
    (void)lpSecurityAttributes; (void)lpdwDisposition;
    if (!lpSubKey || !phkResult) return ERROR_INVALID_PARAMETER;
    pthread_mutex_lock(&g_regLock);
    regLoadOnce();
    g_tree[normPath(lpSubKey)];
    *phkResult = (HKEY)(uintptr_t)(normPath(lpSubKey).c_str());
    pthread_mutex_unlock(&g_regLock);
    return ERROR_SUCCESS;
}

long RegQueryValueExA(HKEY hKey, const char* lpValueName, DWORD* lpReserved,
                      DWORD* lpType, unsigned char* lpData, DWORD* lpcbData)
{
    (void)lpReserved;
    if (!hKey || !lpValueName) return ERROR_INVALID_PARAMETER;
    pthread_mutex_lock(&g_regLock);
    regLoadOnce();
    std::string key = normPath((const char*)hKey);
    KeyMap::const_iterator it = g_tree.find(key);
    if (it == g_tree.end())
    {
        pthread_mutex_unlock(&g_regLock);
        return ERROR_FILE_NOT_FOUND;
    }
    ValMap::const_iterator v = it->second.find(lpValueName);
    if (v == it->second.end())
    {
        pthread_mutex_unlock(&g_regLock);
        return ERROR_FILE_NOT_FOUND;
    }
    if (lpType) *lpType = v->second.type;
    if (lpcbData)
    {
        if (lpData && *lpcbData >= v->second.data.size())
            memcpy(lpData, v->second.data.data(), v->second.data.size());
        *lpcbData = (unsigned long)v->second.data.size();
    }
    pthread_mutex_unlock(&g_regLock);
    return ERROR_SUCCESS;
}

long RegSetValueExA(HKEY hKey, const char* lpValueName, DWORD Reserved,
                    DWORD dwType, const unsigned char* lpData, DWORD cbData)
{
    (void)Reserved;
    if (!hKey || !lpValueName) return ERROR_INVALID_PARAMETER;
    pthread_mutex_lock(&g_regLock);
    regLoadOnce();
    std::string key = normPath((const char*)hKey);
    RegValue& rv = g_tree[key][lpValueName];
    rv.type = dwType;
    rv.data.assign(lpData, lpData + cbData);
    regSave();
    pthread_mutex_unlock(&g_regLock);
    return ERROR_SUCCESS;
}

long RegDeleteValueA(HKEY hKey, const char* lpValueName)
{
    if (!hKey || !lpValueName) return ERROR_INVALID_PARAMETER;
    pthread_mutex_lock(&g_regLock);
    regLoadOnce();
    std::string key = normPath((const char*)hKey);
    KeyMap::iterator it = g_tree.find(key);
    if (it == g_tree.end()) { pthread_mutex_unlock(&g_regLock); return ERROR_FILE_NOT_FOUND; }
    ValMap::iterator v = it->second.find(lpValueName);
    if (v == it->second.end()) { pthread_mutex_unlock(&g_regLock); return ERROR_FILE_NOT_FOUND; }
    it->second.erase(v);
    regSave();
    pthread_mutex_unlock(&g_regLock);
    return ERROR_SUCCESS;
}

long RegDeleteKeyA(HKEY hKey, const char* lpSubKey)
{
    (void)hKey;
    if (!lpSubKey) return ERROR_INVALID_PARAMETER;
    pthread_mutex_lock(&g_regLock);
    regLoadOnce();
    g_tree.erase(normPath(lpSubKey));
    regSave();
    pthread_mutex_unlock(&g_regLock);
    return ERROR_SUCCESS;
}

long RegCloseKey(HKEY hKey) { (void)hKey; return ERROR_SUCCESS; }

void G2Platform_DeleteRegistrySubtree(const char* path)
{
    if (!path) return;
    pthread_mutex_lock(&g_regLock);
    regLoadOnce();
    g_tree.erase(normPath(path));
    regSave();
    pthread_mutex_unlock(&g_regLock);
}

void G2Platform_SetDebugString(const char* valueName, const char* value)
{
    if (!valueName || !value) return;
    std::string key = "SOFTWARE/DMA Design Ltd/GTA2/Debug";
    pthread_mutex_lock(&g_regLock);
    regLoadOnce();
    RegValue& rv = g_tree[key][valueName];
    rv.type = REG_SZ;
    rv.data.assign(value, value + strlen(value) + 1);
    regSave();
    pthread_mutex_unlock(&g_regLock);
}

void G2Platform_SetDebugDword(const char* valueName, unsigned long value)
{
    if (!valueName) return;
    std::string key = "SOFTWARE/DMA Design Ltd/GTA2/Debug";
    pthread_mutex_lock(&g_regLock);
    regLoadOnce();
    RegValue& rv = g_tree[key][valueName];
    rv.type = REG_DWORD;
    rv.data.assign((const unsigned char*)&value, (const unsigned char*)&value + sizeof(value));
    regSave();
    pthread_mutex_unlock(&g_regLock);
}

} // extern "C"