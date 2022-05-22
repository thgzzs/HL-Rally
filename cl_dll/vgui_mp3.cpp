//
// vgui_mp3.cpp
//
// VGUI MP3 Player with Winamp2 skin support
//
// by SaRcaZm
//

// 
// Includes
//
#include <direct.h>

#include "VGUI_Int.h"
#include "VGUI_DataInputStream.h"
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"
#include "VGUI_Color.h"

#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"
#include "..\game_shared\vgui_slider2.h"
#include "..\game_shared\vgui_listbox.h"

#include "rally_vgui.h"		// Need the custom classes from here
#include "rally_sound.h"	// MP3 functions

// 
// Definitions
//
#define MAINWIN_SIZE_X			275
#define MAINWIN_SIZE_Y			116
#define MAINWIN_POS_X			0
#define MAINWIN_POS_Y			0

#define TITLEBAR_IMG_X			27
#define TITLEBAR_IMG_Y			0
#define TITLEBAR_IMG_SIZE_X		MAINWIN_SIZE_X
#define TITLEBAR_IMG_SIZE_Y		14
#define TITLEBAR_POS_X			0
#define TITLEBAR_POS_Y			0

#define BUTTON_BACK_IMG_UP_X	0
#define BUTTON_BACK_IMG_UP_Y	0
#define BUTTON_BACK_IMG_DWN_X	BUTTON_BACK_IMG_UP_X
#define BUTTON_BACK_IMG_DWN_Y	18
#define BUTTON_BACK_POS_X		16
#define BUTTON_BACK_POS_Y		88
#define BUTTON_SIZE_X			23
#define BUTTON_SIZE_Y			18

#define BUTTON_SHUF_IMG_OFF_UP_X	28
#define BUTTON_SHUF_IMG_OFF_UP_Y	0
#define BUTTON_SHUF_IMG_OFF_DWN_X	BUTTON_SHUF_IMG_OFF_UP_X
#define BUTTON_SHUF_IMG_OFF_DWN_Y	15
#define BUTTON_SHUF_IMG_ON_UP_X		BUTTON_SHUF_IMG_OFF_UP_X
#define BUTTON_SHUF_IMG_ON_UP_Y		30
#define BUTTON_SHUF_IMG_ON_DWN_X	BUTTON_SHUF_IMG_OFF_UP_X
#define BUTTON_SHUF_IMG_ON_DWN_Y	45
#define BUTTON_SHUF_POS_X			164
#define BUTTON_SHUF_POS_Y			89
#define BUTTON_SHUF_SIZE_X			47
#define BUTTON_SHUF_SIZE_Y			15

#define BUTTON_REP_IMG_OFF_UP_X		0
#define BUTTON_REP_IMG_OFF_UP_Y		0
#define BUTTON_REP_IMG_OFF_DWN_X	BUTTON_REP_IMG_OFF_UP_X
#define BUTTON_REP_IMG_OFF_DWN_Y	15
#define BUTTON_REP_IMG_ON_UP_X		BUTTON_REP_IMG_OFF_UP_X
#define BUTTON_REP_IMG_ON_UP_Y		30
#define BUTTON_REP_IMG_ON_DWN_X		BUTTON_REP_IMG_OFF_UP_X
#define BUTTON_REP_IMG_ON_DWN_Y		45
#define BUTTON_REP_POS_X			210
#define BUTTON_REP_POS_Y			89
#define BUTTON_REP_SIZE_X			28
#define BUTTON_REP_SIZE_Y			15

#define BUTTON_EQ_IMG_X			0
#define BUTTON_EQ_IMG_Y			61
#define BUTTON_EQ_POS_X			219
#define BUTTON_EQ_POS_Y			58
#define BUTTON_EQ_SIZE_X		23
#define BUTTON_EQ_SIZE_Y		12
#define BUTTON_EQ_GAP_X			1

#define BUTTON_PL_IMG_ON_UP_X	BUTTON_EQ_IMG_X + BUTTON_EQ_SIZE_X
#define BUTTON_PL_IMG_ON_UP_Y	BUTTON_EQ_IMG_Y + BUTTON_EQ_SIZE_Y
#define BUTTON_PL_IMG_ON_DWN_X	69
#define BUTTON_PL_IMG_ON_DWN_Y	BUTTON_PL_IMG_ON_UP_Y
#define BUTTON_PL_IMG_OFF_UP_X	BUTTON_EQ_IMG_X + BUTTON_EQ_SIZE_X
#define BUTTON_PL_IMG_OFF_UP_Y	BUTTON_EQ_IMG_Y
#define BUTTON_PL_IMG_OFF_DWN_X	BUTTON_PL_IMG_ON_DWN_X
#define BUTTON_PL_IMG_OFF_DWN_Y	BUTTON_PL_IMG_OFF_UP_Y

#define BUTTON_CLOSE_IMG_UP_X	18
#define BUTTON_CLOSE_IMG_UP_Y	0
#define BUTTON_CLOSE_IMG_DWN_X	BUTTON_CLOSE_IMG_UP_X
#define BUTTON_CLOSE_IMG_DWN_Y	9
#define BUTTON_CLOSE_POS_X		263
#define BUTTON_CLOSE_POS_Y		3
#define BUTTON_CLOSE_SIZE_X		9
#define BUTTON_CLOSE_SIZE_Y		9

#define MONO_IMG_ON_X			28
#define MONO_IMG_ON_Y			0
#define MONO_IMG_OFF_X			MONO_IMG_ON_X
#define MONO_IMG_OFF_Y			12
#define MONO_POS_X				212
#define MONO_POS_Y				41
#define MONO_SIZE_X				28
#define MONO_SIZE_Y				12

#define STEREO_IMG_ON_X			0
#define STEREO_IMG_ON_Y			0
#define STEREO_IMG_OFF_X		STEREO_IMG_ON_X	
#define STEREO_IMG_OFF_Y		12
#define STEREO_POS_X			239
#define STEREO_POS_Y			MONO_POS_Y
#define STEREO_SIZE_X			MONO_SIZE_X
#define STEREO_SIZE_Y			MONO_SIZE_Y

#define SLIDER_VOL_IMG_X		0
#define SLIDER_VOL_IMG_Y		0
#define SLIDER_VOL_POS_X		107
#define SLIDER_VOL_POS_Y		57
#define SLIDER_VOL_SIZE_X		68
#define SLIDER_VOL_SIZE_Y		13
#define SLIDER_VOL_GAP_Y		2
#define SLIDER_VOL_POINT_IMG_X	0
#define SLIDER_VOL_POINT_IMG_Y	420
#define SLIDER_VOL_POINT_SIZE_X	14
#define SLIDER_VOL_POINT_SIZE_Y	13
#define SLIDER_VOL_POINT_GAP_X	1

#define SLIDER_BAL_IMG_X		9
#define SLIDER_BAL_IMG_Y		0
#define SLIDER_BAL_POS_X		177
#define SLIDER_BAL_POS_Y		57
#define SLIDER_BAL_SIZE_X		38
#define SLIDER_BAL_SIZE_Y		13
#define SLIDER_BAL_GAP_Y		2
#define SLIDER_BAL_POINT_IMG_X	0
#define SLIDER_BAL_POINT_IMG_Y	420
#define SLIDER_BAL_POINT_SIZE_X	14
#define SLIDER_BAL_POINT_SIZE_Y	13
#define SLIDER_BAL_POINT_GAP_X	1

#define TIME_IMG_X				0
#define TIME_IMG_Y				0
#define TIME_SIZE_X				9
#define TIME_SIZE_Y				13
#define TIME_POS_X				48
#define TIME_POS_Y				26
#define TIME_GAP_X				3
#define TIME_GAP_SECS_X			9

#define POSBAR_IMG_X			0
#define POSBAR_IMG_Y			0
#define POSBAR_POS_X			16
#define POSBAR_POS_Y			72
#define POSBAR_SIZE_X			248
#define POSBAR_SIZE_Y			10
#define POSBAR_POINT_SIZE_X		29
#define POSBAR_POINT_SIZE_Y		POSBAR_SIZE_Y
#define POSBAR_POINT_UP_IMG_X	POSBAR_SIZE_X
#define POSBAR_POINT_UP_IMG_Y	0
#define POSBAR_POINT_DWN_IMG_X	POSBAR_POINT_UP_IMG_X + POSBAR_POINT_SIZE_X + 1
#define POSBAR_POINT_DWN_IMG_Y	POSBAR_POINT_UP_IMG_Y

#define FONT_WIDTH				5
#define FONT_HEIGHT				6

#define TITLE_IMG_X				0
#define TITLE_IMG_Y				0
#define TITLE_POS_X				112
#define TITLE_POS_Y				26
#define TITLE_SIZE_X			153
#define TITLE_SIZE_Y			FONT_HEIGHT

#define KBPS_IMG_X				0
#define KBPS_IMG_Y				0
#define KBPS_POS_X				110
#define KBPS_POS_Y				43
#define KBPS_SIZE_X				16
#define KBPS_SIZE_Y				FONT_HEIGHT

#define KHZ_IMG_X				0
#define KHZ_IMG_Y				0
#define KHZ_POS_X				156
#define KHZ_POS_Y				43
#define KHZ_SIZE_X				11
#define KHZ_SIZE_Y				FONT_HEIGHT

#define PLAYLIST_POS_X			0
#define PLAYLIST_POS_Y			116
#define PLAYLIST_SIZE_X			275
#define PLAYLIST_SIZE_Y			300
#define PLAYLIST_IMG_SIZE_X		280
#define PLAYLIST_IMG_SIZE_Y		186

#define PLAYLIST_TOPLEFT_IMG_X		0
#define PLAYLIST_TOPLEFT_IMG_Y		0
#define PLAYLIST_TOPLEFT_IMG_SIZE_X	25
#define PLAYLIST_TOPLEFT_IMG_SIZE_Y	20
#define PLAYLIST_TOPLEFT_POS_X		0
#define PLAYLIST_TOPLEFT_POS_Y		0

#define PLAYLIST_TOPRIGHT_IMG_X			153
#define PLAYLIST_TOPRIGHT_IMG_Y			0
#define PLAYLIST_TOPRIGHT_IMG_SIZE_X	PLAYLIST_TOPLEFT_IMG_SIZE_X
#define PLAYLIST_TOPRIGHT_IMG_SIZE_Y	PLAYLIST_TOPLEFT_IMG_SIZE_Y
#define PLAYLIST_TOPRIGHT_POS_X			PLAYLIST_SIZE_X - PLAYLIST_TOPRIGHT_IMG_SIZE_X
#define PLAYLIST_TOPRIGHT_POS_Y			0

#define PLAYLIST_TITLE_IMG_X		26
#define PLAYLIST_TITLE_IMG_Y		0
#define PLAYLIST_TITLE_IMG_SIZE_X	100
#define PLAYLIST_TITLE_IMG_SIZE_Y	20
#define PLAYLIST_TITLE_POS_X		(PLAYLIST_SIZE_X / 2) - (PLAYLIST_TITLE_IMG_SIZE_X / 2)
#define PLAYLIST_TITLE_POS_Y		0

#define PLAYLIST_TITLE_REP_LEFT_IMG_X		127
#define PLAYLIST_TITLE_REP_LEFT_IMG_Y		0
#define PLAYLIST_TITLE_REP_LEFT_IMG_SIZE_X	25
#define PLAYLIST_TITLE_REP_LEFT_IMG_SIZE_Y	20
#define PLAYLIST_TITLE_REP_LEFT_POS_X		PLAYLIST_TOPLEFT_IMG_SIZE_X
#define PLAYLIST_TITLE_REP_LEFT_POS_Y		0
#define PLAYLIST_TITLE_REP_LEFT_SIZE_X		PLAYLIST_TITLE_POS_X - PLAYLIST_TITLE_REP_LEFT_POS_X
#define PLAYLIST_TITLE_REP_LEFT_SIZE_Y		20

#define PLAYLIST_TITLE_REP_RIGHT_IMG_X		PLAYLIST_TITLE_REP_LEFT_IMG_X
#define PLAYLIST_TITLE_REP_RIGHT_IMG_Y		PLAYLIST_TITLE_REP_LEFT_IMG_Y
#define PLAYLIST_TITLE_REP_RIGHT_IMG_SIZE_X	PLAYLIST_TITLE_REP_LEFT_IMG_SIZE_X
#define PLAYLIST_TITLE_REP_RIGHT_IMG_SIZE_Y	PLAYLIST_TITLE_REP_LEFT_IMG_SIZE_Y
#define PLAYLIST_TITLE_REP_RIGHT_POS_X		PLAYLIST_TITLE_POS_X + PLAYLIST_TITLE_IMG_SIZE_X
#define PLAYLIST_TITLE_REP_RIGHT_POS_Y		0
#define PLAYLIST_TITLE_REP_RIGHT_SIZE_X		PLAYLIST_TOPRIGHT_IMG_X - PLAYLIST_TITLE_REP_RIGHT_POS_X
#define PLAYLIST_TITLE_REP_RIGHT_SIZE_Y		PLAYLIST_TITLE_REP_LEFT_SIZE_Y

#define PLAYLIST_BOTTOM_LEFT_SIZE_Y	38

#define PLAYLIST_LEFT_IMG_X			0
#define PLAYLIST_LEFT_IMG_Y			42
#define PLAYLIST_LEFT_IMG_SIZE_X	25
#define PLAYLIST_LEFT_IMG_SIZE_Y	29
#define PLAYLIST_LEFT_POS_X			0
#define PLAYLIST_LEFT_POS_Y			PLAYLIST_TITLE_IMG_SIZE_Y
#define PLAYLIST_LEFT_SIZE_X		PLAYLIST_LEFT_IMG_SIZE_X
#define PLAYLIST_LEFT_SIZE_Y		PLAYLIST_SIZE_Y - PLAYLIST_BOTTOM_LEFT_SIZE_Y - PLAYLIST_LEFT_POS_Y

#define PLAYLIST_RIGHT_IMG_X		26
#define PLAYLIST_RIGHT_IMG_Y		PLAYLIST_LEFT_IMG_Y
#define PLAYLIST_RIGHT_IMG_SIZE_X	PLAYLIST_LEFT_IMG_SIZE_X
#define PLAYLIST_RIGHT_IMG_SIZE_Y	PLAYLIST_LEFT_IMG_SIZE_Y
#define PLAYLIST_RIGHT_POS_X		PLAYLIST_SIZE_X - PLAYLIST_RIGHT_IMG_SIZE_X
#define PLAYLIST_RIGHT_POS_Y		PLAYLIST_LEFT_POS_Y
#define PLAYLIST_RIGHT_SIZE_X		PLAYLIST_LEFT_SIZE_X
#define PLAYLIST_RIGHT_SIZE_Y		PLAYLIST_LEFT_SIZE_Y

#define PLAYLIST_BOTTOMLEFT_IMG_X		0
#define PLAYLIST_BOTTOMLEFT_IMG_Y		72
#define PLAYLIST_BOTTOMLEFT_IMG_SIZE_X	125
#define PLAYLIST_BOTTOMLEFT_IMG_SIZE_Y	38
#define PLAYLIST_BOTTOMLEFT_POS_X		0
#define PLAYLIST_BOTTOMLEFT_POS_Y		PLAYLIST_LEFT_POS_Y + PLAYLIST_LEFT_SIZE_Y

