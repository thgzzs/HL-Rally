// rally_vgui.h
//
// Contains all classes related to the vgui
//
// by SaRcaZm

#if !defined ( RALLY_VGUI_H )
#define RALLY_VGUI_H
#if defined( _WIN32 )
#pragma once
#include <windows.h>
#include <vfw.h>
#endif

#include "rally_vguifx.h"

// SaRcaZm - V7 - Start
// Rally_Image.cpp functions
typedef unsigned char byte;
byte *LoadTGA (char *filename, int *width, int *height);
byte *LoadBMP (char *filename, int *width, int *height);
void ScaleRectAvg(unsigned long *Target, unsigned long *Source, int SrcWidth, int SrcHeight, int TgtWidth, int TgtHeight);
unsigned long RGBAto24 (byte r, byte g, byte b, byte a);
void _24toRGBA (unsigned long l, byte *r, byte *g, byte* b, byte *a);
// SaRcaZm - V7 - End

// SaRcaZm - V4 - Start
// This needs to be bigger to hold all of our teams
//#define MAX_TEAMS	11
#define MAX_TEAMS	13
//#define MAX_MODELS	2
#define MAX_MODELS	NUM_CLASS_BUTTONS
// SaRcaZm - V4 - End

// SaRcaZm - V5 - Start
#define MAX_TEAM_BUTTONS MAX_TEAMS + 1
// SaRcaZm - V5 - End

// SaRcaZm - V7 - Start
#define MONEY_R		0
#define MONEY_G		198
#define MONEY_B		0

// Moved this up from below
#define MAX_ANIM_FRAMES		30
// SaRcaZm - V7 - End

typedef enum {
	STAT_ACCEL = 0,
	STAT_HANDLING,
	STAT_TOPSPEED,

	NUM_STATS
} rally_statistics;

// SaRcaZm - V5 - Start
// Changes by FragMented!
// Moved rally_teaminfo_t to rallyfx.h
// SaRcaZm - V5 - End

class CProgressBar : public Panel {
public:
	CProgressBar (int x, int y, int wide, int tall, int divisions, int progress);
	CProgressBar (int x, int y, int wide, int tall, int divisions, int progress, bool vertical);
	void setProgress (int progress);
	int getProgress (void);
	void setExtraProgress (int extraprogress);
	void paintBackground (void);
private:
	int m_iDivisions, m_iProgress, m_iExtraProgress;
	bool m_bVertical;
};

// Custom button that draws a graphic
class CImageButton : public CommandButton {
public:
	CImageButton (char *name, int x, int y, int wide, int tall);
	~CImageButton (void);
	virtual void paint();
	virtual void paintBackground();
	virtual void addButtonClickSignal (CMenuPanel *panel, int iNum);
	// SaRcaZm - V5 - Start
	void removeAllActionSignals (void);
	// SaRcaZm - V5 - End
	BitmapTGA	*m_pTGAUp, *m_pTGADwn;
protected:
	CMenuPanel *m_pPanel;
	int m_iPanelNum;
};

// SaRcaZm - V5 - Start
class CManuImageButton : public CImageButton {
public:
	CManuImageButton (char *name, int x, int y, int wide, int tall);
	virtual void fireActionSignal (void);

private:
	char m_szName[80];
};
// SaRcaZm - V5 - End


// SaRcaZm - V7 - Start
// Class for the class image buttons, it just removes the spinning model
class CClassImageButton : public CommandButton {
public:
	CClassImageButton (char *name, int x, int y, int wide, int tall);
	~CClassImageButton (void);
	virtual void fireActionSignal (void);
	virtual void paint();
	virtual void paintBackground();
	// SaRcaZm - V5 - Start
	void removeAllActionSignals (void);
	// SaRcaZm - V5 - End
private:
	// SaRcaZm - V7 - Start
	//BitmapTGA	*m_pTGAFrames[MAX_ANIM_FRAMES];
	int m_iCurFrame;//, m_iNumFrames;
	// SaRcaZm - V7 - End
	float m_iFrameTime;
};
// SaRcaZm - V7 - End

// Class for the model window so that the model can be manipulated with the mouse
class C3DModelPanel : public CTransparentPanel {
public:
	C3DModelPanel (int iTrans, int x,int y,int wide,int tall);
	void internalMousePressed(MouseCode code);
	void internalMouseReleased(MouseCode code);
	void internalCursorMoved(int x,int y);
	// SaRcaZm - V4 - Start
	void internalCursorExited (void);
	// SaRcaZm - V4 - End

private:
	// SaRcaZm - V4 - Start
	void setMouseDown (bool bDown);
	// SaRcaZm - V4 - End

	int m_iMouseDown;
	int m_iPrevX, m_iPrevY;
};

// SaRcaZm - V5 - Start
class CCustResImageLabel : public CImageLabel {
public:
	CCustResImageLabel (const char* pImageName,int x,int y);
	CCustResImageLabel (const char* pImageName,int x,int y,int wide,int tall);
	~CCustResImageLabel (void);
};

// SaRcaZm - V6 - Start
// Added money amounts to the ActionSignals
extern unsigned int money;

