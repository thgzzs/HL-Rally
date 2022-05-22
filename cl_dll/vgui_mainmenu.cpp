//
// VGUI_MainMenu.cpp
//
// by SaRcaZm

#include <windows.h>
#include <vfw.h>

#include "vgui_int.h"
#include "VGUI_Font.h"
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"

#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"

// Need the custom classes from here
#include "rally_vgui.h"
#include "Rally_Effects.h"

// SaRcaZm - V6 - Start
#include "carinfo.h"
// SaRcaZm - V6 - End

// Definitions of positions
// SaRcaZm - V7 - Start
#define MAINMENU_TITLE_X				XRES(40)
#define MAINMENU_TITLE_Y				YRES(10)

#define MAINMENU_MAPINFO_X				XRES (550)
#define MAINMENU_MAPINFO_Y				YRES (100)
#define MAINMENU_MAPINFO_SIZE_X			XRES (80)
#define MAINMENU_MAPINFO_SIZE_Y			YRES (100)
// SaRcaZm - V7 - End

#define MAINMENU_HLRALLY_LOGO_X			XRES(230)
#define MAINMENU_HLRALLY_LOGO_Y			YRES(4)
#define MAINMENU_MAIN_LOGO_X			XRES(200)
#define MAINMENU_MAIN_LOGO_Y			YRES(85)
#define MAINMENU_TOPLEFT_BUTTON_X		XRES(40)
#define MAINMENU_TOPLEFT_BUTTON_Y		YRES(80)
#define MAINMENU_BUTTON_SIZE_X			XRES(100)
#define MAINMENU_BUTTON_SIZE_Y			YRES(20)
#define MAINMENU_BUTTON_SPACER_Y		YRES(6)
//#define MAINMENU_VIDEOBACK_X			XRES (240)
//#define MAINMENU_VIDEOBACK_Y			YRES (85)
#define MAINMENU_VIDEO_X				XRES (288)
#define MAINMENU_VIDEO_Y				YRES (85)

#define MOTD_TITLE_X					XRES(16)
#define MOTD_TITLE_Y					YRES(16)
#define MOTD_WINDOW_X					XRES(40)
#define MOTD_WINDOW_Y					YRES(240)

// SaRcaZm - V6 - Start
//#define MOTD_WINDOW_SIZE_X				XRES(562)
#define MOTD_WINDOW_SIZE_X				XRES(362)

#define PARTS_WINDOW_X					MOTD_WINDOW_X + MOTD_WINDOW_SIZE_X
#define PARTS_WINDOW_Y					MOTD_WINDOW_Y
#define PARTS_WINDOW_SIZE_X				XRES(200)//XRES(562) - PARTS_WINDOW_X
#define PARTS_WINDOW_SIZE_Y				MOTD_WINDOW_SIZE_Y
#define PARTS_TITLE_X					XRES(10)
#define PARTS_TITLE_Y					YRES(10)
#define PARTS_INFO_Y					MOTD_WINDOW_SIZE_Y - MOTD_WINDOW_SIZE_Y / 3
#define PARTS_LIST_Y					YRES(50)
#define PARTS_LIST_SIZE_X				XRES(20)
#define PARTS_LIST_SIZE_Y				PARTS_INFO_Y - PARTS_LIST_Y - XRES(10)

#define PARTS_LIST_GAP_X				XRES(35)

#define PARTS_CAR_X						PARTS_LIST_GAP_X
#define PARTS_CAR_Y						PARTS_INFO_Y + YRES(15)

#define MAINMENU_MONEY_X				XRES(550)
#define MAINMENU_MONEY_Y				YRES(80)
// SaRcaZm - V6 - End

#define MOTD_WINDOW_SIZE_Y				YRES(220)

typedef enum {
	CARSELECT,
	CUSTOMISE,
	SPECTATOR,
	RACE,
//	NUM_MAINMENU_BUTTONS
};

vec3_t	vUpgrades[8];
vec3_t	vStats;

extern rally_teaminfo_t rallyinfo[MAX_TEAMS];