#define PLAYLIST_BOTTOMRIGHT_IMG_X		126
#define PLAYLIST_BOTTOMRIGHT_IMG_Y		PLAYLIST_BOTTOMLEFT_IMG_Y
#define PLAYLIST_BOTTOMRIGHT_IMG_SIZE_X	150
#define PLAYLIST_BOTTOMRIGHT_IMG_SIZE_Y	PLAYLIST_BOTTOMLEFT_IMG_SIZE_Y
#define PLAYLIST_BOTTOMRIGHT_POS_X		PLAYLIST_SIZE_X - PLAYLIST_BOTTOMRIGHT_IMG_SIZE_X
#define PLAYLIST_BOTTOMRIGHT_POS_Y		PLAYLIST_BOTTOMLEFT_POS_Y

#define PLAYLIST_SCROLLBAR_DWN_IMG_X	61
#define PLAYLIST_SCROLLBAR_DWN_IMG_Y	53
#define PLAYLIST_SCROLLBAR_IMG_SIZE_X	8
#define PLAYLIST_SCROLLBAR_IMG_SIZE_Y	18
#define PLAYLIST_SCROLLBAR_IMG_GAP_X	1
#define PLAYLIST_SCROLLBAR_POS_X		PLAYLIST_RIGHT_POS_X + 10
#define PLAYLIST_SCROLLBAR_POS_Y		19
#define PLAYLIST_SCROLLBAR_SIZE_X		PLAYLIST_SCROLLBAR_IMG_SIZE_X
#define PLAYLIST_SCROLLBAR_SIZE_Y		PLAYLIST_RIGHT_SIZE_Y + 2

//
// Custom Helper Functions
//

__inline char *getSkinDir (void)
{
	return gEngfuncs.pfnGetCvarString ("rally_mp3skin");
	//return "default";
}

class CBitmapMP3 : public Bitmap {
public:
	CBitmapMP3 (int src_width, int src_height) : Bitmap ()
	{
		setSize (src_width, src_height);
		src_rgba = (unsigned char *) malloc (src_width * src_height * 4);
	}

	CBitmapMP3 (int src_width, int src_height, unsigned char *rgba) : Bitmap ()
	{
		setSize (src_width, src_height);
		src_rgba = (unsigned char *) malloc (src_width * src_height * 4);
		setImage (rgba);
	}

	~CBitmapMP3 (void)
	{
		free (src_rgba);
		Bitmap::~Bitmap ();
	}

	void setSize (int width, int height)
	{
		m_iWidth = width;
		m_iHeight = height;
		Bitmap::setSize (width, height);
	}

	void resize (int width, int height)
	{
		// Save off the old values
		int oldwidth = m_iWidth;
		int oldheight = m_iHeight;

		setSize (width, height);

		// Resize the bitmap to fit
		// Copy the current data
		free (src_rgba);
		src_rgba = (unsigned char *) malloc (width * height * 4);

		// Scale it
		ScaleRectAvg ((unsigned long *)src_rgba, (unsigned long *)_rgba,
			oldwidth, oldheight, m_iWidth, m_iHeight);
		// Draw it
		drawImage ();
	}

	void setImage (unsigned char *rgba)
	{
		// Assume rgba is the right size and doesn't need scaling
		memcpy (src_rgba, rgba, m_iWidth * m_iHeight * 4);
		// Draw it
		drawImage ();
	}

	void drawImage (void)
	{
		for (int i = 0; i < m_iHeight; i++)
		{
			for (int j = 0; j < m_iWidth; j++)
			{
				byte *pixel = (src_rgba + (i * m_iWidth * 4) + (j * 4));
				setRGBA (j, i, *(pixel), *(pixel + 1), *(pixel + 2), *(pixel + 3));
			}
		}
	}

	void setImage (unsigned char *rgba, int width, int height)
	{
		int oldwidth = m_iWidth;
		int oldheight = m_iHeight;

		setSize (width, height);

		// Resize the bitmap to fit
		// Copy the current data
		free (src_rgba);
		src_rgba = (unsigned char *) malloc (width * height * 4);

		// Scale it
		ScaleRectAvg ((unsigned long *)src_rgba, (unsigned long *)rgba,
			oldwidth, oldheight, m_iWidth, m_iHeight);

		// Draw it
		drawImage ();
	}

private:
	int m_iWidth, m_iHeight;
	unsigned char *src_rgba;
};

CBitmapMP3 *createCBitmapMP3Region (CBitmapMP3 *bmp, unsigned char *rgba, int x, int y, int width, int height, int src_width, int src_height, int bpp)
{
	// Setup
	unsigned char *buffer = (unsigned char *) malloc (width * height * 4);
	byte *pointer = buffer;

	// They seem to be stored upside down in memory
	int start = src_height - y - 1;
	int end = start - height;
	for (int i = start; i > end; i--)
	{
		for (int j = x; j < x + width; j++)
		{
			// j, i points to the next pixel
			if (bpp == 32)
			{
				// Copy the pixels 1 by 1
				byte *pixel = (rgba + (i * src_width * 4) + (j * 4));
				*pointer++ = *(pixel);
				*pointer++ = *(pixel + 1);
				*pointer++ = *(pixel + 2);
				*pointer++ = *(pixel + 3);
			}
			else if (bpp == 24)
			{
				// Copy the pixels 1 by 1
				byte *pixel = (rgba + (i * src_width * 3) + (j * 3));

				*pointer++ = *(pixel);
				*pointer++ = *(pixel + 1);
				*pointer++ = *(pixel + 2);
				*pointer++ = 255;
			}
		}
	}

	// Create the bitmap
	if (bmp == NULL)
		bmp = new CBitmapMP3 (width, height, buffer);
	else
		bmp->setImage (buffer);

	free (buffer);
	return bmp;
}

CBitmapMP3 *createCBitmapMP3 (CBitmapMP3 *bmp, unsigned char *rgba, int width, int height, int bpp)
{
	return createCBitmapMP3Region (bmp, rgba, 0, 0, width, height, width, height, bpp);
}

CBitmapMP3 *createTiledCBitmapMP3Region (CBitmapMP3 *bmp, unsigned char *rgba, int x, int y, int width, int height, int dest_width, int dest_height, int src_width, int src_height, int bpp)
{
	// Setup
	unsigned char *buffer = (unsigned char *) malloc (dest_width * dest_height * 4);
	byte *pointer = buffer;

	// They seem to be stored upside down in memory
	int start = src_height - y - 1;
	int end = start - height;
	int posy = start;

	for (int i = 0; i < dest_height; i++)
	{
		int posx = x;
		for (int j = 0; j < dest_width; j++)
		{
			// posx, posy points to the next pixel
			if (bpp == 32)
			{
				// Copy the pixels 1 by 1
				byte *pixel = (rgba + (posy * src_width * 4) + (posx * 4));
				*pointer++ = *(pixel);
				*pointer++ = *(pixel + 1);
				*pointer++ = *(pixel + 2);
				*pointer++ = *(pixel + 3);
			}
			else if (bpp == 24)
			{
				// Copy the pixels 1 by 1
				byte *pixel = (rgba + (posy * src_width * 3) + (posx * 3));

				*pointer++ = *(pixel);
				*pointer++ = *(pixel + 1);
				*pointer++ = *(pixel + 2);
				*pointer++ = 255;
			}

			posx++;
			if (posx >= x + width)
				posx = x;
		}

		posy--;
		if (posy <= end)
			posy = start;
	}

	// Create the bitmap
	if (bmp == NULL)
		bmp = new CBitmapMP3 (dest_width, dest_height, buffer);
	else
		bmp->setImage (buffer);

	free (buffer);
	return bmp;
}

// This isn't needed for now
CBitmapMP3 *createScaledCBitmapMP3Region (CBitmapMP3 *bmp, unsigned char *rgba, int x, int y, int width, int height, int dest_width, int dest_height, int src_width, int src_height, int bpp)
{	
	// Setup
	unsigned char *buffer = (unsigned char *) malloc (width * height * 4);
	byte *pointer = buffer;

	// They seem to be stored upside down in memory
	int start = src_height - y - 1;
	int end = start - height;
	for (int i = start; i > end; i--)
	{
		for (int j = x; j < x + width; j++)
		{
			// j, i points to the next pixel
			if (bpp == 32)
			{
				// Copy the pixels 1 by 1
				byte *pixel = (rgba + (i * src_width * 4) + (j * 4));
				*pointer++ = *(pixel);
				*pointer++ = *(pixel + 1);
				*pointer++ = *(pixel + 2);
				*pointer++ = *(pixel + 3);
			}
			else if (bpp == 24)
			{
				// Copy the pixels 1 by 1
				byte *pixel = (rgba + (i * src_width * 3) + (j * 3));

				*pointer++ = *(pixel);
				*pointer++ = *(pixel + 1);
				*pointer++ = *(pixel + 2);
				*pointer++ = 255;
			}
		}
	}

	// Create the bitmap
	if (bmp == NULL)
	{
		bmp = new CBitmapMP3 (width, height);
		bmp->setImage (buffer, dest_width, dest_height);
	}
	else
	{
		bmp->setSize (width, height);
		bmp->setImage (buffer, dest_width, dest_height);
	}

	free (buffer);
	return bmp;
}

//
// Console Commands
//
void ShowMP3Panel (void)
{
	gViewPort->ShowVGUIMenu (10);
}

void MP3Command (void)
{
	int cmds = gEngfuncs.Cmd_Argc ();
	if (cmds < 2)	// Check that we have enough commands
		return;

	// Concatenate the arguments into 1 long string
	char args[256];
	strcpy (args, gEngfuncs.Cmd_Argv (1));
	for (int i = 2; i < cmds; i++)
	{
		strcat (args, " ");
		strcat (args, gEngfuncs.Cmd_Argv (i));
	}

	// Send the command to the player
	gViewPort->mp3Command (args);
}

char rally_playlist[80];
void MP3Playlist (void)
{
	int cmds = gEngfuncs.Cmd_Argc ();
	if (cmds < 2)	// Just want the value
	{
		gEngfuncs.Con_Printf ("\"rally_mp3playlist\" is \"%s\"\n", rally_playlist);
		return;
	}

	// Concatenate the arguments to allow for spaces in the path
	char args[256];
	strcpy (args, gEngfuncs.Cmd_Argv (1));
	for (int i = 2; i < cmds; i++)
	{
		strcat (args, " ");
		strcat (args, gEngfuncs.Cmd_Argv (i));
	}
	strcpy (rally_playlist, args);
}

//
// Component Classes
//
class CMenuHandler_RallyMP3Command : public ActionSignal
{
public:
	CMenuHandler_RallyMP3Command( char *pszCommand )
	{
		strcpy (m_szCommand, pszCommand);
	}

	virtual void actionPerformed(Panel* panel)
	{
		gViewPort->mp3Command (m_szCommand);
	}
protected:
	char	m_szCommand[10];
};

class CSkinnedComponent : public Panel
{
public:
	CSkinnedComponent (char *szFile) : Panel ()
	{
		// Setup
		strcpy (m_szFile, szFile);
		m_iX = 0;
		m_iY = 0;
		m_iWidth = -1;
		m_iHeight = -1;

		m_pTGA = NULL;
	}

	CSkinnedComponent (char *szFile, int src_width, int src_height) : Panel ()
	{
		// Setup
		strcpy (m_szFile, szFile);
		m_iX = 0;
		m_iY = 0;
		m_iWidth = src_width;
		m_iHeight = src_height;

		m_pTGA = NULL;
	}

	CSkinnedComponent (char *szFile, int x, int y, int src_width, int src_height) : Panel ()
	{
		// Setup
		strcpy (m_szFile, szFile);
		m_iX = x;
		m_iY = y;
		m_iWidth = src_width;
		m_iHeight = src_height;

		m_pTGA = NULL;
	}

	~CSkinnedComponent (void)
	{
		unloadBMP ();
	}

	void paint (void)
	{
		if (m_pTGA == NULL)
			return;

		m_pTGA->doPaint (this);
	}

	void paintBackground (void)
	{
		// Do nothing, so the background's left transparent.
	}

	void reloadBMP ()
	{
		//unloadBMP ();
		loadBMP ();
	}

private:
	void unloadBMP (void)
	{
		if (m_pTGA)
		{
			delete m_pTGA;
			m_pTGA = NULL;
		}
	}

	void loadBMP (void)
	{
		byte *rgb;
		int wide, tall;

		// Load the BMP
		strcpy (m_szSkinDir, getSkinDir ());
		sprintf (m_szPath, "hlrally/gfx/skins/%s/%s.bmp", m_szSkinDir, m_szFile);
		rgb = LoadBMP (m_szPath, &wide, &tall);

		// Check the size of the image is correct
		if (m_iWidth < 0)
			m_iWidth = wide;
		if (m_iHeight < 0)
			m_iHeight = tall;

		if (!rgb || (m_iX < 0) || (m_iY < 0) || (m_iX + m_iWidth > wide) || (m_iY + m_iHeight > tall))
		{
			gEngfuncs.Con_DPrintf ("Error loading skin from 'hlrally/gfx/skins/%s/%s.bmp'\nCheck that the file is 8, 24 or 32 bit", m_szSkinDir, m_szFile);

			if (rgb)
				free (rgb);
			m_pTGA = NULL;
			return;
		}
		// Create the TGA for the VGUI
		m_pTGA = createCBitmapMP3Region (m_pTGA, rgb, m_iX, m_iY, m_iWidth, m_iHeight, wide, tall, 24);

		// Clean up
		setSize (m_iWidth, m_iHeight);
		free (rgb);
	}

	CBitmapMP3 *m_pTGA;
	char m_szPath[128], m_szFile[80];
	char m_szSkinDir[30];
	int m_iX, m_iY, m_iWidth, m_iHeight;
};

class CSkinnedTiledComponent : public Panel
{
public:
	CSkinnedTiledComponent (char *szFile, int x, int y, int src_width, int src_height, int panel_width, int panel_height) : Panel ()
	{
		// Setup
		strcpy (m_szFile, szFile);
		m_iX = x;
		m_iY = y;
		m_iWidth = src_width;
		m_iHeight = src_height;
		m_iPanelWidth = panel_width;
		m_iPanelHeight = panel_height;

		setSize (m_iPanelWidth, m_iPanelHeight);
		m_pTGA = NULL;
	}

	~CSkinnedTiledComponent (void)
	{
		unloadBMP ();
	}

	void paint (void)
	{
		if (m_pTGA == NULL)
			return;
		m_pTGA->doPaint (this);
	}

	void paintBackground (void)
	{
		// Do nothing, so the background's left transparent.
	}

	void reloadBMP ()
	{
		//unloadBMP ();
		loadBMP ();
	}

private:
	void unloadBMP (void)
	{
		if (m_pTGA)
		{
			delete m_pTGA;
			m_pTGA = NULL;
		}
	}

	void loadBMP (void)
	{
		byte *rgb;
		int wide, tall;

		// Load the BMP
		strcpy (m_szSkinDir, getSkinDir ());
		sprintf (m_szPath, "hlrally/gfx/skins/%s/%s.bmp", m_szSkinDir, m_szFile);
		rgb = LoadBMP (m_szPath, &wide, &tall);

		if (!rgb || (m_iX < 0) || (m_iY < 0) || (m_iX + m_iWidth > wide) || (m_iY + m_iHeight > tall))
		{
			gEngfuncs.Con_DPrintf ("Error loading skin from 'hlrally/gfx/skins/%s/%s.bmp'\nCheck that the file is 8, 24 or 32 bit", m_szSkinDir, m_szFile);

			if (rgb)
				free (rgb);
			m_pTGA = NULL;
			return;
		}
		// Create the TGA for the left over part
		m_pTGA = createTiledCBitmapMP3Region (m_pTGA, rgb, m_iX, m_iY, m_iWidth, m_iHeight, m_iPanelWidth, m_iPanelHeight, wide, tall, 24);

		// Clean up
		free (rgb);
	}

