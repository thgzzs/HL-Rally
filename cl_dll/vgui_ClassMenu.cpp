//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: TFC Class Menu 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "VGUI_Font.h"
#include <VGUI_TextImage.h>

#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "parsemsg.h"

#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_ServerBrowser.h"

// SaRcaZm - V5 - Start
// Consider the rest of this file completely different....
#include "rally_vgui.h"
#include "carinfo.h"

typedef struct part_info_s {
	char szName[80];
	char szDescription[256];
	int iCost;
	char szValue[80];
} part_info_t;
typedef struct cust_info_s {
	char szPart[80];
	part_info_t partinfo[MAX_CUST_CHOICES];
} cust_info_s;
cust_info_s custinfo[MAX_CUST_BUTTONS];

// Class Menu Dimensions
// SaRcaZm - V7 - Start
#define CLASSMENU_TITLE_X				XRES(40)
#define CLASSMENU_TITLE_Y				YRES(10)
// SaRcaZm - V7 - End

#define CLASSMENU_HLRALLY_LOGO_X		XRES(230)
#define CLASSMENU_HLRALLY_LOGO_Y		YRES(4)
#define CLASSMENU_TITLE_BUTTON_Y		YRES(80)
#define CLASSMENU_TOPLEFT_BUTTON_X		XRES(40)
#define CLASSMENU_TOPLEFT_BUTTON2_X		XRES(124)
#define CLASSMENU_TOPLEFT_BUTTON_Y		YRES(110)
#define CLASSMENU_BUTTON_SIZE_X			XRES(110)
#define CLASSMENU_BUTTON_SIZE_Y			YRES(20)
#define CLASSMENU_BUTTON_SPACER_Y		YRES(8)

#define CLASSMENU_WINDOW_X				XRES(230)
#define CLASSMENU_WINDOW_Y				YRES(80)
#define CLASSMENU_WINDOW_SIZE_X			XRES(374)
#define CLASSMENU_WINDOW_SIZE_Y			YRES(176)

#define CLASSMENU_WINDOW_TEXT_X			XRES(16)
#define CLASSMENU_WINDOW_TEXT2_X		XRES(100)
#define CLASSMENU_WINDOW_TEXT_Y			YRES(16)
#define CLASSMENU_WINDOW_TEXT_INFO_Y	YRES(46)
#define CLASSMENU_WINDOW_TEXT_SPACE_Y	YRES(20)
#define CLASSMENU_WINDOW_TEXT_COST_X	XRES(250)
#define CLASSMENU_WINDOW_TEXT_COST_Y	YRES(140)

#define CLASSMENU_MODEL_WINDOW_PIC_X	XRES(120)
#define CLASSMENU_MODEL_WINDOW_PIC_Y	YRES(260)
#define CLASSMENU_MODEL_WINDOW_X		XRES(160)
#define CLASSMENU_MODEL_WINDOW_Y		YRES(275)
#define CLASSMENU_MODEL_WINDOW_SIZE_X	XRES(340)
#define CLASSMENU_MODEL_WINDOW_SIZE_Y	YRES(180)

// SaRcaZm - V6 - Start
#define CLASSMENU_MONEY_X				XRES(550)
#define CLASSMENU_MONEY_Y				YRES(80)
// SaRcaZm - V6 - End

// SaRcaZm - V7 - Start
#define CLASSMENU_MISC_WINDOW_X			XRES(20)
#define CLASSMENU_MISC_WINDOW_Y			YRES(260)
#define CLASSMENU_MISC_WINDOW_SIZE_X	XRES(120)
#define CLASSMENU_MISC_WINDOW_SIZE_Y	YRES(200)

#define CLASSMENU_MISC_TITLE_X			XRES(6)
#define CLASSMENU_MISC_TITLE_Y			YRES(6)

#define CLASSMENU_MISC_GEARRATIO_X		XRES(6)
#define CLASSMENU_MISC_GEARRATIO_Y		YRES(34)
#define CLASSMENU_MISC_GEARRATIO_SIZE_X	XRES(107)
#define CLASSMENU_MISC_GEARRATIO_SIZE_Y	YRES(20)

#define CLASSMENU_MISC_GEARRATIO_LABEL_X	XRES(34)
#define CLASSMENU_MISC_GEARRATIO_LABEL_Y	YRES(58)

