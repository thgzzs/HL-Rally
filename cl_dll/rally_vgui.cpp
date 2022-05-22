// rally_vgui.cpp
//
// Contains all classes related to the vgui
//
// by SaRcaZm

#include "vgui_int.h"
#include "VGUI_Font.h"
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"

#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"

#include "rally_vgui.h"

rally_teaminfo_t rallyinfo[MAX_TEAMS];

///////////////////////////////////////////////////////////////////////////////////////////////

CImageButton::CImageButton (char *name, int x, int y, int wide, int tall)
	: CommandButton ("", x, y, wide, tall, false)
{
	char sz[80];

	sprintf (sz, "%s_up", name);
	m_pTGAUp = LoadTGAForRes(sz);
	sprintf (sz, "%s_dwn", name);
	m_pTGADwn = LoadTGAForRes(sz);
}

CImageButton::~CImageButton (void)
{
	if (m_pTGAUp)
		delete m_pTGAUp;
	if (m_pTGADwn)
		delete m_pTGADwn;

	for (int i = 0; i < _actionSignalDar.getCount (); i++)
		delete _actionSignalDar[i];
}

void CImageButton::paint (void)
{
	if ( !isArmed() && m_pTGAUp )
	{
		m_pTGAUp->doPaint(this);
	}
	else if ( m_pTGADwn )
	{
		m_pTGADwn->doPaint(this);
	}
}

void CImageButton::paintBackground (void)
{
	// Do nothing so the background is clear
}

void CImageButton::addButtonClickSignal (CMenuPanel *panel, int iNum)
{
	m_pPanel = panel;
	m_iPanelNum = iNum;
}

// SaRcaZm - V5 - Start
void CImageButton::removeAllActionSignals (void)
{
	_actionSignalDar.removeAll ();
}

///////////////////////////////////////////////////////////////////////////////////////////////

CManuImageButton::CManuImageButton (char *name, int x, int y, int wide, int tall)
	: CImageButton (name, x, y, wide, tall)
{
	// Save the name for later reference
	strcpy (m_szName, name);
}

// SaRcaZm - V4 - Start
void CManuImageButton::fireActionSignal (void)
{
	// SaRcaZm - V5 - Start
	if (!stricmp (m_szName, "Spectator"))
	{
		CRallyVGUIFX::getSingleton()->SetMode (MODE_NORM);
		CommandButton::fireActionSignal ();
		return;
	}
	// SaRcaZm - V5 - End
	m_pPanel->SetActiveInfo (m_iPanelNum);
}
// SaRcaZm - V4 - End

///////////////////////////////////////////////////////////////////////////////////////////////

CRaceImageButton::CRaceImageButton (char *name, int x, int y, int wide, int tall)
	: CImageButton (name, x, y, wide, tall)
{
}

void CRaceImageButton::fireActionSignal (void)
{
	gViewPort->ExecuteCommands ();

	Button::fireActionSignal ();
}
// SaRcaZm - V5 - End

///////////////////////////////////////////////////////////////////////////////////////////////

// SaRcaZm - V7 - Start
// Moved this up from below
// Taken from loadtga.cpp
class MemoryInputStream : public vgui::InputStream
{
public:
				MemoryInputStream()
				{
					m_pData = NULL;
					m_DataLen = m_ReadPos = 0;
				}
		
	virtual void  seekStart(bool& success)					{m_ReadPos=0; success=true;}
	virtual void  seekRelative(int count,bool& success)		{m_ReadPos+=count; success=true;}
	virtual void  seekEnd(bool& success)					{m_ReadPos=m_DataLen; success=true;}
	virtual int   getAvailable(bool& success)				{success=false; return 0;} // This is what vgui does for files...
	
	virtual uchar readUChar(bool& success)					
	{
		if(m_ReadPos>=0 && m_ReadPos<m_DataLen)
		{
			success=true;
			uchar ret = m_pData[m_ReadPos];
			++m_ReadPos;
			return ret;
		}
		else
		{
			success=false;
			return 0;
		}
	}

	virtual void  readUChar(uchar* buf,int count,bool& success)
	{
		for(int i=0; i < count; i++)
			buf[i] = readUChar(success);
	}

	virtual void  close(bool& success) 
	{
		m_pData = NULL;
		m_DataLen = m_ReadPos = 0;
	}

