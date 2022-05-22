
///////////////////////////////////////
// HEADERS YOU WOULDN'T NORMALLY USE //
///////////////////////////////////////

#include <jpeglib.h>    // The JPEG library
#include <setjmp.h>     // We need this to use the JPEG library.

///////////
// FLAGS //
///////////

#define PICTURE_NONE                0x0000  // No picture loaded

// File Format Flags
#define PICTURE_BITMAP              0x1000  // Bitmap (*.bmp)
#define PICTURE_JPEG                0x2000  // JPEG (*.jpg)
#define PICTURE_TARGA               0x4000  // Targa (*.tga)

// Compression Flags
#define PICTURE_UNCOMPRESSED        0x0100  // Always true with bitmaps, valid for targas
#define PICTURE_COMPRESSED          0x0200  // Always true with jpegs, valid with targas

// Bits-Per-Pixel Flags
#define PICTURE_1BIT                0x0001  // Valid with bitmaps
#define PICTURE_MONOCHROME          0x0001
#define PICTURE_4BIT                0x0002  // Valid with bitmaps
#define PICTURE_16COLORS            0x0002
#define PICTURE_8BIT                0x0004  // Valid with bitmaps, jpegs, and targas
#define PICTURE_256COLORS           0x0004
#define PICTURE_16BIT               0x0008  // Valid with bitmaps
#define PICTURE_HIGHCOLOR           0x0008
#define PICTURE_24BIT               0x0010  // Most common bit depth, valid with bitmaps, jpegs, and targas
#define PICTURE_TRUECOLOR           0x0010
#define PICTURE_32BIT               0x0020  // Valid with bitmaps

// Gray-Scale Flag
#define PICTURE_GRAYSCALE           0x0040  // Valid with jpegs and targas

/////////////////////////
// CLASSES AND STRUCTS //
/////////////////////////

// This is our picture class
struct Picture
{
    int   Flags;    // Example: PICTURE_BITMAP | PICTURE_UNCOMPRESSED | PICTURE_24BIT
    int   Width;
    int   Height;
    BYTE* RGB;      // The RGB data array. Size is 3*Width*Height. Order is Red-Green-Blue
};

// We need these structs to use the JPEG library.
struct JPGError
{
    jpeg_error_mgr  Public;
    jmp_buf         SetJmp;
};

static void JPEG_OnError ( j_common_ptr JPGInfo )
{
    JPGError*   Error;
    char        Buffer [JMSG_LENGTH_MAX];

    Error = (JPGError*)JPGInfo->err;
    Error->Public.format_message( JPGInfo, Buffer );

    longjmp( Error->SetJmp, 1 );
}

/////////////////
// THE BIG ONE //
/////////////////

/* Note that this function requires as its only parameters
   a pointer to a Stream and, optionally, the size of the picture's
   part of that stream. Stream is a class I wrote, and
   you can modify the function to use either your file
   class, or a FILE*, or a filename, or whatever.

   Picture::Load() uses only four of its functions:

        bool Stream::Input( LPVOID Pointer, int Size )
            This reads Size bytes and puts it in Pointer.

        bool Stream::Seek( int Position, int Flag )
            ::Seek(0,STREAM_BEGINNING) is like fseek( File, 0, SEEK_SET )
            ::Seek(0,STREAM_END) is like fseek( File, 0, SEEK_END )

        int Stream::Position()
            This works like ftell. It returns the caret position.

        int Stream::Size()
            This returns the file size, and is only called if
            you don't specify the file size to Picture::Load().

    So it's pretty easy to integrate this code into your own
    project, I hope.

    You would use this function like this:

    File.Open( STREAM_FILE, STREAM_READ, "Test.jpg" );
    Pic.Load( &File, 0 );
    File.Close( );
*/

