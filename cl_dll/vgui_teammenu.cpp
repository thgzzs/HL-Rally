//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: TFC Team Menu
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// This File has been heavilly hacked by FragMented!
//
// Now it has been even more heavily hacked by SaRcaZm (Frag's code is gone)
//
// $NoKeywords: $
//=============================================================================

#include "vgui_int.h"
#include "VGUI_Font.h"
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"

#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"

// Need the custom classes from here
#include "rally_vgui.h"

// For the spinning model
#include "rally_vguifx.h"
#include "carinfo.h"

extern rally_teaminfo_t rallyinfo[MAX_TEAMS];

// Changed some of these
// Changed some of these to fit the new VGUI
// Team Menu Dimensions
// SaRcaZm - V7 - Start
#define TEAMMENU_TITLE_X				XRES(40)
#define TEAMMENU_TITLE_Y				YRES(10)
// SaRcaZm - V7 - End

#define TEAMMENU_TITLE_BUTTON_Y			YRES(80)	// This one is new  
#define TEAMMENU_TOPLEFT_BUTTON_X		XRES(20)	// was 40
#define TEAMMENU_TOPLEFT_BUTTON2_X		XRES(124)	// This one is new
#define TEAMMENU_TOPLEFT_BUTTON_Y		YRES(110)	// was 80
#define TEAMMENU_BUTTON_SIZE_X			XRES(100)	// was 124
#define TEAMMENU_BUTTON_SIZE_Y			YRES(20)	// was 24
#define TEAMMENU_BUTTON_SPACER_Y		YRES(6)		// was 8
#define TEAMMENU_PROGRESS_SIZE_X		XRES(100)	// This one is new
#define TEAMMENU_PROGRESS_SIZE_Y		YRES(15)	// This one is new
#define TEAMMENU_WINDOW_X				XRES(230)	// was 176
#define TEAMMENU_WINDOW_Y				YRES(80)	// was 80
#define TEAMMENU_WINDOW2_X				XRES(124)	// This one is new
#define TEAMMENU_WINDOW2_Y				YRES(256)	// This one is new
#define TEAMMENU_WINDOW_SIZE_X			XRES(374)	// was 424
#define TEAMMENU_WINDOW_SIZE_Y			YRES(176)	// was 312
#define TEAMMENU_WINDOW2_SIZE_X			YRES(480)	// This one is new
#define TEAMMENU_WINDOW2_SIZE_Y			YRES(216)	// This one is new
#define TEAMMENU_WINDOW_TITLE_X			XRES(16)	// was 16
#define TEAMMENU_WINDOW_TITLE_Y			YRES(16)	// was 16
#define TEAMMENU_WINDOW_TEXT_X			XRES(16)	// was 16
#define TEAMMENU_WINDOW_TEXT_Y			YRES(16)	// was 48
#define TEAMMENU_WINDOW_TEXT_ROTATE_Y	XRES(196)	// This one is new
#define TEAMMENU_WINDOW_TEXT_SIZE_Y		YRES(200)	// was 178
#define TEAMMENU_WINDOW_TEXT_INFO_Y		YRES(46)	// was 48
#define TEAMMENU_WINDOW_TEXT2_X			XRES(100)	// This one is new
#define TEAMMENU_WINDOW_TEXT3_X			XRES(176)	// This one is new
#define TEAMMENU_WINDOW_TEXT4_X			XRES(260)	// This one is new
#define TEAMMENU_WINDOW_TEXT_SPACE_Y	YRES(20)	// This one is new
#define TEAMMENU_WINDOW_INFO_X			XRES(16)	// was 16
#define TEAMMENU_WINDOW_INFO_Y			YRES(234)	// was 234
#define TEAMMENU_HLRALLY_LOGO_X			XRES(230)	// This one is new
#define TEAMMENU_HLRALLY_LOGO_Y			YRES(4)		// This one is new
#define TEAMMENU_MANU_LOGO_X			XRES(280)	// This one is new
#define TEAMMENU_MANU_LOGO_Y			YRES(10)	// This one is new

// SaRcaZm - V6 - Start
#define TEAMMENU_MONEY_X				XRES(550)
#define TEAMMENU_MONEY_Y				YRES(80)
// SaRcaZm - V6 - End


