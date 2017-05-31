// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


//
// Why base-58 instead of standard base-64 encoding?
// - Don't want 0OIl characters that look the same in some fonts and
//      could be used to create visually identical looking account numbers.
// - A string with non-alphanumeric characters is not as easily accepted as an account number.
// - E-mail usually won't line-break if there's no punctuation to break at.
// - Doubleclicking selects the whole number as one word if it's all alphanumeric.
//
#ifndef BITCOIN_BASE58_H
#define BITCOIN_BASE58_H

#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include <string.h>

#include <fc/log/logger.hpp>
#include <fc/string.hpp>
#include <fc/exception/exception.hpp>

#include <stdexcept>
#include <vector>
#include <openssl/bn.h>

/** Errors thrown by the bignum class */
class bignum_error : public std::runtime_error
{
public:
    explicit bignum_error(const std::string& str) : std::runtime_error(str) {}
};


/** RAII encapsulated BN_CTX (OpenSSL bignum context) */
class CAutoBN_CTX
{
protected:
    BN_CTX* pctx;
    BN_CTX* operator=(BN_CTX* pnew) { return pctx = pnew; }

public:
    CAutoBN_CTX()
    {
        pctx = BN_CTX_new();
        if (pctx == NULL)
            throw bignum_error("CAutoBN_CTX : BN_CTX_new() returned NULL");
    }

    ~CAutoBN_CTX()
    {
        if (pctx != NULL)
            BN_CTX_free(pctx);
    }

    operator BN_CTX*() { return pctx; }
    BN_CTX& operator*() { return *pctx; }
    BN_CTX** operator&() { return &pctx; }
    bool operator!() { return (pctx == NULL); }
};


/** C++ wrapper for BIGNUM (OpenSSL bignum) */
class CBigNum : public BIGNUM
{
public:
    CBigNum()
    {
        BN_init(this);
    }

    CBigNum(const CBigNum& b)
    {
        BN_init(this);
        if (!BN_copy(this, &b))
        {
            BN_clear_free(this);
            throw bignum_error("CBigNum::CBigNum(const CBigNum&) : BN_copy failed");
        }
    }

    CBigNum& operator=(const CBigNum& b)
    {
        if (!BN_copy(this, &b))
            throw bignum_error("CBigNum::operator= : BN_copy failed");
        return (*this);
    }

    ~CBigNum()
    {
        BN_clear_free(this);
    }

    //CBigNum(char n) is not portable.  Use 'signed char' or 'unsigned char'.
    CBigNum(signed char n)      { BN_init(this); if (n >= 0) setulong(n); else setint64(n); }
    CBigNum(short n)            { BN_init(this); if (n >= 0) setulong(n); else setint64(n); }
    CBigNum(int n)              { BN_init(this); if (n >= 0) setulong(n); else setint64(n); }
    //CBigNum(long n)             { BN_init(this); if (n >= 0) setulong(n); else setint64(n); }
    CBigNum(int64_t n)          { BN_init(this); setint64(n); }
    CBigNum(unsigned char n)    { BN_init(this); setulong(n); }
    CBigNum(unsigned short n)   { BN_init(this); setulong(n); }
    CBigNum(unsigned int n)     { BN_init(this); setulong(n); }
    //CBigNum(unsigned long n)    { BN_init(this); setulong(n); }
    CBigNum(uint64_t n)           { BN_init(this); setuint64(n); }

    explicit CBigNum(const std::vector<unsigned char>& vch)
    {
        BN_init(this);
        setvch(vch);
    }

    void setulong(unsigned long n)
    {
        if (!BN_set_word(this, n))
            throw bignum_error("CBigNum conversion from unsigned long : BN_set_word failed");
    }

    unsigned long getulong() const
    {
        return BN_get_word(this);
    }

    unsigned int getuint() const
    {
        return BN_get_word(this);
    }

    int getint() const
    {
        unsigned long n = BN_get_word(this);
        if (!BN_is_negative(this))
            return (n > (unsigned long)std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : n);
        else
            return (n > (unsigned long)std::numeric_limits<int>::max() ? std::numeric_limits<int>::min() : -(int)n);
    }