#define CLASSMENU_MISC_MANUAL_LABEL_X	XRES(6)
#define CLASSMENU_MISC_MANUAL_LABEL_Y	YRES(82)
#define CLASSMENU_MISC_MANUAL_CHECK_X	XRES(48)
#define CLASSMENU_MISC_MANUAL_CHECK_Y	YRES(76)
// SaRcaZm - V7 - End

// Creation
CClassMenuPanel::CClassMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	m_pScrollPanel = NULL; // FragMented Beta 1

	// SaRcaZm - V8 - Start
	// Moved the customise.txt stuff down to CClassMenuPanel::Open

	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hClassWindowText = pSchemes->getSchemeHandle( "Briefing Text" );
	Font *pTitleFont = pSchemes->getFont( hTitleScheme );

	// color schemes
	int r, g, b, a;

	// The HLRally logo
	m_pHLRLogo = new CImageLabel ("hlrally", CLASSMENU_HLRALLY_LOGO_X, CLASSMENU_HLRALLY_LOGO_Y);
	m_pHLRLogo->setParent (this);

	// SaRcaZm - V7 - Start
	m_pTitle = new CImageLabel ("header_carmenu", CLASSMENU_TITLE_X, CLASSMENU_TITLE_Y);
	m_pTitle->setParent (this);

	// The manufacturer button label
	m_pButtonLabel1 = new Label ( "", CLASSMENU_TOPLEFT_BUTTON_X, CLASSMENU_TITLE_BUTTON_Y);
	m_pButtonLabel1->setParent( this );
	m_pButtonLabel1->setFont( pTitleFont );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pButtonLabel1->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pButtonLabel1->setBgColor( r, g, b, a );
	m_pButtonLabel1->setContentAlignment( vgui::Label::a_west );
	m_pButtonLabel1->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Title_Part"));

	// The model button label
	m_pButtonLabel2 = new Label ( "", CLASSMENU_TOPLEFT_BUTTON2_X, CLASSMENU_TITLE_BUTTON_Y);
	m_pButtonLabel2->setParent( this );
	m_pButtonLabel2->setFont( pTitleFont );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pButtonLabel2->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pButtonLabel2->setBgColor( r, g, b, a );
	m_pButtonLabel2->setContentAlignment( vgui::Label::a_west );
	m_pButtonLabel2->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Title_Model"));

	// Moved the button creation to CClassMenuPanel::Open
	// But I do create a panel for the buttons now
	m_pPartsPanel = new CTransparentPanel( 255, CLASSMENU_TOPLEFT_BUTTON2_X, CLASSMENU_TOPLEFT_BUTTON_Y, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y * 8 );
	m_pPartsPanel->setParent( this );
	m_pPartsPanel->setVisible( true );

	// The money label
	Font *pWindowFont = pSchemes->getFont( hClassWindowText );
	m_pMoneyLabel = new Label ( "", CLASSMENU_MONEY_X, CLASSMENU_MONEY_Y );
	m_pMoneyLabel->setParent( this );
	m_pMoneyLabel->setFont( pWindowFont );
	m_pMoneyLabel->setFgColor( MONEY_R, MONEY_G, MONEY_B, 0 );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pMoneyLabel->setBgColor( r, g, b, a );
	m_pMoneyLabel->setContentAlignment( vgui::Label::a_west );

	// Create the info window
	m_pPartWindow  = new CTransparentPanel( 255, CLASSMENU_WINDOW_X, CLASSMENU_WINDOW_Y, CLASSMENU_WINDOW_SIZE_X, CLASSMENU_WINDOW_SIZE_Y );
	m_pPartWindow->setParent( this );

	// Create the model window picture
	m_pWndImage = new CCustResImageLabel ("customise_window", CLASSMENU_MODEL_WINDOW_PIC_X, CLASSMENU_MODEL_WINDOW_PIC_Y);
	m_pWndImage->setParent (this);

	// Create the miscellaneous customisations window
	m_pMiscPanel = new CTransparentPanel( 255, CLASSMENU_MISC_WINDOW_X, CLASSMENU_MISC_WINDOW_Y, CLASSMENU_MISC_WINDOW_SIZE_X, CLASSMENU_MISC_WINDOW_SIZE_Y );
	m_pMiscPanel->setParent( this );
	//m_pMiscPanel->setBorder( new LineBorder( Color(255 * 0.7,170 * 0.7,0,0)) );
	m_pMiscPanel->setVisible( true );

	// Create the window title
	m_pMiscLabel = new Label( "", CLASSMENU_MISC_TITLE_X, CLASSMENU_MISC_TITLE_Y );
	m_pMiscLabel->setParent( m_pMiscPanel );
	m_pMiscLabel->setFont( pSchemes->getFont(hTitleScheme) );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pMiscLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pMiscLabel->setBgColor( r, g, b, a );
	m_pMiscLabel->setContentAlignment( vgui::Label::a_west );
	m_pMiscLabel->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Title_MiscCustomise"));

	// Gear ratio slider
	m_pGearSlider = new CRallySlider( "gearratio", CLASSMENU_MISC_GEARRATIO_X, CLASSMENU_MISC_GEARRATIO_Y, CLASSMENU_MISC_GEARRATIO_SIZE_X, CLASSMENU_MISC_GEARRATIO_SIZE_Y );
	m_pGearSlider->setParent( m_pMiscPanel );
	m_pGearSlider->setVisible( true );
	m_pGearSlider->setRange (0, 10);
	m_pGearSlider->setValue (5);
	m_pGearSlider->increaseNobMargin (XRES(5));

	// Gear ratio label
	m_pGearLabel = new Label ( "Gear Ratio", CLASSMENU_MISC_GEARRATIO_LABEL_X, CLASSMENU_MISC_GEARRATIO_LABEL_Y );
	m_pGearLabel->setParent( m_pMiscPanel );
	m_pGearLabel->setFont( pWindowFont );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pGearLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pGearLabel->setBgColor( r, g, b, a );
	m_pGearLabel->setContentAlignment( vgui::Label::a_west );

	// Manual transmission label
	m_pManualLabel = new Label ( "Manual", CLASSMENU_MISC_MANUAL_LABEL_X, CLASSMENU_MISC_MANUAL_LABEL_Y );
	m_pManualLabel->setParent( m_pMiscPanel );
	m_pManualLabel->setFont( pWindowFont );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pManualLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pManualLabel->setBgColor( r, g, b, a );
	m_pManualLabel->setContentAlignment( vgui::Label::a_west );

	// Manual transmission checkbox
	m_pManualCheck = new CImageCheckButton ("checkbox", false, CLASSMENU_MISC_MANUAL_CHECK_X, CLASSMENU_MISC_MANUAL_CHECK_Y);
	m_pManualCheck->setParent (m_pMiscPanel);
	m_pManualCheck->setVisible (true);

	// Initialise variables
	Initialize ();

	for (int i = 0; i < MAX_CUST_BUTTONS; i++)
		m_pCustButtons[i] = NULL;
	for (i = 0; i < MAX_CUST_CHOICES; i++)
		m_pChoiceButtons[i] = NULL;
	// SaRcaZm - V8 - End
}