// Creation
CTeamMenuPanel::CTeamMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	// SaRcaZm - V8 - Start
	// Moved carinfo.txt stuff to CTeamMenuPanel::Open
	int i;

	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hTeamWindowText = pSchemes->getSchemeHandle( "Briefing Text" );
	SchemeHandle_t hTeamInfoText = pSchemes->getSchemeHandle( "Team Info Text" );

	// get the Font used for the Titles
	Font *pTitleFont = pSchemes->getFont( hTitleScheme );
	int r, g, b, a;

	// Create the title
	m_pTitle = new CImageLabel ("header_carmenu", TEAMMENU_TITLE_X, TEAMMENU_TITLE_Y);
	m_pTitle->setParent (this);

	// Create the info window
	m_pTeamWindow  = new CTransparentPanel( 255, TEAMMENU_WINDOW_X, TEAMMENU_WINDOW_Y, TEAMMENU_WINDOW_SIZE_X, TEAMMENU_WINDOW_SIZE_Y );
	m_pTeamWindow->setParent( this );

	// The manufacturer button label
	m_pButtonLabel1 = new Label ( "", TEAMMENU_TOPLEFT_BUTTON_X, TEAMMENU_TITLE_BUTTON_Y);
	m_pButtonLabel1->setParent( this );
	m_pButtonLabel1->setFont( pTitleFont );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pButtonLabel1->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pButtonLabel1->setBgColor( r, g, b, a );
	m_pButtonLabel1->setContentAlignment( vgui::Label::a_west );
	m_pButtonLabel1->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Title_Manufacturer"));

	// The model button label
	m_pButtonLabel2 = new Label ( "", TEAMMENU_TOPLEFT_BUTTON2_X, TEAMMENU_TITLE_BUTTON_Y);
	m_pButtonLabel2->setParent( this );
	m_pButtonLabel2->setFont( pTitleFont );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pButtonLabel2->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pButtonLabel2->setBgColor( r, g, b, a );
	m_pButtonLabel2->setContentAlignment( vgui::Label::a_west );
	m_pButtonLabel2->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Title_Model"));

	// The HLRally logo
	m_pHLRLogo = new CImageLabel ("hlrally", TEAMMENU_HLRALLY_LOGO_X, TEAMMENU_HLRALLY_LOGO_Y);
	m_pHLRLogo->setParent (this);

	// Create the Info Window
	m_pModelWindow  = new C3DModelPanel( 255, TEAMMENU_WINDOW2_X, TEAMMENU_WINDOW2_Y, TEAMMENU_WINDOW2_SIZE_X, TEAMMENU_WINDOW2_SIZE_Y );
	m_pModelWindow->setParent( this );

	// The info label
	m_pModelInfo = new Label( "", TEAMMENU_WINDOW_TEXT_X, TEAMMENU_WINDOW_TEXT_ROTATE_Y );
	m_pModelInfo->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pModelInfo->setParent( m_pModelWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pModelInfo->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pModelInfo->setBgColor( r, g, b, a );
	m_pModelInfo->setContentAlignment( vgui::Label::a_center );
	m_pModelInfo->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Title_3DModel_Rotate"));

	int iYPos = TEAMMENU_TOPLEFT_BUTTON_Y;
	// Add the class buttons
	for (i = 0; i < NUM_CLASS_BUTTONS; i++)
	{
		m_pClassButtons[i] = new CClassImageButton( "", TEAMMENU_TOPLEFT_BUTTON2_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y);
		m_pClassButtons[i]->setParent( this );
		m_pClassButtons[i]->setContentAlignment( vgui::Label::a_west );
		m_pClassButtons[i]->setVisible( false );
		m_pClassButtons[i]->addInputSignal( new CHandler_MenuButtonOver(this, i+MAX_TEAMS+1) );

		iYPos += TEAMMENU_BUTTON_SIZE_Y + TEAMMENU_BUTTON_SPACER_Y;
	}

	// Add the cancel button for the classes
	m_pClassButtons[NUM_CLASS_BUTTONS] = new CClassImageButton( CHudTextMessage::BufferedLocaliseTextString( "#Menu_Cancel" ), TEAMMENU_TOPLEFT_BUTTON2_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y);
	m_pClassButtons[NUM_CLASS_BUTTONS]->setParent( this );
	m_pClassButtons[NUM_CLASS_BUTTONS]->addActionSignal( new CMenuHandler_TextWindow(HIDE_TEXTWINDOW) );

	Font *pWindowFont = pSchemes->getFont( hTeamWindowText );
	m_pMoneyLabel = new Label ( "", TEAMMENU_MONEY_X, TEAMMENU_MONEY_Y );
	m_pMoneyLabel->setParent( this );
	m_pMoneyLabel->setFont( pWindowFont );
	m_pMoneyLabel->setFgColor( MONEY_R, MONEY_G, MONEY_B, 0 );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pMoneyLabel->setBgColor( r, g, b, a );
	m_pMoneyLabel->setContentAlignment( vgui::Label::a_west );

	Initialize();

	for (i = 0; i < MAX_BUTTONS + 1; i++)
		m_pButtons[i] = NULL;
	for (i = 1; i < MAX_TEAM_BUTTONS; i++)
		m_pManuImage[i] = NULL;

	DefaultStats ();
	// SaRcaZm - V8 - End
}