	uchar		*m_pData;
	int			m_DataLen;
	int			m_ReadPos;
};
// SaRcaZm - V7 - End

///////////////////////////////////////////////////////////////////////////////////////////////

#include "..\game_shared\vgui_loadtga.h"

// SaRcaZm - V4 - Start
// Make sure we load something if the right screen res isn't there
#define NUM_RES	8
int iScreenRes[NUM_RES] = { 2048, 1920, 1600, 1280, 1152, 1024, 800, 640 };

char *Custom_GetTGANameForRes(const char *pszName, int res)
{
	char sz[256];
	static char gd[256];
	sprintf(sz, pszName, res);
	sprintf(gd, "gfx/vgui/%s.tga", sz);
	return gd;
}

BitmapTGA *Custom_LoadTGAForRes( const char* pImageName )
{
	if(!strcmp(pImageName, ""))
		return NULL;

	// SaRcaZm - V7 - Start
	BitmapTGA	*pTGA = NULL; // FragMented Beta 1
	int i, resindex, iPicRes;

	char sz[256];
	sprintf(sz, "%%d_%s", pImageName);

	for (resindex = 0; resindex < NUM_RES; resindex++)
		if (iScreenRes[resindex] == ScreenWidth)
			break;

	// Bug fix
	//pTGA = NULL; // FragMented Beta 1

	// Due to the new resizing code, check for the exact res first
	char *file = NULL;
	// Check to see if it exists
	file = (char *) gEngfuncs.COM_LoadFile (Custom_GetTGANameForRes(sz, iScreenRes[resindex]), 5, NULL);
	if (file)
	{
		// It does, so free the check
		gEngfuncs.COM_FreeFile (file);
		pTGA = vgui_LoadTGA(Custom_GetTGANameForRes(sz, iScreenRes[resindex]));
		return pTGA;
	}

	// No exact match, find the largest possible and scale it down
	for (i = 0; i < NUM_RES; i++)
	{
		// Check to see if it exists
		file = (char *) gEngfuncs.COM_LoadFile (Custom_GetTGANameForRes(sz, iScreenRes[i]), 5, NULL);
		if (!file)
			continue;
		// It does, so free the check
		gEngfuncs.COM_FreeFile (file);
		// And load the actual file
		//pTGA = vgui_LoadTGA(Custom_GetTGANameForRes(sz, iScreenRes[i]));
		iPicRes = iScreenRes[i];
		break;
	}

	if(!file)
		return NULL;

	// We have a result, so scale it to the right size
	if (i < NUM_RES)
	{
		float scale = (float)iScreenRes[resindex] / (float)iPicRes;		// Stupid integer division

		// Now, because of HL's screwed up vgui image stuff
		// I need to manually load the TGA from the file and scale it from
		// the data contained therein
		char szFile[80];
		int width, height, src_width, src_height;
		byte *src_rgba, *rgba;

		// Open the file
		sprintf (szFile, "hlrally/%s", Custom_GetTGANameForRes(sz, iPicRes));
		src_rgba = LoadTGA (szFile, &src_width, &src_height);

		if (src_rgba == NULL)
			return NULL;	// I give up

		// Find out the values for the scaled picture
		width = (int) (src_width * scale);
		height = (int) (src_height * scale);
		int rgba_length = width * height * 4;
		rgba = (byte *) malloc (rgba_length);

		ScaleRectAvg ((unsigned long *)rgba, (unsigned long *)src_rgba, src_width, src_height, width, height);

		// Create our TGA file in memory
		byte *buffer;
		int bufflength = width*height*4 + 18;

		buffer = (byte *) malloc (bufflength);
		memset (buffer, 0, 18);
		buffer[2] = 2;		// uncompressed type
		buffer[12] = width&255;
		buffer[13] = width>>8;
		buffer[14] = height&255;
		buffer[15] = height>>8;
//		buffer[16] = 24;	// pixel size
		buffer[16] = 32;	// 32-bit to include the alpha channel

		// Write in the pixels
//		memcpy (buffer + 18, rgba, rgba_length);
		// Convert the RGBA array into BGRA
		// N.B. the bitmap image is stored upside down in memory
		byte *pointer = buffer + 18;
		for (int i = height - 1; i >= 0; i--)
		{
			for (int j = 0; j < width; j++)
			{
				// i, j points to the next pixel
				byte *pixel = (rgba + (i * width * 4) + (j * 4));
				*pointer++ = *(pixel + 2);
				*pointer++ = *(pixel + 1);
				*pointer++ = *(pixel);
				*pointer++ = *(pixel + 3);
			}
		}

		// Create a BitmapTGA out of the scaled image
		MemoryInputStream mis;
		mis.m_pData = buffer;
		mis.m_DataLen = bufflength;
		mis.m_ReadPos = 0;
		pTGA = new vgui::BitmapTGA(&mis, true);

		// Free all of our excessive memory usage
		free (buffer);
		free (rgba);
	}
	// SaRcaZm - V7 - End

	return pTGA;
}
// SaRcaZm - V4 - End