	CBitmapMP3 *m_pTGA;
	char m_szPath[128], m_szFile[80];
	char m_szSkinDir[30];
	int m_iX, m_iY, m_iWidth, m_iHeight;
	int	m_iPanelWidth, m_iPanelHeight;
};

class CSkinnedButton : public CommandButton
{
public:
	CSkinnedButton (char *name, int up_x, int up_y, int dwn_x, int dwn_y, int x, int y, int wide, int tall) : CommandButton ("", x, y, wide, tall)
	{
		strcpy (m_szFile, name);
		m_iUpX = up_x;
		m_iUpY = up_y;
		m_iDwnX = dwn_x;
		m_iDwnY = dwn_y;
		m_iWidth = wide;
		m_iHeight = tall;

		// Setup the rest of our variables
		m_iMouseCode = -1;
		m_bMouseDown = false;
		m_pButtonUp = NULL;
		m_pButtonDwn = NULL;
		m_bAllowInput = true;
		m_bArmed = false;
	}

	~CSkinnedButton (void)
	{
		unloadBMP ();

		// Remove the action signals
		for (int i = 0; i < _actionSignalDar.getCount (); i++)
			delete _actionSignalDar[i];
	}

	void internalMousePressed (MouseCode code)
	{
		if (m_bAllowInput)
		{
			m_iMouseCode = code;
			m_bMouseDown = true;

			CommandButton::internalMousePressed (code);
		}
	}

	void internalMouseReleased (MouseCode code)
	{
		if (m_bAllowInput)
		{
			if (code == m_iMouseCode)
				m_bMouseDown = false;

			CommandButton::internalMouseReleased (code);
		}
	}

	void internalCursorExited (void)
	{
		if (m_bAllowInput)
		{
			m_bMouseDown = false;
			m_iMouseCode = -1;

			CommandButton::internalCursorExited ();
		}
	}

	void paint (void)
	{
		if (m_pButtonUp == NULL || m_pButtonDwn == NULL)
			return;

		if (m_bAllowInput)
		{
			if (!m_bMouseDown)
				m_pButtonUp->doPaint (this);
			else
				m_pButtonDwn->doPaint (this);
		}
		else	// Obviously mouse can never be down if we don't allow input
		{
			if (!m_bArmed)
				m_pButtonUp->doPaint (this);
			else
				m_pButtonDwn->doPaint (this);
		}
	}

	void paintBackground (void)
	{
		// Do nothing so the background is clear
	}

	void allowInput (bool state)
	{
		m_bAllowInput = state;
	}
	
	void setUpState (bool state)
	{
		m_bArmed = state;
	}

	void reloadBMP (void)
	{
		//unloadBMP ();
		loadBMP ();
	}

private:
	void unloadBMP (void)
	{
		if (m_pButtonUp)
		{
			delete m_pButtonUp;
			m_pButtonUp = NULL;
		}
		if (m_pButtonDwn)
		{
			delete m_pButtonDwn;
			m_pButtonDwn = NULL;
		}
	}

	void loadBMP (void)
	{
		byte *rgb;

		// Setup
		int wide, tall;
		strcpy (m_szSkinDir, getSkinDir ());
		sprintf (m_szPath, "hlrally/gfx/skins/%s/%s.bmp", m_szSkinDir, m_szFile);
		rgb = LoadBMP (m_szPath, &wide, &tall);

		// Check the size of the image is correct
		if (!rgb ||
			(m_iUpX < 0) || (m_iUpY < 0) || (m_iUpX + m_iWidth > wide) || (m_iUpY + m_iHeight > tall)
			|| (m_iDwnX < 0) || (m_iDwnY < 0) || (m_iDwnX + m_iWidth > wide) || (m_iDwnY + m_iHeight > tall))
		{
			gEngfuncs.Con_DPrintf ("Error loading skin from 'hlrally/gfx/skins/%s/%s.bmp'\n", m_szSkinDir, m_szFile);
			m_pButtonUp = m_pButtonDwn = NULL;
			if (rgb)
				free (rgb);
			return;
		}
		m_pButtonUp = createCBitmapMP3Region (m_pButtonUp, rgb, m_iUpX, m_iUpY, m_iWidth, m_iHeight, wide, tall, 24);
		m_pButtonDwn = createCBitmapMP3Region (m_pButtonDwn, rgb, m_iDwnX, m_iDwnY, m_iWidth, m_iHeight, wide, tall, 24);

		// Clean up
		free (rgb);
	}

	CBitmapMP3	*m_pButtonUp, *m_pButtonDwn;
	char		m_szSkinDir[30], m_szPath[128], m_szFile[80];
	int			m_iWidth, m_iHeight;
	int			m_iUpX, m_iUpY, m_iDwnX, m_iDwnY;
	int			m_iMouseCode;
	bool		m_bMouseDown;
	bool		m_bAllowInput;
	bool		m_bArmed;

};

class CSkinnedToggleButton : public CommandButton
{
public:
	CSkinnedToggleButton (char *name, int on_up_x, int on_up_y,
								int on_dwn_x, int on_dwn_y,
								int off_up_x, int off_up_y,
								int off_dwn_x, int off_dwn_y,
								int x, int y, int wide, int tall) : CommandButton ("", x, y, wide, tall)
	{
		strcpy (m_szFile, name);
		m_iWidth = wide;
		m_iHeight = tall;
		m_iOnUpX = on_up_x;
		m_iOnUpY = on_up_y;
		m_iOnDwnX = on_dwn_x;
		m_iOnDwnY = on_dwn_y;
		m_iOffUpX = off_up_x;
		m_iOffUpY = off_up_y;
		m_iOffDwnX = off_dwn_x;
		m_iOffDwnY = off_dwn_y;

		// Setup the rest of our variables
		m_iMouseCode = -1;
		m_bMouseDown = false;
		m_bButtonOn = false;
		m_pButtonOnUp = NULL;
		m_pButtonOnDwn = NULL;
		m_pButtonOffUp = NULL;
		m_pButtonOffDwn = NULL;
	}

	~CSkinnedToggleButton (void)
	{
		unloadBMP ();

		// Remove the action signals
		for (int i = 0; i < _actionSignalDar.getCount (); i++)
			delete _actionSignalDar[i];
	}

	void internalMousePressed (MouseCode code)
	{
		m_iMouseCode = code;
		m_bMouseDown = true;

		CommandButton::internalMousePressed (code);
	}

	void internalMouseReleased (MouseCode code)
	{
		if (code == m_iMouseCode)
		{
			m_bMouseDown = false;
			m_bButtonOn = !m_bButtonOn;
		}

		CommandButton::internalMouseReleased (code);
	}

	void internalCursorExited (void)
	{
		m_bMouseDown = false;
		m_iMouseCode = -1;

		CommandButton::internalCursorExited ();
	}

	void paint (void)
	{
		if (m_pButtonOnUp == NULL || m_pButtonOnDwn == NULL
			|| m_pButtonOffUp == NULL || m_pButtonOffDwn == NULL)
			return;

		if (m_bButtonOn)
		{
			if (!m_bMouseDown)
				m_pButtonOnUp->doPaint (this);
			else
				m_pButtonOnDwn->doPaint (this);
		}
		else
		{
			if (!m_bMouseDown)
				m_pButtonOffUp->doPaint (this);
			else
				m_pButtonOffDwn->doPaint (this);
		}
	}

	void paintBackground (void)
	{
		// Do nothing so the background is clear
	}

	void setOn (bool on)
	{
		m_bButtonOn = on;
	}

	bool getOn (void)
	{
		return m_bButtonOn;
	}

	void reloadBMP (void)
	{
		//unloadBMP ();
		loadBMP ();
	}

private:
	void unloadBMP (void)
	{
		if (m_pButtonOnUp != NULL)
		{
			delete m_pButtonOnUp;
			m_pButtonOnUp = NULL;
		}
		if (m_pButtonOnDwn != NULL)
		{
			delete m_pButtonOnDwn;
			m_pButtonOnDwn = NULL;
		}
		if (m_pButtonOffUp != NULL)
		{
			delete m_pButtonOffUp;
			m_pButtonOffUp = NULL;
		}
		if (m_pButtonOffDwn != NULL)
		{
			delete m_pButtonOffDwn;
			m_pButtonOffDwn = NULL;
		}
	}

	void loadBMP (void)
	{
		byte *rgb;
		int wide, tall;

		// Setup
		strcpy (m_szSkinDir, getSkinDir ());
		sprintf (m_szPath, "hlrally/gfx/skins/%s/%s.bmp", m_szSkinDir, m_szFile);
		rgb = LoadBMP (m_szPath, &wide, &tall);

		// Check the size of the image is correct
		if (!rgb ||
			(m_iOnUpX < 0) || (m_iOnUpY < 0) || (m_iOnUpX + m_iWidth > wide) || (m_iOnUpY + m_iHeight > tall)
			|| (m_iOnDwnX < 0) || (m_iOnDwnY < 0) || (m_iOnDwnX + m_iWidth > wide) || (m_iOnDwnY + m_iHeight > tall)
			||(m_iOffUpX < 0) || (m_iOffUpY < 0) || (m_iOffUpX + m_iWidth > wide) || (m_iOffUpY + m_iHeight > tall)
			|| (m_iOffDwnX < 0) || (m_iOffDwnY < 0) || (m_iOffDwnX + m_iWidth > wide) || (m_iOffDwnY + m_iHeight > tall))
		{
			gEngfuncs.Con_DPrintf ("Error loading skin from 'hlrally/gfx/skins/%s/%s.bmp'\n", m_szSkinDir, m_szFile);
			m_pButtonOnUp = m_pButtonOnDwn = m_pButtonOffUp = m_pButtonOffDwn = NULL;
			if (rgb)
				free (rgb);
			return;
		}
		m_pButtonOnUp = createCBitmapMP3Region (m_pButtonOnUp, rgb, m_iOnUpX, m_iOnUpY, m_iWidth, m_iHeight, wide, tall, 24);
		m_pButtonOnDwn = createCBitmapMP3Region (m_pButtonOnDwn, rgb, m_iOnDwnX, m_iOnDwnY, m_iWidth, m_iHeight, wide, tall, 24);
		m_pButtonOffUp = createCBitmapMP3Region (m_pButtonOffUp, rgb, m_iOffUpX, m_iOffUpY, m_iWidth, m_iHeight, wide, tall, 24);
		m_pButtonOffDwn = createCBitmapMP3Region (m_pButtonOffDwn, rgb, m_iOffDwnX, m_iOffDwnY, m_iWidth, m_iHeight, wide, tall, 24);

		// Clean up
		free (rgb);
	}

	CBitmapMP3	*m_pButtonOnUp, *m_pButtonOnDwn;
	CBitmapMP3	*m_pButtonOffUp, *m_pButtonOffDwn;
	bool		m_bButtonOn;
	char		m_szSkinDir[30], m_szPath[128], m_szFile[80];
	int			m_iWidth, m_iHeight;
	int			m_iMouseCode;
	bool		m_bMouseDown;

	int			m_iOnUpX, m_iOnUpY, m_iOnDwnX, m_iOnDwnY;
	int			m_iOffUpX, m_iOffUpY, m_iOffDwnX, m_iOffDwnY;
};

class CSkinnedSlider : public Slider2
{
public:
	CSkinnedSlider (char *name, int x, int y,
					int bg_x, int bg_y, int wide, int tall,
					int pointer_x, int pointer_y, int pointer_wide, int pointer_tall, int pointer_gap,
					bool vertical = false)
		: Slider2 (x, y, wide, tall, false)
	{
		strcpy (m_szFile, name);
		m_iBgX = bg_x;
		m_iBgY = bg_y;
		m_iWidth = wide;
		m_iHeight = tall;
		m_iPointerX = pointer_x;
		m_iPointerY = pointer_y;
		m_iPointerWide = pointer_wide;
		m_iPointerTall = pointer_tall;
		m_iPointerGap = pointer_gap;

		m_pBackground = NULL;
		m_pPointer[0] = m_pPointer[1] = NULL;

		_vertical = vertical;
		_allowinput = true;
	}

	~CSkinnedSlider (void)
	{
		unloadBMP ();
	}

	void paintBackground( void )
	{
		// Background
		if (m_pBackground)
			m_pBackground->doPaint (this);

		// Pointer
		int iPointerNum = 1;
		if (isDragging ())
			iPointerNum = 0;

		if (m_pPointer[iPointerNum])
		{
			int nobx, noby;
			getNobPos (nobx, noby);

			int wide, tall;
			getSize (wide, tall);

			int pointwide, pointtall;
			m_pPointer[iPointerNum]->getSize (pointwide, pointtall);

			if (_vertical)
				m_pPointer[iPointerNum]->setPos (wide - pointwide, noby - pointtall);	// HACK
			else
				m_pPointer[iPointerNum]->setPos (nobx, tall - pointtall);

			m_pPointer[iPointerNum]->doPaint (this);
		}
	}

	virtual void privateMousePressed (MouseCode code, Panel* panel)
	{
		int iPointerNum = 1;
		if (isDragging ())
			iPointerNum = 0;

		if (m_pPointer[iPointerNum])
		{
			if (!_allowinput)
				return;

			int x,y;
			getApp()->getCursorPos(x,y);
			screenToLocal(x,y);

//			gEngfuncs.Con_Printf ("privateMousePressed - %i < %i < %i\n", _nobPos[0] - _margin, x, x<_nobPos[1] + _margin);

			if(_vertical)
			{
				// SaRcaZm - V7 - Start
				if((y>=_nobPos[0] - _margin)&&(y<_nobPos[1] + _margin))
				{
					_dragging=true;
					getApp()->setMouseCapture(this);
					_nobDragStartPos[0]=_nobPos[0];
					_nobDragStartPos[1]=_nobPos[1];
					_dragStartPos[0]=x;
					_dragStartPos[1]=y;
				}
			}
			else
			{
				if((x>=_nobPos[0] - _margin)&&(x<_nobPos[1] + _margin))
				// SaRcaZm - V7 - End
				{
					_dragging=true;
					getApp()->setMouseCapture(this);
					_nobDragStartPos[0]=_nobPos[0];
					_nobDragStartPos[1]=_nobPos[1];
					_dragStartPos[0]=x;
					_dragStartPos[1]=y;
				}
			}
		}
		else
		{
			// We don't have a pointer so Slider2 doesn't know what to do
			int x,y;
			getApp()->getCursorPos(x,y);
			screenToLocal(x,y);
			_nobPos[0] = x;
			_nobPos[1] = y;
			recomputeValueFromNobPos ();

			_dragging = true;
			getApp()->setMouseCapture(this);
			_nobDragStartPos[0] = _nobPos[0];
			_nobDragStartPos[1] = _nobPos[1];
			_dragStartPos[0] = x;
			_dragStartPos[1] = y;
		}
	}

	void reloadBMP (void)
	{
		//unloadBMP ();
		loadBMP ();
		recomputeNobPosFromValue ();
	}

protected:
	void unloadBMP (void)
	{
		if (m_pBackground != NULL)
		{
			delete m_pBackground;
			m_pBackground = NULL;
		}
		if (m_pPointer[0] != NULL)
		{
			delete m_pPointer[0];
			m_pPointer[0] = NULL;
		}
		if (m_pPointer[1] != NULL)
		{
			delete m_pPointer[1];
			m_pPointer[1] = NULL;
		}
	}