// SaRcaZm - V8 - Start
CTeamMenuPanel::~CTeamMenuPanel (void)
{
	// Clear the current statistics from the window
	m_pTeamWindow->removeAllChildren();
	m_pTeamWindow->repaintAll();

	for (int i = 0; i <= NUM_CLASS_BUTTONS; i++)
		if (m_pClassButtons[i])
			delete m_pClassButtons[i];
	for (i = 1; i <= MAX_TEAM_BUTTONS; i++)
		if (m_pButtons[i])
			delete m_pButtons[i];
	for (i = 1; i < MAX_TEAM_BUTTONS; i++)
		if (m_pManuImage[i])
			delete m_pManuImage[i];

	if (m_pModelWindow)
		delete m_pModelWindow;
	if (m_pModelInfo)
		delete m_pModelInfo;
	if (m_pTeamWindow)
		delete m_pTeamWindow;
	if (m_pMoneyLabel)
		delete m_pMoneyLabel;
	if (m_pButtonLabel1)
		delete m_pButtonLabel1;
	if (m_pButtonLabel2)
		delete m_pButtonLabel2;
	if (m_pHLRLogo)
		delete m_pHLRLogo;
	if (m_pTitle)
		delete m_pTitle;

	if (m_pNameLabel)
		delete m_pNameLabel;
	if (m_pTransmissionLabel)
		delete m_pTransmissionLabel;
	if (m_pHPLabel)
		delete m_pHPLabel;
	if (m_pTorqueLabel)
		delete m_pTorqueLabel;
	if (m_pGearBoxLabel)
		delete m_pGearBoxLabel;
	if (m_pWeightLabel)
		delete m_pWeightLabel;
	if (m_pAccelLabel)
		delete m_pAccelLabel;
	if (m_pHandlingLabel)
		delete m_pHandlingLabel;
	if (m_pTopSpeedLabel)
		delete m_pTopSpeedLabel;
	if (m_pTransmissionValue)
		delete m_pTransmissionValue;
	if (m_pHPValue)
		delete m_pHPValue;
	if (m_pTorqueValue)
		delete m_pTorqueValue;
	if (m_pGearBoxValue)
		delete m_pGearBoxValue;
	if (m_pWeightValue)
		delete m_pWeightValue;
	if (m_pAccelBar)
		delete m_pAccelBar;
	if (m_pHandlingBar)
		delete m_pHandlingBar;
	if (m_pTopSpeedBar)
		delete m_pTopSpeedBar;
}
// SaRcaZm - V8 - End

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CTeamMenuPanel::Initialize( void )
{
	m_iCurrentInfo = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Called everytime the Team Menu is displayed
//-----------------------------------------------------------------------------
void CTeamMenuPanel::Update( void )
{
	int	 iYPos = TEAMMENU_TOPLEFT_BUTTON_Y;

	// Set the team buttons
	for (int i = 1; i <= MAX_TEAM_BUTTONS; i++)
	{
		// Create the button here because we don't have the team names in the initialisation
		// SaRcaZm - V8 - Start
		// If the button isn't created yet, create it now
		if (m_pButtons[i] == NULL)
		{
			m_pButtons[i] = new CManuImageButton( gViewPort->GetTeamName(i), TEAMMENU_TOPLEFT_BUTTON_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y);
			m_pButtons[i]->setParent( this );
		}
		// SaRcaZm - V8 - End

		if (m_pButtons[i])
		{
			if ( i <= gViewPort->GetNumberOfTeams() )
			{
				m_pButtons[i]->addButtonClickSignal (this, i);
				m_pButtons[i]->setBoundKey( i );

				m_pButtons[i]->setVisible( true );
				m_pButtons[i]->setPos( TEAMMENU_TOPLEFT_BUTTON_X, iYPos );
				iYPos += TEAMMENU_BUTTON_SIZE_Y + TEAMMENU_BUTTON_SPACER_Y;
				// If this is the spectate button, add an action handler
				if (!stricmp (gViewPort->GetTeamName (i), "Spectator"))
				{
					// SaRcaZm - V7 - Start
					m_pButtons[i]->removeAllActionSignals ();
					m_pButtons[i]->addActionSignal( new CMenuHandler_StringCommand( "spectate", true ) );
					// SaRcaZm - V7 - End
					// Move it into a special position
					m_pButtons[i]->setPos (TEAMMENU_TOPLEFT_BUTTON2_X, TEAMMENU_TOPLEFT_BUTTON_Y + ((TEAMMENU_BUTTON_SIZE_Y + TEAMMENU_BUTTON_SPACER_Y) * (NUM_CLASS_BUTTONS+1)));
				}
			}
			else
			{
				// Hide the button (may be visible from previous maps)
				m_pButtons[i]->setVisible( false );
			}
		}
	}

	// SaRcaZm - V5 - Start
	// Clear the VGUI
	SlotInput (-1);
	SetActiveInfo (-1);
	// SaRcaZm - V5 - End

	// SaRcaZm - V6 - Start
	char sz[80];
	sprintf (sz, "%s: $%i", gHUD.m_TextMessage.BufferedLocaliseTextString("#Label_Money"), money);
	m_pMoneyLabel->setText(sz);
	// SaRcaZm - V6 - End
}

//=====================================
// Key inputs
bool CTeamMenuPanel::SlotInput( int iSlot )
{
	static int iMode = 0;	// 0 for Teams, 1 for Cars
	static int iLastSlot = -1;

	// SaRcaZm - V5 - Start
	// Special case to clear out the previous input
	if (iSlot == -1)
	{
		iMode = 0;
		iLastSlot = -1;
	} else
	// SaRcaZm - V5 - End
	if (!iMode)
	{
		// Otherwise, see if a particular team is selectable
		if ( (iSlot < 1) || (iSlot > gViewPort->GetNumberOfTeams()) )
			return false;
		if ( !m_pButtons[ iSlot ] )
			return false;

		// Is the button pushable?
		if ( m_pButtons[ iSlot ]->isVisible() )
		{
			// Update the active info instead
			SetActiveInfo (iSlot);
			iMode = 1;	// Change to car selection mode
			iLastSlot = -1;
			return true;
		}
	}
	else if (iMode == 1)
	{
		if ((iSlot < 1) || (iSlot > NUM_CLASS_BUTTONS+1))
			return false;
		if (!m_pClassButtons[iSlot-1])
			return false;

		if (m_pClassButtons[iSlot-1]->isVisible ())
		{
			iSlot += MAX_TEAMS;

			// Special case: If cancel button hit, go back to the team selection
			if (iSlot == MAX_TEAMS+NUM_CLASS_BUTTONS+1)
			// SaRcaZm - V5 - Start
			{
				SetActiveInfo (-1);
				iMode = 0;
			}
			// SaRcaZm - V5 - End
			else if (iLastSlot != iSlot)
			{
				SetActiveInfo (iSlot);
				iLastSlot = iSlot;
			}
			else
				m_pClassButtons[iSlot-MAX_TEAMS-1]->fireActionSignal();

			// SaRcaZm - V5 - Start
			return true;
			// SaRcaZm - V5 - End
		}
	}

	return false;
}

//======================================
// Update the Team menu before opening it
void CTeamMenuPanel::Open( void )
{
	Update();

	// SaRcaZm - V6 - Start
//	CCarInfo *carinfo = new CCarInfo ("carinfo.txt");
	//CCarInfo *carinfo = new CCarInfo (clientConst.szCarInfo);
	CCarInfo *carinfo = new CCarInfo (gViewPort->m_szCarInfoTxt, strlen (gViewPort->m_szCarInfoTxt));
	char szMake[20];
	int i = 0;

	carinfo->getNextManufacturer (szMake);
	while (szMake[0])
	{
		char szModel[20];
		int j = 0;
		strcpy (rallyinfo[i].szTeamName, szMake);	// Save the manufacturer

		// Loop through all the models
		carinfo->getNextModel (szModel);
		while (szModel[0])
		{
			strcpy (rallyinfo[i].szModels[j].szModelName, szModel);
			char szAttrib[20], szAttribVal[50];

			// Instead loop through each attribute
			carinfo->getNextAttributeString (szAttrib, szAttribVal);
			while (*szAttrib && *szAttribVal)
			{
				// Copy the attribute value to the correct variable
				if (!stricmp (szAttrib, "Weight"))
					strcpy (rallyinfo[i].szModels[j].szWeight, szAttribVal);
				else if (!stricmp (szAttrib, "Acceleration"))
					rallyinfo[i].szModels[j].iStats[STAT_ACCEL] = atoi (szAttribVal);
				else if (!stricmp (szAttrib, "Handling"))
					rallyinfo[i].szModels[j].iStats[STAT_HANDLING] = atoi (szAttribVal);
				else if (!stricmp (szAttrib, "TopSpeed"))
					rallyinfo[i].szModels[j].iStats[STAT_TOPSPEED] = atoi (szAttribVal);
				else if (!stricmp (szAttrib, "Transmission"))
					strcpy (rallyinfo[i].szModels[j].szTransmission, szAttribVal);
				else if (!stricmp (szAttrib, "HorsePower"))
					strcpy (rallyinfo[i].szModels[j].szHP, szAttribVal);
				else if (!stricmp (szAttrib, "Torque"))
					strcpy (rallyinfo[i].szModels[j].szTorque, szAttribVal);
				else if (!stricmp (szAttrib, "GearBox"))
					strcpy (rallyinfo[i].szModels[j].szGearBox, szAttribVal);
				else if (!stricmp (szAttrib, "BrakeLightDst"))
					rallyinfo[i].szModels[j].fBrakeLightDst = atof (szAttribVal);
				else if (!stricmp (szAttrib, "BrakeLightWidth"))
					rallyinfo[i].szModels[j].fBrakeLightWidth = atof (szAttribVal);
				else if (!stricmp (szAttrib, "BrakeLightHeight"))
					rallyinfo[i].szModels[j].fBrakeLightHeight = atof (szAttribVal);
				else if	(!stricmp (szAttrib, "HeadLightDst"))
					rallyinfo[i].szModels[j].fHeadLightDst = atof (szAttribVal);
				else if (!stricmp (szAttrib, "HeadLightWidth"))
					rallyinfo[i].szModels[j].fHeadLightWidth = atof (szAttribVal);
				else if (!stricmp (szAttrib, "HeadLightHeight"))
					rallyinfo[i].szModels[j].fHeadLightHeight = atof (szAttribVal);
				else if (!stricmp (szAttrib, "DisplayName"))
					strcpy (rallyinfo[i].szModels[j].szDisplayName, szAttribVal);

				// Get the next attribute
				carinfo->getNextAttributeString (szAttrib, szAttribVal);
			}

			j++;
			carinfo->getNextModel (szModel);
		}

		// Get the next manufacturer
		carinfo->getNextManufacturer (szMake);
		i++;
	}
	delete carinfo;
	// SaRcaZm - V6 - End

	// SaRcaZm - V5 - Start
	// Set the fx mode
	CRallyVGUIFX::getSingleton()->SetMode (MODE_SHOWROOM); // FragMented! v2

	// Update the spectator menu so it doesn't appear
	gViewPort->UpdateSpectatorMenu ();
	// SaRcaZm - V5 - End

	CMenuPanel::Open();
}

void CTeamMenuPanel::paintBackground()
{
}

// This is done a lot, so put it in a function
void CTeamMenuPanel::DefaultStats (void)
{
	int r, g, b, a;
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hTeamWindowText = pSchemes->getSchemeHandle( "Briefing Text" );

	// The Statistics label
	m_pNameLabel = new Label( "", TEAMMENU_WINDOW_TEXT_X, TEAMMENU_WINDOW_TEXT_Y );
	m_pNameLabel->setFont( pSchemes->getFont(hTitleScheme) ); 
	m_pNameLabel->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pNameLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pNameLabel->setBgColor( r, g, b, a );
	m_pNameLabel->setContentAlignment( vgui::Label::a_center );
	m_pNameLabel->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Title_Statistics"));

	// Now, fill out the statistics
	int iYPos = TEAMMENU_WINDOW_TEXT_INFO_Y;

	// Transmission
	// Label
	m_pTransmissionLabel = new Label( "Transmission:", TEAMMENU_WINDOW_TEXT_X, iYPos);
	m_pTransmissionLabel->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pTransmissionLabel->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pTransmissionLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pTransmissionLabel->setBgColor( r, g, b, a );
	m_pTransmissionLabel->setContentAlignment( vgui::Label::a_center );
	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;

	// Horse power
	// Label
	m_pHPLabel = new Label( "HP:", TEAMMENU_WINDOW_TEXT_X, iYPos);
	m_pHPLabel->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pHPLabel->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pHPLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pHPLabel->setBgColor( r, g, b, a );
	m_pHPLabel->setContentAlignment( vgui::Label::a_center );
	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;

	// Torque
	// Label
	m_pTorqueLabel = new Label( "Torque:", TEAMMENU_WINDOW_TEXT_X, iYPos);
	m_pTorqueLabel->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pTorqueLabel->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pTorqueLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pTorqueLabel->setBgColor( r, g, b, a );
	m_pTorqueLabel->setContentAlignment( vgui::Label::a_center );
	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;

	// Gearbox
	// Label
	m_pGearBoxLabel = new Label( "GearBox:", TEAMMENU_WINDOW_TEXT_X, iYPos);
	m_pGearBoxLabel->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pGearBoxLabel->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pGearBoxLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pGearBoxLabel->setBgColor( r, g, b, a );
	m_pGearBoxLabel->setContentAlignment( vgui::Label::a_center );
	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;

	// Weight
	// Label
	m_pWeightLabel = new Label( "Weight:", TEAMMENU_WINDOW_TEXT_X, iYPos);
	m_pWeightLabel->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pWeightLabel->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pWeightLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pWeightLabel->setBgColor( r, g, b, a );
	m_pWeightLabel->setContentAlignment( vgui::Label::a_center );

	iYPos -= TEAMMENU_WINDOW_TEXT_SPACE_Y * 2;

	// Acceleration
	m_pAccelLabel = new Label( "Acceleration:", TEAMMENU_WINDOW_TEXT3_X, iYPos);
	m_pAccelLabel->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pAccelLabel->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pAccelLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pAccelLabel->setBgColor( r, g, b, a );
	m_pAccelLabel->setContentAlignment( vgui::Label::a_center );
	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;

	// Handling
	m_pHandlingLabel = new Label( "Handling:", TEAMMENU_WINDOW_TEXT3_X, iYPos);
	m_pHandlingLabel->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pHandlingLabel->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pHandlingLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pHandlingLabel->setBgColor( r, g, b, a );
	m_pHandlingLabel->setContentAlignment( vgui::Label::a_center );
	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;

	// Top speed
	m_pTopSpeedLabel = new Label( "TopSpeed:", TEAMMENU_WINDOW_TEXT3_X, iYPos);
	m_pTopSpeedLabel->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pTopSpeedLabel->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pTopSpeedLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pTopSpeedLabel->setBgColor( r, g, b, a );
	m_pTopSpeedLabel->setContentAlignment( vgui::Label::a_center );
	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;

	// Now, fill out the statistics
	iYPos = TEAMMENU_WINDOW_TEXT_INFO_Y;

	// Transmission
	m_pTransmissionValue = new Label("Trans", TEAMMENU_WINDOW_TEXT2_X, iYPos);
	m_pTransmissionValue->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pTransmissionValue->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pTransmissionValue->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pTransmissionValue->setBgColor( r, g, b, a );
	m_pTransmissionValue->setContentAlignment( vgui::Label::a_center );

	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;

	// Horse power
	m_pHPValue = new Label("HP", TEAMMENU_WINDOW_TEXT2_X, iYPos);
	m_pHPValue->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pHPValue->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pHPValue->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pHPValue->setBgColor( r, g, b, a );
	m_pHPValue->setContentAlignment( vgui::Label::a_center );

	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;

	// Torque
	m_pTorqueValue = new Label("Torque", TEAMMENU_WINDOW_TEXT2_X, iYPos);
	m_pTorqueValue->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pTorqueValue->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pTorqueValue->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pTorqueValue->setBgColor( r, g, b, a );
	m_pTorqueValue->setContentAlignment( vgui::Label::a_center );

	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;

	// Gearbox
	m_pGearBoxValue = new Label("GB", TEAMMENU_WINDOW_TEXT2_X, iYPos);
	m_pGearBoxValue->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pGearBoxValue->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pGearBoxValue->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pGearBoxValue->setBgColor( r, g, b, a );
	m_pGearBoxValue->setContentAlignment( vgui::Label::a_center );

	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;

	// Weight
	m_pWeightValue = new Label("KG", TEAMMENU_WINDOW_TEXT2_X, iYPos);
	m_pWeightValue->setFont( pSchemes->getFont(hTeamWindowText) ); 
	m_pWeightValue->setParent( m_pTeamWindow );
	pSchemes->getFgColor( hTeamWindowText, r, g, b, a );
	m_pWeightValue->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTeamWindowText, r, g, b, a );
	m_pWeightValue->setBgColor( r, g, b, a );
	m_pWeightValue->setContentAlignment( vgui::Label::a_center );

	iYPos -= TEAMMENU_WINDOW_TEXT_SPACE_Y * 2;

	// Acceleration
	m_pAccelBar = new CProgressBar (TEAMMENU_WINDOW_TEXT4_X, iYPos, TEAMMENU_PROGRESS_SIZE_X, TEAMMENU_PROGRESS_SIZE_Y, 100, 0);
	m_pAccelBar->setParent (m_pTeamWindow);
	m_pAccelBar->setVisible (true);
	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;

	// Handling
	m_pHandlingBar = new CProgressBar (TEAMMENU_WINDOW_TEXT4_X, iYPos, TEAMMENU_PROGRESS_SIZE_X, TEAMMENU_PROGRESS_SIZE_Y, 100, 0);
	m_pHandlingBar->setParent (m_pTeamWindow);
	m_pHandlingBar->setVisible (true);
	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;

	// Top speed
	m_pTopSpeedBar = new CProgressBar (TEAMMENU_WINDOW_TEXT4_X, iYPos, TEAMMENU_PROGRESS_SIZE_X, TEAMMENU_PROGRESS_SIZE_Y, 100, 0);
	m_pTopSpeedBar->setParent (m_pTeamWindow);
	m_pTopSpeedBar->setVisible (true);
	iYPos += TEAMMENU_WINDOW_TEXT_SPACE_Y;
}

