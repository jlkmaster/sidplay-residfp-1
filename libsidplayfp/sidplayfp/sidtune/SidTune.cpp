/*
 * /home/ms/files/source/libsidtune/RCS/SidTune.cpp,v
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define _SidTune_cpp_

#include "config.h"
#include "SidTuneCfg.h"
#include "sidplayfp/SidTune.h"
#include "SidTuneTools.h"
#include "sidplayfp/sidendian.h"
#include "utils/MD5/MD5.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif
#include <iostream>
#include <iomanip>
#include <string.h>
#include <limits.h>

#ifdef HAVE_IOS_OPENMODE
    typedef std::ios::openmode openmode;
#else
    typedef int openmode;
#endif


const char SidTune::txt_songNumberExceed[] = "SIDTUNE WARNING: Selected song number was too high";
const char SidTune::txt_empty[] = "SIDTUNE ERROR: No data to load";
const char SidTune::txt_unrecognizedFormat[] = "SIDTUNE ERROR: Could not determine file format";
const char SidTune::txt_noDataFile[] = "SIDTUNE ERROR: Did not find the corresponding data file";
const char SidTune::txt_notEnoughMemory[] = "SIDTUNE ERROR: Not enough free memory";
const char SidTune::txt_cantLoadFile[] = "SIDTUNE ERROR: Could not load input file";
const char SidTune::txt_cantOpenFile[] = "SIDTUNE ERROR: Could not open file for binary input";
const char SidTune::txt_fileTooLong[] = "SIDTUNE ERROR: Input data too long";
const char SidTune::txt_dataTooLong[] = "SIDTUNE ERROR: Size of music data exceeds C64 memory";
const char SidTune::txt_cantCreateFile[] = "SIDTUNE ERROR: Could not create output file";
const char SidTune::txt_fileIoError[] = "SIDTUNE ERROR: File I/O error";
const char SidTune::txt_VBI[] = "VBI";
const char SidTune::txt_CIA[] = "CIA 1 Timer A";
const char SidTune::txt_noErrors[] = "No errors";
const char SidTune::txt_na[] = "N/A";
const char SidTune::txt_badAddr[] = "SIDTUNE ERROR: Bad address data";
const char SidTune::txt_badReloc[] = "SIDTUNE ERROR: Bad reloc data";
const char SidTune::txt_corrupt[] = "SIDTUNE ERROR: File is incomplete or corrupt";

// Default sidtune file name extensions. This selection can be overriden
// by specifying a custom list in the constructor.
const char* defaultFileNameExt[] =
{
    // Preferred default file extension for single-file sidtunes
    // or sidtune description files in SIDPLAY INFOFILE format.
    ".sid", ".SID",
    // File extensions used (and created) by various C64 emulators and
    // related utilities. These extensions are recommended to be used as
    // a replacement for ".dat" in conjunction with two-file sidtunes.
    ".c64", ".prg", ".p00", ".C64", ".PRG", ".P00",
    // Stereo Sidplayer (.mus/.MUS ought not be included because
    // these must be loaded first; it sometimes contains the first
    // credit lines of a MUS/STR pair).
    ".str", ".STR", ".mus", ".MUS",
    // End.
    0
};

// Petscii to Ascii conversion table
static const char _sidtune_CHRtab[256] =  // CHR$ conversion table (0x01 = no output)
{
   0x0, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0xd, 0x1, 0x1,
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
  0x20,0x21, 0x1,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
  0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
  0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x24,0x5d,0x20,0x20,
  // alternative: CHR$(92=0x5c) => ISO Latin-1(0xa3)
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c,
  // 0x80-0xFF
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23,
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23
};

const char** SidTune::fileNameExtensions = defaultFileNameExt;

inline void SidTune::setFileNameExtensions(const char **fileNameExt)
{
    fileNameExtensions = ((fileNameExt!=0)?fileNameExt:defaultFileNameExt);
}

SidTune::SidTune(const char* fileName, const char **fileNameExt,
                 const bool separatorIsSlash)
{
    init();
    isSlashedFileName = separatorIsSlash;
    setFileNameExtensions(fileNameExt);
#if !defined(SIDTUNE_NO_STDIN_LOADER)
    // Filename ``-'' is used as a synonym for standard input.
    if ( fileName!=0 && (strcmp(fileName,"-")==0) )
    {
        getFromStdIn();
    }
    else
#endif
    if (fileName != 0)
    {
        getFromFiles(fileName);
    }
}

SidTune::SidTune(const uint_least8_t* data, const uint_least32_t dataLen)
{
    init();
    getFromBuffer(data,dataLen);
}

SidTune::~SidTune()
{
    cleanup();
}

bool SidTune::load(const char* fileName, const bool separatorIsSlash)
{
    cleanup();
    init();
    isSlashedFileName = separatorIsSlash;
#if !defined(SIDTUNE_NO_STDIN_LOADER)
    if ( strcmp(fileName,"-")==0 )
        getFromStdIn();
    else
#endif
    getFromFiles(fileName);
    return status;
}

bool SidTune::read(const uint_least8_t* data, uint_least32_t dataLen)
{
    cleanup();
    init();
    getFromBuffer(data,dataLen);
    return status;
}

const SidTuneInfo& SidTune::operator[](const uint_least16_t songNum)
{
    selectSong(songNum);
    return info;
}

void SidTune::getInfo(SidTuneInfo& outInfo)
{
    outInfo = info;  // copy
}

const SidTuneInfo& SidTune::getInfo()
{
    return info;
}

// First check, whether a song is valid. Then copy any song-specific
// variable information such a speed/clock setting to the info structure.
uint_least16_t SidTune::selectSong(const uint_least16_t selectedSong)
{
    if ( !status )
        return 0;
    else
        info.statusString = SidTune::txt_noErrors;

    uint_least16_t song = selectedSong;
    // Determine and set starting song number.
    if (selectedSong == 0)
        song = info.startSong;
    if (selectedSong>info.songs || selectedSong>SIDTUNE_MAX_SONGS)
    {
        song = info.startSong;
        info.statusString = SidTune::txt_songNumberExceed;
    }
    info.currentSong = song;
    info.songLength = 0;
    // Retrieve song speed definition.
    if (info.compatibility == SIDTUNE_COMPATIBILITY_R64)
        info.songSpeed = SIDTUNE_SPEED_CIA_1A;
    else if (info.compatibility == SIDTUNE_COMPATIBILITY_PSID)
    {   // This does not take into account the PlaySID bug upon evaluating the
        // SPEED field. It would most likely break compatibility to lots of
        // sidtunes, which have been converted from .SID format and vice versa.
        // The .SID format does the bit-wise/song-wise evaluation of the SPEED
        // value correctly, like it is described in the PlaySID documentation.
        info.songSpeed = songSpeed[(song-1)&31];
    }
    else
        info.songSpeed = songSpeed[song-1];
    info.clockSpeed = clockSpeed[song-1];
    // Assign song speed description string depending on clock speed.
    // Final speed description is available only after song init.
    if (info.songSpeed == SIDTUNE_SPEED_VBI)
        info.speedString = txt_VBI;
    else
        info.speedString = txt_CIA;
    return info.currentSong;
}

// ------------------------------------------------- private member functions

bool SidTune::placeSidTuneInC64mem(uint_least8_t* c64buf)
{
    if ( status && c64buf )
    {
        // The Basic ROM sets these values on loading a file.
        // Program end address
        const uint_least16_t start = info.loadAddr;
        const uint_least16_t end   = start + info.c64dataLen;
        endian_little16(c64buf + 0x2d, end); // Variables start
        endian_little16(c64buf + 0x2f, end); // Arrays start
        endian_little16(c64buf + 0x31, end); // Strings start
        endian_little16(c64buf + 0xac, start);
        endian_little16(c64buf + 0xae, end);

        // Copy data from cache to the correct destination.
        memcpy(c64buf+info.loadAddr, cache.get()+fileOffset, info.c64dataLen);
        info.statusString = SidTune::txt_noErrors;

        if (info.musPlayer)
        {
            MUS_installPlayer(c64buf);
        }
        return true;
    }
    return false;
}

bool SidTune::loadFile(const char* fileName, Buffer_sidtt<const uint_least8_t>& bufferRef)
{
    // This sucks big time
    openmode createAtrr = std::ios::in;
#ifdef HAVE_IOS_NOCREATE
    createAtrr |= std::ios::nocreate;
#endif
    // Open binary input file stream at end of file.
#if defined(HAVE_IOS_BIN)
    createAtrr |= std::ios::bin;
#else
    createAtrr |= std::ios::binary;
#endif

    std::fstream myIn(fileName, createAtrr);

    if ( !myIn.is_open() )
    {
        info.statusString = SidTune::txt_cantOpenFile;
        return false;
    }

    myIn.seekg(0, std::ios::end);
    const uint_least32_t fileLen = (uint_least32_t)myIn.tellg();

    if ( fileLen == 0 )
    {
        info.statusString = SidTune::txt_empty;
        return false;
    }

    Buffer_sidtt<const uint_least8_t> fileBuf;

#ifdef HAVE_EXCEPTIONS
    if ( !fileBuf.assign(new(std::nothrow) uint_least8_t[fileLen], fileLen) )
#else
    if ( !fileBuf.assign(new uint_least8_t[fileLen], fileLen) )
#endif
    {
        info.statusString = SidTune::txt_notEnoughMemory;
        return false;
    }

    myIn.seekg(0, std::ios::beg);

    myIn.read((char*)fileBuf.get(), fileLen);

    if ( myIn.bad() )
    {
        info.statusString = SidTune::txt_cantLoadFile;
        return false;
    }

    info.statusString = SidTune::txt_noErrors;

    myIn.close();

    bufferRef.assign(fileBuf.xferPtr(), fileBuf.xferLen());
    return true;
}

void SidTune::deleteFileNameCopies()
{
    // When will it be fully safe to call delete[](0) on every system?
    if ( info.dataFileName != 0 )
        delete[] info.dataFileName;
    if ( info.infoFileName != 0 )
        delete[] info.infoFileName;
    if ( info.path != 0 )
        delete[] info.path;
    info.dataFileName = 0;
    info.infoFileName = 0;
    info.path = 0;
}

void SidTune::init()
{
    // Initialize the object with some safe defaults.
    status = false;

    info.statusString = SidTune::txt_na;
    info.path = info.infoFileName = info.dataFileName = 0;
    info.dataFileLen = info.c64dataLen = 0;
    info.formatString = SidTune::txt_na;
    info.speedString = SidTune::txt_na;
    info.loadAddr = ( info.initAddr = ( info.playAddr = 0 ));
    info.songs = ( info.startSong = ( info.currentSong = 0 ));
    info.sidChipBase1 = 0xd400;
    info.sidChipBase2 = 0;
    info.musPlayer = false;
    info.fixLoad = false;
    info.songSpeed = SIDTUNE_SPEED_VBI;
    info.clockSpeed = SIDTUNE_CLOCK_UNKNOWN;
    info.sidModel1 = SIDTUNE_SIDMODEL_UNKNOWN;
    info.sidModel2 = SIDTUNE_SIDMODEL_UNKNOWN;
    info.compatibility = SIDTUNE_COMPATIBILITY_C64;
    info.songLength = 0;
    info.relocStartPage = 0;
    info.relocPages = 0;

    for ( uint_least16_t si = 0; si < SIDTUNE_MAX_SONGS; si++ )
    {
        songSpeed[si] = info.songSpeed;
        clockSpeed[si] = info.clockSpeed;
    }

    fileOffset = 0;
    musDataLen = 0;

    for ( uint_least16_t sNum = 0; sNum < SIDTUNE_MAX_CREDIT_STRINGS; sNum++ )
    {
        for ( uint_least16_t sPos = 0; sPos < SIDTUNE_MAX_CREDIT_STRLEN; sPos++ )
        {
            infoString[sNum][sPos] = 0;
        }
    }
    info.numberOfInfoStrings = 0;

    // Not used!!!
    info.numberOfCommentStrings = 1;
#ifdef HAVE_EXCEPTIONS
    info.commentString = new(std::nothrow) char* [info.numberOfCommentStrings];
#else
    info.commentString = new char* [info.numberOfCommentStrings];
#endif
    if (info.commentString != 0)
        info.commentString[0] = SidTuneTools::myStrDup("--- SAVED WITH SIDPLAY ---");
}

void SidTune::cleanup()
{
    // Remove copy of comment field.
    uint_least32_t strNum = 0;
    // Check and remove every available line.
    while (info.numberOfCommentStrings-- > 0)
    {
        if (info.commentString[strNum] != 0)
        {
            delete[] info.commentString[strNum];
            info.commentString[strNum] = 0;
        }
        strNum++;  // next string
    };
    delete[] info.commentString;  // free the array pointer

    deleteFileNameCopies();

    status = false;
}

#if !defined(SIDTUNE_NO_STDIN_LOADER)

void SidTune::getFromStdIn()
{
    // Assume a failure, so we can simply return.
    status = false;
    // Assume the memory allocation to fail.
    info.statusString = SidTune::txt_notEnoughMemory;
    uint_least8_t* fileBuf;
#ifdef HAVE_EXCEPTIONS
    if ( 0 == (fileBuf = new(std::nothrow) uint_least8_t[SIDTUNE_MAX_FILELEN]) )
#else
    if ( 0 == (fileBuf = new uint_least8_t[SIDTUNE_MAX_FILELEN]) )
#endif
    {
        return;
    }
    // We only read as much as fits in the buffer.
    // This way we avoid choking on huge data.
    uint_least32_t i = 0;
    char datb;
    while (std::cin.get(datb) && i<SIDTUNE_MAX_FILELEN)
        fileBuf[i++] = (uint_least8_t) datb;
    info.dataFileLen = i;
    getFromBuffer(fileBuf,info.dataFileLen);
    delete[] fileBuf;
}

#endif

void SidTune::getFromBuffer(const uint_least8_t* const buffer, const uint_least32_t bufferLen)
{
    // Assume a failure, so we can simply return.
    status = false;

    if (buffer==0 || bufferLen==0)
    {
        info.statusString = SidTune::txt_empty;
        return;
    }

    if (bufferLen > SIDTUNE_MAX_FILELEN)
    {
        info.statusString = SidTune::txt_fileTooLong;
        return;
    }

#ifdef HAVE_EXCEPTIONS
    uint_least8_t* tmpBuf = new(std::nothrow) uint_least8_t[bufferLen];
#else
    uint_least8_t* tmpBuf = new uint_least8_t[bufferLen];
#endif
    if ( tmpBuf == 0 )
    {
        info.statusString = SidTune::txt_notEnoughMemory;
        return;
    }
    memcpy(tmpBuf,buffer,bufferLen);

    Buffer_sidtt<const uint_least8_t> buf1(tmpBuf, bufferLen);

    bool foundFormat = false;
    // Here test for the possible single file formats. --------------
    LoadStatus ret = PSID_fileSupport( buf1 );
    if ( ret != LOAD_NOT_MINE )
    {
        if ( ret == LOAD_ERROR )
            return;
        foundFormat = true;
    }
    else
    {
        Buffer_sidtt<const uint_least8_t> buf2;  // empty
        ret = MUS_fileSupport(buf1,buf2);
        if ( ret != LOAD_NOT_MINE )
        {
            if ( ret == LOAD_ERROR )
                return;
            foundFormat = MUS_mergeParts(buf1,buf2);
        }
        else
        {
            // No further single-file-formats available.
            info.statusString = SidTune::txt_unrecognizedFormat;
        }
    }

    if ( foundFormat )
    {
        status = acceptSidTune("-","-",buf1);
    }
}

bool SidTune::acceptSidTune(const char* dataFileName, const char* infoFileName,
                            Buffer_sidtt<const uint_least8_t>& buf)
{
    // @FIXME@ - MUS
    if ( info.numberOfInfoStrings == 3 )
    {   // Add <?> (HVSC standard) to missing title, author, release fields
        for (int i = 0; i < 3; i++)
        {
            if (infoString[i][0] == '\0')
            {
                strcpy (&infoString[i][0], "<?>");
                info.infoString[i] = &infoString[i][0];
            }
        }
    }

    deleteFileNameCopies();
    // Make a copy of the data file name and path, if available.
    if ( dataFileName != 0 )
    {
        info.path = SidTuneTools::myStrDup(dataFileName);
        if (isSlashedFileName)
        {
            info.dataFileName = SidTuneTools::myStrDup(SidTuneTools::slashedFileNameWithoutPath(info.path));
            *SidTuneTools::slashedFileNameWithoutPath(info.path) = 0;  // path only
        }
        else
        {
            info.dataFileName = SidTuneTools::myStrDup(SidTuneTools::fileNameWithoutPath(info.path));
            *SidTuneTools::fileNameWithoutPath(info.path) = 0;  // path only
        }
        if ((info.path==0) || (info.dataFileName==0))
        {
            info.statusString = SidTune::txt_notEnoughMemory;
            return false;
        }
    }
    else
    {
        // Provide empty strings.
        info.path = SidTuneTools::myStrDup("");
        info.dataFileName = SidTuneTools::myStrDup("");
    }
    // Make a copy of the info file name, if available.
    if ( infoFileName != 0 )
    {
        char* tmp = SidTuneTools::myStrDup(infoFileName);
        if (isSlashedFileName)
            info.infoFileName = SidTuneTools::myStrDup(SidTuneTools::slashedFileNameWithoutPath(tmp));
        else
            info.infoFileName = SidTuneTools::myStrDup(SidTuneTools::fileNameWithoutPath(tmp));
        if ((tmp==0) || (info.infoFileName==0))
        {
            info.statusString = SidTune::txt_notEnoughMemory;
            return false;
        }
        delete[] tmp;
    }
    else
    {
        // Provide empty string.
        info.infoFileName = SidTuneTools::myStrDup("");
    }
    // Fix bad sidtune set up.
    if (info.songs > SIDTUNE_MAX_SONGS)
        info.songs = SIDTUNE_MAX_SONGS;
    else if (info.songs == 0)
        info.songs++;
    if (info.startSong > info.songs)
        info.startSong = 1;
    else if (info.startSong == 0)
        info.startSong++;

    if ( info.musPlayer )
        MUS_setPlayerAddress();

    info.dataFileLen = buf.len();
    info.c64dataLen = buf.len() - fileOffset;

    // Calculate any remaining addresses and then
    // confirm all the file details are correct
    if ( resolveAddrs(buf.get() + fileOffset) == false )
        return false;
    if ( checkRelocInfo() == false )
        return false;
    if ( checkCompatibility() == false )
        return false;

    if (info.dataFileLen >= 2)
    {
        // We only detect an offset of two. Some position independent
        // sidtunes contain a load address of 0xE000, but are loaded
        // to 0x0FFE and call player at 0x1000.
        info.fixLoad = (endian_little16(buf.get()+fileOffset)==(info.loadAddr+2));
    }

    // Check the size of the data.
    if ( info.c64dataLen > SIDTUNE_MAX_MEMORY )
    {
        info.statusString = SidTune::txt_dataTooLong;
        return false;
    }
    else if ( info.c64dataLen == 0 )
    {
        info.statusString = SidTune::txt_empty;
        return false;
    }

    cache.assign(buf.xferPtr(),buf.xferLen());

    info.statusString = SidTune::txt_noErrors;
    return true;
}

bool SidTune::createNewFileName(Buffer_sidtt<char>& destString,
                                const char* sourceName,
                                const char* sourceExt)
{
    Buffer_sidtt<char> newBuf;
    uint_least32_t newLen = strlen(sourceName)+strlen(sourceExt)+1;
    // Get enough memory, so we can appended the extension.
#ifdef HAVE_EXCEPTIONS
    newBuf.assign(new(std::nothrow) char[newLen],newLen);
#else
    newBuf.assign(new char[newLen],newLen);
#endif
    if ( newBuf.isEmpty() )
    {
        info.statusString = SidTune::txt_notEnoughMemory;
        return (status = false);
    }
    strcpy(newBuf.get(),sourceName);
    strcpy(SidTuneTools::fileExtOfPath(newBuf.get()),sourceExt);
    destString.assign(newBuf.xferPtr(),newBuf.xferLen());
    return true;
}

// Initializing the object based upon what we find in the specified file.

void SidTune::getFromFiles(const char* fileName)
{
    // Assume a failure, so we can simply return.
    status = false;

    Buffer_sidtt<const uint_least8_t> fileBuf1;

    if ( !loadFile(fileName,fileBuf1) )
        return;

    // File loaded. Now check if it is in a valid single-file-format.
    LoadStatus ret = PSID_fileSupport(fileBuf1);
    if (ret != LOAD_NOT_MINE)
    {
        if (ret == LOAD_OK)
            status = acceptSidTune(fileName,0,fileBuf1);
        return;
    }

// ---------------------------------- Support for multiple-files formats.
    Buffer_sidtt<const uint_least8_t> fileBuf2;

    // Try some native C64 file formats
    ret = MUS_fileSupport(fileBuf1,fileBuf2);
    if (ret != LOAD_NOT_MINE)
    {
        if (ret == LOAD_ERROR)
            return;

        // Try to find second file.
        Buffer_sidtt<char> fileName2;
        int n = 0;
        while (fileNameExtensions[n] != 0)
        {
            if ( !createNewFileName(fileName2,fileName,fileNameExtensions[n]) )
                return;
            // 1st data file was loaded into ``fileBuf1'',
            // so we load the 2nd one into ``fileBuf2''.
            // Do not load the first file again if names are equal.
            if ( MYSTRICMP(fileName,fileName2.get())!=0 &&
                loadFile(fileName2.get(),fileBuf2) )
            {
                // Check if tunes in wrong order and therefore swap them here
                if ( MYSTRICMP (fileNameExtensions[n], ".mus")==0 )
                {
                    if ( MUS_fileSupport(fileBuf2,fileBuf1) == LOAD_OK )
                    {
                        if ( MUS_mergeParts(fileBuf2,fileBuf1) )
                            status = acceptSidTune(fileName2.get(),fileName,
                                                fileBuf2);
                        return;
                    }
                }
                else
                {
                    if ( MUS_fileSupport(fileBuf1,fileBuf2) == LOAD_OK )
                    {
                        if ( MUS_mergeParts(fileBuf1,fileBuf2) )
                            status = acceptSidTune(fileName,fileName2.get(),
                                                fileBuf1);
                        return;
                    }
                }
                // The first tune loaded ok, so ignore errors on the
                // second tune, may find an ok one later
            }
            n++;
        };
        // No (suitable) second file, so reload first without second
        fileBuf2.erase();
        MUS_fileSupport(fileBuf1,fileBuf2);
        status = acceptSidTune(fileName,0,fileBuf1);
        return;
    }

    // Now directly support x00 (p00, etc)
    ret = X00_fileSupport(fileName,fileBuf1);
    if (ret != LOAD_NOT_MINE)
    {
        if (ret == LOAD_OK)
            status = acceptSidTune(fileName,0,fileBuf1);
        return;
    }

    // Now directly support prgs and equivalents
    ret = PRG_fileSupport(fileName,fileBuf1);
    if (ret != LOAD_NOT_MINE)
    {
        if (ret == LOAD_OK)
            status = acceptSidTune(fileName,0,fileBuf1);
        return;
    }

    info.statusString = SidTune::txt_unrecognizedFormat;
    return;
}

void SidTune::convertOldStyleSpeedToTables(uint_least32_t speed, int clock)
{
    // Create the speed/clock setting tables.
    //
    // This routine implements the PSIDv2NG compliant speed conversion.  All tunes
    // above 32 use the same song speed as tune 32
    const int toDo = ((info.songs <= SIDTUNE_MAX_SONGS) ? info.songs : SIDTUNE_MAX_SONGS);
    for (int s = 0; s < toDo; s++)
    {
        clockSpeed[s] = clock;
        if (speed & 1)
            songSpeed[s] = SIDTUNE_SPEED_CIA_1A;
        else
            songSpeed[s] = SIDTUNE_SPEED_VBI;
        if (s < 31)
            speed >>= 1;
    }
}

//
// File format conversion ---------------------------------------------------
//

bool SidTune::saveToOpenFile(std::ofstream& toFile, const uint_least8_t* buffer,
                             uint_least32_t bufLen )
{
    if ( !bufLen  )
        return false;

    toFile.write((char*)buffer, bufLen);

    if ( toFile.bad() )
    {
        info.statusString = SidTune::txt_fileIoError;
        return false;
    }
    else
    {
        info.statusString = SidTune::txt_noErrors;
        return true;
    }
}

bool SidTune::saveC64dataFile( const char* fileName, bool overWriteFlag )
{
    bool success = false;  // assume error
    // This prevents saving from a bad object.
    if ( status )
    {
        // Open binary output file stream.
        openmode createAttr = std::ios::out;
#if defined(HAVE_IOS_BIN)
        createAttr |= std::ios::bin;
#else
        createAttr |= std::ios::binary;
#endif
        if ( overWriteFlag )
            createAttr |= std::ios::trunc;
        else
            createAttr |= std::ios::app;
        std::ofstream fMyOut( fileName, createAttr );
        if ( !fMyOut || fMyOut.tellp()>0 )
        {
            info.statusString = SidTune::txt_cantCreateFile;
        }
        else
        {
            if ( !info.musPlayer )
            {
                // Save c64 lo/hi load address.
                uint_least8_t saveAddr[2];
                saveAddr[0] = info.loadAddr & 255;
                saveAddr[1] = info.loadAddr >> 8;
                fMyOut.write((char*)saveAddr,2);
            }

            // Data starts at: bufferaddr + fileOffset
            // Data length: info.dataFileLen - fileOffset
            if ( !saveToOpenFile( fMyOut,cache.get()+fileOffset, info.dataFileLen - fileOffset ) )
            {
                info.statusString = SidTune::txt_fileIoError;
            }
            else
            {
                info.statusString = SidTune::txt_noErrors;
                success = true;
            }
            fMyOut.close();
        }
    }
    return success;
}

bool SidTune::savePSIDfile( const char* fileName, bool overWriteFlag )
{
    bool success = false;  // assume error
    // This prevents saving from a bad object.
    if ( status )
    {
        // Open binary output file stream.
        openmode createAttr = std::ios::out;
#if defined(HAVE_IOS_BIN)
        createAttr |= std::ios::bin;
#else
        createAttr |= std::ios::binary;
#endif
      if ( overWriteFlag )
            createAttr |= std::ios::trunc;
        else
            createAttr |= std::ios::app;
        std::ofstream fMyOut( fileName, createAttr );
        if ( !fMyOut || fMyOut.tellp()>0 )
        {
            info.statusString = SidTune::txt_cantCreateFile;
        }
        else
        {
            if ( !PSID_fileSupportSave( fMyOut,cache.get() ) )
            {
                info.statusString = SidTune::txt_fileIoError;
            }
            else
            {
                info.statusString = SidTune::txt_noErrors;
                success = true;
            }
            fMyOut.close();
        }
    }
    return success;
}

bool SidTune::checkRelocInfo (void)
{
    uint_least8_t startp, endp;

    // Fix relocation information
    if (info.relocStartPage == 0xFF)
    {
        info.relocPages = 0;
        return true;
    }
    else if (info.relocPages == 0)
    {
        info.relocStartPage = 0;
        return true;
    }

    // Calculate start/end page
    startp = info.relocStartPage;
    endp   = (startp + info.relocPages - 1) & 0xff;
    if (endp < startp)
    {
        info.statusString = txt_badReloc;
        return false;
    }

    {    // Check against load range
        uint_least8_t startlp, endlp;
        startlp = (uint_least8_t) (info.loadAddr >> 8);
        endlp   = startlp;
        endlp  += (uint_least8_t) ((info.c64dataLen - 1) >> 8);

        if ( ((startp <= startlp) && (endp >= startlp)) ||
             ((startp <= endlp)   && (endp >= endlp)) )
        {
            info.statusString = txt_badReloc;
            return false;
        }
    }

    // Check that the relocation information does not use the following
    // memory areas: 0x0000-0x03FF, 0xA000-0xBFFF and 0xD000-0xFFFF
    if ((startp < 0x04)
        || ((0xa0 <= startp) && (startp <= 0xbf))
        || (startp >= 0xd0)
        || ((0xa0 <= endp) && (endp <= 0xbf))
        || (endp >= 0xd0))
    {
        info.statusString = txt_badReloc;
        return false;
    }
    return true;
}

bool SidTune::resolveAddrs (const uint_least8_t *c64data)
{   // Originally used as a first attempt at an RSID
    // style format. Now reserved for future use
    if ( info.playAddr == 0xffff )
        info.playAddr  = 0;

    // loadAddr = 0 means, the address is stored in front of the C64 data.
    if ( info.loadAddr == 0 )
    {
        if ( info.c64dataLen < 2 )
        {
            info.statusString = txt_corrupt;
            return false;
        }
        info.loadAddr = endian_16( *(c64data+1), *c64data );
        fileOffset += 2;
        c64data += 2;
        info.c64dataLen -= 2;
    }

    if ( info.compatibility == SIDTUNE_COMPATIBILITY_BASIC )
    {
        if ( info.initAddr != 0 )
        {
            info.statusString = txt_badAddr;
            return false;
        }
    }
    else if ( info.initAddr == 0 )
        info.initAddr = info.loadAddr;
    return true;
}

bool SidTune::checkCompatibility (void)
{
    switch ( info.compatibility )
    {
    case SIDTUNE_COMPATIBILITY_R64:
        // Check valid init address
        switch (info.initAddr >> 12)
        {
        case 0x0F:
        case 0x0E:
        case 0x0D:
        case 0x0B:
        case 0x0A:
            info.statusString = txt_badAddr;
            return false;
        default:
            if ( (info.initAddr < info.loadAddr) ||
                 (info.initAddr > (info.loadAddr + info.c64dataLen - 1)) )
            {
                info.statusString = txt_badAddr;
                return false;
            }
        }
        // deliberate run on

    case SIDTUNE_COMPATIBILITY_BASIC:
        // Check tune is loadable on a real C64
        if ( info.loadAddr < SIDTUNE_R64_MIN_LOAD_ADDR )
        {
            info.statusString = txt_badAddr;
            return false;
        }
        break;
    }
    return true;
}

int SidTune::convertPetsciiToAscii(SmartPtr_sidtt<const uint8_t>& spPet, char* dest)
{
    int count = 0;
    char c;
    if (dest)
    {
        do
        {
            c = _sidtune_CHRtab[*spPet];  // ASCII CHR$ conversion
            if ((c>=0x20) && (count<=31))
                dest[count++] = c;  // copy to info string

            // If character is 0x9d (left arrow key) then move back.
            if ((*spPet==0x9d) && (count>0))
                count--;
            spPet++;
        }
        while ( !((c==0x0D)||(c==0x00)||spPet.fail()) );
    }
    else
    {   // Just find end of string
        do
        {
            c = _sidtune_CHRtab[*spPet];  // ASCII CHR$ conversion
            spPet++;
        }
        while ( !((c==0x0D)||(c==0x00)||spPet.fail()) );
    }
    return count;
}

const char *SidTune::createMD5(char *md5)
{
    if (!md5)
        md5 = m_md5;
    *md5 = '\0';

    if (status)
    {   // Include C64 data.
        MD5 myMD5;
        md5_byte_t tmp[2];
        myMD5.append (cache.get()+fileOffset,info.c64dataLen);
        // Include INIT and PLAY address.
        endian_little16 (tmp,info.initAddr);
        myMD5.append    (tmp,sizeof(tmp));
        endian_little16 (tmp,info.playAddr);
        myMD5.append    (tmp,sizeof(tmp));
        // Include number of songs.
        endian_little16 (tmp,info.songs);
        myMD5.append    (tmp,sizeof(tmp));
        {   // Include song speed for each song.
            const uint_least16_t currentSong = info.currentSong;
            for (uint_least16_t s = 1; s <= info.songs; s++)
            {
                selectSong (s);
                myMD5.append (&info.songSpeed,sizeof(info.songSpeed));
            }
            // Restore old song
            selectSong (currentSong);
        }
        // Deal with PSID v2NG clock speed flags: Let only NTSC
        // clock speed change the MD5 fingerprint. That way the
        // fingerprint of a PAL-speed sidtune in PSID v1, v2, and
        // PSID v2NG format is the same.
        if (info.clockSpeed == SIDTUNE_CLOCK_NTSC)
            myMD5.append (&info.clockSpeed,sizeof(info.clockSpeed));
        // NB! If the fingerprint is used as an index into a
        // song-lengths database or cache, modify above code to
        // allow for PSID v2NG files which have clock speed set to
        // SIDTUNE_CLOCK_ANY. If the SID player program fully
        // supports the SIDTUNE_CLOCK_ANY setting, a sidtune could
        // either create two different fingerprints depending on
        // the clock speed chosen by the player, or there could be
        // two different values stored in the database/cache.

        myMD5.finish();
        // Construct fingerprint.
        char *m = md5;
        for (int di = 0; di < 16; ++di)
        {
            sprintf (m, "%02x", (int) myMD5.getDigest()[di]);
            m += 2;
        }
    }
    return md5;
}
