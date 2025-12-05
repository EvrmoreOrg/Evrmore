// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2021 The Raven Core developers
// Copyright (c) 2022 The Evrmore Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EVRMORE_SCRIPT_STANDARD_H
#define EVRMORE_SCRIPT_STANDARD_H

#include "script/interpreter.h"
#include "uint256.h"

#include <boost/variant.hpp>

#include <stdint.h>

static const bool DEFAULT_ACCEPT_DATACARRIER = true;

class CKeyID;
class CScript;
class COutPoint;
class CCoinsViewCache;

/** A reference to a CScript: the Hash160 of its serialization (see script.h) */
class CScriptID : public uint160
{
public:
    CScriptID() : uint160() {}
    CScriptID(const CScript& in);
    CScriptID(const uint160& in) : uint160(in) {}
};

/** A reference to an Asset Hash160 for P2AH addresses */
class CAssetID : public uint160
{
public:
    CAssetID() : uint160() {}
    CAssetID(const uint160& in) : uint160(in) {}
};

/**
 * Default setting for nMaxDatacarrierBytes. 80 bytes of data, +1 for OP_RETURN,
 * +2 for the pushdata opcodes.
 */
static const unsigned int MAX_OP_RETURN_RELAY = 83;

/**
 * A data carrying output is an unspendable output containing data. The script
 * type is designated as TX_NULL_DATA.
 */
extern bool fAcceptDatacarrier;

/** Maximum size of TX_NULL_DATA scripts that this node considers standard. */
extern unsigned nMaxDatacarrierBytes;

/**
 * Mandatory script verification flags that all new blocks must comply with for
 * them to be valid. (but old blocks may not comply with) Currently just P2SH,
 * but in the future other flags may be added, such as a soft-fork to enforce
 * strict DER encoding.
 *
 * Failing one of these tests may trigger a DoS ban - see CheckInputs() for
 * details.
 */
static const unsigned int MANDATORY_SCRIPT_VERIFY_FLAGS = SCRIPT_VERIFY_P2SH;

enum txnouttype
{
    TX_NONSTANDARD = 0,
    // 'standard' transaction types:
    TX_PUBKEY = 1,
    TX_PUBKEYHASH = 2,
    TX_SCRIPTHASH = 3,
    TX_MULTISIG = 4,
    TX_NULL_DATA = 5, //!< unspendable OP_RETURN script that carries data
    TX_WITNESS_V0_SCRIPTHASH = 6,
    TX_WITNESS_V0_KEYHASH = 7,
    /** RVN START */
    TX_NEW_ASSET = 8,
    TX_REISSUE_ASSET = 9,
    TX_TRANSFER_ASSET = 10,
    TX_RESTRICTED_ASSET_DATA = 11, //!< unspendable OP_EVRMORE_ASSET script that carries data
    /** RVN END */
    /** P2AH START */
    TX_ASSETHASH_BASE = 12,
    TX_ASSETHASH_MULTISIG = 13,
    TX_ASSETHASH_CHAIN_SIGNING = 14,
    TX_ASSETHASH_RESTRICTED = 15,
    // Reserved transaction type 16 for future use
    TX_ASSETHASH_EPHEMERAL = 17,
    /** P2AH END */
};

class CNoDestination {
public:
    friend bool operator==(const CNoDestination &a, const CNoDestination &b) { return true; }
    friend bool operator<(const CNoDestination &a, const CNoDestination &b) { return true; }
};

/**
 * A txout script template with a specific destination. It is either:
 *  * CNoDestination: no destination set
 *  * CKeyID: TX_PUBKEYHASH destination
 *  * CScriptID: TX_SCRIPTHASH destination
 *  * CAssetID: TX_ASSETHASH destination (P2AH)
 *  A CTxDestination is the internal data type encoded in a evrmore address
 */
typedef boost::variant<CNoDestination, CKeyID, CScriptID, CAssetID> CTxDestination;

/** Check whether a CTxDestination is a CNoDestination. */
bool IsValidDestination(const CTxDestination& dest);

/** Get the name of a txnouttype as a C string, or nullptr if unknown. */
const char* GetTxnOutputType(txnouttype t);

/**
 * Parse a scriptPubKey and identify script type for standard scripts. If
 * successful, returns script type and parsed pubkeys or hashes, depending on
 * the type. For example, for a P2SH script, vSolutionsRet will contain the
 * script hash, for P2PKH it will contain the key hash, etc.
 *
 * @param[in]   scriptPubKey   Script to parse
 * @param[out]  typeRet        The script type
 * @param[out]  vSolutionsRet  Vector of parsed pubkeys and hashes
 * @return                     True if script matches standard template
 */
bool Solver(const CScript& scriptPubKey, txnouttype& typeRet, txnouttype& scriptTypeRet, std::vector<std::vector<unsigned char> >& vSolutionsRet);

/**
 * Parse a standard scriptPubKey for the destination address. Assigns result to
 * the addressRet parameter and returns true if successful. For multisig
 * scripts, instead use ExtractDestinations. Currently only works for P2PK,
 * P2PKH, and P2SH scripts.
 */
bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet);

/**
 * Parse a standard scriptPubKey with one or more destination addresses. For
 * multisig scripts, this populates the addressRet vector with the pubkey IDs
 * and nRequiredRet with the n required to spend. For other destinations,
 * addressRet is populated with a single value and nRequiredRet is set to 1.
 * Returns true if successful. Currently does not extract address from
 * pay-to-witness scripts.
 */
bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, txnouttype& scriptType, std::vector<CTxDestination>& addressRet, int& nRequiredRet);

