//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: VGUI scoreboard
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================


#include<VGUI_LineBorder.h>

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_ScorePanel.h"
#include "..\game_shared\vgui_helpers.h"
#include "..\game_shared\vgui_loadtga.h"

#include "ITrackerUser.h"
extern ITrackerUser *g_pTrackerUser;

// SaRcaZm - V4 - Start
#include "rally_vguifx.h" // FragMented! v2
// SaRcaZm - V4 - End

extern rally_teaminfo_t rallyinfo[MAX_TEAMS]; // FragMented! v2

hud_player_info_t	 g_PlayerInfoList[MAX_PLAYERS+1];	   // player info from the engine
extra_player_info_t  g_PlayerExtraInfo[MAX_PLAYERS+1];   // additional player info sent directly to the client dll
team_info_t			 g_TeamInfo[MAX_TEAMS+1];
int					 g_IsSpectator[MAX_PLAYERS+1];

int HUD_IsGame( const char *game );
int EV_TFC_IsAllyTeam( int iTeam1, int iTeam2 );

// Scoreboard dimensions
#define SBOARD_TITLE_SIZE_Y			YRES(22)

#define X_BORDER					XRES(4)

// Column sizes
class SBColumnInfo
{
public:
	char				*m_pTitle;		// If null, ignore, if starts with #, it's localized, otherwise use the string directly.
	int					m_Width;		// Based on 640 width. Scaled to fit other resolutions.
	Label::Alignment	m_Alignment;	
};

// grid size is marked out for 640x480 screen

// SaRcaZm - V4 - Start
/*SBColumnInfo g_ColumnInfo[NUM_COLUMNS] =
{
	{NULL,			24,			Label::a_east},		// tracker column
	{NULL,			140,		Label::a_east},		// name
	{NULL,			56,			Label::a_east},		// class
	{"#SCORE",		40,			Label::a_east},
	{"#DEATHS",		46,			Label::a_east},
	{"#LATENCY",	46,			Label::a_east},
	{"#VOICE",		40,			Label::a_east},
	{NULL,			2,			Label::a_east},		// blank column to take up the slack
};*/
SBColumnInfo g_ColumnInfo[NUM_COLUMNS] =
{
	{NULL,			5,			Label::a_east},		// tracker column
	{NULL,			95,			Label::a_east},		// name
	{"#CAR",		80,			Label::a_west},
	{"#POSITION",	36,			Label::a_east},
//	{"#RACE_TIME",	46,			Label::a_east},
	{"#BEST_LAP",	46,			Label::a_east},
	{"#SCORE",		46,			Label::a_east},
	{"#LATENCY",	46,			Label::a_east},
	// SaRcaZm - V5 - Start
	//{"#VOICE",		40,			Label::a_east},
	{"#VOICE",		40,			Label::a_center},
	// SaRcaZm - V5 - Start
};
// SaRcaZm - V4 - End

#define TEAM_NO				0
#define TEAM_YES			1
#define TEAM_SPECTATORS		2
#define TEAM_BLANK			3


//-----------------------------------------------------------------------------
// ScorePanel::HitTestPanel.
//-----------------------------------------------------------------------------

void ScorePanel::HitTestPanel::internalMousePressed(MouseCode code)
{
	for(int i=0;i<_inputSignalDar.getCount();i++)
	{
		_inputSignalDar[i]->mousePressed(code,this);
	}
}