    void setint64(int64_t n)
    {
        unsigned char pch[sizeof(n) + 6];
        unsigned char* p = pch + 4;
        bool fNegative = false;
        if (n < (int64_t)0)
        {
            n = -n;
            fNegative = true;
        }
        bool fLeadingZeroes = true;
        for (int i = 0; i < 8; i++)
        {
            unsigned char c = (n >> 56) & 0xff;
            n <<= 8;
            if (fLeadingZeroes)
            {
                if (c == 0)
                    continue;
                if (c & 0x80)
                    *p++ = (fNegative ? 0x80 : 0);
                else if (fNegative)
                    c |= 0x80;
                fLeadingZeroes = false;
            }
            *p++ = c;
        }
        unsigned int nSize = p - (pch + 4);
        pch[0] = (nSize >> 24) & 0xff;
        pch[1] = (nSize >> 16) & 0xff;
        pch[2] = (nSize >> 8) & 0xff;
        pch[3] = (nSize) & 0xff;
        BN_mpi2bn(pch, p - pch, this);
    }

    void setuint64(uint64_t n)
    {
        unsigned char pch[sizeof(n) + 6];
        unsigned char* p = pch + 4;
        bool fLeadingZeroes = true;
        for (int i = 0; i < 8; i++)
        {
            unsigned char c = (n >> 56) & 0xff;
            n <<= 8;
            if (fLeadingZeroes)
            {
                if (c == 0)
                    continue;
                if (c & 0x80)
                    *p++ = 0;
                fLeadingZeroes = false;
            }
            *p++ = c;
        }
        unsigned int nSize = p - (pch + 4);
        pch[0] = (nSize >> 24) & 0xff;
        pch[1] = (nSize >> 16) & 0xff;
        pch[2] = (nSize >> 8) & 0xff;
        pch[3] = (nSize) & 0xff;
        BN_mpi2bn(pch, p - pch, this);
    }


    void setvch(const std::vector<unsigned char>& vch)
    {
        std::vector<unsigned char> vch2(vch.size() + 4);
        unsigned int nSize = vch.size();
        // BIGNUM's byte stream format expects 4 bytes of
        // big endian size data info at the front
        vch2[0] = (nSize >> 24) & 0xff;
        vch2[1] = (nSize >> 16) & 0xff;
        vch2[2] = (nSize >> 8) & 0xff;
        vch2[3] = (nSize >> 0) & 0xff;
        // swap data to big endian
        reverse_copy(vch.begin(), vch.end(), vch2.begin() + 4);
        BN_mpi2bn(&vch2[0], vch2.size(), this);
    }

    std::vector<unsigned char> getvch() const
    {
        unsigned int nSize = BN_bn2mpi(this, NULL);
        if (nSize <= 4)
            return std::vector<unsigned char>();
        std::vector<unsigned char> vch(nSize);
        BN_bn2mpi(this, &vch[0]);
        vch.erase(vch.begin(), vch.begin() + 4);
        reverse(vch.begin(), vch.end());
        return vch;
    }

    CBigNum& SetCompact(unsigned int nCompact)
    {
        unsigned int nSize = nCompact >> 24;
        std::vector<unsigned char> vch(4 + nSize);
        vch[3] = nSize;
        if (nSize >= 1) vch[4] = (nCompact >> 16) & 0xff;
        if (nSize >= 2) vch[5] = (nCompact >> 8) & 0xff;
        if (nSize >= 3) vch[6] = (nCompact >> 0) & 0xff;
        BN_mpi2bn(&vch[0], vch.size(), this);
        return *this;
    }

    unsigned int GetCompact() const
    {
        unsigned int nSize = BN_bn2mpi(this, NULL);
        std::vector<unsigned char> vch(nSize);
        nSize -= 4;
        BN_bn2mpi(this, &vch[0]);
        unsigned int nCompact = nSize << 24;
        if (nSize >= 1) nCompact |= (vch[4] << 16);
        if (nSize >= 2) nCompact |= (vch[5] << 8);
        if (nSize >= 3) nCompact |= (vch[6] << 0);
        return nCompact;
    }