//-----------------------------------------------------------------------------
// Purpose: Creates a new CMessageWindowPanel
// Output : CMenuPanel - interface to the panel
//-----------------------------------------------------------------------------
CMenuPanel *CMessageWindowPanel_Create( const char *szMOTD, const char *szTitle, int iShadeFullscreen, int iRemoveMe, int x, int y, int wide, int tall )
{
	return new CMessageWindowPanel( "", iRemoveMe, x, y, wide, tall );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CMessageWindowPanel::CMessageWindowPanel( const char *szMOTD, int iRemoveMe, int x, int y, int wide, int tall ) : CMenuPanel( 255, iRemoveMe, x, y, wide, tall )
{
	// Initialise the AVI library
	//AVIFileInit ();

	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hWindowText = pSchemes->getSchemeHandle( "Briefing Text" );
	SchemeHandle_t hInfoText = pSchemes->getSchemeHandle( "Team Info Text" );

	// get the Font used for the Titles
	Font *pTitleFont = pSchemes->getFont( hTitleScheme );
	int r, g, b, a;
	int iXSize,iYSize,iXPos,iYPos;

	// SaRcaZm - V7 - Start
	// Create the title
	CImageLabel *pTitle = new CImageLabel ("header_mainmenu", MAINMENU_TITLE_X, MAINMENU_TITLE_Y);
	pTitle->setParent (this);
	// SaRcaZm - V7 - End

	// The HLRally logo
	CImageLabel *pImage = new CImageLabel ("hlrally", MAINMENU_HLRALLY_LOGO_X, MAINMENU_HLRALLY_LOGO_Y);
	pImage->setParent (this);

	// The buttons
	// SaRcaZm - V7 - Start
	iYPos = MAINMENU_TOPLEFT_BUTTON_Y;
	m_pButtons[0] = new CImageButton("carselect", MAINMENU_TOPLEFT_BUTTON_X, iYPos, MAINMENU_BUTTON_SIZE_X, MAINMENU_BUTTON_SIZE_Y);
	m_pButtons[0]->setParent( this );
	m_pButtons[0]->addInputSignal(new CHandler_MenuButtonOver(this, 1));
	m_pButtons[0]->addActionSignal( new CMenuHandler_TextWindow (MENU_TEAM) );
	m_pButtons[0]->addActionSignal( new CMenuHandler_RallySound ("vgui/select.wav") );

	iYPos += MAINMENU_BUTTON_SIZE_Y + MAINMENU_BUTTON_SPACER_Y;
	m_pButtons[1] = new CImageButton("customise", MAINMENU_TOPLEFT_BUTTON_X, iYPos, MAINMENU_BUTTON_SIZE_X, MAINMENU_BUTTON_SIZE_Y);
	m_pButtons[1]->setParent( this );
	m_pButtons[1]->addInputSignal(new CHandler_MenuButtonOver(this, 2));
	m_pButtons[1]->addActionSignal( new CMenuHandler_TextWindow (MENU_CLASS) );
	m_pButtons[1]->addActionSignal( new CMenuHandler_RallySound ("vgui/select.wav") );

	iYPos += MAINMENU_BUTTON_SIZE_Y + MAINMENU_BUTTON_SPACER_Y;
	m_pButtons[2] = new CImageButton("spectator", MAINMENU_TOPLEFT_BUTTON_X, iYPos, MAINMENU_BUTTON_SIZE_X, MAINMENU_BUTTON_SIZE_Y);
	m_pButtons[2]->setParent( this );
	m_pButtons[2]->addInputSignal(new CHandler_MenuButtonOver(this, 3));
	iYPos += MAINMENU_BUTTON_SIZE_Y + MAINMENU_BUTTON_SPACER_Y;
	m_pButtons[2]->addActionSignal( new CMenuHandler_TextWindow (HIDE_TEXTWINDOW) );
	m_pButtons[2]->addActionSignal( new CMenuHandler_StringCommand( "spectate", true ) );
	m_pButtons[2]->addActionSignal( new CMenuHandler_RallySound ("vgui/select.wav") );

	iYPos += MAINMENU_BUTTON_SIZE_Y + MAINMENU_BUTTON_SPACER_Y;
	//m_pButtons[3] = new CImageButton("race", MAINMENU_TOPLEFT_BUTTON_X, iYPos, MAINMENU_BUTTON_SIZE_X, MAINMENU_BUTTON_SIZE_Y);
	m_pButtons[3] = new CRaceImageButton("race", MAINMENU_TOPLEFT_BUTTON_X, iYPos, MAINMENU_BUTTON_SIZE_X, MAINMENU_BUTTON_SIZE_Y);
	m_pButtons[3]->setParent( this );
	m_pButtons[3]->addInputSignal(new CHandler_MenuButtonOver(this, 4));
	m_pButtons[3]->addActionSignal( new CMenuHandler_TextWindow (HIDE_TEXTWINDOW) );
	m_pButtons[3]->addActionSignal( new CMenuHandler_RallySound ("vgui/select.wav") );
	// SaRcaZm - V7 - End

	// Create the MOTD panel
	// Create the window
	if (CRallyEffects::getSingleton()->m_pCvarPartslist->value)
		iXSize = MOTD_WINDOW_SIZE_X;
	else
		iXSize = MOTD_WINDOW_SIZE_X + PARTS_WINDOW_SIZE_X;

	m_pBackgroundPanel = new CTransparentPanel( 255, MOTD_WINDOW_X, MOTD_WINDOW_Y, iXSize, MOTD_WINDOW_SIZE_Y );
	m_pBackgroundPanel->setParent( this );
	// SaRcaZm - V7 - Start
	m_pBackgroundPanel->setBorder( new LineBorder( Color(128,128,128,0)) );
	// SaRcaZm - V7 - End
	m_pBackgroundPanel->setVisible( true );

	m_pBackgroundPanel->getPos( iXPos,iYPos );
	m_pBackgroundPanel->getSize( iXSize,iYSize );

	// Create the Scroll panel
	m_pScrollPanel = new CTFScrollPanel( iXPos + XRES(16), iYPos + YRES(16), iXSize - XRES(32), iYSize - YRES(16) );
	m_pScrollPanel->setParent(this);

	//force the scrollbars on so clientClip will take them in account after the validate
	m_pScrollPanel->setScrollBarAutoVisible(false, false);
	m_pScrollPanel->setScrollBarVisible(true, true);
	m_pScrollPanel->validate();

	//turn the scrollbars back into automode
	m_pScrollPanel->setScrollBarAutoVisible(true, true);
	m_pScrollPanel->setScrollBarVisible(false, false);

	m_pScrollPanel->validate();

	if (CRallyEffects::getSingleton()->m_pCvarPartslist->value)
	{
		// SaRcaZm - V6 - Start
		// Create the parts window
		// Create the window
		m_pPartsPanel = new CTransparentPanel( 255, PARTS_WINDOW_X, PARTS_WINDOW_Y, PARTS_WINDOW_SIZE_X, PARTS_WINDOW_SIZE_Y );
		m_pPartsPanel->setParent( this );
		//m_pPartsPanel->setBorder( new LineBorder( Color(255 * 0.7,170 * 0.7,0,0)) );
		m_pPartsPanel->setVisible( true );

		// The header
		CImageLabel *pPartsLabel = new CImageLabel ("header_partslist", PARTS_TITLE_X, PARTS_TITLE_Y);
		pPartsLabel->setParent (m_pPartsPanel);

		// Picture
		CImageLabel *pCar = new CImageLabel ("parts_car", PARTS_CAR_X, PARTS_CAR_Y);
		pCar->setParent (m_pPartsPanel);

		m_pCarPart[0] = new CImageLabel ("parts_tyres", PARTS_CAR_X, PARTS_CAR_Y);
		m_pCarPart[0]->setParent (m_pPartsPanel);
		m_pCarPart[0]->setVisible (false);

		m_pCarPart[1] = new CImageLabel ("parts_muffler", PARTS_CAR_X, PARTS_CAR_Y);
		m_pCarPart[1]->setParent (m_pPartsPanel);
		m_pCarPart[1]->setVisible (false);

		m_pCarPart[2] = new CImageLabel ("parts_intercooler", PARTS_CAR_X, PARTS_CAR_Y);
		m_pCarPart[2]->setParent (m_pPartsPanel);
		m_pCarPart[2]->setVisible (false);

		m_pCarPart[3] = new CImageLabel ("parts_turbo", PARTS_CAR_X, PARTS_CAR_Y);
		m_pCarPart[3]->setParent (m_pPartsPanel);
		m_pCarPart[3]->setVisible (false);

		// Acceleration Bar
		iXPos = PARTS_LIST_GAP_X;
		m_pAccelBar = new CProgressBar (iXPos, PARTS_LIST_Y, PARTS_LIST_SIZE_X, PARTS_LIST_SIZE_Y, 100, 0, true);
		m_pAccelBar->setParent (m_pPartsPanel);
		m_pAccelBar->setVisible (true);
		iXPos += PARTS_LIST_SIZE_X + PARTS_LIST_GAP_X;

		// Handling Bar
		m_pHandlingBar = new CProgressBar (iXPos, PARTS_LIST_Y, PARTS_LIST_SIZE_X, PARTS_LIST_SIZE_Y, 100, 0, true);
		m_pHandlingBar->setParent (m_pPartsPanel);
		m_pHandlingBar->setVisible (true);
		iXPos += PARTS_LIST_SIZE_X + PARTS_LIST_GAP_X;

		// Top speed Bar
		m_pTopSpeedBar = new CProgressBar (iXPos, PARTS_LIST_Y, PARTS_LIST_SIZE_X, PARTS_LIST_SIZE_Y, 100, 0, true);
		m_pTopSpeedBar->setParent (m_pPartsPanel);
		m_pTopSpeedBar->setVisible (true);

		// Acceleration Label
		iXPos = PARTS_LIST_GAP_X - XRES(10);
		m_pAccelLabel = new Label( "Accel", iXPos, PARTS_INFO_Y);
		m_pAccelLabel->setFont( pSchemes->getFont(hWindowText) ); 
		m_pAccelLabel->setParent( m_pPartsPanel );
		pSchemes->getFgColor( hWindowText, r, g, b, a );
		m_pAccelLabel->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hWindowText, r, g, b, a );
		m_pAccelLabel->setBgColor( r, g, b, a );
		m_pAccelLabel->setContentAlignment( vgui::Label::a_center );
		iXPos += PARTS_LIST_SIZE_X + PARTS_LIST_GAP_X;

		// Handling Label
		m_pHandlingLabel = new Label( "Handling", iXPos, PARTS_INFO_Y);
		m_pHandlingLabel->setFont( pSchemes->getFont(hWindowText) ); 
		m_pHandlingLabel->setParent( m_pPartsPanel );
		pSchemes->getFgColor( hWindowText, r, g, b, a );
		m_pHandlingLabel->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hWindowText, r, g, b, a );
		m_pHandlingLabel->setBgColor( r, g, b, a );
		m_pHandlingLabel->setContentAlignment( vgui::Label::a_center );
		iXPos += PARTS_LIST_SIZE_X + PARTS_LIST_GAP_X;

		// Top speed Label
		m_pTopSpeedLabel = new Label( "TopSpeed", iXPos, PARTS_INFO_Y);
		m_pTopSpeedLabel->setFont( pSchemes->getFont(hWindowText) ); 
		m_pTopSpeedLabel->setParent( m_pPartsPanel );
		pSchemes->getFgColor( hWindowText, r, g, b, a );
		m_pTopSpeedLabel->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hWindowText, r, g, b, a );
		m_pTopSpeedLabel->setBgColor( r, g, b, a );
		m_pTopSpeedLabel->setContentAlignment( vgui::Label::a_center );
	}
	else
	{
		m_pPartsPanel = NULL;
		m_pAccelBar = NULL;
		m_pHandlingBar = NULL;
		m_pTopSpeedBar = NULL;
		m_pAccelLabel = NULL;
		m_pHandlingLabel = NULL;
		m_pTopSpeedLabel = NULL;
	}

	// Money label
	Font *pWindowFont = pSchemes->getFont( hWindowText );
	m_pMoneyLabel = new Label ( "", MAINMENU_MONEY_X, MAINMENU_MONEY_Y );
	m_pMoneyLabel->setParent( this );
	m_pMoneyLabel->setFont( pWindowFont );
	m_pMoneyLabel->setFgColor( MONEY_R, MONEY_G, MONEY_B, 0 );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pMoneyLabel->setBgColor( r, g, b, a );
	m_pMoneyLabel->setContentAlignment( vgui::Label::a_west );

	// The map info window
	m_pMapInfoPanel = new TextPanel( "", MAINMENU_MAPINFO_X, MAINMENU_MAPINFO_Y, MAINMENU_MAPINFO_SIZE_X, MAINMENU_MAPINFO_SIZE_Y );
	m_pMapInfoPanel->setParent( this );
	//m_pMapInfoPanel->setBorder( new LineBorder( Color(255 * 0.7,170 * 0.7,0,0)) );

	// get the font and colors from the scheme
	m_pMapInfoPanel->setFont( pSchemes->getFont(hWindowText) );
	pSchemes->getFgColor( hWindowText, r, g, b, a );
	m_pMapInfoPanel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hWindowText, r, g, b, a );
	m_pMapInfoPanel->setBgColor( r, g, b, a );

	CRallyVGUIFX::getSingleton()->SetMode (MODE_VGUI);

	// The HLRally logo
	CImageLabel *pMainImage = new CImageLabel ("biglogo", MAINMENU_MAIN_LOGO_X, MAINMENU_MAIN_LOGO_Y);
	pMainImage->setParent (this);

	// Now, add the AVI video playing above the MOTD
	// First the background.....