// Deconstructor
CClassMenuPanel::~CClassMenuPanel (void)
{
	for (int i = 0; i < MAX_CUST_BUTTONS; i++)
		if (m_pCustButtons[i])
			delete m_pCustButtons[i];
	for (i = 0; i < MAX_CUST_CHOICES; i++)
		if (m_pChoiceButtons[i])
			delete m_pChoiceButtons[i];

	if (m_pPartWindow)
		delete m_pPartWindow;
	if (m_pScrollPanel)
		delete m_pScrollPanel;
	if (m_pHLRLogo)
		delete m_pHLRLogo;
	if (m_pTitle)
		delete m_pTitle;
	if (m_pButtonLabel1)
		delete m_pButtonLabel1;
	if (m_pButtonLabel2)
		delete m_pButtonLabel2;
	if (m_pMiscLabel)
		delete m_pMiscLabel;
	if (m_pGearLabel)
		delete m_pGearLabel;
	if (m_pManualLabel)
		delete m_pManualLabel;
	if (m_pMiscPanel)
		delete m_pMiscPanel;
	if (m_pPartsPanel)
		delete m_pPartsPanel;
	if (m_pMoneyLabel)
		delete m_pMoneyLabel;
	if (m_pGearSlider)
		delete m_pGearSlider;
	if (m_pManualCheck)
		delete m_pManualCheck;
	if (m_pWndImage)
		delete m_pWndImage;
}

// SaRcaZm - V7 - Start
void CClassMenuPanel::Close (void)
{
	// This is where the menu is closing
	// Add the gear ratio command
	char sz[80];
	sprintf (sz, "gearratio %i", m_pGearSlider->getValue ());
	gViewPort->AddCommand (sz);

	gEngfuncs.Cvar_SetValue ("rally_manual", (m_pManualCheck->isChecked() ? 1.0 : 0.0));
	CMenuPanel::Close ();
}
// SaRcaZm - V7 - End

