//**************************************
//     
// Name: ROT 13 Encryption
// Description: Encrypts FragMented's Messages

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#pragma warning(disable:4018)

char * Encrypt(char *sName, char *data)
{
	char *message = "";

	int key = sName[1];
	key = key / 10;

   	int step = 0, y;		// step for array handle
    						// y for ASCII handler 

	for (step = 0; step <= strlen(data); step++)
	{
		y = data[step];		//assign ASCII value


		// lower case encryption:
		if(y>=97)
		{	
			if(y<=109)
			{
				message[step] = y + key; // move up  characters 
			} // if between 'a' to 'm'
	        
			if(y>=110)
			{
				if(y<=122)
				{
					message[step] = y - key; // move down key characters 
				}							 // if between 'n' to 'z'
			}
		}
		else	 // upper case encryption
		{
			if(y>=65)
			{
				if(y<=77)
				{
					message[step] = y + key; // move up key characters if 
				}							 // if between 'A' to 'M'
				if(y>=78)
				{
					if(y<=90)
					{
						message[step] = y - key; // move down key characters
					}							 // if between 'N' to 'Z'
				}
			}
		}
	}

	return message;
}
