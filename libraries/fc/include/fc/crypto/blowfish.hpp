////////////////////////////////////////////////////////////////////////////
///
// Blowfish.h Header File
//
//    BLOWFISH ENCRYPTION ALGORITHM
//
//    encryption and decryption of Byte Strings using the Blowfish encryption Algorithm.
//    Blowfish is a block cipher that encrypts data in 8-byte blocks. The algorithm consists
//    of two parts: a key-expansion part and a data-ancryption part. Key expansion converts a
//    variable key of at least 1 and at most 56 bytes into several subkey arrays totaling
//    4168 bytes. Blowfish has 16 rounds. Each round consists of a key-dependent permutation,
//    and a key and data-dependent substitution. All operations are XORs and additions on 32-bit words.
//    The only additional operations are four indexed array data lookups per round.
//    Blowfish uses a large number of subkeys. These keys must be precomputed before any data
//    encryption or decryption. The P-array consists of 18 32-bit subkeys: P0, P1,...,P17.
//    There are also four 32-bit S-boxes with 256 entries each: S0,0, S0,1,...,S0,255;
//    S1,0, S1,1,...,S1,255; S2,0, S2,1,...,S2,255; S3,0, S3,1,...,S3,255;
//
//    The Electronic Code Book (ECB), Cipher Block Chaining (CBC) and Cipher Feedback modes
//    are used:
//
//    In ECB mode if the same block is encrypted twice with the same key, the resulting
//    ciphertext blocks are the same.
//
//    In CBC Mode a ciphertext block is obtained by first xoring the
//    plaintext block with the previous ciphertext block, and encrypting the resulting value.
//
//    In CFB mode a ciphertext block is obtained by encrypting the previous ciphertext block
//    and xoring the resulting value with the plaintext
//
//    The previous ciphertext block is usually stored in an Initialization Vector (IV).
//    An Initialization Vector of zero is commonly used for the first block, though other
//    arrangements are also in use.

