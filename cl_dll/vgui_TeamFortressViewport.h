
#ifndef TEAMFORTRESSVIEWPORT_H
#define TEAMFORTRESSVIEWPORT_H

#include<VGUI_Panel.h>
#include<VGUI_Frame.h>
#include<VGUI_TextPanel.h>
#include<VGUI_Label.h>
#include<VGUI_Button.h>
#include<VGUI_ActionSignal.h>
#include<VGUI_InputSignal.h>
#include<VGUI_Scheme.h>
#include<VGUI_Image.h>
#include<VGUI_FileInputStream.h>
#include<VGUI_BitmapTGA.h>
#include<VGUI_DesktopIcon.h>
#include<VGUI_App.h>
#include<VGUI_MiniApp.h>
#include<VGUI_LineBorder.h>
#include<VGUI_String.h>
#include<VGUI_ScrollPanel.h>
#include<VGUI_ScrollBar.h>
#include<VGUI_Slider.h>

// custom scheme handling
#include "vgui_SchemeManager.h"

#define TF_DEFS_ONLY
#include "tf_defs.h"

using namespace vgui;

class Cursor;
class ScorePanel;
class CCommandMenu;
class CommandLabel;
class CommandButton;
class BuildButton;
class ClassButton;
class CMenuPanel;
class ServerBrowser;
class DragNDropPanel;
class CTransparentPanel;
class CClassMenuPanel;
class CTeamMenuPanel;


char* GetVGUITGAName(const char *pszName);
BitmapTGA *LoadTGAForRes(const char* pImageName);
void ScaleColors( int &r, int &g, int &b, int a );
extern char *sTFClassSelection[];
extern int sTFValidClassInts[];
extern char *sLocalisedClasses[];
extern int iTeamColors[5][3];
extern int iNumberOfTeamColors;

#define MAX_SERVERNAME_LENGTH	32

// Command Menu positions 
#define MAX_MENUS				80
#define MAX_BUTTONS				100

#define BUTTON_SIZE_Y			YRES(30)
#define CMENU_SIZE_X			XRES(160)

#define SUBMENU_SIZE_X			(CMENU_SIZE_X / 8)
#define SUBMENU_SIZE_Y			(BUTTON_SIZE_Y / 6)

#define CMENU_TOP				(BUTTON_SIZE_Y * 4)

#define MAX_TEAMNAME_SIZE		64
#define MAX_BUTTON_SIZE			32

// Map Briefing Window
#define MAPBRIEF_INDENT			30

// Team Menu
#define TMENU_INDENT_X			(30 * ((float)ScreenHeight / 640))
#define TMENU_HEADER			100
#define TMENU_SIZE_X			(ScreenWidth - (TMENU_INDENT_X * 2))
#define TMENU_SIZE_Y			(TMENU_HEADER + BUTTON_SIZE_Y * 7)
#define TMENU_PLAYER_INDENT		(((float)TMENU_SIZE_X / 3) * 2)
#define TMENU_INDENT_Y			(((float)ScreenHeight - TMENU_SIZE_Y) / 2)

// Class Menu
#define CLMENU_INDENT_X			(30 * ((float)ScreenHeight / 640))
#define CLMENU_HEADER			100
#define CLMENU_SIZE_X			(ScreenWidth - (CLMENU_INDENT_X * 2))
#define CLMENU_SIZE_Y			(CLMENU_HEADER + BUTTON_SIZE_Y * 11)
#define CLMENU_PLAYER_INDENT	(((float)CLMENU_SIZE_X / 3) * 2)
#define CLMENU_INDENT_Y			(((float)ScreenHeight - CLMENU_SIZE_Y) / 2)

// Arrows
enum
{
	ARROW_UP,
	ARROW_DOWN,
	ARROW_LEFT,
	ARROW_RIGHT,
};

//==============================================================================
// VIEWPORT PIECES
//============================================================
// Wrapper for an Image Label without a background
class CImageLabel : public Label
{
public:
	BitmapTGA	*m_pTGA;

public:
	CImageLabel( const char* pImageName,int x,int y );
	CImageLabel( const char* pImageName,int x,int y,int wide,int tall );
	~CImageLabel (void);

	virtual int getImageTall();
	virtual int getImageWide();

	virtual void paintBackground()
	{
		// Do nothing, so the background's left transparent.
	}
};

// Command Label
// Overridden label so we can darken it when submenus open
class CommandLabel : public Label
{
private:
	int		m_iState;

public:
	CommandLabel(const char* text,int x,int y,int wide,int tall) : Label(text,x,y,wide,tall)
	{
		m_iState = false;
	}

	void PushUp()
	{
		m_iState = false;
		repaint();
	}

	void PushDown()
	{
		m_iState = true;
		repaint();
	}
};

//============================================================
// Command Buttons
class CommandButton : public Button
{
private:
	int		m_iPlayerClass;

	// Submenus under this button
	CCommandMenu *m_pSubMenu;
	CCommandMenu *m_pParentMenu;
	CommandLabel *m_pSubLabel;

// SaRcaZm - V6 - Start
protected:
	char m_sMainText[MAX_BUTTON_SIZE];
private:
// SaRcaZm - V6 - End
	char m_cBoundKey;

	SchemeHandle_t m_hTextScheme;

	void RecalculateText( void );

public:
	bool	m_bNoHighlight;

public:
	// Constructors
	CommandButton( const char* text,int x,int y,int wide,int tall, bool bNoHighlight = false);
	CommandButton( int iPlayerClass, const char* text,int x,int y,int wide,int tall);
	// SaRcaZm - V8 - Start
	~CommandButton (void);
	// SaRcaZm - V8 - End
	void Init( void );

	// Menu Handling
	void AddSubMenu( CCommandMenu *pNewMenu );
	void AddSubLabel( CommandLabel *pSubLabel )
	{
		m_pSubLabel = pSubLabel;
	}

	virtual int IsNotValid( void )
	{
		return false;
	}