/**
 * Generate a Evrmore scriptPubKey for the given CTxDestination. Returns a P2PKH
 * script for a CKeyID destination, a P2SH script for a CScriptID, and an empty
 * script for CNoDestination.
 */
CScript GetScriptForDestination(const CTxDestination& dest);

/** Generate a P2PK script for the given pubkey. */
CScript GetScriptForRawPubKey(const CPubKey& pubkey);

/** Generate a multisig script. */
CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys);

/** Generate a P2AH multisig script. */
CScript GetScriptForP2AHMultisig(int nRequired, const std::vector<CPubKey>& keys);

/** Get deterministic P2AH address for a root asset (calculated from asset name). */
CScript GetScriptForRootAssetP2AH(const std::string& assetName);
std::string GetRootAssetP2AHAddress(const std::string& assetName);

/** Check if script is a P2AH chain signing script (OP_P2AH_CHAIN_SIGNING). */
bool IsP2AHChainSigning(const CScript& scriptPubKey);

/** Check if script is a P2AH restricted asset script (OP_P2AH_RESTRICTED). */
bool IsP2AHRestricted(const CScript& scriptPubKey);

/** Extract asset hash from P2AH script (for chain signing validation). */
bool ExtractAssetHashFromP2AH(const CScript& scriptPubKey, uint160& assetHash);

/** Check if script is a P2AH ephemeral script (OP_P2AH_EPHEMERAL). */
bool IsP2AHEphemeral(const CScript& scriptPubKey);

/** Resolve asset name from P2AH asset hash (requires asset database access). */
bool ResolveAssetNameFromHash(const uint160& assetHash, CAssetsCache* assetCache, std::string& assetName);

/** Validate chain signing for P2AH addresses (Asset A signing for Asset B). */
bool ValidateP2AHChainSigning(const CTransaction& tx, const CScript& signingAssetScript, const CScript& targetAssetScript, CAssetsCache* assetCache, int64_t nBlocktime, std::string& strError, const CCoinsViewCache* inputs = nullptr);

/** Validate ephemeral asset expiration and burn fee. */
bool ValidateP2AHEphemeralAsset(const CScript& ephemeralScript, const CTransaction& tx, CAssetsCache* assetCache, int64_t nBlocktime, int nBlockHeight, std::string& strError, const CCoinsViewCache* inputs = nullptr);

/** Validate address requirements for restricted P2AH addresses only (whitelist/blacklist).
 *  Basic P2AH addresses have no requirements - keep it simple.
 */
bool ValidateRestrictedP2AHAddressRequirements(const CScript& scriptPubKey, const std::string& assetName, CAmount amount, CAssetsCache* assetCache, std::string& strError);

/** Check if ephemeral asset script is proof-only (no UTXO created). */
bool IsEphemeralAssetProofOnly(const CScript& scriptPubKey, CAssetsCache* assetCache = nullptr);

/** Check if ephemeral asset script creates a UTXO. */
bool IsEphemeralAssetUTXO(const CScript& scriptPubKey, CAssetsCache* assetCache = nullptr);

/** Check if ephemeral asset has expired. */
bool IsEphemeralAssetExpired(const CEphemeralAsset& asset, int nCurrentHeight, int64_t nCurrentTime);

/** Check if a P2AH UTXO is locked by an ephemeral asset. Returns true if locked and not expired. */
bool IsP2AHUTXOLocked(const COutPoint& outpoint, CAssetsCache* assetCache, std::string& strError, int nCurrentHeight = 0, int64_t nCurrentTime = 0);

/** Validate that parent UTXO is not already locked when creating ephemeral asset. */
bool ValidateEphemeralAssetParentNotLocked(const CScript& ephemeralScript, const CTransaction& tx, CAssetsCache* assetCache, std::string& strError, const CCoinsViewCache* inputs);

/** Check if ephemeral assets are being spent to authorize spending a locked UTXO (for M-of-N multisig). */
bool ValidateEphemeralAssetsAuthorizeSpending(const CTransaction& tx, const COutPoint& lockedOutpoint, CAssetsCache* assetCache, std::string& strError, const CCoinsViewCache* inputs);

/** Extract M-of-N values from a P2AH multisig script. Returns false if not a multisig script. */
bool ExtractMultisigMN(const CScript& scriptPubKey, uint8_t& m, uint8_t& n, std::string& strError);

/** Extract M-of-N values from a multisig redeem script. */
bool ExtractMultisigMNFromRedeemScript(const CScript& redeemScript, uint8_t& m, uint8_t& n, std::string& strError);

/** Extract redeem script from scriptSig (last element). */
bool ExtractRedeemScriptFromScriptSig(const CScript& scriptSig, CScript& redeemScript, std::string& strError);

/** Check if M-of-N multisig has already created an ephemeral asset for this UTXO. */
bool ValidateMultisigEphemeralNotDuplicate(const CScript& utxoScript, const COutPoint& outpoint, const CScript& ephemeralScript, const CScript& scriptSig, CAssetsCache* assetCache, std::string& strError);

/** Check if script is a P2AH multisig script. */
bool IsP2AHMultisig(const CScript& scriptPubKey);

/** Generate a script that contains an address used for qualifier, and restricted assets data transactions */
CScript GetScriptForNullAssetDataDestination(const CTxDestination &dest);

/**
 * Generate a pay-to-witness script for the given redeem script. If the redeem
 * script is P2PK or P2PKH, this returns a P2WPKH script, otherwise it returns a
 * P2WSH script.
 */
CScript GetScriptForWitness(const CScript& redeemscript);

#endif // EVRMORE_SCRIPT_STANDARD_H