//-----------------------------------------------------------------------------
// Purpose: Create the ScoreBoard panel
//-----------------------------------------------------------------------------
ScorePanel::ScorePanel(int x,int y,int wide,int tall) : Panel(x,y,wide,tall)
{
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle("Scoreboard Title Text");
	SchemeHandle_t hSmallScheme = pSchemes->getSchemeHandle("Scoreboard Small Text");
	Font *tfont = pSchemes->getFont(hTitleScheme);
	Font *smallfont = pSchemes->getFont(hSmallScheme);

	setBgColor(0, 0, 0, 96);
	m_pCurrentHighlightLabel = NULL;
	m_iHighlightRow = -1;

	m_pTrackerIcon = NULL;
	m_pTrackerIcon = vgui_LoadTGANoInvertAlpha("gfx/vgui/640_scoreboardtracker.tga");

	// Initialize the top title.
	m_TitleLabel.setFont(tfont);
	m_TitleLabel.setText("");
	m_TitleLabel.setBgColor( 0, 0, 0, 255 );
	m_TitleLabel.setFgColor( Scheme::sc_primary1 );
	m_TitleLabel.setContentAlignment( vgui::Label::a_west );

	m_Border = new LineBorder(Color(60, 60, 60, 128));
	setBorder(m_Border);
	setPaintBorderEnabled(true);

	int xpos = g_ColumnInfo[0].m_Width + 3;
	if (ScreenWidth >= 640)
	{
		// only expand column size for res greater than 640
		xpos = XRES(xpos);
	}
	m_TitleLabel.setBounds(xpos, 4, wide, SBOARD_TITLE_SIZE_Y);
	m_TitleLabel.setContentFitted(false);
	m_TitleLabel.setParent(this);

	// Setup the header (labels like "name", "class", etc..).
	m_HeaderGrid.SetDimensions(NUM_COLUMNS, 1);
	m_HeaderGrid.SetSpacing(0, 0);
	
	for(int i=0; i < NUM_COLUMNS; i++)
	{
		if (g_ColumnInfo[i].m_pTitle && g_ColumnInfo[i].m_pTitle[0] == '#')
			m_HeaderLabels[i].setText(CHudTextMessage::BufferedLocaliseTextString(g_ColumnInfo[i].m_pTitle));
		else if(g_ColumnInfo[i].m_pTitle)
			m_HeaderLabels[i].setText(g_ColumnInfo[i].m_pTitle);

		int xwide = g_ColumnInfo[i].m_Width;
		if (ScreenWidth >= 640)
		{
			xwide = XRES(xwide);
		}
		else if (ScreenWidth == 400)
		{
			// hack to make 400x300 resolution scoreboard fit
			if (i == 1)
			{
				// reduces size of player name cell
				xwide -= 28;
			}
			else if (i == 0)
			{
				// tracker icon cell
				xwide -= 8;
			}
		}
		
		m_HeaderGrid.SetColumnWidth(i, xwide);
		m_HeaderGrid.SetEntry(i, 0, &m_HeaderLabels[i]);

		m_HeaderLabels[i].setBgColor(0,0,0,255);
		m_HeaderLabels[i].setBgColor(0,0,0,255);
		m_HeaderLabels[i].setFgColor(Scheme::sc_primary1);
		m_HeaderLabels[i].setFont(smallfont);
		m_HeaderLabels[i].setContentAlignment(g_ColumnInfo[i].m_Alignment);

		int yres = 12;
		if (ScreenHeight >= 480)
		{
			yres = YRES(yres);
		}
		m_HeaderLabels[i].setSize(50, yres);
	}

	// Set the width of the last column to be the remaining space.
	int ex, ey, ew, eh;
	m_HeaderGrid.GetEntryBox(NUM_COLUMNS - 2, 0, ex, ey, ew, eh);
	m_HeaderGrid.SetColumnWidth(NUM_COLUMNS - 1, (wide - X_BORDER) - (ex + ew));

	m_HeaderGrid.AutoSetRowHeights();
	m_HeaderGrid.setBounds(X_BORDER, SBOARD_TITLE_SIZE_Y, wide - X_BORDER*2, m_HeaderGrid.GetRowHeight(0));
	m_HeaderGrid.setParent(this);
	m_HeaderGrid.setBgColor(0,0,0,255);


	// Now setup the listbox with the actual player data in it.
	int headerX, headerY, headerWidth, headerHeight;
	m_HeaderGrid.getBounds(headerX, headerY, headerWidth, headerHeight);
	m_PlayerList.setBounds(headerX, headerY+headerHeight, headerWidth, tall - headerY - headerHeight - 6);
	m_PlayerList.setBgColor(0,0,0,255);
	m_PlayerList.setParent(this);

	for(int row=0; row < NUM_ROWS; row++)
	{
		CGrid *pGridRow = &m_PlayerGrids[row];

		pGridRow->SetDimensions(NUM_COLUMNS, 1);
		
		for(int col=0; col < NUM_COLUMNS; col++)
		{
			m_PlayerEntries[col][row].setContentFitted(false);
			m_PlayerEntries[col][row].setRow(row);
			m_PlayerEntries[col][row].addInputSignal(this);
			pGridRow->SetEntry(col, 0, &m_PlayerEntries[col][row]);
		}

		pGridRow->setBgColor(0,0,0,255);
//		pGridRow->SetSpacing(2, 0);
		pGridRow->SetSpacing(0, 0);
		pGridRow->CopyColumnWidths(&m_HeaderGrid);
		pGridRow->AutoSetRowHeights();
		pGridRow->setSize(PanelWidth(pGridRow), pGridRow->CalcDrawHeight());
		pGridRow->RepositionContents();

		m_PlayerList.AddItem(pGridRow);
	}


	// Add the hit test panel. It is invisible and traps mouse clicks so we can go into squelch mode.
	m_HitTestPanel.setBgColor(0,0,0,255);
	m_HitTestPanel.setParent(this);
	m_HitTestPanel.setBounds(0, 0, wide, tall);
	m_HitTestPanel.addInputSignal(this);

	Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void ScorePanel::Initialize( void )
{
	// Clear out scoreboard data
	m_iLastKilledBy = 0;
	m_fLastKillTime = 0;
	m_iPlayerNum = 0;
	m_iNumTeams = 0;
	memset( g_PlayerExtraInfo, 0, sizeof g_PlayerExtraInfo );
	memset( g_TeamInfo, 0, sizeof g_TeamInfo );
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the internal scoreboard data
//-----------------------------------------------------------------------------
void ScorePanel::Update()
{
	// Set the title
	if (gViewPort->m_szServerName)
	{
		char sz[MAX_SERVERNAME_LENGTH + 16];
		sprintf(sz, "%s", gViewPort->m_szServerName );
		m_TitleLabel.setText(sz);
	}

	m_iRows = 0;
	gViewPort->GetAllPlayersInfo();

	// Clear out sorts
	for (int i = 0; i < NUM_ROWS; i++)
	{
		m_iSortedRows[i] = 0;
		m_iIsATeam[i] = TEAM_NO;
		m_bHasBeenSorted[i] = false;
	}

	// If it's not teamplay, sort all the players. Otherwise, sort the teams.
	if ( !gHUD.m_Teamplay )
		SortPlayers( 0, NULL );
	else
		SortTeams();

	// set scrollbar range
	m_PlayerList.SetScrollRange(m_iRows);

	FillGrid();
}

//-----------------------------------------------------------------------------
// Purpose: Sort all the teams
//-----------------------------------------------------------------------------
void ScorePanel::SortTeams()
{
	// clear out team scores
	for ( int i = 1; i <= m_iNumTeams; i++ )
	{
		if ( !g_TeamInfo[i].scores_overriden )
			g_TeamInfo[i].frags = g_TeamInfo[i].deaths = 0;
		g_TeamInfo[i].ping = g_TeamInfo[i].packetloss = 0;

		// SaRcaZm - V4 - Start
		g_TeamInfo[i].bestlap = -1.0f;
		// SaRcaZm - V4 - End
	}

	// SaRcaZm - V4 - Start
	// Add a summary row up the top
	g_TeamInfo[0].bestlap = -1.0f;
	g_TeamInfo[0].frags = 0;
	// SaRcaZm - V5 - Start
	g_TeamInfo[0].ping = 0;
	// SaRcaZm - V5 - End
	g_TeamInfo[0].packetloss = 0;
	g_TeamInfo[0].players = 0;

	for ( i = 1; i < MAX_PLAYERS; i++ )
	{
		if ( g_PlayerInfoList[i].name == NULL )
			continue; // empty player slot, skip

		if (g_TeamInfo[0].bestlap == -1.0f)
			g_TeamInfo[0].bestlap = g_PlayerExtraInfo[i].bestlap;
		else if (g_PlayerExtraInfo[i].bestlap < g_TeamInfo[0].bestlap)
			g_TeamInfo[0].bestlap = g_PlayerExtraInfo[i].bestlap;

		g_TeamInfo[0].frags += g_PlayerExtraInfo[i].frags;
		g_TeamInfo[0].ping += g_PlayerInfoList[i].ping;
		g_TeamInfo[0].packetloss += g_PlayerInfoList[i].packetloss;
		g_TeamInfo[0].players++;
	}
	// SaRcaZm - V7 - Start
	if (g_TeamInfo[0].players)
	{
		g_TeamInfo[0].ping /= g_TeamInfo[0].players;  // use the average ping of all the players in the team as the teams ping
		g_TeamInfo[0].packetloss /= g_TeamInfo[0].players;  // use the average ping of all the players in the team as the teams ping
	}
	// SaRcaZm - V7 - End

	m_iSortedRows[m_iRows] = 0;
	m_iIsATeam[m_iRows] = TEAM_YES;
	m_iRows++;

	SortPlayers( 0, NULL );
	// SaRcaZm - V4 - End
}

//-----------------------------------------------------------------------------
// Purpose: Sort a list of players
//-----------------------------------------------------------------------------
void ScorePanel::SortPlayers( int iTeam, char *team )
{
	bool bCreatedTeam = false;

	// draw the players, in order,  and restricted to team if set
	while ( 1 )
	{
		// Find the top ranking player
		// SaRcaZm - V4 - Start
		//int highest_frags = -99999;	int lowest_deaths = 99999;
		int highest_pos = 99999;
		int best_player;
		best_player = 0;

		for ( int i = 1; i < MAX_PLAYERS; i++ )
		{
			//if ( m_bHasBeenSorted[i] == false && g_PlayerInfoList[i].name && g_PlayerExtraInfo[i].frags >= highest_frags )
			if ( m_bHasBeenSorted[i] == false && g_PlayerInfoList[i].name && g_PlayerExtraInfo[i].position <= highest_pos )
			{
				cl_entity_t *ent = gEngfuncs.GetEntityByIndex( i );

				if ( ent && !(team && stricmp(g_PlayerExtraInfo[i].teamname, team)) )  
				{
					// Sort this out according to Position and not frags
					extra_player_info_t *pl_info = &g_PlayerExtraInfo[i];
					//if ( pl_info->frags > highest_frags || pl_info->deaths < lowest_deaths )
					if ((pl_info->position < highest_pos) && (pl_info->position > 0))
					{
						best_player = i;
						//lowest_deaths = pl_info->deaths;
						//highest_frags = pl_info->frags;
						highest_pos = pl_info->position;
					}
				}
			}
		}

		// If we don't have a best player, check to see if there is anyone left
		if (!best_player)
		{
			for (i = 1; i < MAX_PLAYERS; i++)
			{
				if (!m_bHasBeenSorted[i] && g_PlayerInfoList[i].name)
				{
					best_player = i;
					break;
				}
			}
		}
		// SaRcaZm - V4 - End

		if ( !best_player )
			break;

		// If we haven't created the Team yet, do it first
		if (!bCreatedTeam && iTeam)
		{
			m_iIsATeam[ m_iRows ] = iTeam;
			m_iRows++;

			bCreatedTeam = true;
		}

		// Put this player in the sorted list
		m_iSortedRows[ m_iRows ] = best_player;
		m_bHasBeenSorted[ best_player ] = true;
		m_iRows++;
	}

	if (team)
	{
		m_iIsATeam[m_iRows++] = TEAM_BLANK;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the existing teams in the match
//-----------------------------------------------------------------------------
void ScorePanel::RebuildTeams()
{
	// clear out player counts from teams
	for ( int i = 1; i <= m_iNumTeams; i++ )
	{
		g_TeamInfo[i].players = 0;
	}

	// rebuild the team list
	gViewPort->GetAllPlayersInfo();
	m_iNumTeams = 0;
	for ( i = 1; i < MAX_PLAYERS; i++ )
	{
		if ( g_PlayerInfoList[i].name == NULL )
			continue;

		if ( g_PlayerExtraInfo[i].teamname[0] == 0 )
			continue; // skip over players who are not in a team

		// is this player in an existing team?
		for ( int j = 1; j <= m_iNumTeams; j++ )
		{
			if ( g_TeamInfo[j].name[0] == '\0' )
				break;

			if ( !stricmp( g_PlayerExtraInfo[i].teamname, g_TeamInfo[j].name ) )
				break;
		}

		if ( j > m_iNumTeams )
		{ // they aren't in a listed team, so make a new one
			// search through for an empty team slot
			for ( int j = 1; j <= m_iNumTeams; j++ )
			{
				if ( g_TeamInfo[j].name[0] == '\0' )
					break;
			}
			m_iNumTeams = max( j, m_iNumTeams );

			strncpy( g_TeamInfo[j].name, g_PlayerExtraInfo[i].teamname, MAX_TEAM_NAME );
			g_TeamInfo[j].players = 0;
		}

		g_TeamInfo[j].players++;
	}

	// clear out any empty teams
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		if ( g_TeamInfo[i].players < 1 )
			memset( &g_TeamInfo[i], 0, sizeof(team_info_t) );
	}

	// Update the scoreboard
	Update();
}


void ScorePanel::FillGrid()
{
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hScheme = pSchemes->getSchemeHandle("Scoreboard Text");
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle("Scoreboard Title Text");
	SchemeHandle_t hSmallScheme = pSchemes->getSchemeHandle("Scoreboard Small Text");

	Font *sfont = pSchemes->getFont(hScheme);
	Font *tfont = pSchemes->getFont(hTitleScheme);
	Font *smallfont = pSchemes->getFont(hSmallScheme);

	// update highlight position
	int x, y;
	getApp()->getCursorPos(x, y);
	cursorMoved(x, y, this);

	// remove highlight row if we're not in squelch mode
	if (!GetClientVoiceMgr()->IsInSquelchMode())
	{
		m_iHighlightRow = -1;
	}

	bool bNextRowIsGap = false;

	for(int row=0; row < NUM_ROWS; row++)
	{
		CGrid *pGridRow = &m_PlayerGrids[row];
		pGridRow->SetRowUnderline(0, false, 0, 0, 0, 0, 0);

		if(row >= m_iRows)
		{
			for(int col=0; col < NUM_COLUMNS; col++)
				m_PlayerEntries[col][row].setVisible(false);
		
			continue;
		}

		bool bRowIsGap = false;
		if (bNextRowIsGap)
		{
			bNextRowIsGap = false;
			bRowIsGap = true;
		}

		for(int col=0; col < NUM_COLUMNS; col++)
		{
			CLabelHeader *pLabel = &m_PlayerEntries[col][row];

			pLabel->setVisible(true);
			pLabel->setText2("");
			pLabel->setImage(NULL);
			pLabel->setFont(sfont);
			pLabel->setTextOffset(0, 0);
			
			// SaRcaZm - V4 - Start
			pLabel->setCheckeredBg (false);
			// SaRcaZm - V4 - End

			int rowheight = 13;
			if (ScreenHeight > 480)
			{
				rowheight = YRES(rowheight);
			}
			else
			{
				// more tweaking, make sure icons fit at low res
				rowheight = 15;
			}
			pLabel->setSize(pLabel->getWide(), rowheight);
			pLabel->setBgColor(0, 0, 0, 255);
			
			char sz[128];
			hud_player_info_t *pl_info = NULL;
			team_info_t *team_info = NULL;

			if (m_iIsATeam[row] == TEAM_BLANK)
			{
				pLabel->setText(" ");
				continue;
			}
			else if ( m_iIsATeam[row] == TEAM_YES )
			{
				// Get the team's data
				team_info = &g_TeamInfo[ m_iSortedRows[row] ];

				// team color text for team names
				pLabel->setFgColor(	iTeamColors[team_info->teamnumber % iNumberOfTeamColors][0],
									iTeamColors[team_info->teamnumber % iNumberOfTeamColors][1],
									iTeamColors[team_info->teamnumber % iNumberOfTeamColors][2],
									0 );

				// different height for team header rows
				rowheight = 20;
				if (ScreenHeight >= 480)
				{
					rowheight = YRES(rowheight);
				}
				pLabel->setSize(pLabel->getWide(), rowheight);

				// SaRcaZm - V4 - Start
				// If a time column, default font is too large
				if (col == COLUMN_BESTLAP) //|| col == COLUMN_RACETIME)
					pLabel->setFont (smallfont);
				else
				// SaRcaZm - V4 - End
					pLabel->setFont(tfont);

				pGridRow->SetRowUnderline(	0,
											true,
											YRES(3),
											iTeamColors[team_info->teamnumber % iNumberOfTeamColors][0],
											iTeamColors[team_info->teamnumber % iNumberOfTeamColors][1],
											iTeamColors[team_info->teamnumber % iNumberOfTeamColors][2],
											0 );
			}
			else if ( m_iIsATeam[row] == TEAM_SPECTATORS )
			{
				// grey text for spectators
				pLabel->setFgColor(100, 100, 100, 0);

				// different height for team header rows
				rowheight = 20;
				if (ScreenHeight >= 480)
				{
					rowheight = YRES(rowheight);
				}
				pLabel->setSize(pLabel->getWide(), rowheight);
				pLabel->setFont(tfont);

				pGridRow->SetRowUnderline(0, true, YRES(3), 100, 100, 100, 0);
			}
			else
			{
				// team color text for player names
				pLabel->setFgColor(	iTeamColors[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber % iNumberOfTeamColors ][0],
									iTeamColors[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber % iNumberOfTeamColors ][1],
									iTeamColors[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber % iNumberOfTeamColors ][2],
									0 );

				// Get the player's data
				pl_info = &g_PlayerInfoList[ m_iSortedRows[row] ];

				// Set background color
				if ( pl_info->thisplayer ) // if it is their name, draw it a different color
				{
					// Highlight this player
					pLabel->setFgColor(Scheme::sc_white);
					pLabel->setBgColor(	iTeamColors[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber % iNumberOfTeamColors ][0],
										iTeamColors[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber % iNumberOfTeamColors ][1],
										iTeamColors[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber % iNumberOfTeamColors ][2],
										196 );
				}
				else if ( m_iSortedRows[row] == m_iLastKilledBy && m_fLastKillTime && m_fLastKillTime > gHUD.m_flTime )
				{
					// Killer's name
					pLabel->setBgColor( 255,0,0, 255 - ((float)15 * (float)(m_fLastKillTime - gHUD.m_flTime)) );
				}
			}				

			// Align 
			// SaRcaZm - V4 - Start
			if (col == COLUMN_NAME )//|| col == COLUMN_CLASS)
			// SaRcaZm - V4 - End
			{
				pLabel->setContentAlignment( vgui::Label::a_west );
			}
			else if (col == COLUMN_TRACKER)
			{
				pLabel->setContentAlignment( vgui::Label::a_center );
			}
			else
			{
				pLabel->setContentAlignment( vgui::Label::a_east );
			}

			// SaRcaZm - V4 - Start
			//float time;
			int time_ms, time_Seconds, time_Minutes;
			// SaRcaZm - V4 - End

			// Fill out with the correct data
			strcpy(sz, "");
			if ( m_iIsATeam[row] )
			{
				char sz2[128];

				switch (col)
				{
				case COLUMN_NAME:
					if ( m_iIsATeam[row] == TEAM_SPECTATORS )
					{
						sprintf( sz2, CHudTextMessage::BufferedLocaliseTextString( "#Spectators" ) );
					}
					// SaRcaZm - V4 - Start
					else if (m_iSortedRows[row] == 0)
					{
						sprintf (sz, "Competitors");	// Special case for summary row
						break;
					}
					// SaRcaZm - V4 - End
					else
					{
						sprintf( sz2, gViewPort->GetTeamName(team_info->teamnumber) );
					}

					strcpy(sz, sz2);

					// Append the number of players
					if ( m_iIsATeam[row] == TEAM_YES )
					{
						if (team_info->players == 1)
						{
							sprintf(sz2, "(%d %s)", team_info->players, CHudTextMessage::BufferedLocaliseTextString( "#Player" ) );
						}
						else
						{
							sprintf(sz2, "(%d %s)", team_info->players, CHudTextMessage::BufferedLocaliseTextString( "#Player_plural" ) );
						}

						pLabel->setText2(sz2);
						pLabel->setFont2(smallfont);
					}
					break;
				case COLUMN_VOICE:
					break;
				// SaRcaZm - V4 - Start
				/*
				case COLUMN_CLASS:
					break;
				case COLUMN_KILLS:
					if ( m_iIsATeam[row] == TEAM_YES )
						sprintf(sz, "%d",  team_info->frags );
					break;
				case COLUMN_DEATHS:
					if ( m_iIsATeam[row] == TEAM_YES )
						sprintf(sz, "%d",  team_info->deaths );
					break;
				*/
				// SaRcaZm - V4 - End
				case COLUMN_LATENCY:
					if ( m_iIsATeam[row] == TEAM_YES )
						sprintf(sz, "%d", team_info->ping );
					break;

				// SaRcaZm - V4 - Start
				// Team heading values
				case COLUMN_POSITION:
					break;
/*				case COLUMN_RACETIME:
					break;
*/				case COLUMN_SCORE:
					if ( m_iIsATeam[row] == TEAM_YES )
						sprintf(sz, "%d",  team_info->frags );
					break;
				case COLUMN_BESTLAP:
					if (m_iIsATeam[row] == TEAM_YES)
					{
						if (team_info->bestlap == -1)
							sprintf (sz, "-:--.--");
						else
						{
							time_ms = (team_info->bestlap - (int) team_info->bestlap) * 100;
							time_Seconds = ((int) team_info->bestlap) % 60;
							time_Minutes = ((int) team_info->bestlap) / 60;
							sprintf (sz, "%2i:%.2i.%.2i", time_Minutes, time_Seconds, time_ms);
						}
					}
					break;
				case COLUMN_CAR:
					break;
				// SaRcaZm - V4 - End

				default:
					break;
				}
			}
			else
			{
				bool bShowClass = false;

				switch (col)
				{
				case COLUMN_NAME:
					/*
					if (g_pTrackerUser)
					{
						int playerSlot = m_iSortedRows[row];
						int trackerID = gEngfuncs.GetTrackerIDForPlayer(playerSlot);
						const char *trackerName = g_pTrackerUser->GetUserName(trackerID);
						if (trackerName && *trackerName)
						{
							sprintf(sz, "   (%s)", trackerName);
							pLabel->setText2(sz);
						}
					}
					*/
					sprintf(sz, "%s  ", pl_info->name);
					break;
				case COLUMN_VOICE:
					sz[0] = 0;
					// in HLTV mode allow spectator to turn on/off commentator voice
					if (!pl_info->thisplayer || gEngfuncs.IsSpectateOnly() )
					{
						GetClientVoiceMgr()->UpdateSpeakerImage(pLabel, m_iSortedRows[row]);
					}
					break;

				// SaRcaZm - V4 - Start
				/*
				case COLUMN_CLASS:
					// No class for other team's members (unless allied or spectator)
					if ( gViewPort && EV_TFC_IsAllyTeam( g_iTeamNumber, g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber )  )
						bShowClass = true;
					// Don't show classes if this client hasnt picked a team yet
					if ( g_iTeamNumber == 0 )
						bShowClass = false;
#ifdef _TFC
					// in TFC show all classes in spectator mode
					if ( g_iUser1 )
						bShowClass = true;
#endif

					if (bShowClass)
					{
						// Only print Civilian if this team are all civilians
						bool bNoClass = false;
						if ( g_PlayerExtraInfo[ m_iSortedRows[row] ].playerclass == 0 )
						{
							if ( gViewPort->GetValidClasses( g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber ) != -1 )
								bNoClass = true;
						}

						if (bNoClass)
							sprintf(sz, "");
						else
							sprintf( sz, "%s", CHudTextMessage::BufferedLocaliseTextString( sLocalisedClasses[ g_PlayerExtraInfo[ m_iSortedRows[row] ].playerclass ] ) );
					}
					else
					{
						strcpy(sz, "");
					}
					break;
				*/// SaRcaZm - V4 - End

				case COLUMN_TRACKER:
					if (g_pTrackerUser)
					{
						int playerSlot = m_iSortedRows[row];
						int trackerID = gEngfuncs.GetTrackerIDForPlayer(playerSlot);

						if (g_pTrackerUser->IsFriend(trackerID) && trackerID != g_pTrackerUser->GetTrackerID())
						{
							pLabel->setImage(m_pTrackerIcon);
							pLabel->setFgColorAsImageColor(false);
							m_pTrackerIcon->setColor(Color(255, 255, 255, 0));
						}
					}
					break;

#ifdef _TFC
				// SaRcaZm - V4 - Start
				/*
				case COLUMN_KILLS:
					if (g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber)
						sprintf(sz, "%d",  g_PlayerExtraInfo[ m_iSortedRows[row] ].frags );
					break;
				case COLUMN_DEATHS:
					if (g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber)
						sprintf(sz, "%d",  g_PlayerExtraInfo[ m_iSortedRows[row] ].deaths );
					break;
				*/
				case COLUMN_LATENCY:
					if (g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber)
						sprintf(sz, "%d", g_PlayerInfoList[ m_iSortedRows[row] ].ping );
					break;
#else
				/*
				case COLUMN_KILLS:
						sprintf(sz, "%d",  g_PlayerExtraInfo[ m_iSortedRows[row] ].frags );
					break;
				case COLUMN_DEATHS:
						sprintf(sz, "%d",  g_PlayerExtraInfo[ m_iSortedRows[row] ].deaths );
					break;
				*/
				// SaRcaZm - V4 - End
				case COLUMN_LATENCY:
						sprintf(sz, "%d", g_PlayerInfoList[ m_iSortedRows[row] ].ping );
					break;
#endif

				// SaRcaZm - V4 - Start
				// Individual player
				case COLUMN_POSITION:
					sprintf (sz, "%i", g_PlayerExtraInfo[m_iSortedRows[row]].position);
					break;
/*				case COLUMN_RACETIME:
					if (g_PlayerExtraInfo[m_iSortedRows[row]].finished == 0)
						time = (float)((int)(gEngfuncs.GetClientTime () - g_PlayerExtraInfo[m_iSortedRows[row]].racetime));
					else
						time = g_PlayerExtraInfo[m_iSortedRows[row]].racetime;

					time_ms = (time - (int) time) * 100;
					time_Seconds = ((int) time) % 60;
					time_Minutes = ((int) time) / 60;
					sprintf (sz, "%2i:%2i.%2i", time_Minutes, time_Seconds, time_ms);
					break;
*/				case COLUMN_SCORE:
					sprintf (sz, "%d",  g_PlayerExtraInfo[m_iSortedRows[row]].frags );
					break;
				case COLUMN_BESTLAP:
					if (g_PlayerExtraInfo[m_iSortedRows[row]].bestlap == -1)
						sprintf (sz, "-:--.--");
					else
					{
						time_ms = (g_PlayerExtraInfo[m_iSortedRows[row]].bestlap - (int) g_PlayerExtraInfo[m_iSortedRows[row]].bestlap) * 100;
						time_Seconds = ((int) g_PlayerExtraInfo[m_iSortedRows[row]].bestlap) % 60;
						time_Minutes = ((int) g_PlayerExtraInfo[m_iSortedRows[row]].bestlap) / 60;
						sprintf (sz, "%2i:%.2i.%.2i", time_Minutes, time_Seconds, time_ms);
					}
					break;
				case COLUMN_CAR:
					// SaRcaZm - V8 - Start
					if (g_PlayerExtraInfo[m_iSortedRows[row]].teamname[0])
					{
						char model[80];
						strcpy (model, g_PlayerInfoList[m_iSortedRows[row]].model);	// Default
						for (int i = 0; i < MAX_TEAMS; i++)
						{
							for (int j = 0; j < 3; j++)
							{
								if (!stricmp (g_PlayerInfoList[m_iSortedRows[row]].model, rallyinfo[i].szModels[j].szModelName))
									strcpy (model, rallyinfo[i].szModels[j].szDisplayName);
							}
						}

						sprintf (sz, "%s %s", g_PlayerExtraInfo[m_iSortedRows[row]].teamname, model);
					}
					// SaRcaZm - V8 - End
					else
						pLabel->setCheckeredBg (true);
					pLabel->setContentAlignment (vgui::Label::a_west);
					break;
				// SaRcaZm - V4 - End

				default:
					break;
				}
			}

			pLabel->setText(sz);
		}
	}

	for(row=0; row < NUM_ROWS; row++)
	{
		CGrid *pGridRow = &m_PlayerGrids[row];

		pGridRow->AutoSetRowHeights();
		pGridRow->setSize(PanelWidth(pGridRow), pGridRow->CalcDrawHeight());
		pGridRow->RepositionContents();
	}

	// hack, for the thing to resize
	m_PlayerList.getSize(x, y);
	m_PlayerList.setSize(x, y);
}


//-----------------------------------------------------------------------------
// Purpose: Setup highlights for player names in scoreboard
//-----------------------------------------------------------------------------
void ScorePanel::DeathMsg( int killer, int victim )
{
	// if we were the one killed,  or the world killed us, set the scoreboard to indicate suicide
	if ( victim == m_iPlayerNum || killer == 0 )
	{
		m_iLastKilledBy = killer ? killer : m_iPlayerNum;
		m_fLastKillTime = gHUD.m_flTime + 10;	// display who we were killed by for 10 seconds

		if ( killer == m_iPlayerNum )
			m_iLastKilledBy = m_iPlayerNum;
	}
}


void ScorePanel::Open( void )
{
	RebuildTeams();
	setVisible(true);
	m_HitTestPanel.setVisible(true);
}


void ScorePanel::mousePressed(MouseCode code, Panel* panel)
{
	if(gHUD.m_iIntermission)
		return;

	if (!GetClientVoiceMgr()->IsInSquelchMode())
	{
		GetClientVoiceMgr()->StartSquelchMode();
		m_HitTestPanel.setVisible(false);
	}
	else if (m_iHighlightRow >= 0)
	{
		// mouse has been pressed, toggle mute state
		int iPlayer = m_iSortedRows[m_iHighlightRow];
		if (iPlayer > 0)
		{
			// print text message
			hud_player_info_t *pl_info = &g_PlayerInfoList[iPlayer];

			if (pl_info && pl_info->name && pl_info->name[0])
			{
				char string[256];
				if (GetClientVoiceMgr()->IsPlayerBlocked(iPlayer))
				{
					char string1[1024];

					// remove mute
					GetClientVoiceMgr()->SetPlayerBlockedState(iPlayer, false);

					sprintf( string1, CHudTextMessage::BufferedLocaliseTextString( "#Unmuted" ), pl_info->name );
					sprintf( string, "%c** %s\n", HUD_PRINTTALK, string1 );

					gHUD.m_TextMessage.MsgFunc_TextMsg(NULL, strlen(string)+1, string );
				}
				else
				{
					char string1[1024];
					char string2[1024];

					// mute the player
					GetClientVoiceMgr()->SetPlayerBlockedState(iPlayer, true);

					sprintf( string1, CHudTextMessage::BufferedLocaliseTextString( "#Muted" ), pl_info->name );
					sprintf( string2, CHudTextMessage::BufferedLocaliseTextString( "#No_longer_hear_that_player" ) );
					sprintf( string, "%c** %s %s\n", HUD_PRINTTALK, string1, string2 );

					gHUD.m_TextMessage.MsgFunc_TextMsg(NULL, strlen(string)+1, string );
				}
			}
		}
	}
}

void ScorePanel::cursorMoved(int x, int y, Panel *panel)
{
	if (GetClientVoiceMgr()->IsInSquelchMode())
	{
		// look for which cell the mouse is currently over
		for (int i = 0; i < NUM_ROWS; i++)
		{
			int row, col;
			if (m_PlayerGrids[i].getCellAtPoint(x, y, row, col))
			{
				MouseOverCell(i, col);
				return;
			}
		}
	}
}

// SaRcaZm - V4 - Start
void ScorePanel::setVisible (bool state)
{
	// SaRcaZm - V5 - Start
	// Don't show scoreboard if in the vgui
	if ((CRallyVGUIFX::getSingleton()->GetMode () != MODE_VGUI) && (CRallyVGUIFX::getSingleton()->GetMode () != MODE_SHOWROOM))
	// SaRcaZm - V5
	{
		Panel::setVisible (state);
	}
}
// SaRcaZm - V4 - End

//-----------------------------------------------------------------------------
// Purpose: Handles mouse movement over a cell
// Input  : row - 
//			col - 
//-----------------------------------------------------------------------------
void ScorePanel::MouseOverCell(int row, int col)
{
	CLabelHeader *label = &m_PlayerEntries[col][row];

	// clear the previously highlighted label
	if (m_pCurrentHighlightLabel != label)
	{
		m_pCurrentHighlightLabel = NULL;
		m_iHighlightRow = -1;
	}
	if (!label)
		return;

	// don't act on teams
	if (m_iIsATeam[row] != TEAM_NO)
		return;

	// don't act on disconnected players or ourselves
	hud_player_info_t *pl_info = &g_PlayerInfoList[ m_iSortedRows[row] ];
	if (!pl_info->name || !pl_info->name[0])
		return;

	if (pl_info->thisplayer && !gEngfuncs.IsSpectateOnly() )
		return;

	// only act on audible players
	if (!GetClientVoiceMgr()->IsPlayerAudible(m_iSortedRows[row]))
		return;

	// setup the new highlight
	m_pCurrentHighlightLabel = label;
	m_iHighlightRow = row;
}

//-----------------------------------------------------------------------------
// Purpose: Label paint functions - take into account current highligh status
//-----------------------------------------------------------------------------
void CLabelHeader::paintBackground()
{
	Color oldBg;
	getBgColor(oldBg);

	// SaRcaZm - V4 - Start
	// Hack in a checkered background if they have finished the race
	if (_checkeredBg)
	{
		int size = _size[1] / 2;
		int numX = _size[0] / size;

		// White background
		drawSetColor (0, 0, 0, 0);
		drawFilledRect(0, 0, size * numX, _size[1]);

		drawSetColor(255, 255, 255, 0);
		for (int i = 0; i < numX; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				if (i % 2 == j)
				{
					// Draw the check
					drawFilledRect (size * i, size * j, size * (i + 1), size * (j + 1));
				}
			}
		}

		return;
	}
	// SaRcaZm - V4 - End

	if (gViewPort->GetScoreBoard()->m_iHighlightRow == _row)
	{
		setBgColor(134, 91, 19, 0);
	}

	Panel::paintBackground();

	setBgColor(oldBg);
}


//-----------------------------------------------------------------------------
// Purpose: Label paint functions - take into account current highligh status
//-----------------------------------------------------------------------------
void CLabelHeader::paint()
{
	Color oldFg;
	getFgColor(oldFg);

	if (gViewPort->GetScoreBoard()->m_iHighlightRow == _row)
	{
		setFgColor(255, 255, 255, 0);
	}

	// draw text
	int x, y, iwide, itall;
	getTextSize(iwide, itall);
	calcAlignment(iwide, itall, x, y);
	_dualImage.setPos(x, y);

	int x1, y1;
	_dualImage.GetImage(1)->getPos(x1, y1);
	_dualImage.GetImage(1)->setPos(_gap, y1);

	_dualImage.doPaint(this);

	// get size of the panel and the image
	if (_image)
	{
		_image->getSize(iwide, itall);
		calcAlignment(iwide, itall, x, y);
		_image->setPos(x, y);
		_image->doPaint(this);
	}

	setFgColor(oldFg[0], oldFg[1], oldFg[2], oldFg[3]);
}


void CLabelHeader::calcAlignment(int iwide, int itall, int &x, int &y)
{
	// calculate alignment ourselves, since vgui is so broken
	int wide, tall;
	getSize(wide, tall);

	x = 0, y = 0;

	// align left/right
	switch (_contentAlignment)
	{
		// left
		case Label::a_northwest:
		case Label::a_west:
		case Label::a_southwest:
		{
			x = 0;
			break;
		}
		
		// center
		case Label::a_north:
		case Label::a_center:
		case Label::a_south:
		{
			x = (wide - iwide) / 2;
			break;
		}
		
		// right
		case Label::a_northeast:
		case Label::a_east:
		case Label::a_southeast:
		{
			x = wide - iwide;
			break;
		}
	}

	// top/down
	switch (_contentAlignment)
	{
		// top
		case Label::a_northwest:
		case Label::a_north:
		case Label::a_northeast:
		{
			y = 0;
			break;
		}
		
		// center
		case Label::a_west:
		case Label::a_center:
		case Label::a_east:
		{
			y = (tall - itall) / 2;
			break;
		}
		
		// south
		case Label::a_southwest:
		case Label::a_south:
		case Label::a_southeast:
		{
			y = tall - itall;
			break;
		}
	}

// don't clip to Y
//	if (y < 0)
//	{
//		y = 0;
//	}
	if (x < 0)
	{
		x = 0;
	}

	x += _offset[0];
	y += _offset[1];
}

// SaRcaZm - V4 - Start
void CLabelHeader::setCheckeredBg (bool checked)
{
	_checkeredBg = checked;
}
// SaRcaZm - V4 - End