//	CImageLabel *pVideoBack = new CImageLabel ("videoback", MAINMENU_VIDEOBACK_X, MAINMENU_VIDEOBACK_Y);
//	pVideoBack->setParent (this);

//	m_pVideo = new CAviImage ("vgui", MAINMENU_VIDEO_X, MAINMENU_VIDEO_Y, 160, 120);
//	m_pVideo->setParent (this);
	m_pVideo = NULL;

	Initialize ();

	// Misc variables
	for (int i = 0; i < 8; i++)
		vUpgrades[i] = Vector (0, 0, 0);
	vStats = Vector (0, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor.
//-----------------------------------------------------------------------------
CMessageWindowPanel::~CMessageWindowPanel (void)
{
	//AVIFileExit ();
	if (m_pBackgroundPanel)
		delete m_pBackgroundPanel;
	if (m_pVideo)
		delete m_pVideo;
	if (m_pMOTD)
	{
		m_pMOTD->setText("");
		m_pMOTD->setVisible(false);
		delete m_pMOTD;
	}
	if (m_pScrollPanel)
		delete m_pScrollPanel;
	if (m_pPartsPanel)
		delete m_pPartsPanel;
	if (m_pMoneyLabel)
		delete m_pMoneyLabel;
	if (m_pMapInfoPanel)
		delete m_pMapInfoPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the window (called at level change).
//-----------------------------------------------------------------------------
void CMessageWindowPanel::Initialize (void)
{
	m_pMOTD = NULL;
	memset (m_szMOTD, 0, sizeof (m_szMOTD));
	SetActiveInfo (0);
}

//-----------------------------------------------------------------------------
// Purpose: Called when the information should be updated.
//-----------------------------------------------------------------------------
void CMessageWindowPanel::Update (char *szMOTD)
{
//	m_pMOTD->setText(szMOTD);
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hWindowText = pSchemes->getSchemeHandle( "Briefing Text" );
	int r, g, b, a;

	// Create the text panel
	if (!strcmp (szMOTD, m_szMOTD))
		return;

	strcpy (m_szMOTD, szMOTD);
	if (m_pMOTD)
	{
//		m_pScrollPanel->removeAllChildren ();
//		m_pScrollPanel->repaintAll ();
		m_pMOTD->setText("");
		m_pMOTD->setVisible(false);
		delete m_pMOTD;
	}
	m_pMOTD = new TextPanel("", 0,0, 64,64);
	m_pMOTD->setParent( m_pScrollPanel->getClient() );

	// get the font and colors from the scheme
	m_pMOTD->setFont( pSchemes->getFont(hWindowText) );
	pSchemes->getFgColor( hWindowText, r, g, b, a );
	m_pMOTD->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hWindowText, r, g, b, a );
	m_pMOTD->setBgColor( r, g, b, a );
	m_pMOTD->setText(szMOTD);


	// Get the total size of the MOTD text and resize the text panel
	int iScrollSizeX, iScrollSizeY;

	// First, set the size so that the client's wdith is correct at least because the
	//  width is critical for getting the "wrapped" size right.
	// You'll see a horizontal scroll bar if there is a single word that won't wrap in the
	//  specified width.
	m_pMOTD->getTextImage()->setSize(m_pScrollPanel->getClientClip()->getWide(), m_pScrollPanel->getClientClip()->getTall());
	m_pMOTD->getTextImage()->getTextSizeWrapped( iScrollSizeX, iScrollSizeY );

	// Now resize the textpanel to fit the scrolled size
	m_pMOTD->setSize( iScrollSizeX , iScrollSizeY );
	m_pScrollPanel->validate();

	
	// SaRcaZm - V7 - Start
	// Load the map info file...
	char *file = NULL;
	char filename[80];
	strcpy (filename, gEngfuncs.pfnGetLevelName ());
	strcpy (&filename[strlen (filename) - 4], "_mapinfo.txt");

	file = (char *) gEngfuncs.COM_LoadFile (filename, 5, NULL);
	if (file)
	{
		m_pMapInfoPanel->setText (file);
		gEngfuncs.COM_FreeFile (file);
	}
	else	// No custom one, so just use the default
	{
		file = (char *) gEngfuncs.COM_LoadFile ("maps/default_mapinfo.txt", 5, NULL);
		if (file)
		{
			m_pMapInfoPanel->setText (file);
			gEngfuncs.COM_FreeFile (file);
		}
	}
	// SaRcaZm - V7 - End
}

// SaRcaZm - V6 - Start

//-----------------------------------------------------------------------------
// Purpose: Called when the window is about to be opened.
//-----------------------------------------------------------------------------
void RemoveQuotes (char *sz);
void CMessageWindowPanel::Open (void)
{
	// Set the fx mode
	CRallyVGUIFX::getSingleton()->SetMode (MODE_VGUI);

	// Update the spectator menu so it doesn't appear
	gViewPort->UpdateSpectatorMenu ();

	// Race and Customise shouldn't be visible if they haven't selected a car yet
	if (gViewPort->SearchCommands ("changecar"))
	{
		m_pButtons[CUSTOMISE]->setVisible (true);
		m_pButtons[RACE]->setVisible (true);
	}
	else
	{
		m_pButtons[CUSTOMISE]->setVisible (false);
		m_pButtons[RACE]->setVisible (false);
	}

	// The Money Label
	char sz[80];
	sprintf (sz, "%s: $%i", gHUD.m_TextMessage.BufferedLocaliseTextString("#Label_Money"), money);
	m_pMoneyLabel->setText(sz);

	if (CRallyEffects::getSingleton()->m_pCvarPartslist->value)
	{
		if (m_pPartsPanel == NULL || m_pAccelBar == NULL || m_pHandlingBar == NULL || m_pTopSpeedBar == NULL ||
			m_pAccelLabel == NULL || m_pHandlingLabel == NULL || m_pTopSpeedLabel == NULL)
		{
			CMenuPanel::Open ();
			return;
		}

		for (int i = 0; i < 4; i++)
		{
			if (vUpgrades[i][0] || vUpgrades[i][1] || vUpgrades[i][2])
				m_pCarPart[i]->setVisible (true);
			else
				m_pCarPart[i]->setVisible (false);
		}

		// Update the stats
		char szModel[20];
		memset (szModel, 0, 20);
		gViewPort->GetCommandParam ("changecar", szModel);
		RemoveQuotes (szModel);

		// Search through the stats till we find the right one
		if (szModel[0])
		{
			for (int i = 0; i < 13; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					if(!stricmp(szModel, rallyinfo[i].szModels[j].szModelName))
					{
						m_pAccelBar->setProgress (rallyinfo[i].szModels[j].iStats[0]);
						m_pHandlingBar->setProgress (rallyinfo[i].szModels[j].iStats[1]);
						m_pTopSpeedBar->setProgress (rallyinfo[i].szModels[j].iStats[2]);

						vStats[0] = rallyinfo[i].szModels[j].iStats[0];
						vStats[1] = rallyinfo[i].szModels[j].iStats[1];
						vStats[2] = rallyinfo[i].szModels[j].iStats[2];
					}
				}
			}

			vec3_t tmpupd = vStats;
			for (i = 0; i < 8; i++)
				tmpupd = tmpupd + vUpgrades[i];

			m_pAccelBar->setExtraProgress (tmpupd[0]);
			m_pHandlingBar->setExtraProgress (tmpupd[1]);
			m_pTopSpeedBar->setExtraProgress (tmpupd[2]);
		}
	}

	CMenuPanel::Open ();
}
// SaRcaZm - V6 - End

