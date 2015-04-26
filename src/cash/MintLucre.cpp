/************************************************************
 *
 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Server, CLI, GUI
 *
 *       -- Anonymous Numbered Accounts.
 *       -- Untraceable Digital Cash.
 *       -- Triple-Signed Receipts.
 *       -- Cheques, Vouchers, Transfers, Inboxes.
 *       -- Basket Currencies, Markets, Payment Plans.
 *       -- Signed, XML, Ricardian-style Contracts.
 *       -- Scripted smart contracts.
 *
 *  EMAIL:
 *  fellowtraveler@opentransactions.org
 *
 *  WEBSITE:
 *  http://www.opentransactions.org/
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This Source Code Form is subject to the terms of the
 *   Mozilla Public License, v. 2.0. If a copy of the MPL
 *   was not distributed with this file, You can obtain one
 *   at http://mozilla.org/MPL/2.0/.
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will
 *   be useful, but WITHOUT ANY WARRANTY; without even the
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A
 *   PARTICULAR PURPOSE.  See the Mozilla Public License
 *   for more details.
 *
 ************************************************************/

#include <opentxs/core/stdafx.hpp>

#include <opentxs/cash/MintLucre.hpp>
#include <opentxs/cash/DigitalCash.hpp>
#include <opentxs/cash/Token.hpp>

#include <opentxs/core/crypto/OTAsymmetricKey.hpp>
#include <opentxs/core/crypto/OTEnvelope.hpp>

#if defined(OT_CASH_USING_LUCRE)
#include <opentxs/core/crypto/OpenSSL_BIO.hpp>
#endif

#include <opentxs/core/Log.hpp>
#include <opentxs/core/Nym.hpp>