bool Picture :: Load ( Stream* File, int PictureSize )
{
//  General-purpose variables
    BYTE                    TryToGetType    [2];
    BYTE                    Palette         [768];
    int                     NumColors;
    int                     CurrPixel;
    int                     Dest;
    int                     Src;
    int                     Front;
    int                     HStart;
    int                     HEnd;
    int                     HDir;

//  Bitmap variables
    BYTE                    BMRed;
    BYTE                    BMGreen;
    BYTE                    BMBlue;
    BYTE                    BMBlank;
    BITMAPFILEHEADER        BMFile;
    BITMAPINFOHEADER        BMInfo;
    int                     BMPadding;
    int                     BMPad;
    BYTE                    BMGroup;
    BYTE                    BMPixel8;
    WORD                    BMPixel16;
    int                     BMScanWidth;

//  JPEG variables
    jpeg_decompress_struct  JPGInfo;
    JPGError                JPGErr;
    JSAMPARRAY              JPGBuffer;
    int                     JPGStride;
    BYTE*                   JPGRed;
    BYTE*                   JPGGreen;
    BYTE*                   JPGBlue;
    BYTE*                   JPGSource;
    FILE*                   JPGTempFile;
    int                     JPGImageSize;
    BYTE*                   JPGImage;
    BYTE*                   JPGPixel;
    BYTE*                   JPGContents;

//  Targa variables
    BYTE                    TGAFile         [26];
    BYTE                    TGAHeader       [18];
    BYTE                    TGAID           [256];
    int                     TGANumColors;
    BYTE                    TGAPacket;
    int                     TGANumPixels;
    BYTE                    TGAScanline     [384];
    BYTE*                   TGAImage;

    Flags = PICTURE_NONE;
    Width = 0;
    Height = 0;
    RGB = NULL;

    if( !FileSize )
        FileSize = File->Size( );

//  Find out where in the file we are. Do this in case it's an archive
//  and the picture might start in the middle somewhere:
    Front = File->Position( );

//  Read the first two bytes to figure out what image this is:
    if( !File->Input( TryToGetType, 2 ) )
    {
//      You will see this alot. When an error occurs, I
//      seek to what was supposed to be the end of the image data,
//      in case the stream is an archive. So the next item in
//      the archive can be loaded.
        File->Seek( Front + FileSize, STREAM_BEGINNING );
        return false;
    }

//  Go back to where we were
    if( !File->Seek( Front, STREAM_BEGINNING ) )
        return false;

    if( TryToGetType[0] == 'B' &&
        TryToGetType[1] == 'M' )
    {
//      The file is a bitmap

        if( !File->Input( &BMFile, sizeof BMFile ) ||
            !File->Input( &BMInfo, sizeof BMInfo ) )
        {
            File->Seek( Front + FileSize, STREAM_BEGINNING );
            return false;
        }

//      Only uncompressed bitmaps are supported by this code
        if( BMInfo.biCompression != BI_RGB )
        {
            File->Seek( Front + FileSize, STREAM_BEGINNING );
            return false;
        }

//      Create the palette now, if we must
        if( BMInfo.biBitCount <= 8 )
        {
            NumColors = 1 << BMInfo.biBitCount;

            for( int C = 0; C < NumColors * 3; C += 3 )
            {
/*              Palette information in bitmaps is stored in BGR_ format.
                That means it's blue-green-red-blank, for each entry.
*/
                if( !File->Input( &BMBlue, 1 ) ||
                    !File->Input( &BMGreen, 1 ) ||
                    !File->Input( &BMRed, 1 ) ||
                    !File->Input( &BMBlank, 1 ) )
                {
                    File->Seek( Front + FileSize, STREAM_BEGINNING );
                    return false;
                }

                Palette[C] = BMRed;
                Palette[C+1] = BMGreen;
                Palette[C+2] = BMBlue;
            }
        }

        Width = BMInfo.biWidth;

        if( BMInfo.biHeight < 0 )
        {
            Height = -BMInfo.biHeight;
            HStart = 0;
            HEnd = Height;
            HDir = 1;
        }
        else
        {
            Height = BMInfo.biHeight;
            HStart = Height - 1;
            HEnd = -1;
            HDir = -1;
        }

        RGB = new BYTE [Width * Height * 3];

        if( BMInfo.biBitCount == 1 )
        {
//          Note that this file is not necessarily grayscale, since it's possible
//          the palette is blue-and-white, or whatever. But of course most image
//          programs only write 1-bit images if they're black-and-white.
            Flags = PICTURE_BITMAP | PICTURE_UNCOMPRESSED | PICTURE_MONOCHROME;

//          For bitmaps, each scanline is dword-aligned.
            BMScanWidth = (Width+7) >> 3;
            if( BMScanWidth & 3 )
                BMScanWidth += 4 - (BMScanWidth & 3);

            for( int H = HStart; H != HEnd; H += HDir )
            {
                CurrPixel = 0;

                for( int W = 0; W < BMScanWidth; W ++ )
                {
                    if( !File->Input( &BMGroup, 1 ) )
                    {
                        delete [] RGB;
                        File->Seek( Front + FileSize, STREAM_BEGINNING );
                        return false;
                    }

/*                  Now we read the pixels. Usually there are
                    eight pixels per byte, since each pixel
                    is represented by one bit, but if the width
                    is not a multiple of eight, the last byte
                    will have some bits set, with the others just
                    being extra. Plus there's the dword-alignment
                    padding. So we keep checking to see if we've
                    already read "Width" number of pixels.
*/
                    if( CurrPixel < Width )
                    {
                        Dest = 3 * ((H * Width) + CurrPixel);
                        Src = 3 * ((BMGroup & 0x80) >> 7);
                        RGB[Dest] = Palette[Src];
                        RGB[Dest+1] = Palette[Src+1];
                        RGB[Dest+2] = Palette[Src+2];
                        CurrPixel ++;
                    }

                    if( CurrPixel < Width )
                    {
                        Dest = 3 * ((H * Width) + CurrPixel);
                        Src = 3 * ((BMGroup & 0x40) >> 6);
                        RGB[Dest] = Palette[Src];
                        RGB[Dest+1] = Palette[Src+1];
                        RGB[Dest+2] = Palette[Src+2];
                        CurrPixel ++;
                    }

                    if( CurrPixel < Width )
                    {
                        Dest = 3 * ((H * Width) + CurrPixel);
                        Src = 3 * ((BMGroup & 0x20) >> 5);
                        RGB[Dest] = Palette[Src];
                        RGB[Dest+1] = Palette[Src+1];
                        RGB[Dest+2] = Palette[Src+2];
                        CurrPixel ++;
                    }

                    if( CurrPixel < Width )
                    {
                        Dest = 3 * ((H * Width) + CurrPixel);
                        Src = 3 * ((BMGroup & 0x10) >> 4);
                        RGB[Dest] = Palette[Src];
                        RGB[Dest+1] = Palette[Src+1];
                        RGB[Dest+2] = Palette[Src+2];
                        CurrPixel ++;
                    }

                    if( CurrPixel < Width )
                    {
                        Dest = 3 * ((H * Width) + CurrPixel);
                        Src = 3 * ((BMGroup & 0x08) >> 3);
                        RGB[Dest] = Palette[Src];
                        RGB[Dest+1] = Palette[Src+1];
                        RGB[Dest+2] = Palette[Src+2];
                        CurrPixel ++;
                    }

                    if( CurrPixel < Width )
                    {
                        Dest = 3 * ((H * Width) + CurrPixel);
                        Src = 3 * ((BMGroup & 0x04) >> 2);
                        RGB[Dest] = Palette[Src];
                        RGB[Dest+1] = Palette[Src+1];
                        RGB[Dest+2] = Palette[Src+2];
                        CurrPixel ++;
                    }

                    if( CurrPixel < Width )
                    {
                        Dest = 3 * ((H * Width) + CurrPixel);
                        Src = 3 * ((BMGroup & 0x02) >> 1);
                        RGB[Dest] = Palette[Src];
                        RGB[Dest+1] = Palette[Src+1];
                        RGB[Dest+2] = Palette[Src+2];
                        CurrPixel ++;
                    }

                    if( CurrPixel < Width )
                    {
                        Dest = 3 * ((H * Width) + CurrPixel);
                        Src = 3 * (BMGroup & 0x01);
                        RGB[Dest] = Palette[Src];
                        RGB[Dest+1] = Palette[Src+1];
                        RGB[Dest+2] = Palette[Src+2];
                        CurrPixel ++;
                    }
                }
            }
        }
        else if( BMInfo.biBitCount == 4 )
        {
            Flags = PICTURE_BITMAP | PICTURE_UNCOMPRESSED | PICTURE_4BIT;

//          For bitmaps, each scanline is dword-aligned.
            BMScanWidth = (Width+1) >> 1;
            if( BMScanWidth & 3 )
                BMScanWidth += 4 - (BMScanWidth & 3);

            for( int H = HStart; H != HEnd; H += HDir )
            {
                CurrPixel = 0;

                for( int W = 0; W < BMScanWidth; W ++ )
                {
                    if( !File->Input( &BMGroup, 1 ) )
                    {
                        delete [] RGB;
                        File->Seek( Front + FileSize, STREAM_BEGINNING );
                        return false;
                    }

/*                  Now we read the pixels. Usually there are
                    two pixels per byte, since each pixel
                    is represented by four bits, but if the width
                    is not a multiple of two, the last byte
                    will have only four bits set, with the others just
                    being extra. Plus there's the dword-alignment
                    padding. So we keep checking to see if we've
                    already read "Width" number of pixels.
*/
                    if( CurrPixel < Width )
                    {
                        Dest = 3 * ((H * Width) + CurrPixel);
                        Src = 3 * ((BMGroup & 0xF0) >> 4);
                        RGB[Dest] = Palette[Src];
                        RGB[Dest+1] = Palette[Src+1];
                        RGB[Dest+2] = Palette[Src+2];
                        CurrPixel ++;
                    }

                    if( CurrPixel < Width )
                    {
                        Dest = 3 * ((H * Width) + CurrPixel);
                        Src = 3 * (BMGroup & 0x0F);
                        RGB[Dest] = Palette[Src];
                        RGB[Dest+1] = Palette[Src+1];
                        RGB[Dest+2] = Palette[Src+2];
                        CurrPixel ++;
                    }
                }
            }
        }
        else if( BMInfo.biBitCount == 8 )
        {
            Flags = PICTURE_BITMAP | PICTURE_UNCOMPRESSED | PICTURE_8BIT;

//          For bitmaps, each scanline is dword-aligned.
            BMScanWidth = Width;
            if( BMScanWidth & 3 )
                BMScanWidth += 4 - (BMScanWidth & 3);

            for( int H = HStart; H != HEnd; H += HDir )
            {
                CurrPixel = 0;

                for( int W = 0; W < BMScanWidth; W ++ )
                {
                    if( !File->Input( &BMPixel8, 1 ) )
                    {
                        delete [] RGB;
                        File->Seek( Front + FileSize, STREAM_BEGINNING );
                        return false;
                    }

                    if( CurrPixel < Width )
                    {
                        Dest = 3 * ((H * Width) + CurrPixel);
                        Src = 3 * BMPixel8;
                        RGB[Dest] = Palette[Src];
                        RGB[Dest+1] = Palette[Src+1];
                        RGB[Dest+2] = Palette[Src+2];
                        CurrPixel ++;
                    }
                }
            }
        }
        else if( BMInfo.biBitCount == 16 )
        {
            Flags = PICTURE_BITMAP | PICTURE_UNCOMPRESSED | PICTURE_16BIT;

//          For bitmaps, each scanline is dword-aligned.
            BMScanWidth = Width << 1;

            if( BMScanWidth & 3 )
                BMPadding = 4 - (BMScanWidth & 3);
            else
                BMPadding = 0;

            for( int H = HStart; H != HEnd; H += HDir )
            {
                for( int W = 0; W < Width; W ++ )
                {
                    if( !File->Input( &BMPixel16, 2 ) )
                    {
                        delete [] RGB;
                        File->Seek( Front + FileSize, STREAM_BEGINNING );
                        return false;
                    }

/*                  Now we seperate each WORD into the correct RGB values.
                    The highest bit is zero, the next 5 are red, the next
                    5 are green, and the lowest 5 are blue.
*/
                    BMRed = ((BMPixel16 >> 10) & 0x1F) << 3;
                    BMGreen = ((BMPixel16 >> 5) & 0x1F) << 3;
                    BMBlue = (BMPixel16 & 0x1F) << 3;

                    Dest = ((H * Width) + W) << 1;
                    RGB[Dest] = BMRed;
                    RGB[Dest+1] = BMGreen;
                    RGB[Dest+2] = BMBlue;
                }

                if( BMPadding )
                    File->Input( &BMPad, BMPadding );
            }
        }
        else if( BMInfo.biBitCount == 24 )
        {
            Flags = PICTURE_BITMAP | PICTURE_UNCOMPRESSED | PICTURE_24BIT;

//          For bitmaps, each scanline is dword-aligned.
            BMScanWidth = Width * 3;

            if( BMScanWidth & 3 )
                BMPadding = 4 - (BMScanWidth & 3);
            else
                BMPadding = 0;

            for( int H = HStart; H != HEnd; H += HDir )
            {
                for( int W = 0; W < Width; W ++ )
                {
                    if( !File->Input( &BMBlue, 1 ) ||
                        !File->Input( &BMGreen, 1 ) ||
                        !File->Input( &BMRed, 1 ) )
                    {
                        delete [] RGB;
                        File->Seek( Front + FileSize, STREAM_BEGINNING );
                        return false;
                    }

                    Dest = 3 * ((H * Width) + W);
                    RGB[Dest] = BMRed;
                    RGB[Dest+1] = BMGreen;
                    RGB[Dest+2] = BMBlue;
                }

                if( BMPadding )
                    File->Input( &BMPad, BMPadding );
            }
        }
        else if( BMInfo.biBitCount == 32 )
        {
            Flags = PICTURE_BITMAP | PICTURE_UNCOMPRESSED | PICTURE_32BIT;

//          Since each pixel is a dword, it is obviously dword-aligned

            for( int H = HStart; H != HEnd; H += HDir )
            {
                for( int W = 0; W < Width; W ++ )
                {
                    if( !File->Input( &BMBlank, 1 ) ||
                        !File->Input( &BMBlue, 1 ) ||
                        !File->Input( &BMGreen, 1 ) ||
                        !File->Input( &BMRed, 1 ) )
                    {
                        delete [] RGB;
                        File->Seek( Front + FileSize, STREAM_BEGINNING );
                        return false;
                    }

                    Dest = 3 * ((H * Width) + W);
                    RGB[Dest] = BMRed;
                    RGB[Dest+1] = BMGreen;
                    RGB[Dest+2] = BMBlue;
                }
            }
        }
        else // We support all possible bit depths, so if the
        {//     code gets here, it's not even a real bitmap.
            delete [] RGB;
            File->Seek( Front + FileSize, STREAM_BEGINNING );
            return false;
        }
    }
    else if( TryToGetType[0] == 0xFF && // These unlikely-looking characters are
             TryToGetType[1] == 0xD8 )  // used to identify JPEGs
    {
/*      Unfortunately, the JPEG library seems to require a FILE*
        passed as a parameter, as far as I can tell. I may be wrong,
        but for now I save the JPEG to a temporary file and load
        from that. This is necessary if it's loaded from a resource,
        for example.
*/
        JPGContents = new BYTE [FileSize];

        if( !File->Input( JPGContents, FileSize ) )
        {
            delete [] JPGContents;
            File->Seek( Front + FileSize, STREAM_BEGINNING );
            return false;
        }

        File->Seek( Front + FileSize, STREAM_BEGINNING );

//      JPGContents now holds the contents of the JPEG.

        JPGInfo.err = jpeg_std_error( &JPGErr.Public );
        JPGErr.Public.error_exit = JPEG_OnError;

        if( setjmp( JPGErr.SetJmp ) )
        {
//          This part of the code is only called when a JPEG
//          library function fails. When it does, it will jump
//          to this point.

            jpeg_destroy_decompress( &JPGInfo );
            File->Seek( Front + FileSize, STREAM_BEGINNING );
            fclose( JPGTempFile );
            return false;
        }

        JPGTempFile = fopen( "__PicLoad.jpg", "wb" ); // Create the temporary File.
        fwrite( JPGContents, 1, FileSize, JPGTempFile );
        fclose( JPGTempFile );

        delete [] JPGContents; // No longer needed
        JPGTempFile = fopen( "__PicLoad.jpg", "rb" );

        jpeg_create_decompress( &JPGInfo );
        jpeg_stdio_src( &JPGInfo, JPGTempFile );
        jpeg_read_header( &JPGInfo, TRUE );
        jpeg_start_decompress( &JPGInfo );

        Width = JPGInfo.image_width;
        Height = JPGInfo.image_height;

        if( JPGInfo.jpeg_color_space == JCS_GRAYSCALE )
        {
            Flags = PICTURE_JPEG | PICTURE_COMPRESSED | PICTURE_8BIT | PICTURE_GRAYSCALE;

            for( int N = 0; N < 256; N ++ )
            {
                Palette[N*3] = N;
                Palette[(N*3)+1] = N;
                Palette[(N*3)+2] = N;
            }
        }
        else
        {
            if( JPGInfo.num_components == 1 )
                Flags = PICTURE_JPEG | PICTURE_COMPRESSED | PICTURE_8BIT;
            else
                Flags = PICTURE_JPEG | PICTURE_COMPRESSED | PICTURE_24BIT;

            if( JPGInfo.quantize_colors && JPGInfo.num_components == 1 )
            {
                JPGRed = JPGInfo.colormap[0];
                JPGGreen = JPGInfo.colormap[1];
                JPGBlue = JPGInfo.colormap[2];

                if( !JPGRed )
                {
                    fclose( JPGTempFile );
                    jpeg_destroy_decompress( &JPGInfo );
                    return false;
                }

                if( !JPGGreen )
                    JPGGreen = JPGRed;
                if( !JPGBlue )
                    JPGBlue = JPGGreen;

                for( int N = 0; N < 256; N ++ )
                {
                    Palette[N*3] = JPGRed[N];
                    Palette[(N*3)+1] = JPGGreen[N];
                    Palette[(N*3)+2] = JPGBlue[N];
                }
            }
        }

        JPGStride = JPGInfo.output_width * JPGInfo.num_components;
        JPGBuffer = (*JPGInfo.mem->alloc_sarray)( (j_common_ptr)&JPGInfo, JPOOL_IMAGE, JPGStride, 1 );

        JPGImageSize = JPGInfo.num_components * Width * Height;
        JPGImage = new BYTE [JPGImageSize];
        JPGPixel = JPGImage;

        while( JPGInfo.output_scanline < JPGInfo.output_height )
        {
            jpeg_read_scanlines( &JPGInfo, JPGBuffer, 1 );

            if( JPGInfo.num_components == 4 &&
                !JPGInfo.quantize_colors )
            {
                JPGSource = JPGBuffer[0];

                for( int X1=0, X2=0; X1 < JPGInfo.num_components * Width && X2 < JPGStride; X1 += 3, X2 += 4 )
                {
                    JPGPixel[X1] = (JPGSource[X2+3] * JPGSource[X2+2]) / 255;
                    JPGPixel[X1+1] = (JPGSource[X2+3] * JPGSource[X2+1]) / 255;
                    JPGPixel[X1+2] = (JPGSource[X2+3] * JPGSource[X2]) / 255;
                }
            }
            else
            {
                if( JPGStride < JPGInfo.num_components * Width )
                    memcpy( JPGPixel, JPGBuffer[0], JPGStride );
                else
                    memcpy( JPGPixel, JPGBuffer[0], JPGInfo.num_components * Width );
            }

            JPGPixel += JPGInfo.num_components * Width;
        }

        jpeg_finish_decompress( &JPGInfo );
        jpeg_destroy_decompress( &JPGInfo );

        if( JPGInfo.num_components == 1 )
        {
            RGB = new BYTE [Width * Height * 3];

            for( int N = 0; N < JPGImageSize; N ++ )
            {
                RGB[N*3] = Palette[JPGImage[N]*3];
                RGB[(N*3)+1] = Palette[(JPGImage[N]*3)+1];
                RGB[(N*3)+2] = Palette[(JPGImage[N]*3)+2];
            }

            delete [] JPGImage;
        }
        else
            RGB = JPGImage;

        fclose( JPGTempFile );
        remove( "__PicLoad.jpg" );
    }
    else
    {
//      The image is either a Targa or unknown

/*      We want to read the last 26 bytes of the File-> We could
        seek to the end, but in the case of archives this is
        not desirable. So we do it this way.
*/
        if( !File->Seek( Front + FileSize - 26, STREAM_BEGINNING ) )
            return false;

        if( !File->Input( TGAFile, 26 ) )
        {
            File->Seek( Front + FileSize, STREAM_BEGINNING );
            return false;
        }

        if( !File->Seek( Front, STREAM_BEGINNING ) )
        {
            File->Seek( Front + FileSize, STREAM_BEGINNING );
            return false;
        }

        if( memcmp( &TGAFile[8], "TRUEVISION-XFILE", 16 ) )
        {
//          The image is not a Targa
            File->Seek( Front + FileSize, STREAM_BEGINNING );
            return false;
        }

//      The image is a Targa

        if( !File->Input( TGAHeader, 18 ) )
        {
            File->Seek( Front + FileSize, STREAM_BEGINNING );
            return false;
        }

        Width = ((WORD*)TGAHeader)[6];
        Height = ((WORD*)TGAHeader)[7];

/*      The ID field is an optional string that you can use to give
        the image a name or description or whatever.
*/
        if( TGAHeader[0] && !File->Input( TGAID, TGAHeader[0] ) )
        {
            File->Seek( Front + FileSize, STREAM_BEGINNING );
            return false;
        }

        if( TGAHeader[2] == 1 )
        {
            Flags = PICTURE_TARGA | PICTURE_UNCOMPRESSED | PICTURE_8BIT;

            TGANumColors = ((WORD*)(TGAHeader+1))[2];

/*          Unlike bitmaps, Targas support palettes that are in other
            bit depths besides 24-bit. But I haven't seen any and I am
            lazy so this code ignores them.
*/
            if( TGAHeader[7] != 24 )
            {
                File->Seek( Front + FileSize, STREAM_BEGINNING );
                return false;
            }

            if( !File->Input( Palette, 3 * TGANumColors ) )
            {
                File->Seek( Front + FileSize, STREAM_BEGINNING );
                return false;
            }

            TGAImage = new BYTE [Width * Height];

            if( !File->Input( TGAImage, Width * Height ) )
            {
                delete [] TGAImage;
                File->Seek( Front + FileSize, STREAM_BEGINNING );
                return false;
            }

            RGB = new BYTE [Width * Height * 3];

            for( int N = 0; N < Width * Height; N ++ )
            {
                Src = 3 * TGAImage[N];

//              The palette is in blue-green-red order, so we read it backwards
                RGB[N*3] = Palette[Src+2];
                RGB[(N*3)+1] = Palette[Src+1];
                RGB[(N*3)+2] = Palette[Src];
            }

            delete [] TGAImage;
        }
        else if( TGAHeader[2] == 2 )
        {
            Flags = PICTURE_TARGA | PICTURE_UNCOMPRESSED | PICTURE_24BIT;

            TGAImage = new BYTE [Width * Height * 3];
            if( !File->Input( TGAImage, Width * Height * 3 ) )
            {
                delete [] TGAImage;
                File->Seek( Front + FileSize, STREAM_BEGINNING );
                return false;
            }

            RGB = new BYTE [Width * Height * 3];

            for( long N = 0; N < Width * Height * 3; N += 3 )
            {
//              The image data is in blue-green-red format, so reverse it
                RGB[N] = TGAImage[N+2];
                RGB[N+1] = TGAImage[N+1];
                RGB[N+2] = TGAImage[N];
            }

            delete [] TGAImage;
        }
        else if( TGAHeader[2] == 3 )
        {
            Flags = PICTURE_TARGA | PICTURE_UNCOMPRESSED | PICTURE_8BIT | PICTURE_GRAYSCALE;

            TGAImage = new BYTE [Width * Height];
            if( !File->Input( TGAImage, Width * Height ) )
            {
                delete [] TGAImage;
                File->Seek( Front + FileSize, STREAM_BEGINNING );
                return false;
            }

            RGB = new BYTE [Width * Height * 3];

            for( long N = 0; N < Width * Height; N ++ )
            {
                RGB[N*3] = TGAImage[N];
                RGB[(N*3)+1] = TGAImage[N];
                RGB[(N*3)+2] = TGAImage[N];
            }

            delete [] TGAImage;
        }

/*      The following TGA formats are RLE encoded. This means the image
        data comes in two types of packets: RLE packets and raw packets.

        RLE Packets: If there is a row of 12 red pixels in a 24-bit
                     image, the RLE packet consists of one byte whose
                     value is 12, and three more bytes whose values are
                     255,0,0. Red=255, Green=0, Blue=0.

        Raw Packets: For parts of the image that don't have nice long rows
                     of the same color, rather than use RLE packets for
                     each individual pixel, it's fastest to do it with raw
                     packets. If, after the last packet read, there is a
                     red pixel, a green pixel, and a yellow pixel, and then
                     a row of blue pixels, the first byte of the raw packet
                     will be 3, and it will be followed by the pixel data
                     for the red, green, and yellow pixels. Then a RLE
                     packet will be written for the blue pixels.

        That's basically the theory behind RLE encoding. The way TGAs use it,
        the highest bit of the first byte of the packet is used to tell whether
        it's a RLE or raw packet. Then the other 7 bits are used for the length
        of the packet pixels. Also packets cannot span several scanlines. So if
        you have a 8x8 image that's pure white, then theoretically you'd be able
        to use one RLE packet for the entire thing. But TGAs don't allow that.
        You'd need 8 RLE packets, one for each scanline.
*/
        else if( TGAHeader[2] == 9 )
        {
            Flags = PICTURE_TARGA | PICTURE_COMPRESSED | PICTURE_8BIT;

            TGANumColors = ((WORD*)(TGAHeader+1))[2];

/*          Unlike bitmaps, Targas support palettes that are in other
            bit depths besides 24-bit. But I haven't seen any and I am
            lazy so this code ignores them.
*/
            if( TGAHeader[7] != 24 )
            {
                File->Seek( Front + FileSize, STREAM_BEGINNING );
                return false;
            }

            if( !File->Input( Palette, 3 * TGANumColors ) )
            {
                File->Seek( Front + FileSize, STREAM_BEGINNING );
                return false;
            }

            TGAImage = new BYTE [Width * Height];

            for( int H = 0; H < Height; H ++ )
            {
                CurrPixel = 0;

                while( CurrPixel < Width )
                {
                    if( !File->Input( &TGAPacket, 1 ) )
                    {
                        File->Seek( Front + FileSize, STREAM_BEGINNING );
                        delete [] TGAImage;
                        return false;
                    }

                    TGANumPixels = 1 + (TGAPacket & 0x7F);

                    if( TGAPacket & 0x80 ) // Run-length packet
                    {
                        if( !File->Input( TGAScanline, 1 ) )
                        {
                            File->Seek( Front + FileSize, STREAM_BEGINNING );
                            delete [] TGAImage;
                            return false;
                        }

                        for( int N = 0; N < TGANumPixels; N ++ )
                        {
                            TGAImage[(H*Width) + CurrPixel] = TGAScanline[0];
                            CurrPixel ++;
                        }
                    }
                    else // Raw packet
                    {
                        if( !File->Input( TGAScanline, TGANumPixels ) )
                        {
                            File->Seek( Front + FileSize, STREAM_BEGINNING );
                            delete [] TGAImage;
                            return false;
                        }

                        for( int N = 0; N < TGANumPixels; N ++ )
                        {
                            TGAImage[(H*Width) + CurrPixel] = TGAScanline[N];
                            CurrPixel ++;
                        }
                    }
                }
            }

            RGB = new BYTE [Width * Height * 3];
            for( int N = 0; N < Width * Height; N ++ )
            {
                Src = 3 * TGAImage[N];

                RGB[N*3] = Palette[Src+2];
                RGB[(N*3)+1] = Palette[Src+1];
                RGB[(N*3)+2] = Palette[Src];
            }

            delete [] TGAImage;
        }
        else if( TGAHeader[2] == 10 )
        {
            Flags = PICTURE_TARGA | PICTURE_COMPRESSED | PICTURE_24BIT;

            TGAImage = new BYTE [Width * Height * 3];

            for( int H = 0; H < Height; H ++ )
            {
                CurrPixel = 0;

                while( CurrPixel < Width * 3 )
                {
                    if( !File->Input( &TGAPacket, 1 ) )
                    {
                        File->Seek( Front + FileSize, STREAM_BEGINNING );
                        delete [] TGAImage;
                        return false;
                    }

                    TGANumPixels = 1 + (TGAPacket & 0x7F);

                    if( TGAPacket & 0x80 ) // Run-length packet
                    {
                        if( !File->Input( TGAScanline, 3 ) )
                        {
                            File->Seek( Front + FileSize, STREAM_BEGINNING );
                            delete [] TGAImage;
                            return false;
                        }

                        for( int N = 0; N < TGANumPixels; N ++ )
                        {
                            TGAImage[(H*Width*3) + CurrPixel] = TGAScanline[0];
                            TGAImage[(H*Width*3) + CurrPixel+1] = TGAScanline[1];
                            TGAImage[(H*Width*3) + CurrPixel+2] = TGAScanline[2];

                            CurrPixel += 3;
                        }
                    }
                    else // Raw packet
                    {
                        if( !File->Input( TGAScanline, TGANumPixels * 3 ) )
                        {
                            File->Seek( Front + FileSize, STREAM_BEGINNING );
                            delete [] TGAImage;
                            return false;
                        }

                        for( int N = 0; N < TGANumPixels * 3; N += 3 )
                        {
                            TGAImage[(H*Width*3) + CurrPixel] = TGAScanline[N];
                            TGAImage[(H*Width*3) + CurrPixel+1] = TGAScanline[N+1];
                            TGAImage[(H*Width*3) + CurrPixel+2] = TGAScanline[N+2];

                            CurrPixel += 3;
                        }
                    }
                }
            }

            RGB = new BYTE [Width * Height * 3];
            for( int N = 0; N < Width * Height * 3; N += 3 )
            {
                RGB[N] = TGAImage[N+2];
                RGB[N+1] = TGAImage[N+1];
                RGB[N+2] = TGAImage[N];
            }

            delete [] TGAImage;
        }
        else if( TGAHeader[2] == 11 )
        {
            Flags = PICTURE_TARGA | PICTURE_COMPRESSED | PICTURE_8BIT | PICTURE_GRAYSCALE;

            TGAImage = new BYTE [Width * Height];

            for( int H = 0; H < Height; H ++ )
            {
                CurrPixel = 0;

                while( CurrPixel < Width )
                {
                    if( !File->Input( &TGAPacket, 1 ) )
                    {
                        File->Seek( Front + FileSize, STREAM_BEGINNING );
                        delete [] TGAImage;
                        return false;
                    }

                    TGANumPixels = 1 + (TGAPacket & 0x7F);

                    if( TGAPacket & 0x80 ) // Run-length packet
                    {
                        if( !File->Input( TGAScanline, 1 ) )
                        {
                            File->Seek( Front + FileSize, STREAM_BEGINNING );
                            delete [] TGAImage;
                            return false;
                        }

                        for( int N = 0; N < TGANumPixels; N ++ )
                        {
                            TGAImage[(H*Width) + CurrPixel] = TGAScanline[0];
                            CurrPixel ++;
                        }
                    }
                    else // Raw packet
                    {
                        if( !File->Input( TGAScanline, TGANumPixels ) )
                        {
                            File->Seek( Front + FileSize, STREAM_BEGINNING );
                            delete [] TGAImage;
                            return false;
                        }

                        for( int N = 0; N < TGANumPixels; N ++ )
                        {
                            TGAImage[(H*Width) + CurrPixel] = TGAScanline[N];
                            CurrPixel ++;
                        }
                    }
                }
            }

            RGB = new BYTE [Width * Height * 3];
            for( int N = 0; N < Width * Height; N ++ )
            {
                RGB[N*3] = TGAImage[N];
                RGB[(N*3)+1] = TGAImage[N];
                RGB[(N*3)+2] = TGAImage[N];
            }

            delete [] TGAImage;
        }
        else
        {
//          Unsupported image type, but we supported all six,
//          so this should never happen.
            File->Seek( Front + FileSize, STREAM_BEGINNING );
            return false;
        }
    }

    return true; // Success! Yay!
}