/*
http://www.counterpane.com/vectors.txt
Test vectors by Eric Young.  These tests all assume Blowfish with 16
rounds.

All data is shown as a hex string with 012345 loading as
data[0]=0x01;
data[1]=0x23;
data[2]=0x45;
ecb test data (taken from the DES validation tests)

key bytes               clear bytes             cipher bytes
0000000000000000        0000000000000000        4EF997456198DD78
FFFFFFFFFFFFFFFF        FFFFFFFFFFFFFFFF        51866FD5B85ECB8A
3000000000000000        1000000000000001        7D856F9A613063F2  ???
1111111111111111        1111111111111111        2466DD878B963C9D
0123456789ABCDEF        1111111111111111        61F9C3802281B096
1111111111111111        0123456789ABCDEF        7D0CC630AFDA1EC7
0000000000000000        0000000000000000        4EF997456198DD78
FEDCBA9876543210        0123456789ABCDEF        0ACEAB0FC6A0A28D
7CA110454A1A6E57        01A1D6D039776742        59C68245EB05282B
0131D9619DC1376E        5CD54CA83DEF57DA        B1B8CC0B250F09A0
07A1133E4A0B2686        0248D43806F67172        1730E5778BEA1DA4
3849674C2602319E        51454B582DDF440A        A25E7856CF2651EB
04B915BA43FEB5B6        42FD443059577FA2        353882B109CE8F1A
0113B970FD34F2CE        059B5E0851CF143A        48F4D0884C379918
0170F175468FB5E6        0756D8E0774761D2        432193B78951FC98
43297FAD38E373FE        762514B829BF486A        13F04154D69D1AE5
07A7137045DA2A16        3BDD119049372802        2EEDDA93FFD39C79
04689104C2FD3B2F        26955F6835AF609A        D887E0393C2DA6E3
37D06BB516CB7546        164D5E404F275232        5F99D04F5B163969
1F08260D1AC2465E        6B056E18759F5CCA        4A057A3B24D3977B
584023641ABA6176        004BD6EF09176062        452031C1E4FADA8E
025816164629B007        480D39006EE762F2        7555AE39F59B87BD
49793EBC79B3258F        437540C8698F3CFA        53C55F9CB49FC019
4FB05E1515AB73A7        072D43A077075292        7A8E7BFA937E89A3
49E95D6D4CA229BF        02FE55778117F12A        CF9C5D7A4986ADB5
018310DC409B26D6        1D9D5C5018F728C2        D1ABB290658BC778
1C587F1C13924FEF        305532286D6F295A        55CB3774D13EF201
0101010101010101        0123456789ABCDEF        FA34EC4847B268B2
1F1F1F1F0E0E0E0E        0123456789ABCDEF        A790795108EA3CAE
E0FEE0FEF1FEF1FE        0123456789ABCDEF        C39E072D9FAC631D
0000000000000000        FFFFFFFFFFFFFFFF        014933E0CDAFF6E4
FFFFFFFFFFFFFFFF        0000000000000000        F21E9A77B71C49BC
0123456789ABCDEF        0000000000000000        245946885754369A
FEDCBA9876543210        FFFFFFFFFFFFFFFF        6B5C5A9C5D9E0A5A

set_key test data
data[8]= FEDCBA9876543210
c=F9AD597C49DB005E k[ 1]=F0
c=E91D21C1D961A6D6 k[ 2]=F0E1
c=E9C2B70A1BC65CF3 k[ 3]=F0E1D2
c=BE1E639408640F05 k[ 4]=F0E1D2C3
c=B39E44481BDB1E6E k[ 5]=F0E1D2C3B4
c=9457AA83B1928C0D k[ 6]=F0E1D2C3B4A5
c=8BB77032F960629D k[ 7]=F0E1D2C3B4A596
c=E87A244E2CC85E82 k[ 8]=F0E1D2C3B4A59687
c=15750E7A4F4EC577 k[ 9]=F0E1D2C3B4A5968778
c=122BA70B3AB64AE0 k[10]=F0E1D2C3B4A596877869
c=3A833C9AFFC537F6 k[11]=F0E1D2C3B4A5968778695A
c=9409DA87A90F6BF2 k[12]=F0E1D2C3B4A5968778695A4B
c=884F80625060B8B4 k[13]=F0E1D2C3B4A5968778695A4B3C
c=1F85031C19E11968 k[14]=F0E1D2C3B4A5968778695A4B3C2D
c=79D9373A714CA34F k[15]=F0E1D2C3B4A5968778695A4B3C2D1E ???
c=93142887EE3BE15C k[16]=F0E1D2C3B4A5968778695A4B3C2D1E0F
c=03429E838CE2D14B k[17]=F0E1D2C3B4A5968778695A4B3C2D1E0F00
c=A4299E27469FF67B k[18]=F0E1D2C3B4A5968778695A4B3C2D1E0F0011
c=AFD5AED1C1BC96A8 k[19]=F0E1D2C3B4A5968778695A4B3C2D1E0F001122
c=10851C0E3858DA9F k[20]=F0E1D2C3B4A5968778695A4B3C2D1E0F00112233
c=E6F51ED79B9DB21F k[21]=F0E1D2C3B4A5968778695A4B3C2D1E0F0011223344
c=64A6E14AFD36B46F k[22]=F0E1D2C3B4A5968778695A4B3C2D1E0F001122334455
c=80C7D7D45A5479AD k[23]=F0E1D2C3B4A5968778695A4B3C2D1E0F00112233445566
c=05044B62FA52D080 k[24]=F0E1D2C3B4A5968778695A4B3C2D1E0F0011223344556677

chaining mode test data
key[16]   = 0123456789ABCDEFF0E1D2C3B4A59687
iv[8]     = FEDCBA9876543210
data[29]  = "7654321 Now is the time for " (includes trailing '\0')
data[29]  = 37363534333231204E6F77206973207468652074696D6520666F722000
cbc cipher text
cipher[32]= 6B77B4D63006DEE605B156E27403979358DEB9E7154616D959F1652BD5FF92CC
cfb64 cipher text cipher[29]= 
E73214A2822139CAF26ECF6D2EB9E76E3DA3DE04D1517200519D57A6C3 
ofb64 cipher text cipher[29]= 
E73214A2822139CA62B343CC5B65587310DD908D0C241B2263C2CF80DA

*/

#pragma once
#include <stdint.h>

namespace fc {

//Block Structure
struct sblock
{
	//Constructors
	sblock(unsigned int l=0, unsigned int r=0) : m_uil(l), m_uir(r) {}
	//Copy Constructor
	sblock(const sblock& roBlock) : m_uil(roBlock.m_uil), m_uir(roBlock.m_uir) {}
	sblock& operator^=(sblock& b) { m_uil ^= b.m_uil; m_uir ^= b.m_uir; return *this; }
	unsigned int m_uil, m_uir;
};

class blowfish
{
public:
	enum { ECB=0, CBC=1, CFB=2 };

	//Constructor - Initialize the P and S boxes for a given Key
	blowfish();
	void start(unsigned char* ucKey, uint64_t n, const sblock& roChain = sblock(0UL,0UL));

	//Resetting the chaining block
	void reset_chain() { m_oChain = m_oChain0; }

	// encrypt/decrypt Buffer in Place
	void encrypt(unsigned char* buf, uint64_t n, int iMode=CFB);
	void decrypt(unsigned char* buf, uint64_t n, int iMode=CFB);

	// encrypt/decrypt from Input Buffer to Output Buffer
	void encrypt(const unsigned char* in, unsigned char* out, uint64_t n, int iMode=CFB);
	void decrypt(const unsigned char* in, unsigned char* out, uint64_t n, int iMode=CFB);

//Private Functions
private:
	unsigned int F(unsigned int ui);
	void encrypt(sblock&);
	void decrypt(sblock&);

private:
	//The Initialization Vector, by default {0, 0}
	sblock m_oChain0;
	sblock m_oChain;
	unsigned int m_auiP[18];
	unsigned int m_auiS[4][256];
	static const unsigned int scm_auiInitP[18];
	static const unsigned int scm_auiInitS[4][256];
};


} // namespace fc