//======================================
// Mouse is over a team button, bring up the class info
void CTeamMenuPanel::SetActiveInfo( int iInput )
{
	if (iInput == m_iCurrentIndex)
		return;
	else
		m_iCurrentIndex = iInput;

	// Special case: If iInput = -1, just clear all the stats and buttons
	if (iInput < 0)
	{
		for (int i = 0; i < NUM_CLASS_BUTTONS; i++)
		{
			m_pClassButtons[i]->setArmed(false);
			m_pClassButtons[i]->setVisible(false);
		}
		m_iCurrentInfo = iInput;
		m_pTeamWindow->setVisible (false);
		CRallyVGUIFX::getSingleton()->ClearVGUIModels (); // FragMented! v2
	} else
	// Anything below 13 is dealt with here (i.e. any team choices)
	//if (iInput <= 13)
	if (iInput <= MAX_TEAMS)
	{
		for (int i = 1; i <= MAX_TEAM_BUTTONS; i++)
		{
			m_pButtons[i]->setArmed( false );
		}
		m_iCurrentInfo = iInput;

		// Clear the current statistics from the window
		m_pTeamWindow->setVisible (false);

		m_pButtons[iInput]->setArmed( true );
		CRallyVGUIFX::getSingleton()->ClearVGUIModels ();  // FragMented! v2

		// Setup the manufacturer logo
		if (m_pManuImage[1] == NULL)
		{
			for (int i = 1; i < MAX_TEAM_BUTTONS; i++)
			{
				// Create an array of all possible images
				if (*rallyinfo[i-1].szTeamName)
				{
					m_pManuImage[i] = new CImageLabel (rallyinfo[i-1].szTeamName, TEAMMENU_MANU_LOGO_X, TEAMMENU_MANU_LOGO_Y);
					m_pManuImage[i]->setParent (m_pTeamWindow);
				}
			}
		}
		// The manufacturer label
		for (i = 1; i < MAX_TEAM_BUTTONS; i++)
			if (m_pManuImage[i])
				m_pManuImage[i]->setVisible (false);
		m_pManuImage[m_iCurrentInfo]->setVisible (true);

		// Add the buttons for each class
		for (i = 0; i < NUM_CLASS_BUTTONS; i++)
		{
			if (rallyinfo[iInput-1].szModels[i].szModelName[0])
			{
				if (rallyinfo[iInput-1].szModels[i].szDisplayName[0])
					m_pClassButtons[i]->setText (rallyinfo[iInput-1].szModels[i].szDisplayName);
				else
					m_pClassButtons[i]->setText (rallyinfo[iInput-1].szModels[i].szModelName);
				m_pClassButtons[i]->setVisible (true);
				m_pClassButtons[i]->setArmed (false);
				char sz[80];
				sprintf (sz, "jointeam %s; changecar \"%s\"", rallyinfo[m_iCurrentInfo-1].szTeamName, rallyinfo[m_iCurrentInfo-1].szModels[i].szModelName);

				m_pClassButtons[i]->removeAllActionSignals ();
				m_pClassButtons[i]->addActionSignal( new CMenuHandler_RallyCommand( sz, 0, true ) );
				m_pClassButtons[i]->addActionSignal( new CMenuHandler_RallySound( "vgui/select.wav", false, 0 ) );
			}
			else
				m_pClassButtons[i]->setVisible (false);
		}

		// SaRcaZm - V7 - Start
		RallySound->fmodPlay ("vgui/move.wav", false, 0);
		// SaRcaZm - V7 - End
	}
	else
	{
		// We have the mouse over a class, so add the details
		//iInput -= 14;	// This brings it back to a 0-1 range
		iInput -= MAX_TEAM_BUTTONS;

		for (int i = 0; i < NUM_CLASS_BUTTONS; i++)
		{
			m_pClassButtons[i]->setArmed (false);
			// Update the hit button signals
			// Why the hell was I ever doing this here????? Creme must have moved it :)
			//sprintf (sz, "jointeam %s; model %s", rallyinfo[m_iCurrentInfo-1].szTeamName, rallyinfo[m_iCurrentInfo-1].szModels[i].szModelName);
			//sprintf (sz, "jointeam %s; changecar %s", rallyinfo[m_iCurrentInfo-1].szTeamName, rallyinfo[m_iCurrentInfo-1].szModels[i].szModelName);
			//m_pClassButtons[i]->addActionSignal( new CMenuHandler_StringCommandWatch( sz, true ) );
		}
		m_pClassButtons[NUM_CLASS_BUTTONS]->setArmed (false);

		// Arm the highlighted button
		m_pClassButtons[iInput]->setArmed (true);

		if (iInput == NUM_CLASS_BUTTONS)	// Cancel button
			return;

		// Get the scheme used for the Titles
		CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
		// schemes
		SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
		SchemeHandle_t hTeamWindowText = pSchemes->getSchemeHandle( "Briefing Text" );

		// Clear the current statistics from the window
		m_pTeamWindow->setVisible (true);

		// Set the text for all the stats
		m_pTransmissionValue->setText (rallyinfo[m_iCurrentInfo-1].szModels[iInput].szTransmission);
		m_pHPValue->setText (rallyinfo[m_iCurrentInfo-1].szModels[iInput].szHP);
		m_pTorqueValue->setText (rallyinfo[m_iCurrentInfo-1].szModels[iInput].szTorque);
		m_pGearBoxValue->setText (rallyinfo[m_iCurrentInfo-1].szModels[iInput].szGearBox);
		m_pWeightValue->setText (rallyinfo[m_iCurrentInfo-1].szModels[iInput].szWeight);
		m_pAccelBar->setProgress (rallyinfo[m_iCurrentInfo-1].szModels[iInput].iStats[STAT_ACCEL]);
		m_pHandlingBar->setProgress (rallyinfo[m_iCurrentInfo-1].szModels[iInput].iStats[STAT_HANDLING]);
		m_pTopSpeedBar->setProgress (rallyinfo[m_iCurrentInfo-1].szModels[iInput].iStats[STAT_TOPSPEED]);

		// The manufacturer label
		for (i = 1; i < MAX_TEAM_BUTTONS; i++)
			if (m_pManuImage[i])
				m_pManuImage[i]->setVisible (false);
		m_pManuImage[m_iCurrentInfo]->setVisible (true);

		bool bHaveModel = false;
		// See if we have the model already displayed
		modellife_t *tmp = vehicles;
		while ((tmp != NULL) && !bHaveModel)
		{
//			gEngfuncs.Con_DPrintf ("Searching for Model %s - %s\n", rallyinfo[m_iCurrentInfo-1].szModels[iInput].szModelName, tmp->name);

			if ((!stricmp (rallyinfo[m_iCurrentInfo-1].szModels[iInput].szModelName, tmp->name)) &&
				(tmp->life > 0.0f))
				bHaveModel = true;
			tmp = tmp->next;
		}

		if (!bHaveModel)
		{
//			gEngfuncs.Con_DPrintf ("Clearing models\n");
//			m_pRallyFX.ClearVGUIModels ();	// Don't need this as it is done in m_pRallyFX.VGUI

			// Create the spinning 3d model
			CRallyVGUIFX::getSingleton()->VGUI (rallyinfo[m_iCurrentInfo-1].szModels[iInput].szModelName); // FragMented! v2
		}

		// SaRcaZm - V7 - Start
		RallySound->fmodPlay ("vgui/move.wav", false, 0);
		// SaRcaZm - V7 - End
	}
}
