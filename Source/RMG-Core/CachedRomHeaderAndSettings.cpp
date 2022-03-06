/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "CachedRomHeaderAndSettings.hpp"
#include "Directories.hpp"
#include "Error.hpp"

#include "osal/osal_files.hpp"

#include <cstring>
#include <vector>
#include <fstream>

//
// Local Defines
//

#define MAX_FILENAME_LEN 4096
#define ROMHEADER_NAME_LEN 20
#define GOODNAME_LEN 256
#define MD5_LEN 33

#define CACHE_FILE_MAGIC "RMGCoreHeaderAndSettingsCache_01"

//
// Local Structures
//

struct l_CacheEntry
{
    std::string          fileName;
    osal_files_file_time fileTime;

    CoreRomHeader   header;
    CoreRomSettings settings;
};

//
// Local Variables
//

static bool                      l_HasReadCache = false;
static std::vector<l_CacheEntry> l_CacheEntries;

//
// Internal Functions
//

static std::string get_cache_file_name()
{
    std::string file;

    file = CoreGetUserCacheDirectory();
    file += OSAL_FILES_DIR_SEPERATOR_STR;
    file += "RomHeaderAndSettingsCache.cache";

    return file;
}

static std::vector<l_CacheEntry>::iterator get_cache_entry_iter(std::string file, bool checkFileTime = true)
{
    osal_files_file_time fileTime = osal_files_get_file_time(file);

    auto predicate = [file, fileTime, checkFileTime](const auto& entry)
    {
        return entry.fileName == file &&
                (!checkFileTime || entry.fileTime == fileTime);
    };

    return std::find_if(l_CacheEntries.begin(), l_CacheEntries.end(), predicate);
}

//
// Exported Functions
//

void CoreReadRomHeaderAndSettingsCache(void)
{
    std::ifstream inputStream;
    char magicBuf[sizeof(CACHE_FILE_MAGIC)];
    char fileNameBuf[MAX_FILENAME_LEN];
    char headerNameBuf[ROMHEADER_NAME_LEN];
    char goodNameBuf[GOODNAME_LEN];
    char md5Buf[MD5_LEN];
    l_CacheEntry cacheEntry;

    inputStream.open(get_cache_file_name());
    if (!inputStream.good())
    {
        return;
    }

    // when magic doesn't match, don't read cache file
    inputStream.read((char*)magicBuf, sizeof(CACHE_FILE_MAGIC));
    if (std::string(magicBuf) != std::string(CACHE_FILE_MAGIC))
    {
        inputStream.close();
        return;
    }

    // read all file entries
#define FREAD(x) inputStream.read((char*)&x, sizeof(x))
    while (!inputStream.eof())
    {
        // file info
        inputStream.read((char*)fileNameBuf, sizeof(fileNameBuf));
        cacheEntry.fileName = std::string(fileNameBuf);
        FREAD(cacheEntry.fileTime);
        // header
        inputStream.read((char*)headerNameBuf, sizeof(headerNameBuf));
        cacheEntry.header.Name = std::string(headerNameBuf);
        FREAD(cacheEntry.header.CRC1);
        FREAD(cacheEntry.header.CRC2);
        // (partial) settings
        inputStream.read((char*)goodNameBuf, sizeof(goodNameBuf));
        cacheEntry.settings.GoodName = std::string(goodNameBuf);
        inputStream.read((char*)md5Buf, sizeof(md5Buf));
        cacheEntry.settings.MD5 = std::string(md5Buf);

        // add to cached entries
        l_CacheEntries.push_back(cacheEntry);
    }
#undef FREAD

    inputStream.close();
}

bool CoreSaveRomHeaderAndSettingsCache(void)
{
    std::ofstream outputStream;
    char fileNameBuf[MAX_FILENAME_LEN];
    char headerNameBuf[ROMHEADER_NAME_LEN];
    char goodNameBuf[GOODNAME_LEN];
    char md5Buf[MD5_LEN];

    outputStream.open(get_cache_file_name());
    if (!outputStream.good())
    {
        return false;
    }

    // write magic header
    outputStream.write((char*)CACHE_FILE_MAGIC, sizeof(CACHE_FILE_MAGIC));

    // write each entry in the file
#define FWRITE(x) outputStream.write((char*)&x, sizeof(x))
    for (const auto& entry : l_CacheEntries)
    {
        strncpy(fileNameBuf, entry.fileName.c_str(), MAX_FILENAME_LEN);
        strncpy(headerNameBuf, entry.header.Name.c_str(), sizeof(headerNameBuf));
        strncpy(goodNameBuf, entry.settings.GoodName.c_str(), sizeof(goodNameBuf));
        strncpy(md5Buf, entry.settings.MD5.c_str(), sizeof(md5Buf));

        // file info
        outputStream.write((char*)fileNameBuf, sizeof(fileNameBuf));
        FWRITE(entry.fileTime);
        // header
        outputStream.write((char*)headerNameBuf, sizeof(headerNameBuf));
        FWRITE(entry.header.CRC1);
        FWRITE(entry.header.CRC2);
        // (partial) settings
        outputStream.write((char*)goodNameBuf, sizeof(goodNameBuf));
        outputStream.write((char*)md5Buf, sizeof(md5Buf));
    }
#undef FWRITE

    outputStream.close();
    return true;
}

bool CoreHasRomHeaderAndSettingsCached(std::string file)
{
    return get_cache_entry_iter(file) != l_CacheEntries.end();
}

bool CoreGetCachedRomHeaderAndSettings(std::string file, CoreRomHeader& header, CoreRomSettings& settings)
{
    auto iter = get_cache_entry_iter(file);
    if (iter == l_CacheEntries.end())
    {
        return false;
    }

    header   = (*iter).header;
    settings = (*iter).settings;
    return true;
}

bool CoreAddCachedRomHeaderAndSettings(std::string file, CoreRomHeader header, CoreRomSettings settings)
{
    l_CacheEntry cacheEntry;

    // try to find existing entry with same filename,
    // when found, remove it from the cache
    auto iter = get_cache_entry_iter(file, false);
    if (iter != l_CacheEntries.end())
    {
        l_CacheEntries.erase(iter);
    }

    cacheEntry.fileName = file;
    cacheEntry.fileTime = osal_files_get_file_time(file);
    cacheEntry.header   = header;
    cacheEntry.settings = settings;

    l_CacheEntries.push_back(cacheEntry);
    return true;
}