	void UpdateSubMenus( int iAdjustment );
	int GetPlayerClass() { return m_iPlayerClass; };
	CCommandMenu *GetSubMenu() { return m_pSubMenu; };
	
	CCommandMenu *getParentMenu( void );
	void setParentMenu( CCommandMenu *pParentMenu );

	// Overloaded vgui functions
	virtual void paint();
	virtual void setText( const char *text );
	virtual void paintBackground();

	void cursorEntered( void );
	void cursorExited( void );

	void setBoundKey( char boundKey );
	char getBoundKey( void );
};

//============================================================
// Command Menus
class CCommandMenu : public Panel
{
private:
	CCommandMenu *m_pParentMenu;
	int			  m_iXOffset;
	int			  m_iYOffset;

	// Buttons in this menu
	CommandButton *m_aButtons[ MAX_BUTTONS ];
	int			  m_iButtons;

public:
	CCommandMenu( CCommandMenu *pParentMenu, int x,int y,int wide,int tall ) : Panel(x,y,wide,tall)
	{
		m_pParentMenu = pParentMenu;
		m_iXOffset = x;
		m_iYOffset = y;
		m_iButtons = 0;
	}

	void		AddButton( CommandButton *pButton );
	bool		RecalculateVisibles( int iNewYPos, bool bHideAll );
	void		RecalculatePositions( int iYOffset );
	void		MakeVisible( CCommandMenu *pChildMenu );

	CCommandMenu *GetParentMenu() { return m_pParentMenu; };
	int			GetXOffset() { return m_iXOffset; };
	int			GetYOffset() { return m_iYOffset; };
	int			GetNumButtons() { return m_iButtons; };
	CommandButton *FindButtonWithSubmenu( CCommandMenu *pSubMenu );

	void		ClearButtonsOfArmedState( void );


	bool		KeyInput( int keyNum );

	virtual void paintBackground();
};

//==============================================================================
// SaRcaZm - V8 - Start
class CMessageWindowPanel;
class CMP3MenuPanel;
// SaRcaZm - V8 - End
// SaRcaZm - V6 - Start
#define MAX_TEXTFILE_LENGTH		5120
#define MAX_TEXTFILE_CHUNK		60
// SaRcaZm - V6 - End

class TeamFortressViewport : public Panel
{
private:
	vgui::Cursor* _cursorNone;
	vgui::Cursor* _cursorArrow;

	int			 m_iInitialized;

	CCommandMenu *m_pCommandMenus[ MAX_MENUS ];
	CCommandMenu *m_pCurrentCommandMenu;
	float		 m_flMenuOpenTime;
	float		 m_flScoreBoardLastUpdated;
	int			 m_iNumMenus;

	int			 m_iCurrentPlayerClass;
	int			 m_iUser1;
	int			 m_iUser2;
	int			 m_iUser3;






	void		 CreateSpectatorMenu( void );
	
	// Scheme handler
	CSchemeManager m_SchemeManager;

	// MOTD
	int		m_iGotAllMOTD;
	char	m_szMOTD[ MAX_MOTD_LENGTH ];

	//  Command Menu Team buttons
	CommandButton *m_pTeamButtons[13];
	CommandButton *m_pDisguiseButtons[5];
	BuildButton   *m_pBuildButtons[3];
	BuildButton   *m_pBuildActiveButtons[3];

	// Server Browser
	ServerBrowser *m_pServerBrowser;

	// Spectator "menu"
	CTransparentPanel	*m_pSpectatorMenu;
	Label				*m_pSpectatorLabel;
	Label				*m_pSpectatorHelpLabel;
	int					m_iAllowSpectators;

	// Data for specific sections of the Command Menu
	int			m_iValidClasses[5];
	int			m_iIsFeigning;
	int			m_iIsSettingDetpack;
	int			m_iNumberOfTeams;
	int			m_iBuildState;
	int			m_iRandomPC;
	char		m_sTeamNames[12][MAX_TEAMNAME_SIZE];

	// Localisation strings
	char		m_sDetpackStrings[3][MAX_BUTTON_SIZE];

	char		m_sMapName[64];

public:
	int			 m_iCurrentTeamNumber;
	short		CurrentRallyTeam;

	// SaRcaZm - V6 - Start
	int		m_iGotAllTextFiles;
	char	m_szAnimTxt[MAX_TEXTFILE_LENGTH], m_szCarInfoTxt[MAX_TEXTFILE_LENGTH], m_szCustomTxt[MAX_TEXTFILE_LENGTH];
	// SaRcaZm - V6 - End

	// VGUI Menus
	void		 CreateTeamMenu( void );
	CMenuPanel*	 ShowTeamMenu( void );
	void		 CreateClassMenu( void );
	CMenuPanel*	 ShowClassMenu( void );

	TeamFortressViewport(int x,int y,int wide,int tall);
	// SaRcaZm - V8 - Start
	~TeamFortressViewport (void);
	// SaRcaZm - V8 - End
	void Initialize( void );

	void CreateCommandMenu( void );
	void CreateScoreBoard( void );
	void CreateServerBrowser( void );
	CommandButton *CreateCustomButton( char *pButtonText, char *pButtonName );
	CCommandMenu *CreateDisguiseSubmenu( CommandButton *pButton, CCommandMenu *pParentMenu, const char *commandText );

	void UpdateCursorState( void );
	void UpdateCommandMenu( void );
	void UpdateOnPlayerInfo( void );
	void UpdateHighlights( void );
	void UpdateSpectatorMenu( void );

	int	 KeyInput( int down, int keynum, const char *pszCurrentBinding );
	void InputPlayerSpecial( void );
	void GetAllPlayersInfo( void );
	void DeathMsg( int killer, int victim );

	void ShowCommandMenu( void );
	void InputSignalHideCommandMenu( void );
	void HideCommandMenu( void );
	void SetCurrentCommandMenu( CCommandMenu *pNewMenu );
	void SetCurrentMenu( CMenuPanel *pMenu );