    void SetHex(const std::string& str)
    {
        // skip 0x
        const char* psz = str.c_str();
        while (isspace(*psz))
            psz++;
        bool fNegative = false;
        if (*psz == '-')
        {
            fNegative = true;
            psz++;
        }
        if (psz[0] == '0' && tolower(psz[1]) == 'x')
            psz += 2;
        while (isspace(*psz))
            psz++;

        // hex string to bignum
        static signed char phexdigit[256] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0, 0,0xa,0xb,0xc,0xd,0xe,0xf,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0xa,0xb,0xc,0xd,0xe,0xf,0,0,0,0,0,0,0,0,0 };
        *this = 0;
        while (isxdigit(*psz))
        {
            *this <<= 4;
            int n = phexdigit[(unsigned char)*psz++];
            *this += n;
        }
        if (fNegative)
            *this = 0 - *this;
    }

    std::string ToString(int nBase=10) const
    {
        CAutoBN_CTX pctx;
        CBigNum bnBase = nBase;
        CBigNum bn0 = 0;
        std::string str;
        CBigNum bn = *this;
        BN_set_negative(&bn, false);
        CBigNum dv;
        CBigNum rem;
        if (BN_cmp(&bn, &bn0) == 0)
            return "0";
        while (BN_cmp(&bn, &bn0) > 0)
        {
            if (!BN_div(&dv, &rem, &bn, &bnBase, pctx))
                throw bignum_error("CBigNum::ToString() : BN_div failed");
            bn = dv;
            unsigned int c = rem.getulong();
            str += "0123456789abcdef"[c];
        }
        if (BN_is_negative(this))
            str += "-";
        reverse(str.begin(), str.end());
        return str;
    }

    std::string GetHex() const
    {
        return ToString(16);
    }



    bool operator!() const
    {
        return BN_is_zero(this);
    }

    CBigNum& operator+=(const CBigNum& b)
    {
        if (!BN_add(this, this, &b))
            throw bignum_error("CBigNum::operator+= : BN_add failed");
        return *this;
    }

    CBigNum& operator-=(const CBigNum& b)
    {
        *this = *this - b;
        return *this;
    }

    CBigNum& operator*=(const CBigNum& b)
    {
        CAutoBN_CTX pctx;
        if (!BN_mul(this, this, &b, pctx))
            throw bignum_error("CBigNum::operator*= : BN_mul failed");
        return *this;
    }

    CBigNum& operator/=(const CBigNum& b)
    {
        *this = *this / b;
        return *this;
    }

    CBigNum& operator%=(const CBigNum& b)
    {
        *this = *this % b;
        return *this;
    }

    CBigNum& operator<<=(unsigned int shift)
    {
        if (!BN_lshift(this, this, shift))
            throw bignum_error("CBigNum:operator<<= : BN_lshift failed");
        return *this;
    }

    CBigNum& operator>>=(unsigned int shift)
    {
        // Note: BN_rshift segfaults on 64-bit if 2^shift is greater than the number
        //   if built on ubuntu 9.04 or 9.10, probably depends on version of openssl
        CBigNum a = 1;
        a <<= shift;
        if (BN_cmp(&a, this) > 0)
        {
            *this = 0;
            return *this;
        }

        if (!BN_rshift(this, this, shift))
            throw bignum_error("CBigNum:operator>>= : BN_rshift failed");
        return *this;
    }


    CBigNum& operator++()
    {
        // prefix operator
        if (!BN_add(this, this, BN_value_one()))
            throw bignum_error("CBigNum::operator++ : BN_add failed");
        return *this;
    }

    const CBigNum operator++(int)
    {
        // postfix operator
        const CBigNum ret = *this;
        ++(*this);
        return ret;
    }

    CBigNum& operator--()
    {
        // prefix operator
        CBigNum r;
        if (!BN_sub(&r, this, BN_value_one()))
            throw bignum_error("CBigNum::operator-- : BN_sub failed");
        *this = r;
        return *this;
    }

    const CBigNum operator--(int)
    {
        // postfix operator
        const CBigNum ret = *this;
        --(*this);
        return ret;
    }


    friend inline const CBigNum operator-(const CBigNum& a, const CBigNum& b);
    friend inline const CBigNum operator/(const CBigNum& a, const CBigNum& b);
    friend inline const CBigNum operator%(const CBigNum& a, const CBigNum& b);
};