///////////////////////////////////////////////////////////////////////////////////////////////

// SaRcaZm - V7 - Start
BitmapTGA	*pTGAFrames[MAX_ANIM_FRAMES];
int iNumFrames;
bool bResetFrames = true;

CClassImageButton::CClassImageButton (char *name, int x, int y, int wide, int tall) : CommandButton (name, x, y, wide, tall)
{
	if (bResetFrames)
	{
		for (iNumFrames = 0; iNumFrames < MAX_ANIM_FRAMES; iNumFrames++)
		{
			char sz[20];
			sprintf (sz, "class_bg%i", iNumFrames + 1);
			pTGAFrames[iNumFrames] = Custom_LoadTGAForRes (sz);
			if (pTGAFrames[iNumFrames] == NULL)
				break;
		}

		bResetFrames = false;
	}

	m_iCurFrame = 0;
	m_iFrameTime = 0;
}

CClassImageButton::~CClassImageButton (void)
{
	for (int i = 0; i < iNumFrames; i++)
	{
		if (pTGAFrames[i])
		{
			delete pTGAFrames[i];
			pTGAFrames[i] = NULL;
		}
	}

	for (i = 0; i < _actionSignalDar.getCount (); i++)
		delete _actionSignalDar[i];
}

void CClassImageButton::fireActionSignal (void)
{

	// Don't wanna always clear the models
	if (!strnicmp (m_sMainText, "Cancel", 6))
	{
		CRallyVGUIFX::getSingleton()->ClearVGUIModels ();		
		RallySound->fmodPlay ("vgui/cancel.wav", false, 0);
	}
	else
		RallySound->fmodPlay ("vgui/select.wav", false, 0);

	// Set the render mode
	CRallyVGUIFX::getSingleton()->SetMode (MODE_VGUI);
	Button::fireActionSignal ();
}

void CClassImageButton::paint()
{
	// draw armed button text in white
	if ( isArmed() )
	{
		setFgColor( Scheme::sc_secondary2 );
	}
	else
	{
		setFgColor( Scheme::sc_primary1 );
	}

	Button::paint();
}

void CClassImageButton::paintBackground()
{
	if ( isArmed() )
	{
		pTGAFrames[m_iCurFrame]->doPaint (this);
		if (gEngfuncs.GetClientTime () > m_iFrameTime + 0.1)
		{
			m_iCurFrame++;
			m_iFrameTime = gEngfuncs.GetClientTime ();

			if (m_iCurFrame >= iNumFrames)
				m_iCurFrame = 0;
		}
	}
}
// SaRcaZm - V7 - End

// SaRcaZm - V5 - Start
void CClassImageButton::removeAllActionSignals (void)
{
	_actionSignalDar.removeAll ();
}
// SaRcaZm - V5 - End

///////////////////////////////////////////////////////////////////////////////////////////////

// SaRcaZm - V6 - Start
CPartsListButton::CPartsListButton (char *szPartName, int iPartPrice, int x, int y, int wide, int tall)
	: CommandButton ("", x, y, wide, tall, false)
{
	char szTemp[80];
	sprintf (szTemp, "%s - $%i", szPartName, iPartPrice);

	setText (szTemp);
}

void CPartsListButton::paintBackground (void)
{
	if ( isArmed() )
	{
		// Orange highlight background
		drawSetColor( Scheme::sc_primary2 );
		drawFilledRect(0,0,_size[0],_size[1]);
	}
}
// SaRcaZm - V6 - End

///////////////////////////////////////////////////////////////////////////////////////////////