#ifdef __APPLE__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace opentxs
{

#if defined(OT_CASH_USING_LUCRE)

MintLucre::MintLucre()
    : ot_super()
{
}

MintLucre::MintLucre(const String& strNotaryID,
                     const String& strInstrumentDefinitionID)
    : ot_super(strNotaryID, strInstrumentDefinitionID)
{
}

MintLucre::MintLucre(const String& strNotaryID, const String& strServerNymID,
                     const String& strInstrumentDefinitionID)
    : ot_super(strNotaryID, strServerNymID, strInstrumentDefinitionID)
{
}

MintLucre::~MintLucre()
{
}

// The mint has a different key pair for each denomination.
// Pass the actual denomination such as 5, 10, 20, 50, 100...
bool MintLucre::AddDenomination(Nym& theNotary, int64_t lDenomination,
                                int32_t nPrimeLength)
{
    OT_ASSERT(nullptr != m_pKeyPublic);

    bool bReturnValue = false;

    // Let's make sure it doesn't already exist
    OTASCIIArmor theArmor;
    if (GetPublic(theArmor, lDenomination)) {
        otErr << "Error: Denomination public already exists in "
                 "OTMint::AddDenomination\n";
        return false;
    }
    if (GetPrivate(theArmor, lDenomination)) {
        otErr << "Error: Denomination private already exists in "
                 "OTMint::AddDenomination\n";
        return false;
    }

    if ((nPrimeLength / 8) < (MIN_COIN_LENGTH + DIGEST_LENGTH)) {
        otErr << "Prime must be at least "
              << (MIN_COIN_LENGTH + DIGEST_LENGTH) * 8 << " bits\n";
        return false;
    }

    if (nPrimeLength % 8) {
        otErr << "Prime length must be a multiple of 8\n";
        return false;
    }

#ifdef _WIN32
    BIO* out = BIO_new_file("openssl.dump", "w");
    assert(out);
    SetDumper(out);
#else
    SetMonitor(stderr);
#endif

    OpenSSL_BIO bio = BIO_new(BIO_s_mem());
    OpenSSL_BIO bioPublic = BIO_new(BIO_s_mem());

    // Generate the mint private key information
    Bank bank(nPrimeLength / 8);
    bank.WriteBIO(bio);

    // Generate the mint public key information
    PublicBank pbank(bank);
    pbank.WriteBIO(bioPublic);

    // Copy from BIO back to a normal OTString or Ascii-Armor
    char privateBankBuffer[4096],
        publicBankBuffer[4096]; // todo stop hardcoding these string lengths
    int32_t privatebankLen =
        BIO_read(bio, privateBankBuffer, 4000); // cutting it a little short on
                                                // purpose, with the buffer.
    int32_t publicbankLen =
        BIO_read(bioPublic, publicBankBuffer,
                 4000); // Just makes me feel more comfortable for some reason.

    if (privatebankLen && publicbankLen) {
        // With this, we have the Lucre public and private bank info converted
        // to OTStrings
        String strPublicBank;
        strPublicBank.Set(publicBankBuffer, publicbankLen);
        String strPrivateBank;
        strPrivateBank.Set(privateBankBuffer, privatebankLen);

        OTASCIIArmor* pPublic = new OTASCIIArmor();
        OTASCIIArmor* pPrivate = new OTASCIIArmor();

        OT_ASSERT(nullptr != pPublic);
        OT_ASSERT(nullptr != pPrivate);

        // Set the public bank info onto pPublic
        pPublic->SetString(strPublicBank, true); // linebreaks = true

        // Seal the private bank info up into an encrypted Envelope
        // and set it onto pPrivate
        OTEnvelope theEnvelope;
        theEnvelope.Seal(theNotary, strPrivateBank); // Todo check the return
                                                     // values on these two
                                                     // functions
        theEnvelope.GetAsciiArmoredData(*pPrivate);

        // Add the new key pair to the maps, using denomination as the key
        m_mapPublic[lDenomination] = pPublic;
        m_mapPrivate[lDenomination] = pPrivate;

        // Grab the Server Nym ID and save it with this Mint
        theNotary.GetIdentifier(m_ServerNymID);

        // Grab the Server's public key and save it with this Mint
        //
        const OTAsymmetricKey& theNotaryPubKey = theNotary.GetPublicSignKey();
        delete m_pKeyPublic;
        m_pKeyPublic = theNotaryPubKey.ClonePubKey();

        m_nDenominationCount++;
        bReturnValue = true;
        otWarn << "Successfully added denomination: " << lDenomination << "\n";
    }

    return bReturnValue;
}

#if defined(OT_CRYPTO_USING_OPENSSL)

// Lucre step 3: the mint signs the token
//
bool MintLucre::SignToken(Nym& theNotary, Token& theToken, String& theOutput,
                          int32_t nTokenIndex)
{
    bool bReturnValue = false;

    LucreDumper setDumper;

    OpenSSL_BIO bioBank = BIO_new(BIO_s_mem());      // input
    OpenSSL_BIO bioRequest = BIO_new(BIO_s_mem());   // input
    OpenSSL_BIO bioSignature = BIO_new(BIO_s_mem()); // output

    OTASCIIArmor thePrivate;
    GetPrivate(thePrivate, theToken.GetDenomination());

    // The Mint private info is encrypted in
    // m_mapPrivates[theToken.GetDenomination()].
    // So I need to extract that first before I can use it.
    OTEnvelope theEnvelope(thePrivate);

    String strContents; // output from opening the envelope.
    // Decrypt the Envelope into strContents
    if (!theEnvelope.Open(theNotary, strContents)) return false;

    // copy strContents to a BIO
    BIO_puts(bioBank, strContents.Get());

    // Instantiate the Bank with its private key
    Bank bank(bioBank);

    // I need the request. the prototoken.
    OTASCIIArmor ascPrototoken;
    bool bFoundToken = theToken.GetPrototoken(ascPrototoken, nTokenIndex);

    if (bFoundToken) {
        // base64-Decode the prototoken
        String strPrototoken(ascPrototoken);

        // copy strPrototoken to a BIO
        BIO_puts(bioRequest, strPrototoken.Get());

        // Load up the coin request from the bio (the prototoken)
        PublicCoinRequest req(bioRequest);

        // Sign it with the bank we previously instantiated.
        // results will be in bnSignature (BIGNUM)
        BIGNUM* bnSignature = bank.SignRequest(req);

        if (nullptr == bnSignature) {
            otErr << "MAJOR ERROR!: Bank.SignRequest failed in "
                     "MintLucre::SignToken\n";
        }
        else {

            // Write the request contents, followed by the signature contents,
            // to the Signature bio. Then free the BIGNUM.
            req.WriteBIO(bioSignature); // the original request contents
            DumpNumber(bioSignature, "signature=",
                       bnSignature); // the new signature contents
            BN_free(bnSignature);

            // Read the signature bio into a C-style buffer...
            char sig_buf[1024]; // todo stop hardcoding these string lengths

            int32_t sig_len = BIO_read(bioSignature, sig_buf,
                                       1000); // cutting it a little short on
                                              // purpose, with the buffer. Just
                                              // makes me feel more comfortable
                                              // for some reason.

            // Add the null terminator by hand (just in case.)
            sig_buf[sig_len] = '\0';

            if (sig_len) {
                // Copy the original coin request into the spendable field of
                // the token object.
                // (It won't actually be spendable until the client processes
                // it, though.)
                theToken.SetSpendable(ascPrototoken);

                // Base64-encode the signature contents into
                // theToken.m_Signature.
                String strSignature(sig_buf);

                // Here we pass the signature back to the caller.
                // He will probably set it onto the token.
                theOutput.Set(sig_buf, sig_len);
                bReturnValue = true;

                // This is also where we set the expiration date on the token.
                // The client should have already done this, but we are
                // explicitly
                // setting the values here to prevent any funny business.
                theToken.SetSeriesAndExpiration(m_nSeries, m_VALID_FROM,
                                                m_VALID_TO);
            }
        }
    }

    return bReturnValue;
}

// Lucre step 5: mint verifies token when it is redeemed by merchant.
// This function is called by OTToken::VerifyToken.
// That's the one you should be calling, most likely, not this one.
bool MintLucre::VerifyToken(Nym& theNotary, String& theCleartextToken,
                            int64_t lDenomination)
{
    bool bReturnValue = false;
    LucreDumper setDumper;

    OpenSSL_BIO bioBank = BIO_new(BIO_s_mem()); // input
    OpenSSL_BIO bioCoin = BIO_new(BIO_s_mem()); // input

    // --- copy theCleartextToken to bioCoin so lucre can load it
    BIO_puts(bioCoin, theCleartextToken.Get());

    // --- The Mint private info is encrypted in m_mapPrivate[lDenomination].
    // So I need to extract that first before I can use it.
    OTASCIIArmor theArmor;
    GetPrivate(theArmor, lDenomination);
    OTEnvelope theEnvelope(theArmor);

    String strContents; // will contain output from opening the envelope.
    // Decrypt the Envelope into strContents
    if (theEnvelope.Open(theNotary, strContents)) {
        // copy strContents to a BIO
        BIO_puts(bioBank, strContents.Get());

        // ---- Now the bank and coin bios are both ready to go...

        Bank bank(bioBank);
        Coin coin(bioCoin);

        if (bank.Verify(coin)) // Here's the boolean output: coin is verified!
        {
            bReturnValue = true;

            // (Done): When a token is redeemed, need to store it in the spent
            // token database.
            // Right now I can verify the token, but unless I check it against a
            // database, then
            // even though the signature verifies, it doesn't stop people from
            // redeeming the same
            // token again and again and again.
            //
            // (done): also need to make sure issuer has double-entries for
            // total amount outstanding.
            //
            // UPDATE: These are both done now.  The Spent Token database is
            // implemented in the transaction server,
            // (not OTLib proper) and the same server also now keeps a cash
            // account to match all cash withdrawals.
            // (Meaning, if 10,000 clams total have been withdrawn by various
            // users, then the server actually has
            // a clam account containing 10,000 clams. As the cash comes in for
            // redemption, the server debits it from
            // this account again before sending it to its final destination.
            // This way the server tracks total outstanding
            // amount, as an additional level of security after the blind
            // signature itself.)
        }
    }

    return bReturnValue;
}

#endif // defined(OT_CRYPTO_USING_OPENSSL)
#endif // defined(OT_CASH_USING_LUCRE)

} // namespace opentxs