inline const CBigNum operator+(const CBigNum& a, const CBigNum& b)
{
    CBigNum r;
    if (!BN_add(&r, &a, &b))
        throw bignum_error("CBigNum::operator+ : BN_add failed");
    return r;
}

inline const CBigNum operator-(const CBigNum& a, const CBigNum& b)
{
    CBigNum r;
    if (!BN_sub(&r, &a, &b))
        throw bignum_error("CBigNum::operator- : BN_sub failed");
    return r;
}

inline const CBigNum operator-(const CBigNum& a)
{
    CBigNum r(a);
    BN_set_negative(&r, !BN_is_negative(&r));
    return r;
}

inline const CBigNum operator*(const CBigNum& a, const CBigNum& b)
{
    CAutoBN_CTX pctx;
    CBigNum r;
    if (!BN_mul(&r, &a, &b, pctx))
        throw bignum_error("CBigNum::operator* : BN_mul failed");
    return r;
}

inline const CBigNum operator/(const CBigNum& a, const CBigNum& b)
{
    CAutoBN_CTX pctx;
    CBigNum r;
    if (!BN_div(&r, NULL, &a, &b, pctx))
        throw bignum_error("CBigNum::operator/ : BN_div failed");
    return r;
}

inline const CBigNum operator%(const CBigNum& a, const CBigNum& b)
{
    CAutoBN_CTX pctx;
    CBigNum r;
    if (!BN_mod(&r, &a, &b, pctx))
        throw bignum_error("CBigNum::operator% : BN_div failed");
    return r;
}

inline const CBigNum operator<<(const CBigNum& a, unsigned int shift)
{
    CBigNum r;
    if (!BN_lshift(&r, &a, shift))
        throw bignum_error("CBigNum:operator<< : BN_lshift failed");
    return r;
}

inline const CBigNum operator>>(const CBigNum& a, unsigned int shift)
{
    CBigNum r = a;
    r >>= shift;
    return r;
}

inline bool operator==(const CBigNum& a, const CBigNum& b) { return (BN_cmp(&a, &b) == 0); }
inline bool operator!=(const CBigNum& a, const CBigNum& b) { return (BN_cmp(&a, &b) != 0); }
inline bool operator<=(const CBigNum& a, const CBigNum& b) { return (BN_cmp(&a, &b) <= 0); }
inline bool operator>=(const CBigNum& a, const CBigNum& b) { return (BN_cmp(&a, &b) >= 0); }
inline bool operator<(const CBigNum& a, const CBigNum& b)  { return (BN_cmp(&a, &b) < 0); }
inline bool operator>(const CBigNum& a, const CBigNum& b)  { return (BN_cmp(&a, &b) > 0); }


static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

// Encode a byte sequence as a base58-encoded string
inline std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend)
{
    CAutoBN_CTX pctx;
    CBigNum bn58 = 58;
    CBigNum bn0 = 0;

    // Convert big endian data to little endian
    // Extra zero at the end make sure bignum will interpret as a positive number
    std::vector<unsigned char> vchTmp(pend-pbegin+1, 0);
    reverse_copy(pbegin, pend, vchTmp.begin());

    // Convert little endian data to bignum
    CBigNum bn;
    bn.setvch(vchTmp);

    // Convert bignum to std::string
    std::string str;
    // Expected size increase from base58 conversion is approximately 137%
    // use 138% to be safe
    str.reserve((pend - pbegin) * 138 / 100 + 1);
    CBigNum dv;
    CBigNum rem;
    while (bn > bn0)
    {
        if (!BN_div(&dv, &rem, &bn, &bn58, pctx))
            throw bignum_error("EncodeBase58 : BN_div failed");
        bn = dv;
        unsigned int c = rem.getulong();
        str += pszBase58[c];
    }

    // Leading zeroes encoded as base58 zeros
    for (const unsigned char* p = pbegin; p < pend && *p == 0; p++)
        str += pszBase58[0];

    // Convert little endian std::string to big endian
    reverse(str.begin(), str.end());
//    slog( "Encode '%s'", str.c_str() );
    return str;
}

