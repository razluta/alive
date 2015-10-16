#pragma once

#include <string>
#include <map>
#include <vector>
#include "SDL_types.h"

class FileSystem;


class GameData
{
public:
    GameData();
    ~GameData();

    bool Init(FileSystem& fs);

    struct FmvSection
    {
        std::string mPsxFileName;
        Uint32 mStartSector;
        Uint32 mNumberOfSectors;
    };  

    typedef std::map<std::string, std::vector<FmvSection>> FmvDb;

    struct PathEntry
    {
        Uint32 mPathChunkId;
        Uint32 mNumberOfCollisionItems;
        Uint32 mObjectIndexTableOffset;
        Uint32 mObjectDataOffset;
        Uint32 mMapXSize;
        Uint32 mMapYSize;
    };

    typedef std::map<std::string, std::vector<PathEntry>> PathDb;

    const FmvDb Fmvs() const
    {
        return mFmvDb;
    }

    const PathDb& Paths() const
    {
        return mPathDb;
    }

private:
    bool LoadFmvDb(FileSystem& fs);
    void AddPcToPsxFmvNameMappings(FileSystem& fs);
    bool LoadPathDb(FileSystem& fs);
private:

    FmvDb mFmvDb;
    PathDb mPathDb;
};
