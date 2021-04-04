#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include "registry.h"

#define GC_REGISTRY_NAME "Software\\GuiCheckers"
#define HASHSIZE_NAME "HashSize"
#define USE_BOOK_NAME "UseBook"
#define DBPATH_NAME "DBPath"
#define ENABLE_WLD_NAME "enable_wld"
#define DBMBYTES_NAME "dbmbytes"
#define MAX_DBPIECES_NAME "max_dbpieces"


/*
 * Set an integer value in the registry.
 */
void reg_set_int(char *name, int val)
{
	HKEY hKey;
	unsigned long result;
	int stat;
 
	/* Open registry key. */
	stat = RegCreateKeyEx(HKEY_CURRENT_USER, GC_REGISTRY_NAME,
			0, "gc_key", 0, KEY_WRITE, NULL, &hKey, &result);

	stat = RegSetValueEx(hKey, name, 0, REG_DWORD, (LPBYTE)&val, sizeof(DWORD));

	/* Close registry key. */
	stat = RegCloseKey(hKey);
}


/*
 * Get and integer value from the registry.
 * Return non-zero if key not found or other error.
 */
int reg_get_int(char *name, int *val)
{
	HKEY hKey;
	unsigned long datatype = REG_DWORD;
	DWORD buffersize = sizeof(DWORD);
	int stat;
 
	/* Open registry key. */
	stat = RegOpenKeyEx(HKEY_CURRENT_USER, GC_REGISTRY_NAME, 0, KEY_READ, &hKey);
	if (stat)
		return(stat);

	stat = RegQueryValueEx(hKey, name, 0, &datatype, (LPBYTE)val, &buffersize);

	/* Close registry key. */
	RegCloseKey(hKey);

	return(stat);
}


/*
 * Set a string value in the registry.
 */
void reg_set_string(char *name, char *string)
{
	HKEY hKey;
	unsigned long result;
	DWORD stat;
 
	/* Open registry key. */
	stat = RegCreateKeyEx(HKEY_CURRENT_USER, GC_REGISTRY_NAME,
			0, "gc_key", 0, KEY_WRITE, NULL, &hKey, &result);

	stat = RegSetValueEx(hKey, name, 0, REG_SZ, (LPBYTE)string, (DWORD)(strlen(string) + 1));

	/* Close registry key. */
	stat = RegCloseKey(hKey);
}


/*
 * Get a string value from the registry.
 * Return non-zero if key not found or other error.
 */
int reg_get_string(char *name, char *string, int maxlen)
{
	HKEY hKey;
	unsigned long datatype = REG_SZ;
	DWORD buffersize = maxlen;
	int stat;
 
	/* Open registry key. */
	stat = RegOpenKeyEx(HKEY_CURRENT_USER, GC_REGISTRY_NAME, 0, KEY_READ, &hKey);
	if (stat)
		return(stat);

	stat = RegQueryValueEx(hKey, name, 0, &datatype, (LPBYTE)string, &buffersize);

	/* Close registry key. */
	RegCloseKey(hKey);

	return(stat);
}


/*
 * Save the hashtable size in the registry.
 */
void save_hashsize(int val)
{
	reg_set_int(HASHSIZE_NAME, val);
}


void save_book_setting(int val)
{
	reg_set_int(USE_BOOK_NAME, val);
}


void save_dbpath(char *path)
{
	reg_set_string(DBPATH_NAME, path);
}
		

void save_enable_wld(int enable_wld)
{
	reg_set_int(ENABLE_WLD_NAME, enable_wld);
}


/*
 * Save the dbmbytes size in the registry.
 */
void save_dbmbytes(int val)
{
	reg_set_int(DBMBYTES_NAME, val);
}


void save_max_dbpieces(int val)
{
	reg_set_int(MAX_DBPIECES_NAME, val);
}


/*
 * Get the hashtable size from the registry.
 * Return non-zero if key not found or other error.
 */
int get_hashsize(int *size)
{
	return(reg_get_int(HASHSIZE_NAME, size));
}


int get_book_setting(int *setting)
{
	return(reg_get_int(USE_BOOK_NAME, setting));
}


int get_dbpath(char *path, int maxlen)
{
	return(reg_get_string(DBPATH_NAME, path, maxlen));
}


int get_enable_wld(int *enable_wld)
{
	return(reg_get_int(ENABLE_WLD_NAME, enable_wld));
}


/*
 * Get the dbmbytes size from the registry.
 * Return non-zero if key not found or other error.
 */
int get_dbmbytes(int *size)
{
	return(reg_get_int(DBMBYTES_NAME, size));
}


int get_max_dbpieces(int *size)
{
	return(reg_get_int(MAX_DBPIECES_NAME, size));
}