	void loadBMP (void)
	{
		byte *rgb;
		int wide, tall;

		strcpy (m_szSkinDir, getSkinDir ());
		sprintf (m_szPath, "hlrally/gfx/skins/%s/%s.bmp", m_szSkinDir, m_szFile);
		rgb = LoadBMP (m_szPath, &wide, &tall);

		// Get the backgrounds
		if (!rgb || (m_iBgX < 0) || (m_iBgY < 0) || (m_iHeight > tall) || (m_iWidth > wide))
		{
			m_pBackground = NULL;
		}
		else
		{
			// Load this image
			m_pBackground = createCBitmapMP3Region (m_pBackground, rgb, m_iBgX, m_iBgY, m_iWidth, m_iHeight, wide, tall, 24);
		}

		// Get the pointers
		if (!rgb ||
			(m_iPointerX < 0) || (m_iPointerY < 0) ||
			(m_iPointerWide > wide) || (m_iPointerY + m_iPointerTall > tall))
			m_pPointer[0] = m_pPointer[1] = NULL;
		else
		{
			int iXPos = m_iPointerX;
			for (int i = 0; i < 2; i++)
			{
				if (iXPos + m_iPointerWide > wide)
					m_pPointer[i] = NULL;
				else
					m_pPointer[i] = createCBitmapMP3Region (m_pPointer[i], rgb, iXPos, m_iPointerY, m_iPointerWide, m_iPointerTall, wide, tall, 24);

				iXPos += m_iPointerWide + m_iPointerGap;
			}
		}

		free (rgb);
	}

	virtual void recomputeNobPosFromValue (void)
	{
		int iPointerNum = 1;
		if (isDragging ())
			iPointerNum = 0;

		int wide,tall;
		getPaintSize (wide,tall);

		int nob_wide, nob_tall;
		if (m_pPointer[iPointerNum] == NULL)
		{
			nob_wide = 0;
			nob_tall = 0;
		}
		else
			m_pPointer[iPointerNum]->getSize (nob_wide, nob_tall);

		if (_vertical)
		{
			float ftall = (float)tall;
			float frange = (float)(_range[1] - _range[0]);
			float fvalue = (float)(_value - _range[0]);
			float fper = fvalue / frange;
			float fnobsize = (float) nob_tall;
			float freepixels = ftall - fnobsize;
			float firstpixel = freepixels * fper;

			_nobPos[0] = (int)(firstpixel);
			_nobPos[1] = (int)(firstpixel + fnobsize);

			if(_nobPos[1] > tall)
			{
				_nobPos[0] = tall - ((int)fnobsize);
				_nobPos[1] = tall;
			}
		}
		else
		{
			float fwide = (float)wide;
			float frange = (float)(_range[1] - _range[0]);
			float fvalue = (float)(_value - _range[0]);
			float fper = fvalue / frange;
			float fnobsize = (float) nob_wide;
			float freepixels = fwide - fnobsize;
			float firstpixel = freepixels * fper;

			_nobPos[0] = (int)(firstpixel);
			_nobPos[1] = (int)(firstpixel + fnobsize);

			if(_nobPos[1] > wide)
			{
				_nobPos[0] = wide - ((int)fnobsize);
				_nobPos[1] = wide;
			}
		}

//		gEngfuncs.Con_Printf ("_nobPos = %i, %i\n", _nobPos[0], _nobPos[1]);

		repaint();
	}

	virtual void recomputeValueFromNobPos (void)
	{
		int iPointerNum = 1;
		if (isDragging ())
			iPointerNum = 0;

		int wide,tall;
		getPaintSize (wide,tall);

		int nob_wide, nob_tall;
		if (m_pPointer[iPointerNum] == NULL)
		{
			nob_wide = 0;
			nob_tall = 0;
		}
		else
			m_pPointer[iPointerNum]->getSize (nob_wide, nob_tall);

		if (_vertical)
		{
			float fnobtall = (float)nob_tall;
			float ftall = (float)tall - fnobtall;
			float frange = (float)(_range[1] - _range[0]);
			float fnobpos = (float)_nobPos[0];
			float fvalue = (float)(fnobpos / ftall) * frange;

			// Take care of rounding issues.
			_value = (int)(fvalue + _range[0] + 0.5);
		}
		else
		{
			float fnobwide = (float)nob_wide;
			float fwide = (float)wide - fnobwide;
			float frange = (float)(_range[1] - _range[0]);
			float fnobpos = (float)_nobPos[0];
			float fvalue = (float)(fnobpos / fwide) * frange;

			// Take care of rounding issues.
			_value = (int)(fvalue + _range[0] + 0.5);
		}

		// Clamp final result
		_value = ( _value < _range[1] ) ? _value : _range[1];
	}

private:
	CBitmapMP3 *m_pPointer[2];
	CBitmapMP3 *m_pBackground;

	char	m_szSkinDir[30], m_szPath[128], m_szFile[80];
	int		m_iWidth, m_iHeight, m_iBgX, m_iBgY;
	int		m_iPointerX, m_iPointerY, m_iPointerWide, m_iPointerTall, m_iPointerGap;
};

class CSkinnedMultiBGSlider : public CSkinnedSlider
{
public:
	CSkinnedMultiBGSlider (char *name, int count,
							int x, int y,
							int bg_x, int bg_y, int wide, int tall, int gap,
							int pointer_x, int pointer_y, int pointer_wide, int pointer_tall, int pointer_gap)
		: CSkinnedSlider (name, x, y, -1, -1, wide, tall, pointer_x, pointer_y, pointer_wide, pointer_tall, pointer_gap)
	{
		strcpy (m_szFile, name);
		m_iCount = count;
		m_iBgX = bg_x;
		m_iBgY = bg_y;
		m_iBgGap = gap;
		m_iWidth = wide;
		m_iHeight = tall;
		m_iPointerX = pointer_x;
		m_iPointerY = pointer_y;
		m_iPointerWide = pointer_wide;
		m_iPointerTall = pointer_tall;
		m_iPointerGap = pointer_gap;

		for (int i = 0; i < 28; i++)
			m_pBackground[i] = NULL;

		m_iIdeal = -1;
	}

	~CSkinnedMultiBGSlider (void)
	{
		unloadBMP ();
	}

	void setIdeal (int ideal)
	{
		if (ideal < _range[0])
			ideal = _range[0];
		else if (ideal > _range[1])
			ideal = _range[1];
		m_iIdeal = ideal;
	}

	int getIdeal (void)
	{
		return m_iIdeal;
	}

	void setRange (int min, int max)
	{
		if (m_iIdeal < 0)
			m_iIdeal = max;
		Slider2::setRange (min, max);
	}

	void paintBackground( void )
	{
		// Background
		int offset = _value, range = _range[1] - _range[0];
		if (_value > m_iIdeal)
		{
			offset = _value - m_iIdeal;
			range = _range[1] - m_iIdeal;
		}
		else
		{
			offset = m_iIdeal - _value;
			range = m_iIdeal - _range[0];
		}

		int iBGNum = (int) ((float) offset / (float)range * (float)(m_iCount - 1));

		// Sanity check
		if (iBGNum < 0)
			iBGNum = 0;
		else if (iBGNum > m_iCount - 1)
			iBGNum = m_iCount - 1;

		// Draw it
		if (m_pBackground[iBGNum])
			m_pBackground[iBGNum]->doPaint (this);

		CSkinnedSlider::paintBackground ();
	}

	void reloadBMP (void)
	{
		//unloadBMP ();
		loadBMP ();
		CSkinnedSlider::reloadBMP ();
	}

private:
	void unloadBMP (void)
	{
		for (int i = 0; i < m_iCount; i++)
		{
			if (m_pBackground[i])
			{
				delete m_pBackground[i];
				m_pBackground[i] = NULL;
			}
		}

		CSkinnedSlider::unloadBMP ();
	}

	void loadBMP (void)
	{
		byte *rgb;
		int wide, tall;

		strcpy (m_szSkinDir, getSkinDir ());
		sprintf (m_szPath, "hlrally/gfx/skins/%s/%s.bmp", m_szSkinDir, m_szFile);
		rgb = LoadBMP (m_szPath, &wide, &tall);

		// Get the backgrounds
		if (!rgb || (m_iBgX < 0) || (m_iBgY < 0) || (m_iWidth > wide))
		{
			for (int i = 0; i < m_iCount; i++)
				m_pBackground[i] = NULL;
		}
		else
		{
			int iYPos = m_iBgY;
			for (int i = 0; i < m_iCount; i++)
			{
				// Load this image
				// Check the size of the image is correct
				if (iYPos + m_iHeight > tall)
					m_pBackground[i] = NULL;
				else
					m_pBackground[i] = createCBitmapMP3Region (m_pBackground[i], rgb, m_iBgX, iYPos, m_iWidth, m_iHeight, wide, tall, 24);

				iYPos += m_iHeight + m_iBgGap;
			}
		}

		// Clean up
		if (rgb)
			free (rgb);
	}

private:
	CBitmapMP3	*m_pBackground[28];
	int			m_iCount, m_iIdeal, m_iBgGap;
	char		m_szSkinDir[30], m_szPath[128], m_szFile[80];
	int			m_iWidth, m_iHeight, m_iBgX, m_iBgY;
	int			m_iPointerX, m_iPointerY, m_iPointerWide, m_iPointerTall, m_iPointerGap;
};

/*	CSkinnedTime now takes care of the time
	It uses 27 fewer images
class CSkinnedNumber : public Panel
{
public:
	CSkinnedNumber (char *szFile, int x, int y, int width, int height) : Panel (x, y, width, height)
	{
		// Setup our variables
		m_iNum = 0;
		strcpy (m_szFile, szFile);
		m_iX = x;
		m_iY = y;
		m_iWidth = width;
		m_iHeight = height;

		for (int i = 0; i < 10; i++)
			m_pNum[i] = NULL;
	}

	~CSkinnedNumber (void)
	{
		unloadBMP ();
	}

	void paint (void)
	{
		if (m_pNum[m_iNum] == NULL)
			return;

		m_pNum[m_iNum]->doPaint (this);
	}

	void paintBackground (void)
	{
		// Do nothing, so the background's left transparent.
	}

	void setNum (int num)
	{
		// Sanity check
		if (num < 0)
			num = 0;
		else if (num > 9)
			num = 9;

		m_iNum = num;
	}

	void reloadBMP (void)
	{
		//unloadBMP ();
		loadBMP ();
	}

private:
	void unloadBMP (void)
	{
		for (int i = 0; i < 10; i++)
		{
			if (m_pNum[i] != NULL)
			{
				delete m_pNum[i];
				m_pNum[i] = NULL;
			}
		}
	}

	void loadBMP (void)
	{
		byte *rgb;
		int wide, tall;

		// Load the images
		strcpy (m_szSkinDir, getSkinDir ());
		sprintf (m_szPath, "hlrally/gfx/skins/%s/%s.bmp", m_szSkinDir, m_szFile);
		rgb = LoadBMP (m_szPath, &wide, &tall);

		// Check the size of the image is correct
		if (!rgb || (m_iX < 0) || (m_iY < 0) || (m_iHeight > tall))
		{
			gEngfuncs.Con_DPrintf ("Error loading skin from 'hlrally/gfx/skins/%s/%s.bmp'\n", m_szSkinDir, m_szFile);

			if (rgb)
				free (rgb);
			for (int i = 0; i < 10; i++)
				m_pNum[i] = NULL;
			return;
		}

		int iXPos = m_iX;
		for (int i = 0; i < 10; i++)
		{
			if (iXPos + m_iWidth > wide)
				m_pNum[i] = NULL;
			else
				m_pNum[i] = createCBitmapMP3Region (m_pNum[i], rgb, iXPos, m_iY, m_iWidth, m_iHeight, wide, tall, 24);

			iXPos += m_iWidth;
		}

		// Clean up
		free (rgb);
	}

	CBitmapMP3 *m_pNum[10];	// One for each number
	char m_szPath[128], m_szFile[80];
	char m_szSkinDir[30];
	int m_iX, m_iY, m_iWidth, m_iHeight, m_iNum;
};
*/
class CSkinnedTime : public Panel
{
public:
	CSkinnedTime (char *szFile, int x, int y, int width, int height, int gap, int secs_gap) : Panel ()
	{
		// Setup our variables
		m_iTime[0] = m_iTime[1] = m_iTime[2] = m_iTime[3] = 0;
		strcpy (m_szFile, szFile);
		m_iX = x;
		m_iY = y;
		m_iWidth = width;
		m_iHeight = height;
		m_iGap = gap;
		m_iSecsGap = secs_gap;

		for (int i = 0; i < 10; i++)
			m_pNum[i] = NULL;

		setSize ((m_iWidth * 4) + (m_iGap * 2) + m_iSecsGap, m_iHeight);
	}

	~CSkinnedTime (void)
	{
		unloadBMP ();
	}

	void paint (void)
	{
		int iXPos = 0;
		for (int i = 0; i < 4; i++)
		{
			if (m_pNum[m_iTime[i]] == NULL)
				continue;

			m_pNum[m_iTime[i]]->setPos (iXPos, 0);
			m_pNum[m_iTime[i]]->doPaint (this);

			if (i == 1)
				iXPos += m_iWidth + m_iSecsGap;
			else
				iXPos += m_iWidth + m_iGap;
		}
	}

	void paintBackground (void)
	{
		// Do nothing, so the background's left transparent.
	}

	void reloadBMP (void)
	{
		//unloadBMP ();
		loadBMP ();
	}

	void setTime (int time_Minutes, int time_Seconds)
	{
		if (time_Minutes == -1)
		{
			m_iTime[0] = m_iTime[1] = m_iTime[2] = m_iTime[3] = 0;
		}
		else
		{
			m_iTime[0] = time_Minutes / 10;
			m_iTime[1] = time_Minutes % 10;
			m_iTime[2] = time_Seconds / 10;
			m_iTime[3] = time_Seconds % 10;
		}
	}

	void setTime (float time)
	{
		int t_Seconds, t_Minutes;
		
		if (time == -1)
		{
			t_Seconds = -1;
			t_Minutes = -1;
		}
		else
		{
			t_Seconds = ((int) time) % 60;
			t_Minutes = ((int)time) / 60;
		}
		setTime (t_Minutes, t_Seconds);
	}

private:
	void unloadBMP (void)
	{
		for (int i = 0; i < 10; i++)
		{
			if (m_pNum[i] != NULL)
			{
				delete m_pNum[i];
				m_pNum[i] = NULL;
			}
		}
	}

	void loadBMP (void)
	{
		byte *rgb;
		int wide, tall;

		// Load the images
		strcpy (m_szSkinDir, getSkinDir ());
		sprintf (m_szPath, "hlrally/gfx/skins/%s/%s.bmp", m_szSkinDir, m_szFile);
		rgb = LoadBMP (m_szPath, &wide, &tall);

		// Check the size of the image is correct
		if (!rgb || (m_iX < 0) || (m_iY < 0) || (m_iHeight > tall))
		{
			gEngfuncs.Con_DPrintf ("Error loading skin from 'hlrally/gfx/skins/%s/%s.bmp'\n", m_szSkinDir, m_szFile);

			if (rgb)
				free (rgb);
			for (int i = 0; i < 10; i++)
				m_pNum[i] = NULL;
			return;
		}

		int iXPos = m_iX;
		for (int i = 0; i < 10; i++)
		{
			if (iXPos + m_iWidth > wide)
				m_pNum[i] = NULL;
			else
				m_pNum[i] = createCBitmapMP3Region (m_pNum[i], rgb, iXPos, m_iY, m_iWidth, m_iHeight, wide, tall, 24);

			iXPos += m_iWidth;
		}

		// Clean up
		free (rgb);
	}