CProgressBar::CProgressBar (int x, int y, int wide, int tall, int divisions, int progress)
	: Panel (x, y, wide, tall)
{
	m_iDivisions = divisions;
	m_iProgress = progress;
	m_iExtraProgress = 0;
	m_bVertical = false;
}

CProgressBar::CProgressBar (int x, int y, int wide, int tall, int divisions, int progress, bool vertical)
	: Panel (x, y, wide, tall)
{
	m_iDivisions = divisions;
	m_iProgress = progress;
	m_iExtraProgress = 0;
	m_bVertical = vertical;
}

void CProgressBar::setProgress (int progress)
{
	m_iProgress = progress;
}

int CProgressBar::getProgress (void)
{
	return m_iProgress;
}

void CProgressBar::setExtraProgress (int extraprogress)
{
	m_iExtraProgress = extraprogress;
}

void CProgressBar::paintBackground (void)
{
	if (!m_iDivisions)
		return;

	if (!m_bVertical)
	{
		int x = (int)(((float)_size[0] / (float)m_iDivisions) * m_iProgress);
		drawSetColor( Scheme::sc_primary2 );
		drawFilledRect(0,0,x,_size[1]);

		// Border
		drawSetColor( Scheme::sc_secondary1 );
		drawOutlinedRect(0,0,_size[0],_size[1]);
	}
	else
	{
		int y = _size[1] - (int)(((float)_size[1] / (float)m_iDivisions) * m_iProgress);
		int extray = _size[1] - (int)(((float)_size[1] / (float)m_iDivisions) * m_iExtraProgress);

		// Main highlight
		drawSetColor( 0, 255, 0, 0 );
		drawFilledRect(0,y,_size[0],_size[1]);

		// Extra highlight
		drawSetColor( 255, 0, 0, 0 );
		drawFilledRect(0,extray,_size[0],y);

		// Border
		drawSetColor( Scheme::sc_secondary1 );
		drawOutlinedRect(0,0,_size[0],_size[1]);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

C3DModelPanel::C3DModelPanel (int iTrans, int x,int y,int wide,int tall)
	: CTransparentPanel (iTrans, x, y, wide, tall)
{
	m_iMouseDown = -1;
	setMouseDown (false);
}

void C3DModelPanel::setMouseDown (bool bDown)
{
	// SaRcaZm - V5 - Start
	if (vehicles == NULL)
		return;
	// SaRcaZm - V5 - End

	modellife_t *tmp;
	tmp = vehicles;
	while (tmp != NULL)
	{
		tmp->mousedown = bDown;
		tmp = tmp->next;
	}
}

void C3DModelPanel::internalMousePressed(MouseCode code)
{
	// SaRcaZm - V5 - Start
	if (vehicles == NULL)
		return;
	// SaRcaZm - V5 - End

	m_iMouseDown = code;
	setMouseDown (true);
}

void C3DModelPanel::internalMouseReleased (MouseCode code)
{
	// SaRcaZm - V5 - Start
	if (vehicles == NULL)
		return;
	// SaRcaZm - V5 - End

	if (m_iMouseDown == code)
	{
		m_iMouseDown = -1;

		// Stop any rotation
		vehicles->angle[0] =
		vehicles->angle[1] =
		vehicles->angle[2] = 0.0f;

		setMouseDown (false);
	}
}

void C3DModelPanel::internalCursorMoved (int x, int y)
{
	// SaRcaZm - V5 - Start
	if (vehicles == NULL)
		return;
	// SaRcaZm - V5 - End

	// We should only be able to rotate the x axis
	if (m_iMouseDown == MOUSE_LEFT)
	{
		// Rotate the 3DModel
		vehicles->angle[1] += y - m_iPrevY;
		vehicles->angle[1] += x - m_iPrevX;
	}
	else if (m_iMouseDown == MOUSE_RIGHT)
	{
		// Rotate the 3DModel
		vehicles->angle[1] -= x - m_iPrevX;
		vehicles->angle[1] -= y - m_iPrevY;
	}
	// SaRcaZm - V4 - End

	// Save the x, y values for later
	m_iPrevX = x;
	m_iPrevY = y;
}

void C3DModelPanel::internalCursorExited (void)
{
	// SaRcaZm - V5 - Start
	if (vehicles == NULL)
		return;
	// SaRcaZm - V5 - End

	m_iMouseDown = -1;
	setMouseDown (false);

}

// SaRcaZm - V5 - Start
///////////////////////////////////////////////////////////////////////////////////////////////

CCustResImageLabel::CCustResImageLabel (const char* pImageName,int x,int y) : CImageLabel ("", x, y)
{
	m_pTGA = Custom_LoadTGAForRes(pImageName);
	setImage( m_pTGA );
}

CCustResImageLabel::CCustResImageLabel (const char* pImageName,int x,int y,int wide,int tall) : CImageLabel ("", x, y, wide, tall)
{
	m_pTGA = Custom_LoadTGAForRes(pImageName);
	setImage( m_pTGA );
}

CCustResImageLabel::~CCustResImageLabel (void)
{
	if (m_pTGA)
		delete m_pTGA;
}
// SaRcaZm - V7 - Start
///////////////////////////////////////////////////////////////////////////////////////////////

// SaRcaZm - V8 - Start
CRallySlider::CRallySlider(char *name, int x,int y,int wide,int tall)
	: Slider2 (x,y,wide,tall,false)
{
	char sz[80];
	m_pTGABack = Custom_LoadTGAForRes(name);
	sprintf (sz, "%s_pointer", name);
	m_pTGAPoint = Custom_LoadTGAForRes(sz);

	int newwide, newtall;
	m_pTGABack->getSize (newwide, newtall);
	setSize (newwide, newtall);
}

CRallySlider::~CRallySlider (void)
{
	if (m_pTGABack)
		delete m_pTGABack;
	if (m_pTGAPoint)
		delete m_pTGAPoint;
}

void CRallySlider::paintBackground( void )
{
	int wide,tall,nobx,noby;
	getPaintSize(wide,tall);
	getNobPos(nobx,noby);
	int pointwide, pointtall;
	m_pTGAPoint->getSize (pointwide, pointtall);

	// Background
	m_pTGABack->doPaint (this);

	// Pointer
	int posx = nobx - (pointwide / 2);
	if (posx < 0)
		posx = 0;
	else if (posx + pointwide > wide)
		posx = wide - pointwide;

	m_pTGAPoint->setPos (posx, tall - pointtall);
	m_pTGAPoint->doPaint (this);
}
// SaRcaZm - V8 - End

///////////////////////////////////////////////////////////////////////////////////////////////

CImageCheckButton::CImageCheckButton (char *name, bool check, int x, int y)
	: ToggleButton ("", x, y)
{
	char sz[30];
	sprintf (sz, "%s_check", name);
	m_pTGACheck = Custom_LoadTGAForRes (sz);
	sprintf (sz, "%s_uncheck", name);
	m_pTGAUncheck = Custom_LoadTGAForRes (sz);

	m_bChecked = check;

	int wide, tall;
	m_pTGAUncheck->getSize (wide, tall);
	setSize (wide, tall);
}

CImageCheckButton::~CImageCheckButton (void)
{
	if (m_pTGACheck)
		delete m_pTGACheck;
	if (m_pTGAUncheck)
		delete m_pTGAUncheck;
}

void CImageCheckButton::paint (void)
{
	if (m_bChecked)
		m_pTGACheck->doPaint (this);
	else
		m_pTGAUncheck->doPaint (this);
}

void CImageCheckButton::internalMousePressed(MouseCode code)
{
	m_bChecked = !m_bChecked;
}

bool CImageCheckButton::isChecked (void)
{
	return m_bChecked;
}

void CImageCheckButton::setChecked (bool checked)
{
	m_bChecked = checked;
}
// SaRcaZm - V7 - End

///////////////////////////////////////////////////////////////////////////////////////////////
// I would have added this to src\utils\vgui\ except I saw this in VGUI.h and got scared....
//No platform specific code in vgui\lib-src\vgui dir. Code in vgui\lib-src\vgui should 
//only include from vgui\include or standard C includes. ie, if I see windows.h included
//anywhere but vgui\lib-src\win32 I will hunt you down and kill you. Don't give me any crap
//that mfc is platform inspecific.

/*
// This one crashes when calling drawSetTextureRGBA
class CAnimFrame : public Bitmap
{
public:
	CAnimFrame (int width, int height, int src_width, int src_height) : Bitmap ()
	{
		setSize (width, height);
		src_rgba = (unsigned char *)malloc (src_width * src_height * 4);
		rgba = (unsigned char *)malloc (width * height * 4);
		m_iIndex = 2700;
	}

	~CAnimFrame (void)
	{
		free (src_rgba);
		free (rgba);
		Bitmap::~Bitmap ();
	}

	void setSize(int width,int height)
	{
		m_iWidth = width;
		m_iHeight = height;
		Bitmap::setSize (width, height);
	}

	void setImage (LPBITMAPINFO bi)
	{
		unsigned char *dib = (unsigned char *)bi->bmiColors;
		unsigned char *tmp = src_rgba;

		// Read the bitmap info into the temporary file
		for (int i = bi->bmiHeader.biHeight - 1; i >= 0; i--)
		{
			for (int j = 0; j < bi->bmiHeader.biWidth; j++)
			{
				byte *pixel = (dib + (i * bi->bmiHeader.biWidth * 3) + (j * 3));
				// This covers RGB (note it is in BGR in dib)
				*tmp++ = *(pixel + 2);
				*tmp++ = *(pixel + 1);
				*tmp++ = *(pixel);
				*tmp++ = 255;
			}
		}

		// Scale the image down to the correct size
		ScaleRectAvg ((unsigned long *)rgba, (unsigned long *)src_rgba,
			bi->bmiHeader.biWidth, bi->bmiHeader.biHeight,
			m_iWidth, m_iHeight);

		drawSetTextureRGBA (m_iIndex, (char *)rgba, m_iWidth, m_iHeight);
	}

	virtual void paint (Panel *panel)
	{
		drawSetTexture (m_iIndex);
		drawTexturedRect (0, 0, m_iWidth, m_iHeight);
	}

private:
	int m_iWidth, m_iHeight, m_iIndex;
	unsigned char *src_rgba, *rgba;
};
*/

///////////////////////////////////////////////////////////////////////////////////////////////

/*
// This one draws the frame incorrectly and it never changes
class CAnimFrame : public Bitmap {
public:
	CAnimFrame (int width, int height, int src_width, int src_height) : Bitmap ()
	{
		setSize (width, height);
	}

	~CAnimFrame (void)
	{
		Bitmap::~Bitmap ();
	}

	void setSize(int width,int height)
	{
		m_iWidth = width;
		m_iHeight = height;
		Bitmap::setSize (width, height);
	}

	void setImage (LPBITMAPINFO bi)
	{
		unsigned char *dib = (unsigned char *)bi->bmiColors;

		setSize (lpbi->biWidth, lpbi->biHeight);
		for (int i = 0; i < lpbi->biHeight; i++)
		{
			for (int j = 0; j < lpbi->biWidth; j++)
			{
				//int r = bi->bmiColors[(i * lpbi->biWidth) + j].rgbRed;
				//int g = bi->bmiColors[(i * lpbi->biWidth) + j].rgbGreen;
				//int b = bi->bmiColors[(i * lpbi->biWidth) + j].rgbBlue;
				int r = *(dib + 2);
				int g = *(dib + 1);
				int b = *dib;
				dib += 3;
			}
		}
	}
	void setImage (LPBITMAPINFO bi)
	{
		unsigned char *dib = (unsigned char *)bi->bmiColors;

		// Read the bitmap info into the temporary file
		for (int i = bi->bmiHeader.biHeight - 1; i >= 0; i--)
		{
			for (int j = 0; j < bi->bmiHeader.biWidth; j++)
			{
				byte *pixel = (_rgba + (i * bi->bmiHeader.biWidth * 3) + (j * 3));
				// This covers RGB (note it is in BGR in dib)
				unsigned char r = *(pixel + 2);
				unsigned char g = *(pixel + 1);
				unsigned char b = *(pixel);
				unsigned char a = 255;
				setRGBA (j, i, r, g, b, a);
			}
		}
	}

private:
	int m_iWidth, m_iHeight;
};
*/
///////////////////////////////////////////////////////////////////////////////////////////////

// This one draws the frame correctly, but it never changes
class CAnimFrame : public Bitmap {
public:
	CAnimFrame (int width, int height, int src_width, int src_height) : Bitmap ()
	{
		src_rgba = (unsigned char *) malloc (src_width * src_height * 4);
		setSize (width, height);
	}

	~CAnimFrame (void)
	{
		free (src_rgba);
		Bitmap::~Bitmap ();
	}

	void setSize(int width,int height)
	{
		m_iWidth = width;
		m_iHeight = height;
		Bitmap::setSize (width, height);
	}

	void setImage (LPBITMAPINFO bi)
	{
		unsigned char *dib = (unsigned char *)bi->bmiColors;
		unsigned char *tmp = src_rgba;

		// Read the bitmap info into the temporary file
		for (int i = bi->bmiHeader.biHeight - 1; i >= 0; i--)
		{
			for (int j = 0; j < bi->bmiHeader.biWidth; j++)
			{
				byte *pixel = (dib + (i * bi->bmiHeader.biWidth * 3) + (j * 3));
				// This covers RGB (note it is in BGR in dib)
				*tmp++ = *(pixel + 2);
				*tmp++ = *(pixel + 1);
				*tmp++ = *(pixel);
				*tmp++ = 255;
			}
		}

		// Scale the image down to the correct size
		ScaleRectAvg ((unsigned long *)_rgba, (unsigned long *)src_rgba,
			bi->bmiHeader.biWidth, bi->bmiHeader.biHeight,
			m_iWidth, m_iHeight);
	}

private:
	int m_iWidth, m_iHeight;
	unsigned char *src_rgba;
};

///////////////////////////////////////////////////////////////////////////////////////////////
/*
// This one just draws a green frame which doesn't change
// That seems to indicate that _rgba is never being set properly
class CAnimFrame : public Bitmap {
public:
	CAnimFrame (int width, int height, int src_width, int src_height) : Bitmap ()
	{
		setSize (width, height);

		bufflength = width * height * 3 + 18;
		buffer = (unsigned char *)malloc(bufflength);

		mis = new MemoryInputStream ();
	}

	~CAnimFrame (void)
	{
		delete mis;
		free (buffer);
		Bitmap::~Bitmap ();
	}

	void setSize(int width,int height)
	{
		m_iWidth = width;
		m_iHeight = height;
		Bitmap::setSize (width, height);
	}

	void setImage (LPBITMAPINFO bi)
	{
		// Create a TGA file in memory and then display it through a BitmapTGA class
		memset (buffer, 0, 18);
		buffer[2] = 2;		// uncompressed type
		buffer[12] = bi->bmiHeader.biWidth & 255;
		buffer[13] = bi->bmiHeader.biWidth >> 8;
		buffer[14] = bi->bmiHeader.biHeight & 255;
		buffer[15] = bi->bmiHeader.biHeight >> 8;
		buffer[16] = 24;	// pixel size

		// They are already bgr, so just copy them
		memcpy (buffer + 18, bi->bmiColors, bufflength - 18);

		// Now that we have the tga file ready, create it from memory
		mis->m_pData = buffer;
		mis->m_DataLen = bufflength;
		mis->m_ReadPos = 0;

		if (m_pTGA)
			delete m_pTGA;
		m_pTGA = new vgui::BitmapTGA(mis, true);
	}

	void paint (Panel *panel)
	{
		m_pTGA->doPaint (panel);
	}

private:
	int m_iWidth, m_iHeight, bufflength;
	unsigned char *buffer;
	BitmapTGA *m_pTGA;
	MemoryInputStream *mis;
};
*/

///////////////////////////////////////////////////////////////////////////////////////////////

vgui::BitmapTGA *createBitmap (LPBITMAPINFOHEADER lpbi)
{
	LPBITMAPINFO bi = (LPBITMAPINFO) lpbi;

	// Create a TGA file in memory and then display it through a BitmapTGA class
	// This goes so slowly it drops the framerate to 4fps
	unsigned char *buffer;
	int bufflength = lpbi->biWidth*lpbi->biHeight*3 + 18;

	buffer = (unsigned char *)malloc(bufflength);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = lpbi->biWidth&255;
	buffer[13] = lpbi->biWidth>>8;
	buffer[14] = lpbi->biHeight&255;
	buffer[15] = lpbi->biHeight>>8;
	buffer[16] = 24;	// pixel size

	// Now, write the pixels out as bgr
//	int j = 0;
//	for (int i = 18; i < bufflength; i += 3)
//	{
//		buffer[i]   = bi->bmiColors[j].rgbBlue;
//		buffer[i+1] = bi->bmiColors[j].rgbGreen;
//		buffer[i+2] = bi->bmiColors[j].rgbRed;
//		j++;
//	}
	// They are already bgr, so just copy them
	memcpy (buffer+18, bi->bmiColors, bufflength-18);

	// Write it to a file for testing
//	static int tmp = 0;
//	char sz[80];
//	sprintf (sz, "avi%i.tga", tmp);
//	tmp++;
//	FILE *file = fopen (sz, "wb");
//	fwrite (buffer, bufflength, 1, file);
//	fclose (file);

	// Now that we have the tga file ready, create it from memory
	static MemoryInputStream mis;
	mis.m_pData = buffer;
	mis.m_DataLen = bufflength;
	mis.m_ReadPos = 0;
	vgui::BitmapTGA *pRet = new vgui::BitmapTGA(&mis, true);

	free (buffer);
	return pRet;
}

///////////////////////////////////////////////////////////////////////////////////////////////

CAviImage::CAviImage (const char* pAviName, int x, int y, int wide, int tall) : CImageLabel ("", x, y, wide, tall)
{
	sprintf (m_szFileName, "hlrally\\media\\%s.avi", pAviName);
	// Open the AVI file
	if (AVIFileOpen (&m_pAviFile, m_szFileName, OF_READ, NULL))
	{
		m_pAviFile = NULL;
		m_pAviStream = NULL;
		m_pFrame = NULL;
		return;
	}
	// Get the streams
	if (AVIFileGetStream (m_pAviFile, &m_pAviStream, 0L, 0))
	{
		AVIFileRelease (m_pAviFile);
		m_pAviFile = NULL;
		m_pAviStream = NULL;
		m_pFrame = NULL;
		return;
	}
	// Prepare to receive each individual frame
	if ((m_pFrame = AVIStreamGetFrameOpen (m_pAviStream, NULL)) == NULL)
	{
		AVIStreamRelease (m_pAviStream);
		AVIFileRelease (m_pAviFile);
		m_pAviFile = NULL;
		m_pAviStream = NULL;
		m_pFrame = NULL;
		return;
	}

	// Set up some variables
	m_flStartTime = gEngfuncs.GetClientTime ();
	m_flLength = AVIStreamLengthTime (m_pAviStream) / 1000;

	// Get the AVI width and height from the first frame
	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) AVIStreamGetFrame(m_pFrame, 1);
	m_pBitmap = new CAnimFrame (wide, tall, lpbi->biWidth, lpbi->biHeight);
	setImage (m_pBitmap);
}

CAviImage::~CAviImage ()
{
	if (m_pFrame != NULL)
		AVIStreamGetFrameClose (m_pFrame);

	if (m_pAviStream != NULL)
		AVIStreamRelease (m_pAviStream);

	if (m_pAviFile != NULL)
		AVIFileRelease (m_pAviFile);

	if (m_pBitmap != NULL)
		delete m_pBitmap;

	if (m_pTGA != NULL)
		delete m_pTGA;
}

void CAviImage::paint (void)
{
	if ((m_pAviFile == NULL) || (m_pAviStream == NULL))
		return;

	LPBITMAPINFOHEADER lpbi;
	float flCurrentTime = gEngfuncs.GetClientTime ();

	// Which frame belongs at this time?
	if ((flCurrentTime - m_flStartTime) > m_flLength)
		m_flStartTime = flCurrentTime;		// Reset the start time

	long lFrame = AVIStreamTimeToSample(m_pAviStream, (flCurrentTime - m_flStartTime) * 1000);

	//gEngfuncs.Con_DPrintf ("Frame %i\n", lFrame);

	// Get the current frame
	lpbi = (LPBITMAPINFOHEADER) AVIStreamGetFrame(m_pFrame, lFrame);
	((CAnimFrame *)_image)->setImage ((LPBITMAPINFO) lpbi);
	_image->doPaint (this);

	// Draw the frame
//	if (m_pTGA)
//		delete m_pTGA;

//	m_pTGA = createBitmap (m_pTGA, lpbi);
//	setImage (m_pTGA);
//	_image->doPaint (this);
}
// SaRcaZm - V5 - End
