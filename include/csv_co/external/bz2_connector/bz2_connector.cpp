#include "bz2_connector.h"
#include "../bzip2/bzlib.h"
#include <stdexcept>
//#include <iostream>
//#define ERROR_IF_EOF(i)       { if ((i) == EOF)  ioError(); }
//#define ERROR_IF_NOT_ZERO(i)  { if ((i) != 0)    ioError(); }
////#define ERROR_IF_MINUS_ONE(i) { if ((i) == (-1)) ioError(); }

#if defined(__unix__)
#define BZ_UNIX 1
 #define BZ_LCCWIN32 0
#else
#define BZ_UNIX 0
#define BZ_LCCWIN32 1
#endif

#if BZ_UNIX
#   include <fcntl.h>
#   include <sys/types.h>
#   include <utime.h>
#   include <unistd.h>
#   include <sys/stat.h>
#   include <sys/times.h>

#   define PATH_SEP    '/'
#   define MY_LSTAT    lstat
#   define MY_STAT     stat
#   define MY_S_ISREG  S_ISREG
#   define MY_S_ISDIR  S_ISDIR

#   define APPEND_FILESPEC(root, name) \
      root=snocString((root), (name))

#   define APPEND_FLAG(root, name) \
      root=snocString((root), (name))

#   define SET_BINARY_MODE(fd) /**/

#   ifdef __GNUC__
#      define NORETURN __attribute__ ((noreturn))
#   else
#      define NORETURN /**/
#   endif

#   ifdef __DJGPP__
#     include <io.h>
#     include <fcntl.h>
#     undef MY_LSTAT
#     undef MY_STAT
#     define MY_LSTAT stat
#     define MY_STAT stat
#     undef SET_BINARY_MODE
#     define SET_BINARY_MODE(fd)                        \
        do {                                            \
           int retVal = setmode ( fileno ( fd ),        \
                                  O_BINARY );           \
           ERROR_IF_MINUS_ONE ( retVal );               \
        } while ( 0 )
#   endif

#   ifdef __CYGWIN__
#     include <io.h>
#     include <fcntl.h>
#     undef SET_BINARY_MODE
#     define SET_BINARY_MODE(fd)                        \
        do {                                            \
           int retVal = setmode ( fileno ( fd ),        \
                                  O_BINARY );           \
           ERROR_IF_MINUS_ONE ( retVal );               \
        } while ( 0 )
#   endif
#endif /* BZ_UNIX */

#if BZ_LCCWIN32
#   include <io.h>
#   include <fcntl.h>
#   include <sys/stat.h>
#include <fstream>

#   define NORETURN       /**/
#   define PATH_SEP       '\\'
#   define MY_LSTAT       _stati64
#   define MY_STAT        _stati64
#   define MY_S_ISREG(x)  ((x) & _S_IFREG)
#   define MY_S_ISDIR(x)  ((x) & _S_IFDIR)

#   define APPEND_FLAG(root, name) \
      root=snocString((root), (name))

#   define APPEND_FILESPEC(root, name)                \
      root = snocString ((root), (name))

#   define SET_BINARY_MODE(fd)                        \
      setmode ( fileno ( fd ), O_BINARY );

#endif /* BZ_LCCWIN32 */

#if _WIN32
#define fileno          _fileno
#define write           _write
#define isatty          _isatty
#define setmode         _setmode
#ifndef STDERR_FILENO
#define STDERR_FILENO   _fileno(stderr)
#endif
#endif

typedef char            Char;
typedef unsigned char   Bool;
typedef unsigned char   UChar;
typedef int             Int32;
typedef unsigned int    UInt32;
typedef short           Int16;
typedef unsigned short  UInt16;

#define True  ((Bool)1)
#define False ((Bool)0)

namespace bz2_connector::detail {
    static
    Bool myfeof ( FILE* f )
    {
        Int32 c = fgetc ( f );
        if (c == EOF) return True;
        ungetc ( c, f );
        return False;
    }

    static
    Bool fileExists ( Char const * name )
    {
        FILE *tmp   = fopen ( name, "rb" );
        Bool exists = (tmp != nullptr);
        if (tmp != nullptr) fclose ( tmp );
        return exists;
    }
}

namespace bz2_connector {
    std::string read_all(char const * filename)
    {
        using namespace detail;
        if (!fileExists(filename))
            throw std::runtime_error("bz2_connector: No file found.");
        FILE *zStream = fopen (filename, "rb" );

        BZFILE* bzf;
        Int32   verbosity = 0;
        Int32   bzerr, bzerr_dummy, ret, nread, streamNo, i;

        constexpr std::size_t obuf_size = 1000000;
        static_assert(obuf_size == BZ_MAX_UNUSED);

        UChar   obuf[obuf_size];
        UChar   unused[BZ_MAX_UNUSED];
        Int32   nUnused;
        void*   unusedTmpV;
        UChar*  unusedTmp;

        nUnused = 0;
        streamNo = 0;

        std::string str;

        SET_BINARY_MODE(zStream)

        Bool smallMode = false;

        if (ferror(zStream))
            goto errhandler_io;

        while (True) {

            bzf = BZ2_bzReadOpen (&bzerr, zStream, verbosity, (int)smallMode, unused, nUnused);

            if (bzf == nullptr || bzerr != BZ_OK)
                goto errhandler;
            streamNo++;

            while (bzerr == BZ_OK) {
                nread = BZ2_bzRead ( &bzerr, bzf, obuf, obuf_size);
                if (bzerr == BZ_DATA_ERROR_MAGIC)
                    goto trycat;
                if ((bzerr == BZ_OK || bzerr == BZ_STREAM_END) && nread > 0)
                    str += std::string{obuf, obuf + nread};
            }
            if (bzerr != BZ_STREAM_END)
                goto errhandler;

            BZ2_bzReadGetUnused ( &bzerr, bzf, &unusedTmpV, &nUnused );
            if (bzerr != BZ_OK)
                throw std::runtime_error("bz2_connector: decompress: BZ2_bzReadGetUnused.");

            unusedTmp = (UChar*)unusedTmpV;
            for (i = 0; i < nUnused; i++)
                unused[i] = unusedTmp[i];

            BZ2_bzReadClose ( &bzerr, bzf );
            if (bzerr != BZ_OK)
                throw std::runtime_error("bz2_connector: decompress: BZ2_bzReadClose.");

            if (nUnused == 0 && myfeof(zStream))
                break;
        }

        if (ferror(zStream))
            goto errhandler_io;

        ret = fclose ( zStream );
        if (ret == EOF)
            goto errhandler_io;

        return str;

        trycat:
        errhandler:

        BZ2_bzReadClose ( &bzerr_dummy, bzf );
        switch (bzerr) {
            case BZ_CONFIG_ERROR:
            case BZ_IO_ERROR:
                errhandler_io:
                break;
            case BZ_DATA_ERROR:
            case BZ_MEM_ERROR:
            case BZ_UNEXPECTED_EOF:
            case BZ_DATA_ERROR_MAGIC:
                if (zStream != stdin)
                    fclose(zStream);

                if (streamNo == 1)
                    return {};
                else
                    return str;
            default:
                throw std::runtime_error("bz2_connector: decompress: unexpected error.");
        }

        throw std::runtime_error("bz2_connector: decompress:end");
    }
}