// Encode a byte vector as a base58-encoded string
inline std::string EncodeBase58(const std::vector<unsigned char>& vch)
{
    return EncodeBase58(&vch[0], &vch[0] + vch.size());
}

// Decode a base58-encoded string psz into byte vector vchRet
// returns true if decoding is succesful
inline bool DecodeBase58(const char* psz, std::vector<unsigned char>& vchRet)
{
    CAutoBN_CTX pctx;
    vchRet.clear();
    CBigNum bn58 = 58;
    CBigNum bn = 0;
    CBigNum bnChar;
    while (isspace(*psz))
        psz++;

    // Convert big endian string to bignum
    for (const char* p = psz; *p; p++)
    {
        const char* p1 = strchr(pszBase58, *p);
        if (p1 == NULL)
        {
            while (isspace(*p))
                p++;
            if (*p != '\0') {
                //slog( "%s  '%c'", pszBase58,*p );
                return false;
            }
            break;
        }
        bnChar.setulong(p1 - pszBase58);
        if (!BN_mul(&bn, &bn, &bn58, pctx))
            throw bignum_error("DecodeBase58 : BN_mul failed");
        bn += bnChar;
    }

    // Get bignum as little endian data
    std::vector<unsigned char> vchTmp = bn.getvch();

    // Trim off sign byte if present
    if (vchTmp.size() >= 2 && vchTmp.end()[-1] == 0 && vchTmp.end()[-2] >= 0x80)
        vchTmp.erase(vchTmp.end()-1);

    // Restore leading zeros
    int nLeadingZeros = 0;
    for (const char* p = psz; *p == pszBase58[0]; p++)
        nLeadingZeros++;
    vchRet.assign(nLeadingZeros + vchTmp.size(), 0);

    // Convert little endian data to big endian
    reverse_copy(vchTmp.begin(), vchTmp.end(), vchRet.end() - vchTmp.size());
    return true;
}

// Decode a base58-encoded string str into byte vector vchRet
// returns true if decoding is succesful
inline bool DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet)
{
    return DecodeBase58(str.c_str(), vchRet);
}


namespace fc {

std::string to_base58( const char* d, size_t s ) {
  return EncodeBase58( (const unsigned char*)d, (const unsigned char*)d+s ).c_str();
}

std::string to_base58( const std::vector<char>& d )
{
  if( d.size() )
     return to_base58( d.data(), d.size() );
  return std::string();
}
std::vector<char> from_base58( const std::string& base58_str ) {
   std::vector<unsigned char> out;
   if( !DecodeBase58( base58_str.c_str(), out ) ) {
     FC_THROW_EXCEPTION( parse_error_exception, "Unable to decode base58 string ${base58_str}", ("base58_str",base58_str) );
   }
   return std::vector<char>((const char*)out.data(), ((const char*)out.data())+out.size() );
}
/**
 *  @return the number of bytes decoded
 */
size_t from_base58( const std::string& base58_str, char* out_data, size_t out_data_len ) {
  //slog( "%s", base58_str.c_str() );
  std::vector<unsigned char> out;
  if( !DecodeBase58( base58_str.c_str(), out ) ) {
    FC_THROW_EXCEPTION( parse_error_exception, "Unable to decode base58 string ${base58_str}", ("base58_str",base58_str) );
  }
  FC_ASSERT( out.size() <= out_data_len );
  memcpy( out_data, out.data(), out.size() );
  return out.size();
}
}

#endif
