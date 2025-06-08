/*
 * Header file for the message encryption
 */

#ifndef _CryptMessage_H
#define _CryptMessage_H

// Returns a heap allocated buffer containing the encrypted message. The caller
// must free the returned pointer with delete[]
extern char *Encrypt(const char *sName, const char *data);

#endif