//-----------------------------------------------------------------------------
// Purpose: Handle key inputs.
//-----------------------------------------------------------------------------
bool CMessageWindowPanel::SlotInput (int iSlot)
{
	if ((iSlot < NUM_MAINMENU_BUTTONS) && m_pButtons[iSlot] && m_pButtons[iSlot]->isVisible ())
	{
		m_pButtons[iSlot]->fireActionSignal();
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: To paint the background duh!
//-----------------------------------------------------------------------------
void CMessageWindowPanel::paintBackground (void)
{
	// We don't wanna paint the background
}

//-----------------------------------------------------------------------------
// Purpose: Set the information when the user moves the mouse over a button.
//-----------------------------------------------------------------------------
void CMessageWindowPanel::SetActiveInfo (int iInput)
{
	// Piece of shit AVI code chugs and the VGUI drops to 4fps
	// UPDATE: Now it seems to run ok, but it only displays the first 2 seconds of video
//	char szVideoNames[NUM_MAINMENU_BUTTONS+1][20] = { "hlrvideo", "carselectvideo", "customisevideo", "spectatorvideo", "racevideo" };
//	m_pVideo = new CAviImage (szVideoNames[iInput], MAINMENU_VIDEO_X, MAINMENU_VIDEO_Y);
//	m_pVideo->setParent (this);
}