	void ShowScoreBoard( void );
	void HideScoreBoard( void );
	bool IsScoreBoardVisible( void );

	bool AllowedToPrintText( void );

	void ShowVGUIMenu( int iMenu );
	void HideVGUIMenu( void );
	void HideTopMenu( void );

	void ToggleServerBrowser( void );

	CMenuPanel* CreateTextWindow( int iTextToShow );

	CCommandMenu *CreateSubMenu( CommandButton *pButton, CCommandMenu *pParentMenu );

	// Data Handlers
	int GetValidClasses(int iTeam) { return m_iValidClasses[iTeam]; };
	int GetNumberOfTeams() { return m_iNumberOfTeams; };
	int GetIsFeigning() { return m_iIsFeigning; };
	int GetIsSettingDetpack() { return m_iIsSettingDetpack; };
	int GetBuildState() { return m_iBuildState; };
	int IsRandomPC() { return m_iRandomPC; };
	char *GetTeamName( int iTeam ) { return m_sTeamNames[iTeam]; };
	int GetAllowSpectators() { return m_iAllowSpectators; };

	// Message Handlers
	int MsgFunc_ValClass(const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamNames(const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Feign(const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Detpack(const char *pszName, int iSize, void *pbuf );
	int MsgFunc_VGUIMenu(const char *pszName, int iSize, void *pbuf );
	int MsgFunc_VGUIClasses(const char *pszName, int iSize, void *pbuf ); // FragMented!
	int MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_BuildSt( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_RandomPC( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_ServerName( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Spectator( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_AllowSpec( const char *pszName, int iSize, void *pbuf );

	// SaRcaZm - V6 - Start
	int MsgFunc_TextFiles( const char *pszName, int iSize, void *pbuf );
	// SaRcaZm - V6 - End

	// Input
	bool SlotInput( int iSlot );

	virtual void paintBackground();

	CSchemeManager *GetSchemeManager( void ) { return &m_SchemeManager; }
	ScorePanel *GetScoreBoard( void ) { return m_pScoreBoard; }

// SaRcaZm - V8 - Start
#include "nommgr.h"
	void *operator new( size_t stAllocateBlock );
#include "mmgr.h"
// SaRcaZm - V8 - End

	// SaRcaZm - V5 - Start
	CMenuPanel *ShowMainMenu ();
	void CreateMainMenu ();

	void AddCommand (const char *pszCmd);
	void ExecuteCommands (void);
	bool SearchCommands (const char *pszCmd);
	bool GetCommandParam (const char *pszCmd, char *pszArgs);
	char m_szCmd[256];
	char m_szTeam[80], m_szModel[80]; // Creme this was missing. I hope its 80 ;)
	// SaRcaZm - V5 - End
	// SaRcaZm - V6 - Start
	bool RemoveCommand (const char *pszKeyWord);
	// SaRcaZm - V6 - End
	// SaRcaZm - V7 - Start
	void ClearCommands (void);
	// SaRcaZm - V7 - End
	// SaRcaZm - V8 - Start
	CMenuPanel *ShowMp3Menu ();
	void CreateMp3Menu ();
	void mp3Command (char *szCommand);
	void mp3Frame (double time);
	// SaRcaZm - V8 - End

public:
	// VGUI Menus
	CMenuPanel					*m_pCurrentMenu;
	CTeamMenuPanel				*m_pTeamMenu;
	CClassMenuPanel				*m_pClassMenu;
	ScorePanel					*m_pScoreBoard;

	// SaRcaZm - V8 - Start
	CMessageWindowPanel			*m_pMainMenu;
	CMP3MenuPanel				*m_pMp3Menu;
	// SaRcaZm - V8 - End

	char	m_szServerName[ MAX_SERVERNAME_LENGTH ];
};

//============================================================
// Command Menu Button Handlers
#define MAX_COMMAND_SIZE	256

class CMenuHandler_StringCommand : public ActionSignal
{
protected:
	char	m_pszCommand[MAX_COMMAND_SIZE];
	int		m_iCloseVGUIMenu;
public:
	CMenuHandler_StringCommand( char *pszCommand )
	{
		strncpy( m_pszCommand, pszCommand, MAX_COMMAND_SIZE);
		m_pszCommand[MAX_COMMAND_SIZE-1] = '\0';
		m_iCloseVGUIMenu = false;
	}

	CMenuHandler_StringCommand( char *pszCommand, int iClose )
	{
		strncpy( m_pszCommand, pszCommand, MAX_COMMAND_SIZE);
		m_pszCommand[MAX_COMMAND_SIZE-1] = '\0';
		m_iCloseVGUIMenu = true;
	}

	virtual void actionPerformed(Panel* panel)
	{
		gEngfuncs.pfnClientCmd(m_pszCommand);

		if (m_iCloseVGUIMenu)
			gViewPort->HideTopMenu();
		else
			gViewPort->HideCommandMenu();
	}
};

// This works the same as CMenuHandler_StringCommand, except it watches the string command 
// for specific commands, and modifies client vars based upon them.
class CMenuHandler_StringCommandWatch : public CMenuHandler_StringCommand
{
private:
public:
	CMenuHandler_StringCommandWatch( char *pszCommand ) : CMenuHandler_StringCommand( pszCommand )
	{
	}

	CMenuHandler_StringCommandWatch( char *pszCommand, int iClose ) : CMenuHandler_StringCommand( pszCommand, iClose )
	{
	}

	virtual void actionPerformed(Panel* panel)
	{
		CMenuHandler_StringCommand::actionPerformed( panel );

		// Try to guess the player's new team (it'll be corrected if it's wrong)
		if ( !strcmp( m_pszCommand, "jointeam 1" ) )
			g_iTeamNumber = 1;
		else if ( !strcmp( m_pszCommand, "jointeam 2" ) )
			g_iTeamNumber = 2;
		else if ( !strcmp( m_pszCommand, "jointeam 3" ) )
			g_iTeamNumber = 3;
		else if ( !strcmp( m_pszCommand, "jointeam 4" ) )
			g_iTeamNumber = 4;
	}
};

// Used instead of CMenuHandler_StringCommand for Class Selection buttons.
// Checks the state of hud_classautokill and kills the player if set
class CMenuHandler_StringCommandClassSelect : public CMenuHandler_StringCommand
{
private:
public:
	CMenuHandler_StringCommandClassSelect( char *pszCommand ) : CMenuHandler_StringCommand( pszCommand )
	{
	}

	CMenuHandler_StringCommandClassSelect( char *pszCommand, int iClose ) : CMenuHandler_StringCommand( pszCommand, iClose )
	{
	}

	virtual void actionPerformed(Panel* panel);
};

class CMenuHandler_PopupSubMenuInput : public InputSignal
{
private:
	CCommandMenu *m_pSubMenu;
	Button		 *m_pButton;
public:
	CMenuHandler_PopupSubMenuInput( Button *pButton, CCommandMenu *pSubMenu )
	{
		m_pSubMenu = pSubMenu;
		m_pButton = pButton;
	}

	virtual void cursorMoved(int x,int y,Panel* panel)
	{
		//gViewPort->SetCurrentCommandMenu( m_pSubMenu );
	}

	virtual void cursorEntered(Panel* panel) 
	{
		gViewPort->SetCurrentCommandMenu( m_pSubMenu );

		if (m_pButton)
			m_pButton->setArmed(true);
	};
	virtual void cursorExited(Panel* Panel) {};
	virtual void mousePressed(MouseCode code,Panel* panel)  {};
	virtual void mouseDoublePressed(MouseCode code,Panel* panel)  {};
	virtual void mouseReleased(MouseCode code,Panel* panel) {};
	virtual void mouseWheeled(int delta,Panel* panel) {};
	virtual void keyPressed(KeyCode code,Panel* panel) {};
	virtual void keyTyped(KeyCode code,Panel* panel) {};
	virtual void keyReleased(KeyCode code,Panel* panel) {};
	virtual void keyFocusTicked(Panel* panel) {};
};

class CMenuHandler_LabelInput : public InputSignal
{
private:
	ActionSignal *m_pActionSignal;
public:
	CMenuHandler_LabelInput( ActionSignal *pSignal )
	{
		m_pActionSignal = pSignal;
	}

	virtual void mousePressed(MouseCode code,Panel* panel)
	{
		m_pActionSignal->actionPerformed( panel );
	}

	virtual void mouseReleased(MouseCode code,Panel* panel) {};
	virtual void cursorEntered(Panel* panel) {};
	virtual void cursorExited(Panel* Panel) {};
	virtual void cursorMoved(int x,int y,Panel* panel) {};
	virtual void mouseDoublePressed(MouseCode code,Panel* panel)  {};
	virtual void mouseWheeled(int delta,Panel* panel) {};
	virtual void keyPressed(KeyCode code,Panel* panel) {};
	virtual void keyTyped(KeyCode code,Panel* panel) {};
	virtual void keyReleased(KeyCode code,Panel* panel) {};
	virtual void keyFocusTicked(Panel* panel) {};
};

#define HIDE_TEXTWINDOW		0
#define SHOW_MAPBRIEFING	1
#define SHOW_CLASSDESC		2
#define SHOW_MOTD			3

class CMenuHandler_TextWindow : public ActionSignal
{
private:
	int	m_iState;
public:
	CMenuHandler_TextWindow( int iState )
	{
		m_iState = iState;
	}

	virtual void actionPerformed(Panel* panel)
	{
		if (m_iState == HIDE_TEXTWINDOW)
		{
			gViewPort->HideTopMenu();
		}
		else 
		{
			gViewPort->HideCommandMenu();
			gViewPort->ShowVGUIMenu( m_iState );
		}
	}
};

class CDragNDropHandler : public InputSignal
{
private:
	DragNDropPanel*	m_pPanel;
	bool			m_bDragging;
	int				m_iaDragOrgPos[2];
	int				m_iaDragStart[2];

public:
	CDragNDropHandler(DragNDropPanel* pPanel)
	{
		m_pPanel = pPanel;
		m_bDragging = false;
	}

	void cursorMoved(int x,int y,Panel* panel);
	void mousePressed(MouseCode code,Panel* panel);
	void mouseReleased(MouseCode code,Panel* panel);

	void mouseDoublePressed(MouseCode code,Panel* panel) {};
	void cursorEntered(Panel* panel) {};
	void cursorExited(Panel* panel) {};
	void mouseWheeled(int delta,Panel* panel) {};
	void keyPressed(KeyCode code,Panel* panel) {};
	void keyTyped(KeyCode code,Panel* panel) {};
	void keyReleased(KeyCode code,Panel* panel) {};
	void keyFocusTicked(Panel* panel) {};
};

class CHandler_MenuButtonOver : public InputSignal
{
private:
	int			m_iButton;
	CMenuPanel	*m_pMenuPanel;
public:
	CHandler_MenuButtonOver( CMenuPanel *pPanel, int iButton )
	{
		m_iButton = iButton;
		m_pMenuPanel = pPanel;
	}
		
	void cursorEntered(Panel *panel);

	void cursorMoved(int x,int y,Panel* panel) {};
	void mousePressed(MouseCode code,Panel* panel) {};
	void mouseReleased(MouseCode code,Panel* panel) {};
	void mouseDoublePressed(MouseCode code,Panel* panel) {};
	void cursorExited(Panel* panel) {};
	void mouseWheeled(int delta,Panel* panel) {};
	void keyPressed(KeyCode code,Panel* panel) {};
	void keyTyped(KeyCode code,Panel* panel) {};
	void keyReleased(KeyCode code,Panel* panel) {};
	void keyFocusTicked(Panel* panel) {};
};

class CHandler_ButtonHighlight : public InputSignal
{
private:
	Button *m_pButton;
public:
	CHandler_ButtonHighlight( Button *pButton )
	{
		m_pButton = pButton;
	}

	virtual void cursorEntered(Panel* panel) 
	{ 
		m_pButton->setArmed(true);
	};
	virtual void cursorExited(Panel* Panel) 
	{
		m_pButton->setArmed(false);
	};
	virtual void mousePressed(MouseCode code,Panel* panel) {};
	virtual void mouseReleased(MouseCode code,Panel* panel) {};
	virtual void cursorMoved(int x,int y,Panel* panel) {};
	virtual void mouseDoublePressed(MouseCode code,Panel* panel)  {};
	virtual void mouseWheeled(int delta,Panel* panel) {};
	virtual void keyPressed(KeyCode code,Panel* panel) {};
	virtual void keyTyped(KeyCode code,Panel* panel) {};
	virtual void keyReleased(KeyCode code,Panel* panel) {};
	virtual void keyFocusTicked(Panel* panel) {};
};

//-----------------------------------------------------------------------------
// Purpose: Special handler for highlighting of command menu buttons
//-----------------------------------------------------------------------------
class CHandler_CommandButtonHighlight : public CHandler_ButtonHighlight
{
private:
	CommandButton *m_pCommandButton;
public:
	CHandler_CommandButtonHighlight( CommandButton *pButton ) : CHandler_ButtonHighlight( pButton )
	{
		m_pCommandButton = pButton;
	}

	virtual void cursorEntered( Panel *panel )
	{
		m_pCommandButton->cursorEntered();
	}

	virtual void cursorExited( Panel *panel )
	{
		m_pCommandButton->cursorExited();
	}
};


//================================================================
// Overidden Command Buttons for special visibilities
class ClassButton : public CommandButton
{
protected:
	int	m_iPlayerClass;

public:
	ClassButton( int iClass, const char* text,int x,int y,int wide,int tall, bool bNoHighlight ) : CommandButton( text,x,y,wide,tall, bNoHighlight)
	{
		m_iPlayerClass = iClass;
	}

	virtual int IsNotValid();
};

class TeamButton : public CommandButton
{
private:
	int	m_iTeamNumber;
public:
	TeamButton( int iTeam, const char* text,int x,int y,int wide,int tall ) : CommandButton( text,x,y,wide,tall)
	{
		m_iTeamNumber = iTeam;
	}

	virtual int IsNotValid()
	{
		int iTeams = gViewPort->GetNumberOfTeams();
		// Never valid if there's only 1 team
		if (iTeams == 1)
			return true;

		// Auto Team's always visible
		if (m_iTeamNumber == 12)
			return false;

		if (iTeams >= m_iTeamNumber && m_iTeamNumber != g_iTeamNumber)
			return false;

		return true;
	}
};

class FeignButton : public CommandButton
{
private:
	int	m_iFeignState;
public:
	FeignButton( int iState, const char* text,int x,int y,int wide,int tall ) : CommandButton( text,x,y,wide,tall)
	{
		m_iFeignState = iState;
	}

	virtual int IsNotValid()
	{
		// Only visible for spies
		if (g_iPlayerClass != PC_SPY)
			return true;

		if (m_iFeignState == gViewPort->GetIsFeigning())
			return false;

		return true;
	}
};

class SpectateButton : public CommandButton
{
public:
	SpectateButton( const char* text,int x,int y,int wide,int tall, bool bNoHighlight ) : CommandButton( text,x,y,wide,tall, bNoHighlight)
	{
	}

	virtual int IsNotValid()
	{
		// Only visible if the server allows it
		if ( gViewPort->GetAllowSpectators() != 0 )
			return false;

		return true;
	}
};

#define		DISGUISE_TEAM1		(1<<0)
#define		DISGUISE_TEAM2		(1<<1)
#define		DISGUISE_TEAM3		(1<<2)
#define		DISGUISE_TEAM4		(1<<3)

class DisguiseButton : public CommandButton
{
private:
	int m_iValidTeamsBits;
	int m_iThisTeam;
public:
	DisguiseButton( int iValidTeamNumsBits, const char* text,int x,int y,int wide,int tall ) : CommandButton( text,x,y,wide,tall,false )
	{
		m_iValidTeamsBits = iValidTeamNumsBits;
	}

	virtual int IsNotValid()
	{
		// Only visible for spies
		if ( g_iPlayerClass != PC_SPY )
			return true;

		// if it's not tied to a specific team, then always show (for spies)
		if ( !m_iValidTeamsBits )
			return false;

		// if we're tied to a team make sure we can change to that team
		int iTmp = 1 << (gViewPort->GetNumberOfTeams() - 1);
		if ( m_iValidTeamsBits & iTmp )
			return false;

		return true;
	}
};

class DetpackButton : public CommandButton
{
private:
	int	m_iDetpackState;
public:
	DetpackButton( int iState, const char* text,int x,int y,int wide,int tall ) : CommandButton( text,x,y,wide,tall)
	{
		m_iDetpackState = iState;
	}

	virtual int IsNotValid()
	{
		// Only visible for demomen
		if (g_iPlayerClass != PC_DEMOMAN)
			return true;

		if (m_iDetpackState == gViewPort->GetIsSettingDetpack())
			return false;

		return true;
	}
};

extern int iBuildingCosts[];
#define BUILDSTATE_HASBUILDING		(1<<0)		// Data is building ID (1 = Dispenser, 2 = Sentry)
#define BUILDSTATE_BUILDING			(1<<1)
#define BUILDSTATE_BASE				(1<<2)
#define BUILDSTATE_CANBUILD			(1<<3)		// Data is building ID (0 = Dispenser, 1 = Sentry)

class BuildButton : public CommandButton
{
private:
	int	m_iBuildState;
	int m_iBuildData;

public:
	enum Buildings
	{
		DISPENSER = 0,
		SENTRYGUN = 1,
	};

	BuildButton( int iState, int iData, const char* text,int x,int y,int wide,int tall ) : CommandButton( text,x,y,wide,tall)
	{
		m_iBuildState = iState;
		m_iBuildData = iData;
	}

	virtual int IsNotValid()
	{
		// Only visible for engineers
		if (g_iPlayerClass != PC_ENGINEER)
			return true;

		// If this isn't set, it's only active when they're not building
		if (m_iBuildState & BUILDSTATE_BUILDING)
		{
			// Make sure the player's building
			if ( !(gViewPort->GetBuildState() & BS_BUILDING) )
				return true;
		}
		else
		{
			// Make sure the player's not building
			if ( gViewPort->GetBuildState() & BS_BUILDING )
				return true;
		}

		if (m_iBuildState & BUILDSTATE_BASE)
		{
			// Only appear if we've got enough metal to build something, or something already built
			if ( gViewPort->GetBuildState() & (BS_HAS_SENTRYGUN | BS_HAS_DISPENSER | BS_CANB_SENTRYGUN | BS_CANB_DISPENSER) )
				return false;

			return true;
		}

		// Must have a building
		if (m_iBuildState & BUILDSTATE_HASBUILDING)
		{
			if ( m_iBuildData == BuildButton::DISPENSER && !(gViewPort->GetBuildState() & BS_HAS_DISPENSER) )
				return true;
			if ( m_iBuildData == BuildButton::SENTRYGUN && !(gViewPort->GetBuildState() & BS_HAS_SENTRYGUN) )
				return true;
		}

		// Can build something
		if (m_iBuildState & BUILDSTATE_CANBUILD)
		{
			// Make sure they've got the ammo and don't have one already
			if ( m_iBuildData == BuildButton::DISPENSER && (gViewPort->GetBuildState() & BS_CANB_DISPENSER) )
				return false;
			if ( m_iBuildData == BuildButton::SENTRYGUN && (gViewPort->GetBuildState() & BS_CANB_SENTRYGUN) )
				return false;

			return true;
		}

		return false;
	}
};

#define MAX_MAPNAME 256

class MapButton : public CommandButton
{
private:
	char m_szMapName[ MAX_MAPNAME ];

public:
	MapButton( const char *pMapName, const char* text,int x,int y,int wide,int tall ) : CommandButton( text,x,y,wide,tall)
	{
		sprintf( m_szMapName, "maps/%s.bsp", pMapName );
	}

	virtual int IsNotValid()
	{
		const char *level = gEngfuncs.pfnGetLevelName();
		if (!level)
			return true;

		// Does it match the current map name?
		if ( strcmp(m_szMapName, level) )
			return true;

		return false;
	}
};

//-----------------------------------------------------------------------------
// Purpose: CommandButton which is only displayed if the player is on team X
//-----------------------------------------------------------------------------
class TeamOnlyCommandButton : public CommandButton
{
private:
	int m_iTeamNum;

public:
	TeamOnlyCommandButton( int iTeamNum, const char* text,int x,int y,int wide,int tall ) : 
	  CommandButton( text, x, y, wide, tall ), m_iTeamNum(iTeamNum) {}

	virtual int IsNotValid()
	{
		if ( g_iTeamNumber != m_iTeamNum )
			return true;

		return CommandButton::IsNotValid();
	}
};

//============================================================
// Panel that can be dragged around
class DragNDropPanel : public Panel
{
private:
	bool		m_bBeingDragged;
	LineBorder	*m_pBorder;
public:
	DragNDropPanel(int x,int y,int wide,int tall) : Panel(x,y,wide,tall)
	{
		m_bBeingDragged = false;

		// Create the Drag Handler
		addInputSignal( new CDragNDropHandler(this) );

		// Create the border (for dragging)
		m_pBorder = new LineBorder();
	}

	virtual void setDragged( bool bState )
	{
		m_bBeingDragged = bState;

		if (m_bBeingDragged)
			setBorder(m_pBorder);
		else
			setBorder(NULL);
	}
};

//================================================================
// Panel that draws itself with a transparent black background
class CTransparentPanel : public Panel
{
private:
	int	m_iTransparency;
public:
	CTransparentPanel(int iTrans, int x,int y,int wide,int tall) : Panel(x,y,wide,tall)
	{
		m_iTransparency = iTrans;
	}

	virtual void paintBackground()
	{
		if (m_iTransparency)
		{
			// Transparent black background
			drawSetColor( 0,0,0, m_iTransparency );
			drawFilledRect(0,0,_size[0],_size[1]);
		}
	}
};

//================================================================
// Menu Panel that supports buffering of menus
class CMenuPanel : public CTransparentPanel
{
private:
	CMenuPanel *m_pNextMenu;
	int			m_iMenuID;
	int			m_iRemoveMe;
	int			m_iIsActive;
	float		m_flOpenTime;
public:
	CMenuPanel(int iRemoveMe, int x,int y,int wide,int tall) : CTransparentPanel(100, x,y,wide,tall)
	{
		Reset();
		m_iRemoveMe = iRemoveMe;
	}

	CMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CTransparentPanel(iTrans, x,y,wide,tall)
	{
		Reset();
		m_iRemoveMe = iRemoveMe;
	}

	virtual void Reset( void )
	{
		m_pNextMenu = NULL;
		m_iIsActive = false;
		m_flOpenTime = 0;
	}

	void SetNextMenu( CMenuPanel *pNextPanel )
	{
		if (m_pNextMenu)
			m_pNextMenu->SetNextMenu( pNextPanel );
		else
			m_pNextMenu = pNextPanel;
	}

	void SetMenuID( int iID )
	{
		m_iMenuID = iID;
	}

	void SetActive( int iState )
	{
		m_iIsActive = iState;
	}

	virtual void Open( void )
	{
		setVisible( true );

		// Note the open time, so we can delay input for a bit
		m_flOpenTime = gHUD.m_flTime;
	}

	virtual void Close( void )
	{
		setVisible( false );
		m_iIsActive = false;

		if ( m_iRemoveMe )
			gViewPort->removeChild( this );

		// This MenuPanel has now been deleted. Don't append code here.
	}

	int			ShouldBeRemoved() { return m_iRemoveMe; };
	CMenuPanel* GetNextMenu() { return m_pNextMenu; };
	int			GetMenuID() { return m_iMenuID; };
	int			IsActive() { return m_iIsActive; };
	float		GetOpenTime() { return m_flOpenTime; };

	// Numeric input
	virtual bool SlotInput( int iSlot ) { return false; };
	virtual void SetActiveInfo( int iInput ) {};
};

//================================================================
// Custom drawn scroll bars
class CTFScrollButton : public CommandButton
{
private:
	BitmapTGA	*m_pTGA;

public:
	CTFScrollButton(int iArrow, const char* text,int x,int y,int wide,int tall);

	virtual void paint( void );
	virtual void paintBackground( void );
};

// Custom drawn slider bar
class CTFSlider : public Slider
{
public:
	CTFSlider(int x,int y,int wide,int tall,bool vertical) : Slider(x,y,wide,tall,vertical)
	{
	};

	virtual void paintBackground( void );
};

// Custom drawn scrollpanel
class CTFScrollPanel : public ScrollPanel
{
public:
	CTFScrollPanel(int x,int y,int wide,int tall);
};

// SaRcaZm - V5 - Start
class CImageButton;
class CManuImageButton;
class CClassImageButton;
class C3DModelPanel;
class CAviImage;

// SaRcaZm - V7 - Start
class CRallySlider;
class CImageCheckButton;
// SaRcaZm - V7 - End
// SaRcaZm - V8 - Start
class CCustResImageLabel;
class CProgressBar;
// SaRcaZm - V8 - End

#define MAX_CUST_BUTTONS	5
#define MAX_CUST_CHOICES	4

//================================================================
// Menu Panels that take key input
//============================================================
class CClassMenuPanel : public CMenuPanel
{
private:
	ScrollPanel			*m_pScrollPanel;
	int					m_iCurrentInfo;
	CManuImageButton	*m_pCustButtons[MAX_CUST_BUTTONS];
	CClassImageButton	*m_pChoiceButtons[MAX_CUST_CHOICES + 1];
	CTransparentPanel	*m_pPartWindow;
	char				m_szModelName[80];
	int					m_iAnimNum;

	// SaRcaZm - V8 - Start
	CImageLabel			*m_pHLRLogo, *m_pTitle;
	Label				*m_pButtonLabel1, *m_pButtonLabel2;
	Label				*m_pMiscLabel, *m_pGearLabel, *m_pManualLabel;
	CTransparentPanel	*m_pMiscPanel;
	CCustResImageLabel	*m_pWndImage;
	// SaRcaZm - V8 - End
	// SaRcaZm - V6 - Start
	CTransparentPanel	*m_pPartsPanel;
	Label				*m_pMoneyLabel;
	// SaRcaZm - V6 - End
	// SaRcaZm - V7 - Start
	int					m_iCustomAnim, m_iCurrentIndex;
	CRallySlider		*m_pGearSlider;
	CImageCheckButton	*m_pManualCheck;

public:
	virtual void Close (void);
	// SaRcaZm - V7 - End

	CClassMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall);
	~CClassMenuPanel (void);
	virtual void paintBackground (void);
	virtual bool SlotInput( int iSlot );
	virtual void Open( void );
	virtual void Update( void );
	virtual void SetActiveInfo( int iInput );
	virtual void Initialize( void );

	virtual void Reset( void )
	{
		CMenuPanel::Reset();
		m_iCurrentInfo = 0;
	}
};

#define NUM_CLASS_BUTTONS		3	// Doesn't include the cancel button

class CTeamMenuPanel : public CMenuPanel
{
public:
	CTransparentPanel	*m_pTeamWindow;
	CManuImageButton	*m_pButtons[MAX_BUTTONS+1];
	C3DModelPanel		*m_pModelWindow;
	CClassImageButton	*m_pClassButtons[NUM_CLASS_BUTTONS + 1];
	int					m_iCurrentInfo;

	// SaRcaZm - V6 - Start
	Label				*m_pMoneyLabel;
	// SaRcaZm - V6 - End
	// SaRcaZm - V7 - Start
	int					m_iCurrentIndex;
	// SaRcaZm - V7 - End
	// SaRcaZm - V8 - Start
	Label				*m_pModelInfo;
	CImageLabel			*m_pHLRLogo, *m_pTitle;
	Label				*m_pButtonLabel1, *m_pButtonLabel2;
	CImageLabel			*m_pManuImage[MAX_TEAMS + 1];

	Label				*m_pNameLabel, *m_pTransmissionLabel, *m_pHPLabel;
	Label				*m_pTorqueLabel, *m_pGearBoxLabel, *m_pWeightLabel;
	Label				*m_pAccelLabel, *m_pHandlingLabel, *m_pTopSpeedLabel;

	Label				*m_pTransmissionValue, *m_pHPValue, *m_pTorqueValue;
	Label				*m_pGearBoxValue, *m_pWeightValue;
	CProgressBar		*m_pAccelBar, *m_pHandlingBar, *m_pTopSpeedBar;
	// SaRcaZm - V8 - End

public:
	CTeamMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall);
	// SaRcaZm - V8 - Start
	~CTeamMenuPanel (void);
	// SaRcaZm - V8 - End

	virtual bool SlotInput( int iSlot );
	virtual void Open( void );
	virtual void Update( void );
	virtual void SetActiveInfo( int iInput );
	virtual void paintBackground( void );

	virtual void Initialize( void );

	virtual void Reset( void )
	{
		CMenuPanel::Reset();
		m_iCurrentInfo = 0;
	}

private:
	void DefaultStats (void);
};

#define NUM_MAINMENU_BUTTONS	4
class CMessageWindowPanel : public CMenuPanel
{
public:
	CImageButton		*m_pButtons[NUM_MAINMENU_BUTTONS];
	int					m_iCurrentInfo;

public:
	CMessageWindowPanel( const char *szMOTD, int iRemoveMe, int x, int y, int wide, int tall );
	~CMessageWindowPanel (void);

	virtual bool SlotInput( int iSlot );
	virtual void SetActiveInfo( int iInput );
	virtual void paintBackground( void );
	virtual void Initialize (void);
	virtual void Update (char *szMOTD);
	virtual void Open (void);

private:
	CTransparentPanel *m_pBackgroundPanel;
	CAviImage *m_pVideo;
	TextPanel *m_pMOTD;
	ScrollPanel *m_pScrollPanel;

	CTransparentPanel	*m_pPartsPanel;
	Label				*m_pMoneyLabel;
	TextPanel			*m_pMapInfoPanel;
	CProgressBar		*m_pAccelBar, *m_pHandlingBar, *m_pTopSpeedBar;
	CImageLabel			*m_pCarPart[4];
	Label				*m_pAccelLabel, *m_pHandlingLabel, *m_pTopSpeedLabel;

	char m_szMOTD[2056];
};
// SaRcaZm - V5 - End

// SaRcaZm - V8 - Start
class CSkinnedComponent;
class CSkinnedButton;
class CSkinnedToggleButton;
class CSkinnedSlider;
class CSkinnedMultiBGSlider;
class CSkinnedTime;
class CSkinnedString;
class CMp3Playlist;
class CSkinnedPlaylist;

class CMP3MenuPanel : public CMenuPanel
{
public:
	CMP3MenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall);
	~CMP3MenuPanel (void);

	virtual bool SlotInput( int iSlot );
	virtual void Open( void );
	virtual void Update( void );
	virtual void SetActiveInfo( int iInput );
	virtual void paintBackground( void );
	virtual void paint( void );
	virtual void Initialize( void );
	virtual void frame (double time);

	void Command (char *szCommand);

	void ReloadSkin( void );
	void LoadPlaylist (void);

private:
	void setTime (int time_Minutes, int time_Seconds);
	void setTime (float time);
	void UnloadSkin( void );
	void CreateSkin( void );

	CSkinnedComponent		*m_pMainWindow;
	CSkinnedComponent		*m_pTitleBar;
	CSkinnedButton			*m_pButtons[5];
	CSkinnedToggleButton	*m_pShuffle;
	CSkinnedToggleButton	*m_pRepeat;
	CSkinnedButton			*m_pClose;
	CSkinnedComponent		*m_pEqButton;
	CSkinnedToggleButton	*m_pPlButton;
	CSkinnedButton			*m_pStereo;
	CSkinnedButton			*m_pMono;
	CSkinnedMultiBGSlider	*m_pVolume;
	CSkinnedMultiBGSlider	*m_pBalance;
	CSkinnedTime			*m_pTime;
	CSkinnedSlider			*m_pPosBar;
	CSkinnedString			*m_pSongTitle;
	CSkinnedString			*m_pKbps;
	CSkinnedString			*m_pKhz;
	CSkinnedPlaylist		*m_pPlaylistPanel;

	CMp3Playlist			*m_pPlaylist;

//	cvar_t					*rally_mp3skin;
	char					m_szSkin[30];
	char					m_szPlaylist[128];
	bool					m_bWasDraggingPosBar;
	bool					m_bStopPressed;
	float					m_fNextSong;
	bool					m_bUpdateNeeded;
};
// SaRcaZm - V8 - End

//=========================================================
// Specific Menus to handle old HUD sections
class CHealthPanel : public DragNDropPanel
{
private:
	BitmapTGA	*m_pHealthTGA;
	Label		*m_pHealthLabel;
public:
	CHealthPanel(int x,int y,int wide,int tall) : DragNDropPanel(x,y,wide,tall)
	{
		// Load the Health icon
		FileInputStream* fis = new FileInputStream( GetVGUITGAName("%d_hud_health"), false);
		m_pHealthTGA = new BitmapTGA(fis,true);
		fis->close();

		// Create the Health Label
		int iXSize,iYSize;
		m_pHealthTGA->getSize(iXSize,iYSize);
		m_pHealthLabel = new Label("",0,0,iXSize,iYSize);
		m_pHealthLabel->setImage(m_pHealthTGA);
		m_pHealthLabel->setParent(this);

		// Set panel dimension
		// Shouldn't be needed once Billy's fized setImage not recalculating the size
		//setSize( iXSize + 100, gHUD.m_iFontHeight + 10 );
		//m_pHealthLabel->setPos( 10, (getTall() - iYSize) / 2 );
	}

	virtual void paintBackground()
	{
	}

	void paint()
	{
		// Get the paint color
		int r,g,b,a;
		// Has health changed? Flash the health #
		if (gHUD.m_Health.m_fFade)
		{
			gHUD.m_Health.m_fFade -= (gHUD.m_flTimeDelta * 20);
			if (gHUD.m_Health.m_fFade <= 0)
			{
				a = MIN_ALPHA;
				gHUD.m_Health.m_fFade = 0;
			}

			// Fade the health number back to dim
			a = MIN_ALPHA +  (gHUD.m_Health.m_fFade/FADE_TIME) * 128;
		}
		else
			a = MIN_ALPHA;

		gHUD.m_Health.GetPainColor( r, g, b );
		ScaleColors(r, g, b, a );

		// If health is getting low, make it bright red
		if (gHUD.m_Health.m_iHealth <= 15)
			a = 255;

		int iXSize,iYSize, iXPos, iYPos;
		m_pHealthTGA->getSize(iXSize,iYSize);
		m_pHealthTGA->getPos(iXPos, iYPos);

		// Paint the player's health
		int x = gHUD.DrawHudNumber( iXPos + iXSize + 5, iYPos + 5, DHN_3DIGITS | DHN_DRAWZERO, gHUD.m_Health.m_iHealth, r, g, b);

		// Draw the vertical line
		int HealthWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;
		x += HealthWidth / 2;
		FillRGBA(x, iYPos + 5, HealthWidth / 10, gHUD.m_iFontHeight, 255, 160, 0, a);
	}
};

#endif