	CBitmapMP3 *m_pNum[10];	// One for each number
	char m_szPath[128], m_szFile[80];
	char m_szSkinDir[30];
	int m_iX, m_iY, m_iWidth, m_iHeight;
	int m_iTime[4], m_iGap, m_iSecsGap;
};

#define NUM_LETTERS_WIDE	31

class CSkinnedString : public Panel
{
public:
	CSkinnedString (char *szFile, int x, int y)
	{
		// Setup the index
		strcpy (m_szFontIndex[0], "abcdefghijklmnopqrstuvwxyz\"@   ");
		strcpy (m_szFontIndex[1], "0123456789  :()-'!_+\\/[]^&%.=$#");
		strcpy (m_szFontIndex[2], "   ?*");

		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < NUM_LETTERS_WIDE; j++)
			{
				m_pLetters[i][j] = NULL;
			}
		}

		strcpy (m_szFile, szFile);
		m_iX = x;
		m_iY = y;
		m_szText[0] = 0;
	}

	~CSkinnedString (void)
	{
		unloadBMP ();
	}

	void setText (char *szText)
	{
		if (szText == NULL)
			return;

		memset (m_szText, 0, sizeof (m_szText));
		strcpy (m_szText, szText);
	}

	void paint (void)
	{
		int wide, tall;
		getPaintSize (wide, tall);
		int iXPos = 0;
		int iStrLen = strlen (m_szText);

		for (int i = 0; (i < iStrLen) && (iXPos + FONT_WIDTH < wide); i++)
		{
			// Find the letter in our index
			int j, k;
			char c = m_szText[i];
			if ((c >= 'A') && (c <= 'Z'))
				c = 'a' + (c - 'A');

			// Skip the spaces
			if (c == ' ')
			{
				iXPos += FONT_WIDTH;
				continue;
			}

			// Search for the character
			int iIndex1 = -1, iIndex2 = -1;
			for (j = 0; (j < 3) && (iIndex1 == -1); j++)
			{
				for (k = 0; (k < NUM_LETTERS_WIDE) && (iIndex2 == -1); k++)
				{
					if (m_szFontIndex[j][k] == c)
					{
						iIndex1 = j;
						iIndex2 = k;
					}
				}
			}

			if ((iIndex1 != -1) && (iIndex2 != -1) && (m_pLetters[iIndex1][iIndex2] != NULL))	// Found the letter
			{
				// Draw the letter
				m_pLetters[iIndex1][iIndex2]->setPos (iXPos, 0);
				m_pLetters[iIndex1][iIndex2]->doPaint (this);
			}
			iXPos += FONT_WIDTH;
		}
	}

	void paintBackground (void)
	{
		// Do nothing, so the background's left transparent.
	}

	void reloadBMP (void)
	{
		//unloadBMP ();
		loadBMP ();
	}

private:
	void unloadBMP (void)
	{
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < NUM_LETTERS_WIDE; j++)
			{
				if (m_pLetters[i][j] != NULL)
				{
					delete m_pLetters[i][j];
					m_pLetters[i][j] = NULL;
				}
			}
		}
	}

	void loadBMP (void)
	{
		// Load the images
		byte *rgb;
		int wide, tall;
		strcpy (m_szSkinDir, getSkinDir ());
		sprintf (m_szPath, "hlrally/gfx/skins/%s/%s.bmp", m_szSkinDir, m_szFile);
		rgb = LoadBMP (m_szPath, &wide, &tall);

		if (!rgb || (m_iX < 0) || (m_iY < 0))
		{
			gEngfuncs.Con_DPrintf ("Error loading skin from 'hlrally/gfx/skins/%s/%s.bmp'\n", m_szSkinDir, m_szFile);

			for (int i = 0; i < 3; i++)
				for (int j = 0; j < NUM_LETTERS_WIDE; j++)
					m_pLetters[i][j] = NULL;

			return;
		}

		// Loop through and load each of the characters
		int iXPos = m_iX;
		int iYPos = m_iY;
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < NUM_LETTERS_WIDE; j++)
			{
				if ((iXPos + FONT_WIDTH > wide) || (iYPos + FONT_HEIGHT > tall))
					m_pLetters[i][j] = NULL;
				else
					m_pLetters[i][j] = createCBitmapMP3Region (m_pLetters[i][j], rgb, iXPos, iYPos, FONT_WIDTH, FONT_HEIGHT, wide, tall, 24);

				iXPos += FONT_WIDTH;
			}

			iYPos += FONT_HEIGHT;
			iXPos = 0;
		}

		free (rgb);
	}

	char		m_szText[256];
	char		m_szFontIndex[3][NUM_LETTERS_WIDE];
	CBitmapMP3	*m_pLetters[3][NUM_LETTERS_WIDE];
	char		m_szPath[128], m_szSkinDir[30], m_szFile[80];
	int			m_iX, m_iY, m_iWidth, m_iHeight;
};

//
// The playlist class
//
#define MAX_MP3_FILES	2000
#define PLS_VERSION		2

typedef struct rally_mp3info_s {
	char szFile[128];
	char szTitle[80];
	int iLength;
} rally_mp3info_t;

class CMp3Playlist
{
public:
	CMp3Playlist (void)
	{
		m_iNumFiles = m_iPlaying = 0;
		m_iBalance = 128;
		m_iVolume = 255;
		m_bRandom = m_bRepeat = false;

		for (int i = 0; i < MAX_MP3_FILES; i++)
		{
			m_pFiles[i] = NULL;
			m_iRandom[i] = 0;
		}
	}

	CMp3Playlist (char *szFile)
	{
		loadPlaylist (szFile);
		CMp3Playlist ();	// Initialise variables
	}

	~CMp3Playlist (void)
	{
		for (int i = 0; i < MAX_MP3_FILES; i++)
			if (m_pFiles[i])
				free (m_pFiles[i]);
	}

	void loadPlaylist (char *szFile)
	{
		// Setup variables
		strcpy (m_szPlaylist, "hlrally/");
		strcat (m_szPlaylist, szFile);
		m_iNumFiles = 0;

		// Load and parse the playlist
		char *szExt = (m_szPlaylist + strlen (m_szPlaylist) - 3);

		if (!strnicmp (szExt, "pls", 3))
			ParsePLS ();

		// Create the random play order
		for (int i = 0; i < m_iNumFiles; i++)
		{
			bool bFound = true;
			int file;
			while (bFound)
			{
				file = (int) gEngfuncs.pfnRandomFloat( 0, m_iNumFiles );
				bFound = false;
				for (int j = 0; j < i; j++)
				{
					if (m_iRandom[j] == file)
					{
						bFound = true;
						break;
					}
				}
			}
			m_iRandom[i] = file;
		}
	}

	void playFile (int iFileNum)
	{
		if (iFileNum < 1)
			iFileNum = 1;
		else if (iFileNum > m_iNumFiles)
			iFileNum = m_iNumFiles;

		if (m_bRandom)
		{
			// Search through the random list to find the right file
			for (int i = 0; i < m_iNumFiles; i++)
			{
				if (m_iRandom[i] == iFileNum - 1)
				{
					m_iPlaying = i;
					break;
				}
			}
		}
		else
			m_iPlaying = iFileNum - 1;

		play ();
	}

	void playPath (char *szPath)
	{
		RallySound->fmodPlayMp3 (szPath);
		m_iPlaying = -1;
	}

	void next (bool playnow)
	{
		m_iPlaying++;
		if (m_iPlaying >= m_iNumFiles)
		{
			if (m_bRepeat)
				m_iPlaying = 0;
			else
			{
				m_iPlaying = m_iNumFiles - 1;
				stop ();
				return;
			}
		}

		if (playnow)
			play ();
	}

	void prev (bool playnow)
	{
		m_iPlaying--;
		if (m_iPlaying < 0)
		{
			if (m_bRepeat)
				m_iPlaying = m_iNumFiles - 1;
			else
			{
				m_iPlaying = 0;
				stop ();
				return;
			}
		}

		if (playnow)
			play ();
	}

	void pause (void)
	{
		RallySound->fmodPauseMp3 ();
	}

	void play (void)
	{
		if (!m_iNumFiles || (m_iPlaying < 0) || (m_iPlaying > m_iNumFiles))
			return;

		if (m_bRandom)
			RallySound->fmodPlayMp3 (m_pFiles[m_iRandom[m_iPlaying]]->szFile);
		else
			RallySound->fmodPlayMp3 (m_pFiles[m_iPlaying]->szFile);
	}

	void stop (void)
	{
		RallySound->fmodStopMp3 ();
	}

	bool isPlaying (void)
	{
		if (m_iPlaying < 0)
			return false;
		else if (m_bRandom && m_pFiles[m_iRandom[m_iPlaying]] && strnicmp (m_pFiles[m_iRandom[m_iPlaying]]->szFile, "http://", 7))
			return RallySound->fmodIsPlayingMp3 ();
		else if (m_pFiles[m_iPlaying] && strnicmp (m_pFiles[m_iPlaying]->szFile, "http://", 7))
			return RallySound->fmodIsPlayingMp3 ();
		else if (m_pFiles[m_iPlaying])	// Internet radio
			return RallySound->fmodIsInternetRadioPlaying ();

		return false;
	}

	bool isPaused (void)
	{
		return RallySound->fmodIsPausedMp3 ();
	}

	bool isOpen (void)
	{
		return RallySound->fmodIsOpenMp3 ();
	}

	bool isStereo (void)
	{
		return RallySound->fmodIsStereoMp3 ();
	}

	char *getCurrentFileInfo (void)
	{
		if (!m_iNumFiles || (m_iPlaying < -1) || (m_iPlaying > m_iNumFiles))
			return NULL;

		// Part of playlist
		if (m_iPlaying > -1)
		{
			if (m_bRandom)
				return m_pFiles[m_iRandom[m_iPlaying]]->szTitle;
			else
				return m_pFiles[m_iPlaying]->szTitle;
		}
		else if (isOpen ())
		// Another file
		// If the file is playing grab the ID3 tag
		{
			char *title, *artist;
			if ((title = RallySound->fmodGetMp3Tag ("TITLE")) == NULL)		// ID3v1
				title = RallySound->fmodGetMp3Tag ("TIT2");					// ID3v2
			if ((artist = RallySound->fmodGetMp3Tag ("ARTIST")) == NULL)	// ID3v1
				artist = RallySound->fmodGetMp3Tag ("TPE1");				// ID3v2

			if (title && artist)	// We have the tags so create the file info
				sprintf (m_szTmp, "%s - %s", artist, title);

			return m_szTmp;
		}

		return NULL;
	}

	int getKbps (void)
	{
		return RallySound->fmodGetMp3Kbps ();
	}

	int getKhz (void)
	{
		return RallySound->fmodGetMp3Frequency ();
	}

	void setBalance (int bal)
	{
		if (bal != m_iBalance)
		{
			RallySound->fmodSetMp3Balance (bal);
			m_iBalance = bal;
		}
	}

	void setVolume (int vol)
	{
		if (vol != m_iVolume)
		{
			RallySound->fmodSetMp3Volume (vol);
			m_iVolume = vol;
		}
	}

	int getFileTime (void)
	{
		return RallySound->fmodGetMp3Position () / 1000;
	}

	int getFileLength (void)
	{
		return RallySound->fmodGetMp3Length () / 1000;
	}

	void setFileTime (int time)
	{
		RallySound->fmodSetMp3Position (time * 1000);
	}

	void setRandom (bool state)
	{
		m_bRandom = state;

		if (!m_bRandom)
			m_iPlaying = m_iRandom[m_iPlaying];
		else
		{
			// Search through the random list to find the right file
			for (int i = 0; i < m_iNumFiles; i++)
			{
				if (m_iRandom[i] == m_iPlaying)
				{
					m_iPlaying = i;
					break;
				}
			}
		}
	}

	bool getRandom (void)
	{
		return m_bRandom;
	}

	void setRepeat (bool state)
	{
		m_bRepeat = state;
	}

	bool getRepeat (void)
	{
		return m_bRepeat;
	}

	int getNumFiles (void)
	{
		return m_iNumFiles;
	}

	char *getFileNumInfo (int num)
	{
		if ((num > 0) && (num <= m_iNumFiles))
			return m_pFiles[num - 1]->szTitle;
		else
			return NULL;
	}

	int getFileNumLength (int num)
	{
		if ((num > 0) && (num <= m_iNumFiles))
			return m_pFiles[num - 1]->iLength;
		else
			return 0;
	}

	int getFileNum (void)
	{
		if (m_bRandom)
			return m_iRandom[m_iPlaying] + 1;
		else
			return m_iPlaying + 1;
	}

private:

	void ParsePLS (void)
	{
		int iPLSFiles = 0;

		// Clear out old playlist
		for (int i = 0; i < MAX_MP3_FILES; i++)
		{
			if (m_pFiles[i])
			{
				free (m_pFiles[i]);
				m_pFiles[i] = NULL;
			}
		}

		FileInputStream *fis = new FileInputStream (m_szPlaylist, true);
		DataInputStream *dis = new DataInputStream (fis);

		bool success;
		char buf[256];

		dis->readLine (buf, 256, success);
		while (success)
		{
			// Parse the line
			if (!strnicmp (buf, "File", 4))
			{
				// This line contains a file
				int iNum = atoi (buf + 4) - 1;

				// Allocate the memory if it isn't already
				if (m_pFiles[iNum] == NULL)
				{
					m_pFiles[iNum] = (rally_mp3info_t *) malloc (sizeof (rally_mp3info_t));
					memset (m_pFiles[iNum], 0, sizeof (rally_mp3info_t));
				}

				// Remove 'Filexxx=' from the beginning of the line
				for (int i = 0; (i < 128) && (buf[i] != '='); i++)
					;

				char szTmpFile[128];
				strncpy (szTmpFile, (buf + i + 1), 128);

				if (szTmpFile[1] != ':')
				{
					// Winamp 5.0 is now screwing me with relative paths
					// Steam is screwing me with changing return values
					//const char *tmp = gEngfuncs.pfnGetGameDirectory ();
					char drive = _getdrive () + 'A' - 1;
					char tmp[128];
					_getdcwd (_getdrive (), tmp, 128);

					if (szTmpFile[0] == '\\')	// Just need drive letter
						sprintf (m_pFiles[iNum]->szFile, "%c:%s", drive, szTmpFile);
					else if (!strnicmp ("http://", szTmpFile, 7))
						strcpy (m_pFiles[iNum]->szFile, szTmpFile);
					else
					{
						sprintf (m_pFiles[iNum]->szFile, "%s\\hlrally\\%s", tmp, szTmpFile);

						// replace forward slashes with backslashes
						for( int i=0; i < 128; i++ )
						{
							if( m_pFiles[iNum]->szFile[i] == '/' )
								m_pFiles[iNum]->szFile[i] = '\\';
						}
					}
				}
				else
					strcpy (m_pFiles[iNum]->szFile, szTmpFile);
			}
			else if (!strnicmp (buf, "Title", 5))
			{
				// This line contains a file
				int iNum = atoi (buf + 5) - 1;

				// Allocate the memory if it isn't already
				if (m_pFiles[iNum] == NULL)
				{
					m_pFiles[iNum] = (rally_mp3info_t *) malloc (sizeof (rally_mp3info_t));
					memset (m_pFiles[iNum], 0, sizeof (rally_mp3info_t));
				}

				// Remove 'Titlexxx=' from the beginning of the line
				for (int i = 0; (i < 128) && (buf[i] != '='); i++)
					;

				strncpy (m_pFiles[iNum]->szTitle, (buf + i + 1), 80);
			}
			else if (!strnicmp (buf, "Length", 6))
			{
				// This line contains a file
				int iNum = atoi (buf + 6) - 1;

				// Allocate the memory if it isn't already
				if (m_pFiles[iNum] == NULL)
				{
					m_pFiles[iNum] = (rally_mp3info_t *) malloc (sizeof (rally_mp3info_t));
					memset (m_pFiles[iNum], 0, sizeof (rally_mp3info_t));
				}

				// Remove 'Lengthxxx=' from the beginning of the line
				for (int i = 0; (i < 128) && (buf[i] != '='); i++)
					;

				m_pFiles[iNum]->iLength = atoi (buf + i + 1);
			}
			else if (!strnicmp (buf, "NumberOfEntries", 15))
				iPLSFiles = atoi (buf + 16);
			else if (!strnicmp (buf, "Version", 7))
			{
				if (atoi (buf + 8) != PLS_VERSION)	// Just print a warning if the wrong version
					gEngfuncs.Con_Printf ("WARNING: PLS Version wrong, should be %i", PLS_VERSION);
			}

			// Read lines until it fails
			dis->readLine (buf, 256, success);
		}

		dis->close ();
		fis->close ();
		delete dis;
		delete fis;

		// Count the number of files
		for (i = 0; i < MAX_MP3_FILES; i++)
		{
			if (!m_pFiles[i])
			{
				m_iNumFiles = i;
				break;
			}

			// Check that the title has been filled in
			if (*m_pFiles[i]->szTitle == 0)
			{
				// Otherwise just copy the filename and use that
				for (int j = strlen (m_pFiles[i]->szFile) - 1; j > 0; j--)
					if ((m_pFiles[i]->szFile[j] == '\\') || (m_pFiles[i]->szFile[j] == '/'))
						break;

				strncpy (m_pFiles[i]->szTitle, m_pFiles[i]->szFile + j + 1, strlen (m_pFiles[i]->szFile) - j - 5);
			}
		}

		// Check that everything went ok
		if (m_iNumFiles != iPLSFiles)
			gEngfuncs.Con_Printf ("WARNING: Error reading playlist: %i != %i\n", m_iNumFiles, iPLSFiles);
	}

private:
	int m_iNumFiles, m_iPlaying;
	int m_iRandom[MAX_MP3_FILES];
	char m_szPlaylist[80], m_szTmp[60];
	bool m_bRandom, m_bRepeat;
	rally_mp3info_t *m_pFiles[MAX_MP3_FILES];

	int m_iBalance, m_iVolume;
};

class CPlaylistItem : public Button
{
public:

	CPlaylistItem (char *text, int x, int y, int wide, int tall) : Button ("", x, y, wide, tall)
	{
		// left align
		setContentAlignment( vgui::Label::a_west );
		// Add the Highlight signal
		addInputSignal( new CHandler_ButtonHighlight (this) );

		// Other variables
		m_bHighlight = false;
		m_bShowLength = true;

		// Create the length image
		m_pLength = new TextImage ("");
	}

	~CPlaylistItem (void)
	{
		delete m_pLength;

		// Remove the action signals
		for (int i = 0; i < _actionSignalDar.getCount (); i++)
			delete _actionSignalDar[i];

		// Remove the input signals
//		for (i = 0; i < _inputSignalDar.getCount (); i++)
//			delete _inputSignalDar[i];
		// HACK, our input signal is the second in the list
		// Presumably the other one gets deleted elsewhere
		delete _inputSignalDar[1];
	}

	void paint (void)
	{
		// draw armed button text in white
		if (isHighlighted ())
		{
			setFgColor (m_iSelect[0], m_iSelect[1], m_iSelect[2], 0);
			m_pLength->setColor (m_cSelect);
		}
		else
		{
			setFgColor (m_iNormal[0], m_iNormal[1], m_iNormal[2], 0);
			m_pLength->setColor (m_cNormal);
		}

		Button::paint ();

		if (m_bShowLength)
		{
			m_pLength->setPos (m_iLenStart + 5, -2);
			m_pLength->doPaint (this);
		}
	}

	void paintBackground (void)
	{
		if (isHighlighted ())
		{
			drawSetColor (m_iSelectBG[0], m_iSelectBG[1], m_iSelectBG[2], 0);
			drawFilledRect(0,0,_size[0],_size[1]);
		}
	}

	void setHighlight (bool on)
	{
		m_bHighlight = on;
	}

	bool isHighlighted (void)
	{
		return m_bHighlight;
	}

	void setNormalColour (int r, int g, int b)
	{
		m_iNormal[0] = r;
		m_iNormal[1] = g;
		m_iNormal[2] = b;

		m_cNormal.setColor (m_iNormal[0], m_iNormal[1], m_iNormal[2], 0);
	}

	void setSelectColour (int r, int g, int b)
	{
		m_iSelect[0] = r;
		m_iSelect[1] = g;
		m_iSelect[2] = b;

		m_cSelect.setColor (m_iSelect[0], m_iSelect[1], m_iSelect[2], 0);
	}

	void setHighlightColour (int r, int g, int b)
	{
		m_iSelectBG[0] = r;
		m_iSelectBG[1] = g;
		m_iSelectBG[2] = b;
	}

	void setNormalBGColour (int r, int g, int b)
	{
		m_iNormalBG[0] = r;
		m_iNormalBG[1] = g;
		m_iNormalBG[2] = b;
	}

	void showLength (bool state)
	{
		m_bShowLength = state;
	}

	void setInfo (char *szTitle, char *szLength)
	{
		setText (szTitle);

		if (szLength)
		{
			m_pLength->setText (szLength);
			m_pLength->setFont (_textImage->getFont ());	// Copy the rest of the font
			calcLenStart ();

			// Shrink the text to fit
			int len = strlen (szTitle);
			int x, y;
			_textImage->getSize (x, y);

			if (x > m_iLenStart)
			{
				float charwidth = (float)x / (float)len;
				float numchars = (float)m_iLenStart / charwidth;
				szTitle[(int)numchars - 1] = 0;
			}
		}
		else
			m_bShowLength = false;

		setText (szTitle);
	}

	int calcLenStart (void)
	{
		int textwide, texttall;
		m_pLength->getSize (textwide, texttall);
		m_iLenStart = _size[0] - textwide - 5;

		return m_iLenStart;
	}

	void setLenStart (int lenstart)
	{
		m_iLenStart = lenstart;
	}

private:
	bool	m_bHighlight, m_bShowLength;
	int		m_iSelectBG[3], m_iNormal[3], m_iSelect[3], m_iNormalBG[3];
	int		m_iLenStart;

	TextImage	*m_pLength;
	Color m_cSelect, m_cNormal;
};

//
// The playlist editor class
//
int _htntoi(char *value, int num)
{
	struct CHexMap
	{
		char chr;
		int value;
	};
	const int HexMapL = 16;
	CHexMap HexMap[HexMapL] =
	{
		{'0', 0}, {'1', 1},
		{'2', 2}, {'3', 3},
		{'4', 4}, {'5', 5},
		{'6', 6}, {'7', 7},
		{'8', 8}, {'9', 9},
		{'A', 10}, {'B', 11},
		{'C', 12}, {'D', 13},
		{'E', 14}, {'F', 15}
	};

	char *s = value;
	int result = 0;
	int rep = 0;

	if (*s == '0' && *(s + 1) == 'X')
		s += 2;

	bool firsttime = true;
	while ((rep < num) && *s != '\0')
	{
		bool found = false;
		for (int i = 0; i < HexMapL; i++)
		{
			if (*s == HexMap[i].chr)
			{
				if (!firsttime)
					result <<= 4;

				result |= HexMap[i].value;
				found = true;
				break;
			}
		}

		if (!found)
			break;

		s++;
		rep++;
		firsttime = false;
	}

	return result;
}

class CSkinnedPlaylist : public Panel
{
public:
	CSkinnedPlaylist (int x, int y, int wide, int tall) : Panel (x, y, wide, tall)
	{
		// Create the Scroll panel
		m_pScrollPanel = new CTFScrollPanel( 25, 20, _size[0] - 50, _size[1] - 58 );
		m_pScrollPanel->setParent(this);
		m_pScrollPanel->setScrollBarAutoVisible(false, false);
		m_pScrollPanel->setScrollBarVisible(false, false);
		m_pScrollPanel->validate();

		for (int i = 0; i < MAX_MP3_FILES; i++)
			m_pSongs[i] = NULL;

		CreateSkin ();
	}

	~CSkinnedPlaylist (void)
	{
		if (m_pScrollPanel != NULL)
			delete m_pScrollPanel;
		for (int i = 0; i < MAX_MP3_FILES; i++)
			if (m_pSongs[i] != NULL)
				delete m_pSongs[i];

		if (m_pTopLeft != NULL)
			delete m_pTopLeft;
		if (m_pTopRight != NULL)
			delete m_pTopRight;
		if (m_pTitle != NULL)
			delete m_pTitle;
		if (m_pTitleLeft != NULL)
			delete m_pTitleLeft;
		if (m_pTitleRight != NULL)
			delete m_pTitleRight;
		if (m_pLeftSide != NULL)
			delete m_pLeftSide;
		if (m_pRightSide != NULL)
			delete m_pRightSide;
		if (m_pBottomLeft != NULL)
			delete m_pBottomLeft;
		if (m_pBottomRight != NULL)
			delete m_pBottomRight;
		if (m_pScrollBar != NULL)
			delete m_pScrollBar;
	}

	void Update (CMp3Playlist *playlist)
	{
		m_iNormal[0] = m_iNormal[1] = m_iNormal[2] =
		m_iSelect[0] = m_iSelect[1] = m_iSelect[2] =
		m_iNormalBG[0] = m_iNormalBG[1] = m_iNormalBG[2] =
		m_iSelectBG[0] = m_iSelectBG[1] = m_iSelectBG[2] = 0;

		// Quickly grab the pledit.txt file to get the proper colours
		bool success;
		char buf[256];
		sprintf (buf, "hlrally/gfx/skins/%s/pledit.txt", getSkinDir ());
		FileInputStream *fis = new FileInputStream (buf, true);
		DataInputStream *dis = new DataInputStream (fis);

		dis->readLine (buf, 256, success);
		while (success)
		{
			if (!strnicmp ("Current", buf, 7))
			{
				char *start = buf + 9;
				// Convert it to uppercase
				for (int i = 0; i < 6; i++)
				{
					char *tmp = (start + i);
					if ((*tmp > 'a') && (*tmp < 'z'))
						*tmp = _toupper (*tmp);
				}

				// Convert it to an int
				for (i = 0; i < 3; i++)
					m_iSelect[i] = _htntoi (start + (i * 2), 2);
			}
			else if (!strnicmp ("NormalBG", buf, 8))
			{
				char *start = buf + 10;
				// Convert it to uppercase
				for (int i = 0; i < 6; i++)
				{
					char *tmp = (start + i);
					if ((*tmp > 'a') && (*tmp < 'z'))
						*tmp = _toupper (*tmp);
				}

				// Convert it to an int
				for (i = 0; i < 3; i++)
					m_iNormalBG[i] = _htntoi (start + (i * 2), 2);
			}
			else if (!strnicmp ("Normal", buf, 6))	// This needs to be under "NormalBG" to avoid another match
			{
				char *start = buf + 8;
				// Convert it to uppercase
				for (int i = 0; i < 6; i++)
				{
					char *tmp = (start + i);
					if ((*tmp > 'a') && (*tmp < 'z'))
						*tmp = _toupper (*tmp);
				}

				// Convert it to an int
				for (i = 0; i < 3; i++)
					m_iNormal[i] = _htntoi (start + (i * 2), 2);
			}
			else if (!strnicmp ("SelectedBG", buf, 10))
			{
				char *start = buf + 12;
				// Convert it to uppercase
				for (int i = 0; i < 6; i++)
				{
					char *tmp = (start + i);
					if ((*tmp > 'a') && (*tmp < 'z'))
						*tmp = _toupper (*tmp);
				}

				// Convert it to an int
				for (i = 0; i < 3; i++)
					m_iSelectBG[i] = _htntoi (start + (i * 2), 2);
			}
			dis->readLine (buf, 256, success);
		}

		dis->close ();
		fis->close ();
		delete dis;
		delete fis;

		// Delete all the old items
		for (int i = 0; i < MAX_MP3_FILES; i++)
		{
			if (m_pSongs[i] != NULL)
				delete m_pSongs[i];
			m_pSongs[i] = NULL;
		}

		// Get the scheme used for the Titles
		CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
		SchemeHandle_t hButtonText = pSchemes->getSchemeHandle( "Scoreboard Small Text" );
		Font *pFont = pSchemes->getFont(hButtonText);
//		Font *pFont = new Font ("Times New Roman", 12, 6, 0, 1, false, false, false, false);

		// Save this for later
		m_pPlaylist = playlist;

		// Now add each button
		int iYPos = YRES(5);
		int lenstart = -1;

		int wide, tall;
		m_pScrollPanel->getSize (wide, tall);
		for (i = 0; i < m_pPlaylist->getNumFiles (); i++)
		{
			char tmp[256], tmplen[8];
			m_pSongs[i] = new CPlaylistItem ("", 0, iYPos, wide, YRES(8));	// Take up the entire width of the panel
			m_pSongs[i]->setParent (m_pScrollPanel->getClient ());

			m_pSongs[i]->setFont (pFont);
			m_pSongs[i]->setNormalColour (m_iNormal[0], m_iNormal[1], m_iNormal[2]);
			m_pSongs[i]->setSelectColour (m_iSelect[0], m_iSelect[1], m_iSelect[2]);
			m_pSongs[i]->setHighlightColour (m_iSelectBG[0], m_iSelectBG[1], m_iSelectBG[2]);
			m_pSongs[i]->setNormalBGColour (m_iNormalBG[0], m_iNormalBG[1], m_iNormalBG[2]);

			sprintf (tmp, "%i.  %s", i + 1, m_pPlaylist->getFileNumInfo (i + 1));

			int len = m_pPlaylist->getFileNumLength (i + 1);
			if (len != -1)
			{
				sprintf (tmplen, "%.2i:%.2i", len / 60, len % 60);
				m_pSongs[i]->setInfo (tmp, tmplen);
			}
			else
				m_pSongs[i]->setInfo (tmp, NULL);

			// Make sure the right column is aligned
			if (lenstart < 0)
				lenstart = m_pSongs[i]->calcLenStart ();
			else
				m_pSongs[i]->setLenStart (lenstart);

			sprintf (tmp, "play %i", i + 1);
			m_pSongs[i]->addActionSignal (new CMenuHandler_RallyMP3Command (tmp));

			iYPos += YRES(10);
		}

		// Validate to get the panel to update
		m_pScrollPanel->validate();

		// Update our scrollbar range
		m_pScrollBar->setRange (0, iYPos);
		m_pScrollBar->setValue (0);
		m_iScrollVal = 0;
	}

	void reloadBMP (void)
	{
		m_pTopLeft->reloadBMP ();
		m_pTopRight->reloadBMP ();
		m_pTitle->reloadBMP ();
		m_pTitleLeft->reloadBMP ();
		m_pTitleRight->reloadBMP ();
		m_pLeftSide->reloadBMP ();
		m_pRightSide->reloadBMP ();
		m_pBottomLeft->reloadBMP ();
		m_pBottomRight->reloadBMP ();
		m_pScrollBar->reloadBMP ();
	}

