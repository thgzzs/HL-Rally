#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../api/inc/fmod.h"

#include "sdriver.h"
#include "resource.h"

BOOL CALLBACK SoundDriverDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC oldcomboproc;
HWND	DeviceComboHwnd;

extern int  setting_buffersize;
extern int  setting_prebuffer_percent;
extern int  setting_rebuffer_percent;
extern char setting_http_proxy[2048];


/*
	Function to call to create dialog box 
*/
char SoundDriver_Init(long *freq)
{
	HWND		hwnd = GetForegroundWindow();
	HINSTANCE	hinst = (HINSTANCE)GetWindowLong(hwnd,GWL_HINSTANCE);

	return DialogBoxParam(hinst,MAKEINTRESOURCE(IDD_DSENUMBOX),hwnd,SoundDriverDlgProc, (LPARAM)freq);
}


/*
	SubClassed windowproc for the 'select output' combobox
*/
long CALLBACK ComboWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch (message)
	{
		case WM_COMMAND:
		{
			int count;

			if (ComboBox_GetCurSel(hwnd) == 0) 
				FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND);
			else if (ComboBox_GetCurSel(hwnd) == 1) 
				FSOUND_SetOutput(FSOUND_OUTPUT_WINMM);
			else
				FSOUND_SetOutput(FSOUND_OUTPUT_ASIO);

			SendMessage(DeviceComboHwnd, CB_RESETCONTENT, 0, 0);

			for (count=0; count < FSOUND_GetNumDrivers(); count++)
				ComboBox_AddString(DeviceComboHwnd, FSOUND_GetDriverName(count));

			ComboBox_SetCurSel(DeviceComboHwnd,0);
			UpdateWindow(DeviceComboHwnd);

			break;
		}

	};
	
	return oldcomboproc(hwnd, message, wParam, lParam);
}


