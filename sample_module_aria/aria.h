/*
 *
 *
 * ANSI-C code for ARIA version 1.0
 *
 * Reference code for ARIA
 *
 * 2005. 01. 17.
 * 
 * Note:
 *    - Now we use the following interface:
 *      void Crypt(const Byte *plainText, int numberOfRounds, const Byte *roundKeys, Byte *cipherText);
 *      int EncKeySetup(const Byte *masterKey, Byte *roundKeys, int keyBits);
 *      int DecKeySetup(const Byte *masterKey, Byte *roundKeys, int keyBits);
 *    - EncKeySetup() and DecKeySetup() return the number of rounds.
 *
 */

typedef unsigned char Byte;

// Encryption and decryption rountine
// p: plain text, e: round keys, c: ciphertext
void Crypt(const Byte *p, int R, const Byte *e, Byte *c);

// Encryption round key generation rountine
// w0 : master key, e : encryption round keys
int EncKeySetup(const Byte *w0, Byte *e, int keyBits);

// Decryption round key generation rountine
// w0 : maskter key, d : decryption round keys
int DecKeySetup(const Byte *w0, Byte *d, int keyBits);