class CMenuHandler_RallyCommand : public ActionSignal
{
protected:
	char	m_pszCommand[MAX_COMMAND_SIZE];
	int		m_iCloseVGUIMenu;
	unsigned int		m_iAmount;
public:
	CMenuHandler_RallyCommand( char *pszCommand )
	{
		strncpy( m_pszCommand, pszCommand, MAX_COMMAND_SIZE);
		m_pszCommand[MAX_COMMAND_SIZE-1] = '\0';
		m_iCloseVGUIMenu = false;
		m_iAmount = 0;
	}

	CMenuHandler_RallyCommand( char *pszCommand, int amount, int iClose )
	{
		strncpy( m_pszCommand, pszCommand, MAX_COMMAND_SIZE);
		m_pszCommand[MAX_COMMAND_SIZE-1] = '\0';
		m_iCloseVGUIMenu = iClose;
		m_iAmount = (unsigned int) amount;
	}

	virtual void actionPerformed(Panel* panel)
	{
		if (m_iAmount > money)
		{
			CenterPrint ("Insufficient Funds");
			return;
		}

		gViewPort->AddCommand (m_pszCommand);
		money -= m_iAmount;
		CRallyVGUIFX::getSingleton()->ClearVGUIModels ();

		if (m_iCloseVGUIMenu)
			gViewPort->HideTopMenu();
	}
};
// SaRcaZm - V6 - End

// SaRcaZm - V7 - Start
#include "rally_sound.h"
class CMenuHandler_RallySound : public ActionSignal
{
protected:
	bool	m_bLoop;
	int		m_iPitch;
	char	m_szSound[80];

public:
	CMenuHandler_RallySound( char *pszSound )
	{
		strcpy (m_szSound, pszSound);
		m_bLoop = false;
		m_iPitch = 0;
	}

	CMenuHandler_RallySound( char *pszSound, bool bLoop, int iPitch )
	{
		strcpy (m_szSound, pszSound);
		m_bLoop = bLoop;
		m_iPitch = iPitch;
	}

	virtual void actionPerformed(Panel* panel)
	{
		RallySound->fmodPlay (m_szSound, m_bLoop, m_iPitch);
	}
};
// SaRcaZm - V7 - End

class CRaceImageButton : public CImageButton {
public:
	CRaceImageButton (char *name, int x, int y, int wide, int tall);
	virtual void fireActionSignal (void);
};

class CAnimFrame;

class CAviImage : public CImageLabel{
public:
	CAviImage (const char* pAviName, int x, int y, int wide, int tall);
	~CAviImage (void);
	virtual void paint (void);

private:
	PAVIFILE m_pAviFile;
	PAVISTREAM m_pAviStream;
	PGETFRAME m_pFrame;
	char m_szFileName[80];
	float m_flStartTime, m_flLength;

	CAnimFrame	*m_pBitmap;
};
// SaRcaZm - V5 - End

// SaRcaZm - V6 - Start
class CMenuHandler_RallyRemoveCommand : public ActionSignal
{
protected:
	char	m_pszCommand[MAX_COMMAND_SIZE];
	int		m_iCloseVGUIMenu;
	int		m_iAmount;
public:
	CMenuHandler_RallyRemoveCommand( char *pszCommand )
	{
		strncpy( m_pszCommand, pszCommand, MAX_COMMAND_SIZE);
		m_pszCommand[MAX_COMMAND_SIZE-1] = '\0';
		m_iCloseVGUIMenu = false;
	}

	CMenuHandler_RallyRemoveCommand( char *pszCommand, int amount, int iClose )
	{
		strncpy( m_pszCommand, pszCommand, MAX_COMMAND_SIZE);
		m_pszCommand[MAX_COMMAND_SIZE-1] = '\0';
		m_iCloseVGUIMenu = iClose;
		m_iAmount = amount;
	}

	virtual void actionPerformed(Panel* panel)
	{
		if (gViewPort->RemoveCommand (m_pszCommand))
		{
			money += m_iAmount;
			CRallyVGUIFX::getSingleton()->ClearVGUIModels ();

			if (m_iCloseVGUIMenu)
				gViewPort->HideTopMenu();
		}
	}
};

class CPartsListButton : public CommandButton {
public:
	CPartsListButton (char *szPartName, int iPartPrice, int x, int y, int wide, int tall);
	void paintBackground (void);
};
// SaRcaZm - V6 - End

// SaRcaZm - V7 - Start
#include "..\game_shared\vgui_slider2.h"
class CRallySlider : public Slider2 {
public:
	CRallySlider (char *name, int x,int y,int wide,int tall);
	~CRallySlider (void);
	void paintBackground (void);

private:
	BitmapTGA *m_pTGABack, *m_pTGAPoint;
};

#include "vgui_togglebutton.h"
class CImageCheckButton : public ToggleButton {
public:
	CImageCheckButton (char *name, bool check, int x, int y);
	~CImageCheckButton (void);
	void paint (void);
	void internalMousePressed(MouseCode code);
	bool isChecked (void);
	void setChecked (bool checked);

private:
	bool m_bChecked;
	BitmapTGA *m_pTGACheck, *m_pTGAUncheck;
};
// SaRcaZm - V7 - End

#endif	// RALLY_VGUI_H