/*
	Window proc for dialog box
*/
BOOL CALLBACK SoundDriverDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hCombo, hRadio;
	static long *freq;
	static long lastoutput=0, lastdriver=0, lastmixer=0, lastoutputrate=44100;

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			int count;
            int buffersize, prebuffer_percent, rebuffer_percent;
            char str[1024];

			/*
                Remember what came in last
            */
			lastoutput	    = FSOUND_GetOutput();
			lastdriver      = FSOUND_GetDriver();
			lastmixer	    = FSOUND_GetMixer();
			lastoutputrate  = FSOUND_GetOutputRate();

			if (lastmixer > FSOUND_MIXER_QUALITY_MMXP6)
            {
				lastmixer = FSOUND_MIXER_QUALITY_MMXP6;
            }

			if (lastoutput < FSOUND_OUTPUT_WINMM || lastoutput > FSOUND_OUTPUT_ASIO)
            {
				lastoutput = FSOUND_OUTPUT_DSOUND;		/* somehow lastoutput got corrupted */
            }
			if (lastdriver < 0)
            {
				lastdriver = 0;		/* somehow lastdriver got corrupted */
            }

			FSOUND_SetOutput(lastoutput);

			/*
                SET UP OUTPUT COMBO BOX
            */
			hCombo = GetDlgItem(hwnd,IDC_DSENUM_COMBO3);
			ComboBox_AddString(hCombo,"Direct Sound");
			ComboBox_AddString(hCombo,"Windows Multimedia WaveOut");
			ComboBox_AddString(hCombo,"ASIO Low latency");
			
			if (lastoutput == FSOUND_OUTPUT_DSOUND) 
            {
				ComboBox_SetCurSel(hCombo,0);
            }
			if (lastoutput == FSOUND_OUTPUT_WINMM) 
            {
				ComboBox_SetCurSel(hCombo,1);
            }
			if (lastoutput == FSOUND_OUTPUT_ASIO) 
            {
				ComboBox_SetCurSel(hCombo,2);
            }

			/*
                Subclass this combo box
            */
			oldcomboproc = (WNDPROC)GetWindowLong(hCombo, GWL_WNDPROC);
			SetWindowLong(hCombo, GWL_WNDPROC, (LONG)ComboWindowProc);

			/*
                SET UP MIXER COMBO BOX
            */
			hCombo = GetDlgItem(hwnd,IDC_DSENUM_COMBO2);
			ComboBox_AddString(hCombo,"Autodetect");
			ComboBox_AddString(hCombo,"Interpolation/Volume Ramping - FPU ");
			ComboBox_AddString(hCombo,"Interpolation/Volume Ramping - Pentium MMX");
			ComboBox_AddString(hCombo,"Interpolation/Volume Ramping - P6/P2/P3+ MMX");

			if (lastmixer == FSOUND_MIXER_QUALITY_AUTODETECT)		ComboBox_SetCurSel(hCombo,0);
			if (lastmixer == FSOUND_MIXER_QUALITY_FPU)				ComboBox_SetCurSel(hCombo,1);
			if (lastmixer == FSOUND_MIXER_QUALITY_MMXP5)			ComboBox_SetCurSel(hCombo,2);
			if (lastmixer == FSOUND_MIXER_QUALITY_MMXP6)			ComboBox_SetCurSel(hCombo,3);

			/*
                SET UP DRIVER COMBO BOX
            */
			hCombo = GetDlgItem(hwnd,IDC_DSENUM_COMBO);
			DeviceComboHwnd = hCombo;
			for (count=0; count < FSOUND_GetNumDrivers(); count++)
            {
				ComboBox_AddString(hCombo,FSOUND_GetDriverName(count));
            }

			if (ComboBox_GetCount(hCombo)) 
            {
				ComboBox_SetCurSel(hCombo, lastdriver);
            }

			/*
                SET UP FREQUENCY RADIO BUTTON
            */
			if      (lastoutputrate == 48000) 	hRadio = GetDlgItem(hwnd,IDC_RADIO5);
			else if (lastoutputrate == 44100) 	hRadio = GetDlgItem(hwnd,IDC_RADIO1);
			else if (lastoutputrate == 22050) 	hRadio = GetDlgItem(hwnd,IDC_RADIO2);
			else if (lastoutputrate == 11025) 	hRadio = GetDlgItem(hwnd,IDC_RADIO3);
			else if (lastoutputrate == 8000)	hRadio = GetDlgItem(hwnd,IDC_RADIO4);
			else 								hRadio = GetDlgItem(hwnd,IDC_RADIO1);

			freq = (long *)lParam;
			Button_SetCheck(hRadio, TRUE);

			/*
                SET UP INTERNET STREAMING VALUES
            */
            FSOUND_Stream_Net_GetBufferProperties(&buffersize, &prebuffer_percent, &rebuffer_percent);
            sprintf(str, "%d", buffersize);
            SetWindowText(GetDlgItem(hwnd, IDC_CONFIG_BUFFERSIZE), str);
            sprintf(str, "%d", prebuffer_percent);
            SetWindowText(GetDlgItem(hwnd, IDC_CONFIG_INITIALPERCENT), str);
            sprintf(str, "%d", rebuffer_percent);
            SetWindowText(GetDlgItem(hwnd, IDC_CONFIG_REBUFFERPERCENT), str);
            SetWindowText(GetDlgItem(hwnd, IDC_CONFIG_PROXY), setting_http_proxy);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK :
                {
                    int buffersize, prebuffer_percent, rebuffer_percent;
                    char str[1024];

					hCombo = GetDlgItem(hwnd,IDC_DSENUM_COMBO);
					
					FSOUND_SetDriver((char)ComboBox_GetCurSel(hCombo));
					
					if	    (Button_GetCheck(GetDlgItem(hwnd,IDC_RADIO5))) *freq = 48000;
					else if (Button_GetCheck(GetDlgItem(hwnd,IDC_RADIO1))) *freq = 44100;
					else if (Button_GetCheck(GetDlgItem(hwnd,IDC_RADIO2))) *freq = 22050;
					else if (Button_GetCheck(GetDlgItem(hwnd,IDC_RADIO3))) *freq = 11025;
					else if (Button_GetCheck(GetDlgItem(hwnd,IDC_RADIO4))) *freq = 8000;

					hCombo = GetDlgItem(hwnd,IDC_DSENUM_COMBO2);
					switch (ComboBox_GetCurSel(hCombo))
					{
						case 0: 
							FSOUND_SetMixer(FSOUND_MIXER_QUALITY_AUTODETECT);
							break;
						case 1: 
							FSOUND_SetMixer(FSOUND_MIXER_QUALITY_FPU);
							break;
						case 2: 
							FSOUND_SetMixer(FSOUND_MIXER_QUALITY_MMXP5);
							break;
						case 3: 
							FSOUND_SetMixer(FSOUND_MIXER_QUALITY_MMXP6);
							break;
					}

                    GetWindowText(GetDlgItem(hwnd, IDC_CONFIG_BUFFERSIZE), str, 1023);
                    buffersize = atoi(str);
                    GetWindowText(GetDlgItem(hwnd, IDC_CONFIG_INITIALPERCENT), str, 1023);
                    prebuffer_percent = atoi(str);
                    GetWindowText(GetDlgItem(hwnd, IDC_CONFIG_REBUFFERPERCENT), str, 1023);
                    rebuffer_percent = atoi(str);

                    if (buffersize < 8192)
                    {
                        MessageBox(hwnd, "Buffer size too small!", "Warning", MB_OK | MB_ICONWARNING);
                        return TRUE;
                    }

                    if ((prebuffer_percent <= 0) || (prebuffer_percent > 99))
                    {
                        MessageBox(hwnd, "Initial buffer percent must be between 1 - 99", "Warning", MB_OK | MB_ICONWARNING);
                        return TRUE;
                    }

                    if ((rebuffer_percent <= 0) || (rebuffer_percent > 99))
                    {
                        MessageBox(hwnd, "Rebuffer percent must be between 1 - 99", "Warning", MB_OK | MB_ICONWARNING);
                        return TRUE;
                    }

                    setting_buffersize        = buffersize;
                    setting_prebuffer_percent = prebuffer_percent;
                    setting_rebuffer_percent  = rebuffer_percent;
                    FSOUND_Stream_Net_SetBufferProperties(setting_buffersize, setting_prebuffer_percent, setting_rebuffer_percent);

                    GetWindowText(GetDlgItem(hwnd, IDC_CONFIG_PROXY), setting_http_proxy, 2047);
                    FSOUND_Stream_Net_SetProxy(setting_http_proxy);

					EndDialog(hwnd,TRUE);
					return TRUE;
                }

				case IDCANCEL:
					FSOUND_SetOutput(lastoutput);
					FSOUND_SetDriver(lastdriver);
					FSOUND_SetMixer(lastmixer);

					EndDialog(hwnd, FALSE);
					return TRUE;
			}
			break;
		}
		default:
		{
			return FALSE;
		}
	}

	return FALSE;
}