// Update
void CClassMenuPanel::Update()
{
	// Get the model name
/*	char szModel[20];
	memset (szModel, 0, 20);
	gViewPort->GetCommandParam ("changecar", szModel);
	sprintf (m_szModelName, "player/%s/%s.mdl", szModel, szModel);
	SetActiveInfo (-1);
	SlotInput (-1);*/
}

//======================================
// Key inputs for the Class Menu
bool CClassMenuPanel::SlotInput( int iSlot )
{
	static int iMode = 0;	// 0 for Parts, 1 for Models
	static int iLastSlot = -1;

	if (iSlot == -1)	// Special case to clear out the previous input
	{
		iMode = 0;
		iLastSlot = -1;
	}
	else if (iMode == 0)
	{
		if ((iSlot < 0) || (iSlot >= MAX_CUST_BUTTONS))
			return false;

		iSlot--;
		if (!m_pCustButtons[iSlot] || !m_pCustButtons[iSlot]->isVisible ())
			return false;

		SetActiveInfo (iSlot);
		iMode = 1;
		iLastSlot = -1;
		return true;
	}
	else if (iMode == 1)
	{
		if ((iSlot < 0) || (iSlot > MAX_CUST_CHOICES + 1))
			return false;

		iSlot--;
		if (!m_pChoiceButtons[iSlot] || !m_pChoiceButtons[iSlot]->isVisible ())
			return false;

		if (iSlot == MAX_CUST_CHOICES)
		{
			// Cancel button
			SetActiveInfo (-1);
			iMode = 0;
			return true;
		}

		if (iSlot != iLastSlot)
		{
			SetActiveInfo (iSlot + MAX_CUST_BUTTONS);
			iLastSlot = iSlot;
		}
		else if (iSlot == iLastSlot)
			m_pChoiceButtons[iSlot]->fireActionSignal ();

		return true;
	}

	return false;
}

// SaRcaZm - V8 - Start
void RemoveQuotes (char *sz);
// SaRcaZm - V8 - End

