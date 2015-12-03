#pragma once

#include <vector>
#include <memory>
#include <set>
#include "SDL.h"
#include "sdl_raii.hpp"
#include <string>

namespace Oddlib
{
    class LvlArchive;
    class IStream;
    /*
    * Typical animation file structure :
    * ResHeader
    * Offset to table of AnimSets
    * Max frame W
    * Max frame H
    * Palt size
    * Palt data
    * Frames(FrameHeader + data)
    * AnimSets - an array of offsets / pointers to FrameInfos
    * FrameInfos - offsets / pointers to FrameHeaders
    * EOF
    *
    * However AE data has an extra 0 DWORD before the palt size - in the PSX data
    * sometimes it isn't 0. In this case the format is that there is only one large
    * "sprite sheet" frame, and each frame infos is a sub rect of the larger image.
    *
    * Note: Because compression type 6 is used in AE PSX and AE PC, we have to pass in a flag to
    * know if the source data is PC or PSX. This is because the type 6 decompression is not the same between
    * PC and PSX, and there is no reliable way to detect which format we have.
    */
    class AnimSerializer
    {
    public:
        explicit AnimSerializer(const std::string& fileName, Uint32 id, IStream& stream, bool bIsPsx, const char* dataSetName);
    private:
        void ParseAnimationSets(IStream& stream);
        void ParseFrameInfoHeaders(IStream& stream);
        void GatherUniqueFrameOffsets();
        Uint32 DataSize(std::set<Uint32>::iterator it);
        void DebugDecodeAllFrames(IStream& stream);
        std::vector<Uint8> DecodeFrame(IStream& stream, Uint32 frameOffset, Uint32 frameDataSize);
        
        bool mIsSingleFrame = false;
        std::string mFileName;
        std::string mDataSetName;
        Uint32 mId = 0;
        bool mIsPsx = false;

        struct BanHeader
        {
            Uint16 mMaxW = 0;       // Max frame W
            Uint16 mMaxH = 0;       // Max frame H
            Uint32 mFrameTableOffSet = 0; // Where the frame table begins
            Uint32 mPaltSize = 0;         // Number of palt words

        };
        BanHeader mHeader;

        struct FrameInfoHeader;
        struct AnimationHeader
        {
            Uint16 mFps = 0;            // Seems to be 0x1 or 0x2
            Uint16 mNumFrames = 0;      // Number of frames in the set

            // If loop flag set then this is the frame to loop back to
            Uint16 mLoopStartFrame = 0;
            
            enum eFlags
            {
                eFlipXFlag = 0x4,
                eFlipYFlag = 0x8,
                eNeverUnload = 0x1,
                eLoopFlag = 0x2
            };
            Uint16 mFlags = 0;

            // Offset to each frame, can be duplicated across sets if two animations share the same frame
            std::vector< Uint32 > mFrameInfoOffsets;

            std::vector<std::unique_ptr<FrameInfoHeader>> mFrameInfos;
        };

        // Unique combination of frames from all animations, as each animation can reuse any number of frames
        std::set< Uint32 > mUniqueFrameHeaderOffsets;
        //std::set< Uint32 > mUniqueFrameHeaderStreamOffsets;

        struct FrameInfoHeader
        {
            Uint32 mFrameHeaderOffset = 0;
            Uint32 mMagic = 0;

            // TODO: Actually, number of points and triggers?
            //Uint16 points = 0;
            //Uint16 triggers = 0;

            // Top left
            Sint16 mColx = 0;
            Sint16 mColy = 0;

            // Bottom right
            Sint16 mColw = 0;
            Sint16 mColh = 0;

            Sint16 mOffx = 0;
            Sint16 mOffy = 0;

            //std::vector<Uint32> mTriggers;
        };

        struct FrameHeader
        {
            Uint32 mClutOffset;
            Uint8 mWidth;
            Uint8 mHeight;
            Uint8 mColourDepth;
            Uint8 mCompressionType;
            Uint32 mFrameDataSize; // Actually 2 Uint16's in AE for W/H again
        };
        static_assert(sizeof(FrameHeader) == 12, "Wrong frame header size");
        Uint32 mClutPos = 0;

        std::vector<std::unique_ptr<AnimationHeader>> mAnimationHeaders;
        std::vector<Uint16> mOriginalPalt;
        std::vector<Uint16> mPalt;

        bool mbIsAoFile = true;

        template<class T>
        std::vector<Uint8> Decompress(FrameHeader& header, IStream& stream, Uint32 finalW, Uint32 w, Uint32 h, Uint32 dataSize);

        // TODO: Put this stuff into its own object
        void BeginFrames(int w, int h, int count);
        void AddFrame(FrameHeader& header, Uint32 realWidth, const std::vector<Uint8>& decompressedData);
        void EndFrames();
        void DebugSaveFrame(FrameHeader& header, Uint32 realWidth, const std::vector<Uint8>& decompressedData);
        Uint16 GetPaltValue(Uint32 idx);
        SDL_SurfacePtr MakeFrame(FrameHeader& header, Uint32 realWidth, const std::vector<Uint8>& decompressedData, std::vector<Uint16>& pixels);
        SDL_SurfacePtr mSpriteSheet;
        int mSpritesX = 0;
        int mSpritesY = 0;
        int mXPos = 0;
        int mYPos = 0;
    };

    /*
    class AnimationFactory
    {
    public:
        static std::vector<std::unique_ptr<Animation>> Create(Oddlib::LvlArchive& archive, const std::string& fileName, Uint32 resourceId, bool bIsxPsx);
    };*/
}