	void paintBackground (void)
	{
		drawSetColor( m_iNormalBG[0], m_iNormalBG[1], m_iNormalBG[2], 0 );
		drawFilledRect(0,0,_size[0],_size[1]);

		// HACK
		// Check the value of our scroll bar and see if we need to update the scroll panel
		int iNewVal = m_pScrollBar->getValue ();
		if (iNewVal != m_iScrollVal)
		{
			m_pScrollPanel->setScrollValue (0, iNewVal);
			m_iScrollVal = iNewVal;
		}
	}

	void unhighlightAllItems (void)
	{
		for (int i = 0; i < m_pPlaylist->getNumFiles (); i++)
			m_pSongs[i]->setHighlight (false);
	}

	void highlightItem (int num)
	{
		unhighlightAllItems ();
		m_pSongs[num-1]->setHighlight (true);
	}

private:
	void CreateSkin (void)
	{
		m_pTopLeft = new CSkinnedComponent ("pledit", PLAYLIST_TOPLEFT_IMG_X, PLAYLIST_TOPLEFT_IMG_Y, PLAYLIST_TOPLEFT_IMG_SIZE_X, PLAYLIST_TOPLEFT_IMG_SIZE_Y);
		m_pTopLeft->setParent (this);
		m_pTopLeft->setPos (PLAYLIST_TOPLEFT_POS_X, PLAYLIST_TOPLEFT_POS_Y);

		m_pTopRight = new CSkinnedComponent ("pledit", PLAYLIST_TOPRIGHT_IMG_X, PLAYLIST_TOPRIGHT_IMG_Y, PLAYLIST_TOPRIGHT_IMG_SIZE_X, PLAYLIST_TOPRIGHT_IMG_SIZE_Y);
		m_pTopRight->setParent (this);
		m_pTopRight->setPos (PLAYLIST_TOPRIGHT_POS_X, PLAYLIST_TOPRIGHT_POS_Y);

		m_pTitle = new CSkinnedComponent ("pledit", PLAYLIST_TITLE_IMG_X, PLAYLIST_TITLE_IMG_Y, PLAYLIST_TITLE_IMG_SIZE_X, PLAYLIST_TITLE_IMG_SIZE_Y);
		m_pTitle->setParent (this);
		m_pTitle->setPos (PLAYLIST_TITLE_POS_X, PLAYLIST_TITLE_POS_Y);

		m_pTitleLeft = new CSkinnedTiledComponent ("pledit", PLAYLIST_TITLE_REP_LEFT_IMG_X, PLAYLIST_TITLE_REP_LEFT_IMG_Y,
												PLAYLIST_TITLE_REP_LEFT_IMG_SIZE_X, PLAYLIST_TITLE_REP_LEFT_IMG_SIZE_Y,
												PLAYLIST_TITLE_REP_LEFT_SIZE_X, PLAYLIST_TITLE_REP_LEFT_SIZE_Y);
		m_pTitleLeft->setParent (this);
		m_pTitleLeft->setPos (PLAYLIST_TITLE_REP_LEFT_POS_X, PLAYLIST_TITLE_REP_LEFT_POS_Y);

		m_pTitleRight = new CSkinnedTiledComponent ("pledit", PLAYLIST_TITLE_REP_RIGHT_IMG_X, PLAYLIST_TITLE_REP_RIGHT_IMG_Y,
												PLAYLIST_TITLE_REP_RIGHT_IMG_SIZE_X, PLAYLIST_TITLE_REP_RIGHT_IMG_SIZE_Y,
												PLAYLIST_TITLE_REP_RIGHT_SIZE_X, PLAYLIST_TITLE_REP_RIGHT_SIZE_Y);
		m_pTitleRight->setParent (this);
		m_pTitleRight->setPos (PLAYLIST_TITLE_REP_RIGHT_POS_X, PLAYLIST_TITLE_REP_RIGHT_POS_Y);

		m_pLeftSide = new CSkinnedTiledComponent ("pledit", PLAYLIST_LEFT_IMG_X, PLAYLIST_LEFT_IMG_Y,
												PLAYLIST_LEFT_IMG_SIZE_X, PLAYLIST_LEFT_IMG_SIZE_Y,
												PLAYLIST_LEFT_SIZE_X, PLAYLIST_LEFT_SIZE_Y);
		m_pLeftSide->setParent (this);
		m_pLeftSide->setPos (PLAYLIST_LEFT_POS_X, PLAYLIST_LEFT_POS_Y);

		m_pRightSide = new CSkinnedTiledComponent ("pledit", PLAYLIST_RIGHT_IMG_X, PLAYLIST_RIGHT_IMG_Y,
												PLAYLIST_RIGHT_IMG_SIZE_X, PLAYLIST_RIGHT_IMG_SIZE_Y,
												PLAYLIST_RIGHT_SIZE_X, PLAYLIST_RIGHT_SIZE_Y);
		m_pRightSide->setParent (this);
		m_pRightSide->setPos (PLAYLIST_RIGHT_POS_X, PLAYLIST_RIGHT_POS_Y);

		m_pBottomLeft = new CSkinnedComponent ("pledit", PLAYLIST_BOTTOMLEFT_IMG_X, PLAYLIST_BOTTOMLEFT_IMG_Y, PLAYLIST_BOTTOMLEFT_IMG_SIZE_X, PLAYLIST_BOTTOMLEFT_IMG_SIZE_Y);
		m_pBottomLeft->setParent (this);
		m_pBottomLeft->setPos (PLAYLIST_BOTTOMLEFT_POS_X, PLAYLIST_BOTTOMLEFT_POS_Y);

		m_pBottomRight = new CSkinnedComponent ("pledit", PLAYLIST_BOTTOMRIGHT_IMG_X, PLAYLIST_BOTTOMRIGHT_IMG_Y, PLAYLIST_BOTTOMRIGHT_IMG_SIZE_X, PLAYLIST_BOTTOMRIGHT_IMG_SIZE_Y);
		m_pBottomRight->setParent (this);
		m_pBottomRight->setPos (PLAYLIST_BOTTOMRIGHT_POS_X, PLAYLIST_BOTTOMRIGHT_POS_Y);

		m_pScrollBar = new CSkinnedSlider ("pledit", PLAYLIST_SCROLLBAR_POS_X, PLAYLIST_SCROLLBAR_POS_Y,
						-1, -1, PLAYLIST_SCROLLBAR_SIZE_X, PLAYLIST_SCROLLBAR_SIZE_Y,
						PLAYLIST_SCROLLBAR_DWN_IMG_X, PLAYLIST_SCROLLBAR_DWN_IMG_Y,
						PLAYLIST_SCROLLBAR_IMG_SIZE_X, PLAYLIST_SCROLLBAR_IMG_SIZE_Y, -(PLAYLIST_SCROLLBAR_IMG_SIZE_X * 2 + 1), true);
		m_pScrollBar->setParent (this);
	}

	CMp3Playlist	*m_pPlaylist;
	CTFScrollPanel	*m_pScrollPanel;
	CPlaylistItem	*m_pSongs[MAX_MP3_FILES];

	CSkinnedComponent		*m_pTopLeft, *m_pTopRight, *m_pTitle;
	CSkinnedTiledComponent	*m_pTitleLeft, *m_pTitleRight;
	CSkinnedTiledComponent	*m_pLeftSide, *m_pRightSide;
	CSkinnedComponent		*m_pBottomLeft, *m_pBottomRight;
	CSkinnedSlider			*m_pScrollBar;

	int m_iNormalBG[3], m_iSelectBG[3], m_iNormal[3], m_iSelect[3];
	int m_iScrollVal;
};