//======================================
// Update the Class menu before opening it
void CClassMenuPanel::Open( void )
{
	// SaRcaZm - V6 - Start
	// Create the customisation buttons
	//CCarInfo *carinfo = new CCarInfo ("Customise.txt");
	CCarInfo *carinfo = new CCarInfo (gViewPort->m_szCustomTxt, strlen (gViewPort->m_szCustomTxt));
	char szMake[20];
	int i = 0;

	// So, you think I could create a few more indents??
	carinfo->getNextManufacturer (szMake);
	while (szMake[0])
	{
		char szModel[20];
		int j = 0;
		strcpy (custinfo[i].szPart, szMake);

		// Loop through all the models
		carinfo->getNextModel (szModel);
		while (szModel[0])
		{
			// Read each of the headings in
			char szAttrib[20], szAttribVal[256];
			strcpy (custinfo[i].partinfo[j].szName, szModel);

			// Instead loop through each attribute
			carinfo->getNextAttributeString (szAttrib, szAttribVal);
			while (szAttrib[0] && szAttribVal[0])
			{
				if (!stricmp (szAttrib, "Desc"))
				{
					for (int k = 0; (k < 256) && (szAttribVal[k] != 0); k++)
					{
						if ((szAttribVal[k] == '\\') && (szAttribVal[k+1] == 'n')) // New line
						{
							szAttribVal[k] = ' ';	// Take the easy way out
							szAttribVal[k+1] = '\n';
						}
					}
					strcpy (custinfo[i].partinfo[j].szDescription, szAttribVal);
				}
				else if (!stricmp (szAttrib, "Cost"))
					custinfo[i].partinfo[j].iCost = atoi (szAttribVal);
				else if (!stricmp (szAttrib, "Value"))
					strcpy (custinfo[i].partinfo[j].szValue, szAttribVal);

				// Get the next attribute
				carinfo->getNextAttributeString (szAttrib, szAttribVal);
			}

			// Get the next model
			j++;
			carinfo->getNextModel (szModel);
		}

		// Get the next manufacturer
		carinfo->getNextManufacturer (szMake);
		i++;
	}
	delete carinfo;

	// Create the buttons for the part choices
	int	 iYPos = CLASSMENU_TOPLEFT_BUTTON_Y;
	for (i = 0; (i < MAX_CUST_BUTTONS)/* && custinfo[i].szPart[0]*/; i++)
	{
		m_pCustButtons[i] = new CManuImageButton(custinfo[i].szPart, CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y);
		m_pCustButtons[i]->setParent (this);
		m_pCustButtons[i]->setVisible (true);
		//m_pCustButtons[i]->addInputSignal(new CHandler_MenuButtonOver(this, i));
		m_pCustButtons[i]->addButtonClickSignal (this, i);
		iYPos += CLASSMENU_BUTTON_SIZE_Y + CLASSMENU_BUTTON_SPACER_Y;
	}

	// SaRcaZm - V6 - Start
	// Create the buttons for the individual parts
	if (m_pPartsPanel)
		m_pPartsPanel->removeAllChildren ();

	iYPos = 0;
	for (i = 0; i < MAX_CUST_CHOICES; i++)
	{
		m_pChoiceButtons[i] = new CClassImageButton("", 0, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y);
		m_pChoiceButtons[i]->setParent (m_pPartsPanel);
		m_pChoiceButtons[i]->setVisible (false);
		m_pChoiceButtons[i]->addInputSignal(new CHandler_MenuButtonOver(this, i+MAX_CUST_BUTTONS));
		iYPos += CLASSMENU_BUTTON_SIZE_Y + CLASSMENU_BUTTON_SPACER_Y;
	}
	// Create the Cancel button
	m_pChoiceButtons[MAX_CUST_CHOICES] = new CClassImageButton("Cancel", 0, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y);
	m_pChoiceButtons[MAX_CUST_CHOICES]->setParent (m_pPartsPanel);
	m_pChoiceButtons[MAX_CUST_CHOICES]->setVisible (true);
	m_pChoiceButtons[MAX_CUST_CHOICES]->addActionSignal( new CMenuHandler_TextWindow(HIDE_TEXTWINDOW) );

	// The money label
	char sz[80];
	sprintf (sz, "%s: $%i", gHUD.m_TextMessage.BufferedLocaliseTextString("#Label_Money"), money);
	m_pMoneyLabel->setText(sz);
	// SaRcaZm - V6 - End

//	Update();
	char szModel[20];
	memset (szModel, 0, 20);
	gViewPort->GetCommandParam ("changecar", szModel);
	// SaRcaZm - V8 - Start
	RemoveQuotes (szModel);
	sprintf (m_szModelName, "player/%s/%s.mdl", szModel, szModel);
	//gEngfuncs.Con_DPrintf ("Model - %s\n", m_szModelName);
	// SaRcaZm - V8 - End
	SetActiveInfo (-1);
	SlotInput (-1);

	// Create the initial 3D model
	// SaRcaZm - V7 - Start
	m_iAnimNum = CRallyVGUIFX::getSingleton()->CustomAnim (m_szModelName, CUST_NO_ANIM); // FragMented! v2
	m_iCustomAnim = CUST_NO_ANIM;
	// SaRcaZm - V7 - End

	CMenuPanel::Open();
	// SaRcaZm - V6 - End

	// SaRcaZm - V7 - Start
	// Update the Gear Ratio slider
	memset (sz, 0, sizeof (sz));
	if (gViewPort->GetCommandParam ("gearratio", sz))
		m_pGearSlider->setValue (atoi(sz));

	m_pManualCheck->setChecked (gEngfuncs.pfnGetCvarFloat ("rally_manual") != 0);
	// SaRcaZm - V7 - End
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CClassMenuPanel::Initialize( void )
{
	m_iAnimNum = 0;
}

//======================================
// Mouse is over a class button, bring up the class info
void CClassMenuPanel::SetActiveInfo( int iInput )
{
	// SaRcaZm - V7 - Start
	if (iInput == m_iCurrentIndex)
		return;
	else
		m_iCurrentIndex = iInput;
	// SaRcaZm - V7 - End

	// Three cases
	if (iInput < 0)
	{
		for (int i = 0; i < MAX_CUST_CHOICES; i++)
		{
			m_pChoiceButtons[i]->setArmed (false);
			m_pChoiceButtons[i]->setVisible (false);
		}
		m_iCurrentInfo = iInput;
		m_pPartWindow->removeAllChildren ();
//		m_pPartWindow->repaintAll ();
		if (m_iAnimNum)
		{
			// SaRcaZm - V7 - Start
			CRallyVGUIFX::getSingleton()->SetCustomAnim (m_iAnimNum, CUST_NO_ANIM); // FragMented v2
			m_iCustomAnim = CUST_NO_ANIM;
			// SaRcaZm - V7 - End
		}
	}
	else if (iInput < MAX_CUST_BUTTONS)
	{
		// Customise buttons
		// Deactivate everything
		for (int i = 0; i < MAX_CUST_BUTTONS; i++)
		{
			m_pCustButtons[i]->setArmed(false);
		}
		m_pCustButtons[iInput]->setArmed(true);
		m_iCurrentInfo = iInput;
		for (i = 0; i < MAX_CUST_CHOICES; i++)
		{
			m_pChoiceButtons[i]->setVisible (false);
			m_pChoiceButtons[i]->setArmed (false);
		}
		// Cancel button
		m_pChoiceButtons[MAX_CUST_CHOICES]->setArmed (false);

		// Remove all the info about the previous part
		m_pPartWindow->removeAllChildren ();
		m_pPartWindow->repaintAll ();

		// Create the buttons for each part
		for (i = 0; (i < MAX_CUST_CHOICES) && custinfo[iInput].partinfo[i].szName[0]; i++)
		{
			part_info_t *part = &custinfo[iInput].partinfo[i];
			m_pChoiceButtons[i]->setText (part->szName);
			m_pChoiceButtons[i]->setVisible (true);
			m_pChoiceButtons[i]->removeAllActionSignals ();
			// SaRcaZm - V6 - Start
			char szCmd[80];
			sprintf (szCmd, "buy \"%s\"", part->szName);
			m_pChoiceButtons[i]->addActionSignal( new CMenuHandler_RallyCommand( szCmd, part->iCost, true ) );
			// SaRcaZm - V7 - Start
			m_pChoiceButtons[i]->addActionSignal( new CMenuHandler_RallySound( "vgui/select.wav", false, 0 ) );
			// SaRcaZm - V7 - End
			// SaRcaZm - V6 - End
		}

		// Now that all the boring text-based info is out of the way, bring on the cool 3D animations
		// SaRcaZm - V7 - Start
		if (m_iCustomAnim != CUST_TOPBUTTON_ANIM + iInput)
		{
			CRallyVGUIFX::getSingleton()->SetCustomAnim (m_iAnimNum, CUST_TOPBUTTON_ANIM + iInput); // FragMented! v2
			m_iCustomAnim = CUST_TOPBUTTON_ANIM + iInput;
		}

		RallySound->fmodPlay ("vgui/move.wav", false, 0);
		// SaRcaZm - V7 - End
	}
	else
	{
		// Specific parts
		int index = iInput - MAX_CUST_BUTTONS;

		// Arm the buttons
//		m_iCurrentInfo = iInput;
		for (int i = 0; i < MAX_CUST_CHOICES; i++)
		{
			m_pChoiceButtons[i]->setArmed (false);
		}
		m_pChoiceButtons[index]->setArmed (true);

		// Remove all the info about the previous part
		m_pPartWindow->removeAllChildren ();
		m_pPartWindow->repaintAll ();

		// Display information about the part
		int r, g, b, a;
		// Get the scheme used for the Titles
		CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
		// schemes
		SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
		SchemeHandle_t hWindowText = pSchemes->getSchemeHandle( "Briefing Text" );
		SchemeHandle_t hCostText = pSchemes->getSchemeHandle( "Primary Button Text" );

		// The Part Info Heading
		Label *pNameLabel = new Label( "", CLASSMENU_WINDOW_TEXT_X, CLASSMENU_WINDOW_TEXT_Y );
		pNameLabel->setFont( pSchemes->getFont(hTitleScheme) ); 
		pNameLabel->setParent( m_pPartWindow );
		pSchemes->getFgColor( hTitleScheme, r, g, b, a );
		pNameLabel->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hTitleScheme, r, g, b, a );
		pNameLabel->setBgColor( r, g, b, a );
		pNameLabel->setContentAlignment( vgui::Label::a_center );
		pNameLabel->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Title_PartInfo"));

		// Now, fill out the part info
		int iXPos, iYPos, iXSize, iYSize;
		m_pPartWindow->getPos( iXPos,iYPos );
		m_pPartWindow->getSize( iXSize,iYSize );

		// Create the Scroll panel
		m_pScrollPanel = new CTFScrollPanel( CLASSMENU_WINDOW_TEXT_X, CLASSMENU_WINDOW_TEXT_INFO_Y, iXSize - CLASSMENU_WINDOW_TEXT_X - XRES (16), iYSize - CLASSMENU_WINDOW_TEXT_INFO_Y - YRES(36) );
		m_pScrollPanel->setParent(m_pPartWindow);

		//force the scrollbars on so clientClip will take them in account after the validate
		m_pScrollPanel->setScrollBarAutoVisible(false, false);
		m_pScrollPanel->setScrollBarVisible(true, true);
		m_pScrollPanel->validate();
		//turn the scrollbars back into automode
		m_pScrollPanel->setScrollBarAutoVisible(true, true);
		m_pScrollPanel->setScrollBarVisible(false, false);
		m_pScrollPanel->validate();

		// Value
		TextPanel *pDescription = new TextPanel("", 0,0, 64,64);
		pDescription->setParent( m_pScrollPanel->getClient() );
		pDescription->setFont( pSchemes->getFont(hWindowText) );
		pSchemes->getFgColor( hWindowText, r, g, b, a );
		pDescription->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hWindowText, r, g, b, a );
		pDescription->setBgColor( r, g, b, a );
		pDescription->setText(custinfo[m_iCurrentInfo].partinfo[index].szDescription);
		// Get the total size of the MOTD text and resize the text panel
		int iScrollSizeX, iScrollSizeY;
		pDescription->getTextImage()->setSize(m_pScrollPanel->getClientClip()->getWide(), m_pScrollPanel->getClientClip()->getTall());
		pDescription->getTextImage()->getTextSizeWrapped( iScrollSizeX, iScrollSizeY );
		// Now resize the textpanel to fit the scrolled size
		pDescription->setSize( iScrollSizeX , iScrollSizeY );
		m_pScrollPanel->validate();

		// Cost
		iYPos = CLASSMENU_WINDOW_TEXT_COST_Y;
		Label *pValue = new Label(custinfo[m_iCurrentInfo].partinfo[index].szValue, CLASSMENU_WINDOW_TEXT_COST_X, iYPos);
		pValue->setFont( pSchemes->getFont(hWindowText) ); 
		pValue->setParent( m_pPartWindow );
		pSchemes->getFgColor( hWindowText, r, g, b, a );
		pValue->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hWindowText, r, g, b, a );
		pValue->setBgColor( r, g, b, a );
		pValue->setContentAlignment( vgui::Label::a_center );

		iYPos += CLASSMENU_WINDOW_TEXT_SPACE_Y;
		char szCost[20];
		sprintf (szCost, "Cost: $%i.00", custinfo[m_iCurrentInfo].partinfo[index].iCost);
		Label *pCost = new Label(szCost, CLASSMENU_WINDOW_TEXT_COST_X, iYPos);
		pCost->setFont( pSchemes->getFont(hCostText) ); 
		pCost->setParent( m_pPartWindow );
		pSchemes->getFgColor( hCostText, r, g, b, a );
		pCost->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hCostText, r, g, b, a );
		pCost->setBgColor( r, g, b, a );
		pCost->setContentAlignment( vgui::Label::a_center );

		// SaRcaZm - V7 - Start
		RallySound->fmodPlay ("vgui/move.wav", false, 0);
		// SaRcaZm - V7 - End
	}
}

void CClassMenuPanel::paintBackground (void)
{
	//drawSetColor (0, 0, 255, 1);	// For testing only
	drawSetColor (0, 0, 0, 1);
	drawFilledRect (0, 0, CLASSMENU_MODEL_WINDOW_X, YRES(480));
	drawFilledRect (CLASSMENU_MODEL_WINDOW_X, 0, XRES (640), CLASSMENU_MODEL_WINDOW_Y);
	drawFilledRect (CLASSMENU_MODEL_WINDOW_X + CLASSMENU_MODEL_WINDOW_SIZE_X, CLASSMENU_MODEL_WINDOW_Y, XRES(640), YRES(480));
	drawFilledRect (CLASSMENU_MODEL_WINDOW_X, CLASSMENU_MODEL_WINDOW_Y + CLASSMENU_MODEL_WINDOW_SIZE_Y, CLASSMENU_MODEL_WINDOW_X + CLASSMENU_MODEL_WINDOW_SIZE_X, YRES(480));
}
// SaRcaZm - V5 - End