// 
// The Actual MP3 Player Class
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CMP3MenuPanel::CMP3MenuPanel(int iTrans, int iRemoveMe, int x,int y, int wide, int tall) : CMenuPanel(iTrans, iRemoveMe, x, y, wide, tall)
{
	// Initialise the variables
	//rally_mp3skin = gEngfuncs.pfnRegisterVariable( "rally_mp3skin", "default", FCVAR_ARCHIVE );
	m_pMainWindow = NULL;
	m_pTitleBar = NULL;
	m_pShuffle = NULL;
	m_pRepeat = NULL;
	m_pStereo = NULL;
	m_pMono = NULL;
	m_pPosBar = NULL;
	m_pSongTitle = NULL;
	m_pPlaylist = NULL;
	m_pVolume = NULL;
	m_pBalance = NULL;
	m_pKbps = NULL;
	m_pKhz = NULL;
	m_pEqButton = NULL;
	m_pPlButton = NULL;
	m_pPlaylistPanel = NULL;

	for (int i = 0; i < 5; i++)
		m_pButtons[i] = NULL;

	*m_szSkin = 0;
	*m_szPlaylist = 0;
	m_bWasDraggingPosBar = false;
	m_bStopPressed = true;
	m_fNextSong = -1;
	m_bUpdateNeeded = false;

	CreateSkin ();

	// Console commands
	gEngfuncs.pfnAddCommand ("mp3player", ShowMP3Panel);
	gEngfuncs.pfnAddCommand ("mp3cmd", MP3Command);
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor.
//-----------------------------------------------------------------------------
CMP3MenuPanel::~CMP3MenuPanel (void)
{
	UnloadSkin ();
}

//-----------------------------------------------------------------------------
// Purpose: Handle key inputs.
//-----------------------------------------------------------------------------
bool CMP3MenuPanel::SlotInput( int iSlot )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when the window is about to be opened.
//-----------------------------------------------------------------------------
void CMP3MenuPanel::Open( void )
{
	Update ();

	// Reload the skin if it has changed
	if (stricmp (m_szSkin, getSkinDir ()))//rally_mp3skin->string))
	{
		LoadPlaylist ();
		ReloadSkin ();
	}

	CMenuPanel::Open();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the information should be updated.
//-----------------------------------------------------------------------------
void CMP3MenuPanel::Update( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Set the information when the user moves the mouse over a button.
//-----------------------------------------------------------------------------
void CMP3MenuPanel::SetActiveInfo( int iInput )
{
}

//-----------------------------------------------------------------------------
// Purpose: To paint the background duh!
//-----------------------------------------------------------------------------
void CMP3MenuPanel::paintBackground( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: To paint the window duh!
//-----------------------------------------------------------------------------
void CMP3MenuPanel::paint ( void )
{
	if (m_pBalance)
		m_pPlaylist->setBalance (m_pBalance->getValue ());
	if (m_pVolume)
		m_pPlaylist->setVolume (m_pVolume->getValue ());

	if (m_bUpdateNeeded && m_pPlaylist->isOpen ())
	{
		m_pSongTitle->setText (m_pPlaylist->getCurrentFileInfo ());

		char tmp[5];
		sprintf (tmp, "%i", m_pPlaylist->getKbps ());
		m_pKbps->setText (tmp);
		sprintf (tmp, "%i", m_pPlaylist->getKhz ());
		m_pKhz->setText (tmp);

		bool stereo = m_pPlaylist->isStereo ();
		m_pMono->setUpState (!stereo);
		m_pStereo->setUpState (stereo);

		m_pPlaylistPanel->highlightItem (m_pPlaylist->getFileNum ());

		m_bUpdateNeeded = false;
	}

	if (m_pPlaylist->isOpen () && m_pPosBar)	// Avoid divide by 0 error
	{
		int iFileLength = m_pPlaylist->getFileLength ();
		int iFileTime = m_pPlaylist->getFileTime ();
		bool dragging = m_pPosBar->isDragging ();
		m_pPosBar->allowInput (true);

		if (!dragging && m_bWasDraggingPosBar)	// They have changed the time
		{
			m_pPlaylist->setFileTime ((float) m_pPosBar->getValue () / 100.0f * iFileLength);
			iFileTime = m_pPlaylist->getFileTime ();
		}
		else if (!dragging)
		{			
			float fPosBar = (float)iFileTime / (float)iFileLength * 100.0f;
			m_pPosBar->setValue (fPosBar);
		}

		m_pTime->setTime (iFileTime);

		m_bWasDraggingPosBar = dragging;
	}
	else if (m_pPosBar)
		m_pPosBar->allowInput (false);

	Panel::paint ();
}

//-----------------------------------------------------------------------------
// Purpose: Processing needed for each frame
//-----------------------------------------------------------------------------
void CMP3MenuPanel::frame (double time)
{
	if (!m_bStopPressed && !m_pPlaylist->isPlaying () && (m_fNextSong < gEngfuncs.GetClientTime () - 5))	// Next song
	{
		m_pPlaylist->next (true);
		m_pSongTitle->setText (m_pPlaylist->getCurrentFileInfo ());
		m_fNextSong = gEngfuncs.GetClientTime ();
		m_bUpdateNeeded = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the window (called at level change).
//-----------------------------------------------------------------------------
void CMP3MenuPanel::Initialize( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Parse command to control the mp3 player.
//-----------------------------------------------------------------------------
void CMP3MenuPanel::Command( char *szCommand )
{
	static float m_fLastCommand = -1;
	static char szArtist[128];

	LoadPlaylist ();	// make sure we have a playlist

	if (!strnicmp (szCommand, "playlist", 8))
		m_pPlaylistPanel->setVisible (!m_pPlaylistPanel->isVisible ());
	else if (!strnicmp (szCommand, "play", 4))
	{
		if (strlen (szCommand) > 4)
		{
			int num = atoi (szCommand + 5);
			if (num > 0)	// Number in the playlist
			{
				m_pPlaylist->playFile (num);
				m_pSongTitle->setText (m_pPlaylist->getCurrentFileInfo ());
				m_bUpdateNeeded = true;
			}
			else			// Path filename
			{
				m_pPlaylist->playPath (szCommand + 5);
				m_bUpdateNeeded = false;
			}
		}
		else
		{
			if (m_pPlaylist->isPaused ())
			{
				m_pPlaylist->play ();
				return;
			}

			if (m_pPlaylist->isPlaying ())
				return;

			m_pPlaylist->play ();
			m_pSongTitle->setText (m_pPlaylist->getCurrentFileInfo ());
			m_bUpdateNeeded = true;
		}
		m_bStopPressed = false;
		m_fNextSong = gEngfuncs.GetClientTime ();
	}
	else if (!strnicmp (szCommand, "pause", 5))
	{
		if (m_pPlaylist->isPaused ())
			m_pPlaylist->play ();
		else
			m_pPlaylist->pause ();
	}
	else if (!strnicmp (szCommand, "stop", 4))
	{
		m_pPlaylist->stop ();
		m_pPosBar->setValue (0);
		m_pTime->setTime (0);
		m_bStopPressed = true;
		m_pPlaylistPanel->unhighlightAllItems ();
	}
	else if (!strnicmp (szCommand, "next", 4))
	{
		m_pPlaylist->next (m_pPlaylist->isPlaying ());
		m_pSongTitle->setText (m_pPlaylist->getCurrentFileInfo ());
		m_bUpdateNeeded = true;
		m_bStopPressed = true;
	}
	else if (!strnicmp (szCommand, "prev", 4))
	{
		m_pPlaylist->prev (m_pPlaylist->isPlaying ());
		m_pSongTitle->setText (m_pPlaylist->getCurrentFileInfo ());
		m_bUpdateNeeded = true;
		m_bStopPressed = true;
	}
	else if (!strnicmp (szCommand, "shuffle", 7))
		m_pPlaylist->setRandom (!m_pPlaylist->getRandom ());
	else if (!strnicmp (szCommand, "repeat", 6))
		m_pPlaylist->setRepeat (!m_pPlaylist->getRepeat ());
	else if (!strnicmp (szCommand, "artist", 6))
		strcpy (szArtist, szCommand + 7);
	else if (!strnicmp (szCommand, "title", 5))
	{
		// Update the title on the player
		char tmp[256];
		sprintf (tmp, "%s - %s", szArtist, szCommand + 6);
		m_pSongTitle->setText (tmp);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reload the skin
//-----------------------------------------------------------------------------
void CMP3MenuPanel::ReloadSkin( void )
{
	// Can't change skins after the first time as it doesn't work
	if (*m_szSkin)
		return;

	// Panels
	m_pPlaylistPanel->reloadBMP ();
	m_pPlaylistPanel->Update (m_pPlaylist);

	// Components
	m_pMainWindow->reloadBMP ();
	m_pTitleBar->reloadBMP ();
	m_pEqButton->reloadBMP ();

	// Time
	m_pTime->reloadBMP ();

	// Strings
	m_pSongTitle->reloadBMP ();
	m_pKbps->reloadBMP ();
	m_pKhz->reloadBMP ();

	// Buttons
	for (int i = 0; i < 5; i++)
		m_pButtons[i]->reloadBMP ();
	m_pStereo->reloadBMP ();
	m_pMono->reloadBMP ();
	m_pClose->reloadBMP ();
	m_pShuffle->reloadBMP ();
	m_pRepeat->reloadBMP ();
	m_pPlButton->reloadBMP ();

	// Scroll bars
	m_pPosBar->reloadBMP ();
	m_pVolume->reloadBMP ();
	m_pBalance->reloadBMP ();

	// Save this for later reference
	strcpy (m_szSkin, getSkinDir ());
}

//-----------------------------------------------------------------------------
// Purpose: Create the skin
//-----------------------------------------------------------------------------
void CMP3MenuPanel::CreateSkin( void )
{
	//
	// Main window
	//
	m_pMainWindow = new CSkinnedComponent ("main", MAINWIN_SIZE_X, MAINWIN_SIZE_Y);
	m_pMainWindow->setParent (this);
	m_pMainWindow->setPos (MAINWIN_POS_X, MAINWIN_POS_Y);

	//
	// Titlebar
	//
	m_pTitleBar = new CSkinnedComponent ("titlebar", TITLEBAR_IMG_X, TITLEBAR_IMG_Y, TITLEBAR_IMG_SIZE_X, TITLEBAR_IMG_SIZE_Y);
	m_pTitleBar->setParent (this);
	m_pTitleBar->setPos (TITLEBAR_POS_X, TITLEBAR_POS_Y);

	//
	// Button panel
	//
	char szCommands[5][10] = { "prev", "play", "pause", "stop", "next" };
	int iXPos = BUTTON_BACK_POS_X;
	int iImgXPos = BUTTON_BACK_IMG_UP_X;
	for (int i = 0; i < 4; i++)
	{
		m_pButtons[i] = new CSkinnedButton ("cbuttons",
											iImgXPos, BUTTON_BACK_IMG_UP_Y,
											iImgXPos, BUTTON_BACK_IMG_DWN_Y,
											iXPos, BUTTON_BACK_POS_Y,
											BUTTON_SIZE_X, BUTTON_SIZE_Y);
		m_pButtons[i]->setParent (this);
		m_pButtons[i]->addActionSignal (new CMenuHandler_RallyMP3Command (szCommands[i]));

		iXPos += BUTTON_SIZE_X;
		iImgXPos += BUTTON_SIZE_X;
	}
	// For some unknown reason this button is 1 pixel skinnier
	m_pButtons[4] = new CSkinnedButton ("cbuttons",
										iImgXPos, BUTTON_BACK_IMG_UP_Y,
										iImgXPos, BUTTON_BACK_IMG_DWN_Y,
										iXPos, BUTTON_BACK_POS_Y,
										BUTTON_SIZE_X - 1, BUTTON_SIZE_Y);
	m_pButtons[4]->setParent (this);
	m_pButtons[4]->addActionSignal (new CMenuHandler_RallyMP3Command (szCommands[4]));

	//
	// Shuffle button
	//
	m_pShuffle = new CSkinnedToggleButton ("shufrep",
									BUTTON_SHUF_IMG_ON_UP_X, BUTTON_SHUF_IMG_ON_UP_Y,
									BUTTON_SHUF_IMG_ON_DWN_X, BUTTON_SHUF_IMG_ON_DWN_Y,
									BUTTON_SHUF_IMG_OFF_UP_X, BUTTON_SHUF_IMG_OFF_UP_Y,
									BUTTON_SHUF_IMG_OFF_DWN_X, BUTTON_SHUF_IMG_OFF_DWN_Y,
									BUTTON_SHUF_POS_X, BUTTON_SHUF_POS_Y,
									BUTTON_SHUF_SIZE_X, BUTTON_SHUF_SIZE_Y);
	m_pShuffle->setParent (this);
	m_pShuffle->addActionSignal (new CMenuHandler_RallyMP3Command ("shuffle"));

	//
	// Repeat button
	//
	m_pRepeat = new CSkinnedToggleButton ("shufrep",
									BUTTON_REP_IMG_ON_UP_X, BUTTON_REP_IMG_ON_UP_Y,
									BUTTON_REP_IMG_ON_DWN_X, BUTTON_REP_IMG_ON_DWN_Y,
									BUTTON_REP_IMG_OFF_UP_X, BUTTON_REP_IMG_OFF_UP_Y,
									BUTTON_REP_IMG_OFF_DWN_X, BUTTON_REP_IMG_OFF_DWN_Y,
									BUTTON_REP_POS_X, BUTTON_REP_POS_Y,
									BUTTON_REP_SIZE_X, BUTTON_REP_SIZE_Y);
	m_pRepeat->setParent (this);
	m_pRepeat->addActionSignal (new CMenuHandler_RallyMP3Command ("repeat"));

	//
	// Equaliser button
	//
	iXPos = BUTTON_EQ_POS_X;
	m_pEqButton = new CSkinnedComponent ("shufrep", BUTTON_EQ_IMG_X, BUTTON_EQ_IMG_Y, BUTTON_EQ_SIZE_X, BUTTON_EQ_SIZE_Y);
	m_pEqButton->setParent (this);
	m_pEqButton->setPos (iXPos, BUTTON_EQ_POS_Y);

	iXPos += BUTTON_EQ_SIZE_X + BUTTON_EQ_GAP_X;

	//
	// Playlist button
	//
	m_pPlButton = new CSkinnedToggleButton ("shufrep",
									BUTTON_PL_IMG_ON_UP_X, BUTTON_PL_IMG_ON_UP_Y,
									BUTTON_PL_IMG_ON_DWN_X, BUTTON_PL_IMG_ON_DWN_Y,
									BUTTON_PL_IMG_OFF_UP_X, BUTTON_PL_IMG_OFF_UP_Y,
									BUTTON_PL_IMG_OFF_DWN_X, BUTTON_PL_IMG_OFF_DWN_Y,
									iXPos, BUTTON_EQ_POS_Y,
									BUTTON_EQ_SIZE_X, BUTTON_EQ_SIZE_Y);
	m_pPlButton->setParent (this);
	m_pPlButton->addActionSignal (new CMenuHandler_RallyMP3Command ("playlist"));

	//
	// Close button
	//
	m_pClose = new CSkinnedButton ("titlebar",
									BUTTON_CLOSE_IMG_UP_X, BUTTON_CLOSE_IMG_UP_Y,
									BUTTON_CLOSE_IMG_DWN_X, BUTTON_CLOSE_IMG_DWN_Y,
									BUTTON_CLOSE_POS_X, BUTTON_CLOSE_POS_Y,
									BUTTON_CLOSE_SIZE_X, BUTTON_CLOSE_SIZE_Y);
	m_pClose->setParent (this);
	m_pClose->addActionSignal (new CMenuHandler_TextWindow(HIDE_TEXTWINDOW));

	//
	// Stereo indicator
	//
	m_pStereo = new CSkinnedButton ("monoster",
									STEREO_IMG_OFF_X, STEREO_IMG_OFF_Y,
									STEREO_IMG_ON_X, STEREO_IMG_ON_Y,
									STEREO_POS_X, STEREO_POS_Y,
									STEREO_SIZE_X, STEREO_SIZE_Y);
	m_pStereo->setParent (this);
	m_pStereo->allowInput (false);
	m_pStereo->setUpState (false);

	//
	// Mono indicator
	//
	m_pMono = new CSkinnedButton ("monoster",
									MONO_IMG_OFF_X, MONO_IMG_OFF_Y,
									MONO_IMG_ON_X, MONO_IMG_ON_Y,
									MONO_POS_X, MONO_POS_Y,
									MONO_SIZE_X, MONO_SIZE_Y);
	m_pMono->setParent (this);
	m_pMono->allowInput (false);
	m_pMono->setUpState (false);

	//
	// Volume
	//
	m_pVolume = new CSkinnedMultiBGSlider ("volume", 28,
				SLIDER_VOL_POS_X, SLIDER_VOL_POS_Y,
				SLIDER_VOL_IMG_X, SLIDER_VOL_IMG_Y,
				SLIDER_VOL_SIZE_X, SLIDER_VOL_SIZE_Y, SLIDER_VOL_GAP_Y,
				SLIDER_VOL_POINT_IMG_X, SLIDER_VOL_POINT_IMG_Y,
				SLIDER_VOL_POINT_SIZE_X, SLIDER_VOL_POINT_SIZE_Y, SLIDER_VOL_POINT_GAP_X);
	m_pVolume->setParent (this);
	m_pVolume->setRange (0, 255);
	m_pVolume->setValue (255);
	m_pVolume->setIdeal (0);

	//
	// Balance
	//
	m_pBalance = new CSkinnedMultiBGSlider ("balance", 28,
					SLIDER_BAL_POS_X, SLIDER_BAL_POS_Y,
					SLIDER_BAL_IMG_X, SLIDER_BAL_IMG_Y,
					SLIDER_BAL_SIZE_X, SLIDER_BAL_SIZE_Y, SLIDER_BAL_GAP_Y,
					SLIDER_BAL_POINT_IMG_X, SLIDER_BAL_POINT_IMG_Y,
					SLIDER_BAL_POINT_SIZE_X, SLIDER_BAL_POINT_SIZE_Y, SLIDER_BAL_POINT_GAP_X);
	m_pBalance->setParent (this);
	m_pBalance->setRange (0, 255);
	m_pBalance->setValue (128);
	m_pBalance->setIdeal (128);

	//
	// Time display
	//
	// Numbers is unique as it could be 'numbers.bmp' or 'nums_ex.bmp'
	// Check to see which one we have here
	//
	char szPath[80];
	sprintf (szPath, "hlrally/gfx/skins/%s/numbers.bmp", getSkinDir());
	FILE *f = fopen (szPath, "rb");
	if (f)
	{
		m_pTime = new CSkinnedTime ("numbers", TIME_IMG_X, TIME_IMG_Y, TIME_SIZE_X, TIME_SIZE_Y, TIME_GAP_X, TIME_GAP_SECS_X);
		fclose (f);
	}
	else
		m_pTime = new CSkinnedTime ("nums_ex", TIME_IMG_X, TIME_IMG_Y, TIME_SIZE_X, TIME_SIZE_Y, TIME_GAP_X, TIME_GAP_SECS_X);

	m_pTime->setParent (this);
	m_pTime->setPos (TIME_POS_X, TIME_POS_Y);
	m_pTime->setTime (0);

	//
	// Position bar
	//
	m_pPosBar = new CSkinnedSlider ("posbar", POSBAR_POS_X, POSBAR_POS_Y,
					POSBAR_IMG_X, POSBAR_IMG_Y, POSBAR_SIZE_X, POSBAR_SIZE_Y,
					POSBAR_POINT_DWN_IMG_X, POSBAR_POINT_DWN_IMG_Y,
					POSBAR_POINT_SIZE_X, POSBAR_POINT_SIZE_Y, -(POSBAR_POINT_SIZE_X * 2 + 1));
	m_pPosBar->setParent (this);
	m_pPosBar->setRange (0, 100);
	m_pPosBar->setValue (0);

	//
	// Song title
	//
	m_pSongTitle = new CSkinnedString ("text", TITLE_IMG_X, TITLE_IMG_Y);
	m_pSongTitle->setParent (this);
	m_pSongTitle->setPos (TITLE_POS_X, TITLE_POS_Y);
	m_pSongTitle->setSize (TITLE_SIZE_X, TITLE_SIZE_Y);
	m_pSongTitle->setText ("Welcome to HLRally");

	//
	// Kilobits / second
	//
	m_pKbps = new CSkinnedString ("text", KBPS_IMG_X, KBPS_IMG_Y);
	m_pKbps->setParent (this);
	m_pKbps->setPos (KBPS_POS_X, KBPS_POS_Y);
	m_pKbps->setSize (KBPS_SIZE_X, KBPS_SIZE_Y);

	//
	// Kilohertz
	//
	m_pKhz = new CSkinnedString ("text", KHZ_IMG_X, KHZ_IMG_Y);
	m_pKhz->setParent (this);
	m_pKhz->setPos (KHZ_POS_X, KHZ_POS_Y);
	m_pKhz->setSize (KHZ_SIZE_X, KHZ_SIZE_Y);

	//
	// Playlist
	//
	m_pPlaylistPanel = new CSkinnedPlaylist (PLAYLIST_POS_X, PLAYLIST_POS_Y, PLAYLIST_SIZE_X, PLAYLIST_SIZE_Y);
	m_pPlaylistPanel->setParent (this);
	m_pPlaylistPanel->setVisible (false);
}

//-----------------------------------------------------------------------------
// Purpose: Unload the skin
//-----------------------------------------------------------------------------
void CMP3MenuPanel::UnloadSkin( void )
{
	if (m_pMainWindow)
		delete m_pMainWindow;
	if (m_pTitleBar)
		delete m_pTitleBar;
	if (m_pShuffle)
		delete m_pShuffle;
	if (m_pRepeat)
		delete m_pRepeat;
	if (m_pStereo)
		delete m_pStereo;
	if (m_pMono)
		delete m_pMono;
	if (m_pPosBar)
		delete m_pPosBar;
	if (m_pSongTitle)
		delete m_pSongTitle;
	if (m_pPlaylist)
		delete m_pPlaylist;
	if (m_pVolume)
		delete m_pVolume;
	if (m_pBalance)
		delete m_pBalance;
	if (m_pKbps)
		delete m_pKbps;
	if (m_pKhz)
		delete m_pKhz;
	if (m_pEqButton)
		delete m_pEqButton;
	if (m_pPlButton)
		delete m_pPlButton;
	if (m_pTime)
		delete m_pTime;
	if (m_pPlaylistPanel)
		delete m_pPlaylistPanel;
	if (m_pClose)
		delete m_pClose;

	for (int i = 0; i < 5; i++)
		if (m_pButtons[i])
			delete m_pButtons[i];
}

//-----------------------------------------------------------------------------
// Purpose: Load the playlist
//-----------------------------------------------------------------------------
void CMP3MenuPanel::LoadPlaylist( void )
{
	if (m_pPlaylist == NULL)
	{
		strcpy (m_szPlaylist, rally_playlist);

		m_pPlaylist = new CMp3Playlist ();
		m_pPlaylist->loadPlaylist (m_szPlaylist);
	}
}
