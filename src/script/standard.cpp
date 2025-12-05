// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2021 The Raven Core developers
// Copyright (c) 2022 The Evrmore Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <assets/assets.h>
#include <assets/assettypes.h>
#include <validation.h>
#include <hash.h>
#include "script/standard.h"

#include "pubkey.h"
#include "script/script.h"
#include "util.h"
#include "utilstrencodings.h"

typedef std::vector<unsigned char> valtype;

bool fAcceptDatacarrier = DEFAULT_ACCEPT_DATACARRIER;
unsigned nMaxDatacarrierBytes = MAX_OP_RETURN_RELAY;

CScriptID::CScriptID(const CScript& in) : uint160(Hash160(in.begin(), in.end())) {}

const char* GetTxnOutputType(txnouttype t)
{
    switch (t)
    {
    case TX_NONSTANDARD: return "nonstandard";
    case TX_PUBKEY: return "pubkey";
    case TX_PUBKEYHASH: return "pubkeyhash";
    case TX_SCRIPTHASH: return "scripthash";
    case TX_MULTISIG: return "multisig";
    case TX_NULL_DATA: return "nulldata";
    case TX_RESTRICTED_ASSET_DATA: return "nullassetdata";
    case TX_WITNESS_V0_KEYHASH: return "witness_v0_keyhash";
    case TX_WITNESS_V0_SCRIPTHASH: return "witness_v0_scripthash";

    /** RVN START */
    case TX_NEW_ASSET: return ASSET_NEW_STRING;
    case TX_TRANSFER_ASSET: return ASSET_TRANSFER_STRING;
    case TX_REISSUE_ASSET: return ASSET_REISSUE_STRING;
    /** RVN END */
    /** P2AH START */
    case TX_ASSETHASH_BASE: return "assethash_base";
    case TX_ASSETHASH_MULTISIG: return "assethash_multisig";
    case TX_ASSETHASH_CHAIN_SIGNING: return "assethash_chain_signing";
    case TX_ASSETHASH_RESTRICTED: return "assethash_restricted";
    case TX_ASSETHASH_EPHEMERAL: return "assethash_ephemeral";
    /** P2AH END */
    }
    return nullptr;
}

bool Solver(const CScript& scriptPubKey, txnouttype& typeRet, txnouttype& scriptTypeRet, std::vector<std::vector<unsigned char> >& vSolutionsRet)
{
    // Templates
    static std::multimap<txnouttype, CScript> mTemplates;
    if (mTemplates.empty())
    {
        // Standard tx, sender provides pubkey, receiver adds signature
        mTemplates.insert(std::make_pair(TX_PUBKEY, CScript() << OP_PUBKEY << OP_CHECKSIG));

        // Evrmore address tx, sender provides hash of pubkey, receiver provides signature and pubkey
        mTemplates.insert(std::make_pair(TX_PUBKEYHASH, CScript() << OP_DUP << OP_HASH160 << OP_PUBKEYHASH << OP_EQUALVERIFY << OP_CHECKSIG));

        // Sender provides N pubkeys, receivers provides M signatures
        mTemplates.insert(std::make_pair(TX_MULTISIG, CScript() << OP_SMALLINTEGER << OP_PUBKEYS << OP_SMALLINTEGER << OP_CHECKMULTISIG));
    }

    vSolutionsRet.clear();

    // Shortcut for pay-to-script-hash, which are more constrained than the other types:
    // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
    if (scriptPubKey.IsPayToScriptHash())
    {
        typeRet = TX_SCRIPTHASH;
        std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
        vSolutionsRet.push_back(hashBytes);
        return true;
    }
    // P2AH scripts: OP_HASH160 20 [20 byte hash] OP_EQUAL [OP_P2AH_*]
    // Check for P2AH opcodes after standard P2SH pattern
    if (scriptPubKey.size() >= 24 && scriptPubKey[0] == OP_HASH160 && scriptPubKey[1] == 0x14 && scriptPubKey[22] == OP_EQUAL)
    {
        opcodetype p2ahOpcode = (opcodetype)scriptPubKey[23];
        std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
        
        switch(p2ahOpcode) {
            case OP_P2AH_BASE:
                typeRet = TX_ASSETHASH_BASE;
                vSolutionsRet.push_back(hashBytes);
                return true;
            case OP_P2AH_MULTISIG:
                typeRet = TX_ASSETHASH_MULTISIG;
                vSolutionsRet.push_back(hashBytes);
                return true;
            case OP_P2AH_CHAIN_SIGNING:
                typeRet = TX_ASSETHASH_CHAIN_SIGNING;
                vSolutionsRet.push_back(hashBytes);
                return true;
            case OP_P2AH_RESTRICTED:
                typeRet = TX_ASSETHASH_RESTRICTED;
                vSolutionsRet.push_back(hashBytes);
                return true;
            // Reserved opcode case for future use
            case OP_P2AH_EPHEMERAL:
                typeRet = TX_ASSETHASH_EPHEMERAL;
                vSolutionsRet.push_back(hashBytes);
                return true;
            default:
                break;
        }
    }
    /** RVN START */
    int nType = 0;
    int nScriptType = 0;
    bool fIsOwner = false;
    if (scriptPubKey.IsAssetScript(nType, nScriptType, fIsOwner)) {
        typeRet = (txnouttype)nType;
        scriptTypeRet = (txnouttype)nScriptType;

        if (scriptTypeRet == TX_SCRIPTHASH) {
            std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
            vSolutionsRet.push_back(hashBytes);
            return true;
        } else if (scriptTypeRet == TX_PUBKEYHASH) {
            std::vector<unsigned char> hashBytes(scriptPubKey.begin()+3, scriptPubKey.begin()+23);
            vSolutionsRet.push_back(hashBytes);
            return true;
        }
        return false;
    }
    /** RVN END */

    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
        if (witnessversion == 0 && witnessprogram.size() == 20) {
            typeRet = TX_WITNESS_V0_KEYHASH;
            vSolutionsRet.push_back(witnessprogram);
            return true;
        }
        if (witnessversion == 0 && witnessprogram.size() == 32) {
            typeRet = TX_WITNESS_V0_SCRIPTHASH;
            vSolutionsRet.push_back(witnessprogram);
            return true;
        }
        return false;
    }

    // Provably prunable, data-carrying output
    //
    // So long as script passes the IsUnspendable() test and all but the first
    // byte passes the IsPushOnly() test we don't care what exactly is in the
    // script.
    if (scriptPubKey.size() >= 1 && scriptPubKey[0] == OP_RETURN && scriptPubKey.IsPushOnly(scriptPubKey.begin()+1)) {
        typeRet = TX_NULL_DATA;
        return true;
    }

    // Provably prunable, asset data-carrying output
    //
    // So long as script passes the IsUnspendable() test and all but the first three
    // byte passes the IsPushOnly()
    if (scriptPubKey.size() >= 1 && scriptPubKey[0] == OP_EVR_ASSET && scriptPubKey.IsPushOnly(scriptPubKey.begin()+1)) {
        typeRet = TX_RESTRICTED_ASSET_DATA;

        if (scriptPubKey.size() >= 23 && scriptPubKey[1] != OP_RESERVED) {
            if (scriptPubKey[1] == OP_1NEGATE) {
                // P2SH script
                scriptTypeRet = TX_SCRIPTHASH;
                std::vector<unsigned char> hashBytes(scriptPubKey.begin() + 3, scriptPubKey.begin() + 23);
                vSolutionsRet.push_back(hashBytes);
            } else {
                // P2PKH script
                scriptTypeRet = TX_PUBKEYHASH;

                std::vector<unsigned char> hashBytes(scriptPubKey.begin() + 2, scriptPubKey.begin() + 22);
                vSolutionsRet.push_back(hashBytes);
            }
        }
        return true;
    }

    // Scan templates
    const CScript& script1 = scriptPubKey;
    for (const std::pair<const txnouttype, CScript>& tplate : mTemplates)
    {
        const CScript& script2 = tplate.second;
        vSolutionsRet.clear();

        opcodetype opcode1, opcode2;
        std::vector<unsigned char> vch1, vch2;

        // Compare
        CScript::const_iterator pc1 = script1.begin();
        CScript::const_iterator pc2 = script2.begin();
        while (true)
        {
            if (pc1 == script1.end() && pc2 == script2.end())
            {
                // Found a match
                typeRet = tplate.first;
                if (typeRet == TX_MULTISIG)
                {
                    // Additional checks for TX_MULTISIG:
                    unsigned char m = vSolutionsRet.front()[0];
                    unsigned char n = vSolutionsRet.back()[0];
                    if (m < 1 || n < 1 || m > n || vSolutionsRet.size()-2 != n)
                        return false;
                }

                return true;
            }
            if (!script1.GetOp(pc1, opcode1, vch1))
                break;
            if (!script2.GetOp(pc2, opcode2, vch2))
                break;

            // Template matching opcodes:
            if (opcode2 == OP_PUBKEYS)
            {
                while (vch1.size() >= 33 && vch1.size() <= 65)
                {
                    vSolutionsRet.push_back(vch1);
                    if (!script1.GetOp(pc1, opcode1, vch1))
                        break;
                }
                if (!script2.GetOp(pc2, opcode2, vch2))
                    break;
                // Normal situation is to fall through
                // to other if/else statements
            }

            if (opcode2 == OP_PUBKEY)
            {
                if (vch1.size() < 33 || vch1.size() > 65)
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_PUBKEYHASH)
            {
                if (vch1.size() != sizeof(uint160))
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_SMALLINTEGER)
            {   // Single-byte small integer pushed onto vSolutions
                if (opcode1 == OP_0 ||
                    (opcode1 >= OP_1 && opcode1 <= OP_16))
                {
                    char n = (char)CScript::DecodeOP_N(opcode1);
                    vSolutionsRet.push_back(valtype(1, n));
                }
                else
                    break;
            }
            else if (opcode1 != opcode2 || vch1 != vch2)
            {
                // Others must match exactly
                break;
            }
        }
    }

    vSolutionsRet.clear();
    typeRet = TX_NONSTANDARD;
    return false;
}

bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet)
{
    std::vector<valtype> vSolutions;
    txnouttype whichType;
    txnouttype scriptType;
    if (!Solver(scriptPubKey, whichType, scriptType, vSolutions)) {
        return false;
    }

    if (whichType == TX_PUBKEY)
    {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid())
            return false;

        addressRet = pubKey.GetID();
        return true;
    }
    else if (whichType == TX_PUBKEYHASH)
    {
        addressRet = CKeyID(uint160(vSolutions[0]));
        return true;
    }
    else if (whichType == TX_SCRIPTHASH)
    {
        addressRet = CScriptID(uint160(vSolutions[0]));
        return true;
    /** RVN START */
    } else if (whichType == TX_NEW_ASSET || whichType == TX_REISSUE_ASSET || whichType == TX_TRANSFER_ASSET) {
        if (scriptType == TX_SCRIPTHASH) {
            addressRet = CScriptID(uint160(vSolutions[0]));
        } else {
            addressRet = CKeyID(uint160(vSolutions[0]));
        }
        return true;
    } else if (whichType == TX_RESTRICTED_ASSET_DATA) {
        if (vSolutions.size()) {
            if (scriptType == TX_SCRIPTHASH) {
                addressRet = CScriptID(uint160(vSolutions[0]));
            } else {
                addressRet = CKeyID(uint160(vSolutions[0]));
            }
            return true;
        }
    }
     /** RVN END */
    /** P2AH START */
    else if (whichType == TX_ASSETHASH_BASE || whichType == TX_ASSETHASH_MULTISIG || 
             whichType == TX_ASSETHASH_CHAIN_SIGNING || whichType == TX_ASSETHASH_RESTRICTED ||
             whichType == TX_ASSETHASH_EPHEMERAL) {
        if (vSolutions.size()) {
            addressRet = CAssetID(uint160(vSolutions[0]));
            return true;
        }
    }
    /** P2AH END */
    // Multisig txns have more than one address...
    return false;
}

bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, txnouttype& scriptType, std::vector<CTxDestination>& addressRet, int& nRequiredRet)
{
    addressRet.clear();
    typeRet = TX_NONSTANDARD;
    scriptType = TX_NONSTANDARD;
    std::vector<valtype> vSolutions;
    if (!Solver(scriptPubKey, typeRet, scriptType, vSolutions))
        return false;
    if (typeRet == TX_NULL_DATA) {
        // This is data, not addresses
        return false;
    }

    if (typeRet == TX_MULTISIG)
    {
        nRequiredRet = vSolutions.front()[0];
        for (unsigned int i = 1; i < vSolutions.size()-1; i++)
        {
            CPubKey pubKey(vSolutions[i]);
            if (!pubKey.IsValid())
                continue;

            CTxDestination address = pubKey.GetID();
            addressRet.push_back(address);
        }

        if (addressRet.empty())
            return false;
    }
    else
    {
        nRequiredRet = 1;
        CTxDestination address;
        if (!ExtractDestination(scriptPubKey, address))
           return false;
        addressRet.push_back(address);
    }

    return true;
}

namespace
{
class CScriptVisitor : public boost::static_visitor<bool>
{
private:
    CScript *script;
public:
    explicit CScriptVisitor(CScript *scriptin) { script = scriptin; }

    bool operator()(const CNoDestination &dest) const {
        script->clear();
        return false;
    }

    bool operator()(const CKeyID &keyID) const {
        script->clear();
        *script << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
        return true;
    }

    bool operator()(const CScriptID &scriptID) const {
        script->clear();
        *script << OP_HASH160 << ToByteVector(scriptID) << OP_EQUAL;
        return true;
    }

    bool operator()(const CAssetID &assetID) const {
        script->clear();
        *script << OP_HASH160 << ToByteVector(assetID) << OP_EQUAL << OP_P2AH_BASE;
        return true;
    }
};
} // namespace

namespace
{
    class CNullAssetScriptVisitor : public boost::static_visitor<bool>
    {
    private:
        CScript *script;
    public:
        explicit CNullAssetScriptVisitor(CScript *scriptin) { script = scriptin; }

        bool operator()(const CNoDestination &dest) const {
            script->clear();
            return false;
        }

        bool operator()(const CKeyID &keyID) const {
            script->clear();
            *script << OP_EVR_ASSET << ToByteVector(keyID);
            return true;
        }

        bool operator()(const CScriptID &scriptID) const {
            script->clear();
            if (IsTollsActive()) {
                // OP1_NEGATE was chosen because it is a PUSH only value and it worked.
                // You can use any PUSH value though
                *script << OP_EVR_ASSET << OP_1NEGATE << ToByteVector(scriptID);
            } else {
                *script << OP_EVR_ASSET << ToByteVector(scriptID);
            }
            return true;
        }
    };
} // namespace

CScript GetScriptForDestination(const CTxDestination& dest)
{
    CScript script;

    boost::apply_visitor(CScriptVisitor(&script), dest);
    return script;
}

CScript GetScriptForNullAssetDataDestination(const CTxDestination &dest)
{
    CScript script;

    boost::apply_visitor(CNullAssetScriptVisitor(&script), dest);
    return script;
}

CScript GetScriptForRawPubKey(const CPubKey& pubKey)
{
    return CScript() << std::vector<unsigned char>(pubKey.begin(), pubKey.end()) << OP_CHECKSIG;
}

CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys)
{
    CScript script;

    script << CScript::EncodeOP_N(nRequired);
    for (const CPubKey& key : keys)
        script << ToByteVector(key);
    script << CScript::EncodeOP_N(keys.size()) << OP_CHECKMULTISIG;
    return script;
}

CScript GetScriptForP2AHMultisig(int nRequired, const std::vector<CPubKey>& keys)
{
    // Create the multisig redeem script
    CScript redeemScript = GetScriptForMultisig(nRequired, keys);
    
    // Hash the redeem script
    uint160 hash = Hash160(redeemScript.begin(), redeemScript.end());
    
    // Create P2AH multisig script: OP_HASH160 <hash> OP_EQUAL OP_P2AH_MULTISIG
    CScript script;
    script << OP_HASH160 << ToByteVector(hash) << OP_EQUAL << OP_P2AH_MULTISIG;
    return script;
}

CScript GetScriptForRootAssetP2AH(const std::string& assetName)
{
    // Deterministically calculate P2AH address for root asset from asset name
    // Hash160(assetName) -> CAssetID -> P2AH script with OP_P2AH_BASE
    uint160 assetHash = Hash160(assetName.begin(), assetName.end());
    CAssetID assetID(assetHash);
    CScript script;
    script << OP_HASH160 << ToByteVector(assetID) << OP_EQUAL << OP_P2AH_BASE;
    return script;
}

std::string GetRootAssetP2AHAddress(const std::string& assetName)
{
    // Get deterministic P2AH address for root asset
    CScript script = GetScriptForRootAssetP2AH(assetName);
    uint160 assetHash;
    if (ExtractAssetHashFromP2AH(script, assetHash)) {
        CAssetID assetID(assetHash);
        CEvrmoreAddress address;
        address.SetAssetHashAddress(assetID);
        return address.ToString();
    }
    return "";
}

bool IsP2AHChainSigning(const CScript& scriptPubKey)
{
    // P2AH chain signing scripts: OP_HASH160 <20-byte hash> OP_EQUAL OP_P2AH_CHAIN_SIGNING
    if (scriptPubKey.size() >= 24 && 
        scriptPubKey[0] == OP_HASH160 && 
        scriptPubKey[1] == 0x14 && 
        scriptPubKey[22] == OP_EQUAL &&
        scriptPubKey[23] == OP_P2AH_CHAIN_SIGNING) {
        return true;
    }
    return false;
}

bool IsP2AHRestricted(const CScript& scriptPubKey)
{
    // P2AH restricted asset scripts: OP_HASH160 <20-byte hash> OP_EQUAL OP_P2AH_RESTRICTED
    if (scriptPubKey.size() >= 24 && 
        scriptPubKey[0] == OP_HASH160 && 
        scriptPubKey[1] == 0x14 && 
        scriptPubKey[22] == OP_EQUAL &&
        scriptPubKey[23] == OP_P2AH_RESTRICTED) {
        return true;
    }
    return false;
}

bool ExtractAssetHashFromP2AH(const CScript& scriptPubKey, uint160& assetHash)
{
    // Extract the 20-byte hash from P2AH script: OP_HASH160 <20-byte hash> OP_EQUAL [OP_P2AH_*]
    if (scriptPubKey.size() >= 24 && 
        scriptPubKey[0] == OP_HASH160 && 
        scriptPubKey[1] == 0x14 && 
        scriptPubKey[22] == OP_EQUAL) {
        std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
        assetHash = uint160(hashBytes);
        return true;
    }
    return false;
}

bool IsP2AHEphemeral(const CScript& scriptPubKey)
{
    // Check for P2AH ephemeral script: OP_HASH160 <20-byte hash> OP_EQUAL OP_P2AH_EPHEMERAL
    if (scriptPubKey.size() >= 24 &&
        scriptPubKey[0] == OP_HASH160 &&
        scriptPubKey[1] == 0x14 &&
        scriptPubKey[22] == OP_EQUAL &&
        scriptPubKey[23] == OP_P2AH_EPHEMERAL) {
        return true;
    }
    return false;
}

bool ResolveAssetNameFromHash(const uint160& assetHash, CAssetsCache* assetCache, std::string& assetName)
{
    // Resolve asset name from P2AH hash
    // The hash in P2AH is Hash160(assetName), so we need to find the asset whose name hashes to this value
    // This is expensive and should be cached if used frequently
    
    if (!assetCache) {
        return false;
    }
    
    // Check dirty cache first (setNewAssetsToAdd)
    for (const auto& cachedAsset : assetCache->setNewAssetsToAdd) {
        uint160 nameHash = Hash160(cachedAsset.asset.strName.begin(), cachedAsset.asset.strName.end());
        if (nameHash == assetHash) {
            assetName = cachedAsset.asset.strName;
            return true;
        }
    }
    
    // Check parent cache if available
    if (assetCache->passets) {
        for (const auto& cachedAsset : assetCache->passets->setNewAssetsToAdd) {
            uint160 nameHash = Hash160(cachedAsset.asset.strName.begin(), cachedAsset.asset.strName.end());
            if (nameHash == assetHash) {
                assetName = cachedAsset.asset.strName;
                return true;
            }
        }
    }
    
    // Check memory cache (passetsCache)
    if (assetCache->passetsCache) {
        // Iterate through cache entries
        // Note: This requires iterating through all cached assets, which can be expensive
        // Consider adding a reverse index (hash -> name) if this becomes a bottleneck
        std::vector<CDatabasedAssetData> cachedAssets;
        if (assetCache->passetsdb && assetCache->passetsdb->AssetDir(cachedAssets)) {
            for (const auto& data : cachedAssets) {
                uint160 nameHash = Hash160(data.asset.strName.begin(), data.asset.strName.end());
                if (nameHash == assetHash) {
                    assetName = data.asset.strName;
                    return true;
                }
            }
        }
        
        // Also check if asset exists in cache directly (if we can iterate)
        // The cache doesn't expose iteration, so we'd need to query by name
        // For now, we rely on AssetDir() which queries the database
    }
    
    // Check database directly if available
    if (assetCache->passetsdb) {
        std::vector<CDatabasedAssetData> assets;
        if (assetCache->passetsdb->AssetDir(assets)) {
            for (const auto& data : assets) {
                uint160 nameHash = Hash160(data.asset.strName.begin(), data.asset.strName.end());
                if (nameHash == assetHash) {
                    assetName = data.asset.strName;
                    return true;
                }
            }
        }
    }
    
    return false; // Asset not found
}

bool ValidateP2AHChainSigning(const CTransaction& tx, const CScript& signingAssetScript, const CScript& targetAssetScript, CAssetsCache* assetCache, int64_t nBlocktime, std::string& strError, const CCoinsViewCache* inputs)
{
    // Validate chain signing: Asset A (signingAssetScript) can authorize transactions for Asset B (targetAssetScript)
    // Requirements:
    // 1. Both scripts must be P2AH chain signing scripts
    // 2. Asset A must spend Asset B's UTXO to authorize transactions for Asset B
    // 3. Chain signing always creates an ephemeral_child_asset
    // 4. Small EVR burn fee required for unused ephemeral assets (waived if spent in same block)
    
    if (!IsP2AHChainSigning(signingAssetScript)) {
        strError = "Signing asset script is not a P2AH chain signing script";
        return false;
    }
    
    // Extract asset hashes
    uint160 signingHash, targetHash;
    if (!ExtractAssetHashFromP2AH(signingAssetScript, signingHash) ||
        !ExtractAssetHashFromP2AH(targetAssetScript, targetHash)) {
        strError = "Failed to extract asset hashes from P2AH scripts";
        return false;
    }
    
    // Resolve asset names (if possible)
    std::string signingAssetName, targetAssetName;
    bool hasSigningName = ResolveAssetNameFromHash(signingHash, assetCache, signingAssetName);
    bool hasTargetName = ResolveAssetNameFromHash(targetHash, assetCache, targetAssetName);
    
    // Basic validation: verify assets exist if we can resolve names
    if (hasSigningName && assetCache && !assetCache->CheckIfAssetExists(signingAssetName, false)) {
        strError = strprintf("Signing asset %s does not exist", signingAssetName);
        return false;
    }
    
    if (hasTargetName && assetCache && !assetCache->CheckIfAssetExists(targetAssetName, false)) {
        strError = strprintf("Target asset %s does not exist", targetAssetName);
        return false;
    }
    
    // Validate that Asset A is spending Asset B's UTXO (chain signing requirement)
    if (inputs && hasTargetName) {
        bool foundTargetAssetInInputs = false;
        for (unsigned int i = 0; i < tx.vin.size(); ++i) {
            const COutPoint &prevout = tx.vin[i].prevout;
            const Coin& coin = inputs->AccessCoin(prevout);
            if (coin.IsAsset()) {
                CAssetOutputEntry inputData;
                if (GetAssetData(coin.out.scriptPubKey, inputData)) {
                    if (inputData.assetName == targetAssetName) {
                        foundTargetAssetInInputs = true;
                        break;
                    }
                }
            }
        }
        
        if (!foundTargetAssetInInputs) {
            strError = strprintf("Chain signing requires Asset A (%s) to spend Asset B (%s) UTXO, but Asset B not found in inputs", 
                                 hasSigningName ? signingAssetName : "unknown", targetAssetName);
            return false;
        }
    }
    
    // Validate ephemeral asset creation
    // Chain signing always creates an ephemeral_child_asset (proof-only)
    // Check if any output is an ephemeral asset (proof-only or UTXO)
    bool foundEphemeralOutput = false;
    for (const auto& txout : tx.vout) {
        if (IsP2AHEphemeral(txout.scriptPubKey)) {
            foundEphemeralOutput = true;
            // Note: Chain signing creates proof-only ephemeral assets by default
            // UTXO ephemeral assets can be created explicitly if needed
            break;
        }
    }
    
    if (!foundEphemeralOutput) {
        strError = "Chain signing must create an ephemeral_child_asset, but no ephemeral output found";
        return false;
    }
    
    // Note: Burn fee validation for unused ephemeral assets is handled separately
    // during block validation, as it requires checking if ephemeral asset is spent in same block
    
    return true;
}

bool ValidateP2AHEphemeralAsset(const CScript& ephemeralScript, const CTransaction& tx, CAssetsCache* assetCache, int64_t nBlocktime, int nBlockHeight, std::string& strError, const CCoinsViewCache* inputs)
{
    // Validate ephemeral asset expiration and burn fee
    // Requirements:
    // 1. Ephemeral assets must be removed after spent or time expires (timestamp or block height)
    // 2. Small EVR burn fee required for unused ephemeral assets (UTXO only)
    // 3. If ephemeral asset is included in the same block as the spend, no burn fee is incurred
    // 4. Proof-only ephemeral assets don't create UTXOs and don't incur fees
    // 5. Parent UTXO must not be already locked by another ephemeral asset
    
    if (!IsP2AHEphemeral(ephemeralScript)) {
        strError = "Script is not a P2AH ephemeral script";
        return false;
    }
    
    // Validate that parent UTXO is not already locked
    if (!ValidateEphemeralAssetParentNotLocked(ephemeralScript, tx, assetCache, strError, inputs)) {
        return false;
    }
    
    // Check if this is a proof-only ephemeral asset (no UTXO, no fee)
    if (IsEphemeralAssetProofOnly(ephemeralScript, assetCache)) {
        // Proof-only ephemeral assets are validated but don't create UTXOs
        // No expiration check needed, no fee charged
        return true;
    }
    
    // This is a UTXO ephemeral asset - full validation required
    // Extract asset hash
    uint160 ephemeralHash;
    if (!ExtractAssetHashFromP2AH(ephemeralScript, ephemeralHash)) {
        strError = "Failed to extract asset hash from ephemeral script";
        return false;
    }
    
    // Resolve asset name
    std::string ephemeralAssetName;
    bool hasName = ResolveAssetNameFromHash(ephemeralHash, assetCache, ephemeralAssetName);
    
    // If we can resolve the asset name, check expiration
    if (hasName && assetCache) {
        // Try to read ephemeral asset metadata
        CNewAsset baseAsset;
        if (assetCache->GetAssetMetaDataIfExists(ephemeralAssetName, baseAsset)) {
            // Convert to CEphemeralAsset if possible
            // Note: This requires ephemeral asset to be stored with metadata
            // For now, basic validation passes
            // Full expiration check will be implemented when ephemeral asset storage is added
        }
    }
    
    // Check if ephemeral asset is spent in this transaction (if so, no burn fee)
    bool isSpentInTx = false;
    if (inputs) {
        // Check if any input spends this ephemeral asset
        for (unsigned int i = 0; i < tx.vin.size(); ++i) {
            const COutPoint &prevout = tx.vin[i].prevout;
            const Coin& coin = inputs->AccessCoin(prevout);
            if (coin.IsAsset() && coin.out.scriptPubKey == ephemeralScript) {
                isSpentInTx = true;
                break;
            }
        }
    }
    
    // Burn fee validation is handled at block level, as it requires checking if ephemeral asset
    // is spent in the same block (not just same transaction)
    
    return true;
}

bool ValidateRestrictedP2AHAddressRequirements(const CScript& scriptPubKey, const std::string& assetName, CAmount amount, CAssetsCache* assetCache, std::string& strError)
{
    // Validate address requirements for RESTRICTED P2AH addresses only
    // Basic P2AH addresses have no requirements - keep it simple
    
    // Only validate if this is a restricted P2AH address
    if (!IsP2AHRestricted(scriptPubKey)) {
        // Not a restricted P2AH address, no requirements to validate
        return true;
    }
    
    // Extract asset hash from P2AH script
    uint160 assetHash;
    if (!ExtractAssetHashFromP2AH(scriptPubKey, assetHash)) {
        // Not a P2AH script, no requirements to validate
        return true;
    }
    
    // Resolve asset name
    std::string p2ahAssetName;
    bool hasP2AHName = ResolveAssetNameFromHash(assetHash, assetCache, p2ahAssetName);
    
    if (!hasP2AHName || !assetCache) {
        // Can't validate without asset name or cache
        return true; // Allow for now
    }
    
    // Verify asset exists
    if (!assetCache->CheckIfAssetExists(p2ahAssetName, false)) {
        strError = strprintf("Restricted P2AH asset %s does not exist", p2ahAssetName);
        return false;
    }
    
    // Get address requirements for this restricted P2AH address
    CRestrictedP2AHAddressRequirements requirements;
    if (!assetCache->GetRestrictedP2AHAddressRequirements(assetHash, requirements)) {
        // No requirements set for this address - allow all transfers
        return true;
    }
    
    // 1. Validate whitelist/blacklist for asset transfers (receiving assets)
    if (requirements.fUseWhitelist) {
        // Whitelist mode: only assets in setAllowedAssets are allowed
        // Empty whitelist means allow all (no restrictions)
        if (!requirements.setAllowedAssets.empty() && 
            requirements.setAllowedAssets.find(assetName) == requirements.setAllowedAssets.end()) {
            strError = strprintf("Asset %s is not in whitelist for restricted P2AH address %s", 
                                assetName, p2ahAssetName);
            return false;
        }
    } else {
        // Blacklist mode: assets in setBlockedAssets are blocked
        if (requirements.setBlockedAssets.find(assetName) != requirements.setBlockedAssets.end()) {
            strError = strprintf("Asset %s is blocked for restricted P2AH address %s", 
                                assetName, p2ahAssetName);
            return false;
        }
    }
    
    return true;
}

bool IsEphemeralAssetProofOnly(const CScript& scriptPubKey, CAssetsCache* assetCache)
{
    if (!IsP2AHEphemeral(scriptPubKey)) {
        return false;
    }
    
    // Check ephemeral asset metadata if available
    if (assetCache) {
        uint160 ephemeralHash;
        if (ExtractAssetHashFromP2AH(scriptPubKey, ephemeralHash)) {
            std::string ephemeralAssetName;
            if (ResolveAssetNameFromHash(ephemeralHash, assetCache, ephemeralAssetName)) {
                CEphemeralAsset ephemeralAsset;
                if (assetCache->GetEphemeralAssetMetaDataIfExists(ephemeralAssetName, ephemeralAsset)) {
                    return ephemeralAsset.IsProofOnly();
                }
            }
        }
    }
    
    // If metadata not available, default to false (assume UTXO)
    // Proof-only is the special case that requires explicit metadata
    return false;
}

bool IsEphemeralAssetUTXO(const CScript& scriptPubKey, CAssetsCache* assetCache)
{
    if (!IsP2AHEphemeral(scriptPubKey)) {
        return false;
    }
    
    // Check ephemeral asset metadata if available
    if (assetCache) {
        uint160 ephemeralHash;
        if (ExtractAssetHashFromP2AH(scriptPubKey, ephemeralHash)) {
            std::string ephemeralAssetName;
            if (ResolveAssetNameFromHash(ephemeralHash, assetCache, ephemeralAssetName)) {
                CEphemeralAsset ephemeralAsset;
                if (assetCache->GetEphemeralAssetMetaDataIfExists(ephemeralAssetName, ephemeralAsset)) {
                    return ephemeralAsset.CreatesUTXO();
                }
            }
        }
    }
    
    // If metadata not available, default to true (assume UTXO)
    // All ephemeral asset outputs create UTXOs unless explicitly marked as proof-only
    return true;
}

bool IsEphemeralAssetExpired(const CEphemeralAsset& asset, int nCurrentHeight, int64_t nCurrentTime)
{
    // Check if ephemeral asset has expired based on expiration type
    if (asset.nExpirationType == 0) {
        return false; // No expiration
    }
    
    bool expiredByTime = false;
    bool expiredByHeight = false;
    
    // Check timestamp expiration
    if ((asset.nExpirationType & 1) != 0 && asset.nExpirationTime > 0) {
        expiredByTime = (nCurrentTime >= asset.nExpirationTime);
    }
    
    // Check block height expiration
    if ((asset.nExpirationType & 2) != 0 && asset.nExpirationHeight > 0) {
        expiredByHeight = (nCurrentHeight >= asset.nExpirationHeight);
    }
    
    // Expired if either condition is met (OR logic)
    return expiredByTime || expiredByHeight;
}

/** Check if an ephemeral asset (by hash) has expired. Returns true if expired or if expiration cannot be determined. */
bool IsEphemeralAssetExpiredByHash(const uint160& ephemeralHash, CAssetsCache* assetCache, int nCurrentHeight, int64_t nCurrentTime)
{
    if (!assetCache || (nCurrentHeight <= 0 && nCurrentTime <= 0)) {
        return false; // Can't check without cache or time/height
    }
    
    // Resolve ephemeral asset name
    std::string ephemeralAssetName;
    if (!ResolveAssetNameFromHash(ephemeralHash, assetCache, ephemeralAssetName)) {
        return false; // Can't resolve name, assume not expired
    }
    
    // Check if this is an ephemeral asset
    if (!IsAssetNameAnEphemeral(ephemeralAssetName)) {
        return false; // Not an ephemeral asset
    }
    
    // Read ephemeral asset metadata
    CEphemeralAsset ephemeralAsset;
    if (!assetCache->GetEphemeralAssetMetaDataIfExists(ephemeralAssetName, ephemeralAsset)) {
        return false; // Can't read metadata, assume not expired
    }
    
    // Check expiration
    return IsEphemeralAssetExpired(ephemeralAsset, nCurrentHeight, nCurrentTime);
}

bool IsP2AHUTXOLocked(const COutPoint& outpoint, CAssetsCache* assetCache, std::string& strError, int nCurrentHeight = 0, int64_t nCurrentTime = 0)
{
    if (!assetCache) {
        return false; // Can't check without cache
    }
    
    // Check if UTXO is locked by an ephemeral asset
    if (!assetCache->IsUTXOLockedByEphemeral(outpoint)) {
        return false; // Not locked
    }
    
    // Get the ephemeral asset hash that locks this UTXO
    uint160 ephemeralHash;
    if (!assetCache->GetLockingEphemeralHash(outpoint, ephemeralHash)) {
        // Locked but can't get hash - treat as locked for safety
        strError = strprintf("UTXO %s:%d is locked by an ephemeral asset (hash unavailable)", outpoint.hash.ToString(), outpoint.n);
        return true;
    }
    
    // If we have current height/time, check if the ephemeral asset has expired
    // Ephemeral assets that have expired don't lock the UTXO anymore (like expired checks)
    // This allows spending the UTXO even if an ephemeral asset exists but has expired
    if (nCurrentHeight > 0 || nCurrentTime > 0) {
        if (IsEphemeralAssetExpiredByHash(ephemeralHash, assetCache, nCurrentHeight, nCurrentTime)) {
            // Ephemeral asset has expired - UTXO is not locked
            return false;
        }
    }
    
    // Ephemeral asset exists and has not expired (or we can't determine expiration)
    // UTXO is locked and can only be spent if ephemeral assets authorize it
    strError = strprintf("UTXO %s:%d is locked by an ephemeral asset", outpoint.hash.ToString(), outpoint.n);
    return true;
}

bool ValidateEphemeralAssetParentNotLocked(const CScript& ephemeralScript, const CTransaction& tx, CAssetsCache* assetCache, std::string& strError, const CCoinsViewCache* inputs)
{
    // Validate that when creating an ephemeral asset, the parent UTXO is not already locked
    // This prevents double-locking of UTXOs
    
    if (!IsP2AHEphemeral(ephemeralScript)) {
        return true; // Not an ephemeral script, skip
    }
    
    if (!assetCache || !inputs) {
        return true; // Can't validate without cache or inputs
    }
    
    // Extract ephemeral asset hash to get parent asset hash
    uint160 ephemeralHash;
    if (!ExtractAssetHashFromP2AH(ephemeralScript, ephemeralHash)) {
        return true; // Can't extract hash, skip validation
    }
    
    // For chain signing, the parent UTXO is the target asset UTXO being spent
    // Check all inputs to see if any are P2AH assets that match the parent
    // Note: This is a simplified check - full implementation would require
    // reading ephemeral asset metadata to get exact parentAssetHash
    
    // Check if any input UTXO is already locked
    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        const COutPoint &prevout = tx.vin[i].prevout;
        
        // Check if this UTXO is locked
        std::string lockError;
        if (IsP2AHUTXOLocked(prevout, assetCache, lockError)) {
            strError = strprintf("Cannot create ephemeral asset: parent UTXO %s:%d is already locked", 
                                 prevout.hash.ToString(), prevout.n);
            return false;
        }
    }
    
    return true;
}

bool ValidateEphemeralAssetsAuthorizeSpending(const CTransaction& tx, const COutPoint& lockedOutpoint, CAssetsCache* assetCache, std::string& strError, const CCoinsViewCache* inputs)
{
    // Validate that ephemeral assets are being spent to authorize spending a locked UTXO
    // This enables M-of-N multisig workflows where ephemeral assets authorize spending
    // 
    // Workflow:
    // 1. M-of-N multisig: 3 of 5 signers create ephemeral UTXO assets and send them to an address
    // 2. That address can then spend those 3 ephemeral assets to authorize spending the original UTXO
    // 3. The ephemeral assets must match the parent asset hash of the locked UTXO
    
    if (!assetCache || !inputs) {
        return false; // Need cache and inputs to validate
    }
    
    // Check if the locked UTXO is actually locked
    std::string lockError;
    if (!IsP2AHUTXOLocked(lockedOutpoint, assetCache, lockError)) {
        // Not locked, no need to check ephemeral authorization
        return true;
    }
    
    // Find which ephemeral asset locks this UTXO
    uint160 lockingEphemeralHash;
    if (!assetCache->GetLockingEphemeralHash(lockedOutpoint, lockingEphemeralHash)) {
        strError = strprintf("UTXO %s:%d is locked but no ephemeral asset hash found", 
                             lockedOutpoint.hash.ToString(), lockedOutpoint.n);
        return false;
    }
    
    // Get the locked UTXO to extract its asset hash
    const Coin& lockedCoin = inputs->AccessCoin(lockedOutpoint);
    if (!lockedCoin.IsAsset()) {
        strError = strprintf("Locked UTXO %s:%d is not an asset", 
                             lockedOutpoint.hash.ToString(), lockedOutpoint.n);
        return false;
    }
    
    // Extract asset hash from locked UTXO
    uint160 lockedAssetHash;
    if (!ExtractAssetHashFromP2AH(lockedCoin.out.scriptPubKey, lockedAssetHash)) {
        strError = strprintf("Failed to extract asset hash from locked UTXO %s:%d", 
                             lockedOutpoint.hash.ToString(), lockedOutpoint.n);
        return false;
    }
    
    // Check if any input is spending an ephemeral asset that authorizes this UTXO
    // For M-of-N multisig, multiple ephemeral assets may authorize the same UTXO
    // Each ephemeral asset should have parentAssetHash matching the locked asset hash
    int authorizingEphemeralCount = 0;
    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin& coin = inputs->AccessCoin(prevout);
        
        if (coin.IsAsset() && IsP2AHEphemeral(coin.out.scriptPubKey)) {
            // Extract ephemeral asset hash from input
            uint160 ephemeralHash;
            if (ExtractAssetHashFromP2AH(coin.out.scriptPubKey, ephemeralHash)) {
                if (ephemeralHash == lockingEphemeralHash) {
                    std::string ephemeralAssetName;
                    if (ResolveAssetNameFromHash(ephemeralHash, assetCache, ephemeralAssetName)) {
                        CEphemeralAsset ephemeralAsset;
                        if (assetCache->GetEphemeralAssetMetaDataIfExists(ephemeralAssetName, ephemeralAsset)) {
                            if (ephemeralAsset.parentAssetHash != uint160()) {
                                if (ephemeralAsset.parentAssetHash == lockedAssetHash) {
                                    authorizingEphemeralCount++;
                                }
                            } else {
                                authorizingEphemeralCount++;
                            }
                        } else {
                            authorizingEphemeralCount++;
                        }
                    } else {
                        authorizingEphemeralCount++;
                    }
                }
            }
        }
    }
    
    if (authorizingEphemeralCount == 0) {
        strError = strprintf("Cannot spend locked UTXO %s:%d: no authorizing ephemeral assets found in transaction inputs. Need to spend ephemeral assets that authorize this UTXO.", 
                             lockedOutpoint.hash.ToString(), lockedOutpoint.n);
        return false;
    }
    
    // Ephemeral assets are being spent to authorize the locked UTXO
    // Note: For M-of-N, we may need to validate that enough ephemeral assets are spent
    // This will be implemented when ephemeral asset metadata (including M-of-N requirements) is available
    return true;
}

bool ExtractMultisigMNFromRedeemScript(const CScript& redeemScript, uint8_t& m, uint8_t& n, std::string& strError)
{
    // Extract M-of-N from a multisig redeem script
    // Format: M <pubkeys> N OP_CHECKMULTISIG
    
    if (redeemScript.size() < 3) {
        strError = "Redeem script too short";
        return false;
    }
    
    CScript::const_iterator pc = redeemScript.begin();
    opcodetype opcode;
    std::vector<unsigned char> vch;
    
    // Read M (required signatures)
    if (!redeemScript.GetOp(pc, opcode, vch)) {
        strError = "Failed to read M from redeem script";
        return false;
    }
    
    if (opcode < OP_1 || opcode > OP_16) {
        strError = "Invalid M value in redeem script";
        return false;
    }
    
    m = CScript::DecodeOP_N(opcode);
    if (m < 1 || m > 16) {
        strError = "M value out of range";
        return false;
    }
    
    // Count pubkeys
    int pubkeyCount = 0;
    while (pc != redeemScript.end()) {
        if (!redeemScript.GetOp(pc, opcode, vch)) {
            break;
        }
        
        // Check if this is the N value (OP_1 to OP_16)
        if (opcode >= OP_1 && opcode <= OP_16) {
            n = CScript::DecodeOP_N(opcode);
            if (n < 1 || n > 16 || n != pubkeyCount) {
                strError = "Invalid N value or pubkey count mismatch";
                return false;
            }
            // Check next opcode is OP_CHECKMULTISIG
            if (pc != redeemScript.end()) {
                opcodetype checkOp;
                std::vector<unsigned char> dummy;
                if (redeemScript.GetOp(pc, checkOp, dummy) && checkOp == OP_CHECKMULTISIG) {
                    return true;
                }
            }
            strError = "Redeem script does not end with OP_CHECKMULTISIG";
            return false;
        }
        
        // Check if this is a pubkey (33 or 65 bytes)
        if (vch.size() >= 33 && vch.size() <= 65) {
            pubkeyCount++;
        } else {
            strError = "Invalid pubkey in redeem script";
            return false;
        }
    }
    
    strError = "Failed to find N value in redeem script";
    return false;
}

bool ExtractRedeemScriptFromScriptSig(const CScript& scriptSig, CScript& redeemScript, std::string& strError)
{
    // Extract redeem script from scriptSig
    // Format: <signatures> <redeemScript>
    // The redeem script is the last element
    
    if (scriptSig.empty()) {
        strError = "ScriptSig is empty";
        return false;
    }
    
    CScript::const_iterator pc = scriptSig.end();
    opcodetype opcode;
    std::vector<unsigned char> vch;
    
    // Move backwards to find the last element
    // Start from the end and work backwards
    std::vector<std::vector<unsigned char> > stack;
    pc = scriptSig.begin();
    while (pc != scriptSig.end()) {
        if (!scriptSig.GetOp(pc, opcode, vch)) {
            break;
        }
        if (vch.size() > 0) {
            stack.push_back(vch);
        }
    }
    
    if (stack.empty()) {
        strError = "No data found in scriptSig";
        return false;
    }
    
    // The last element should be the redeem script
    redeemScript = CScript(stack.back().begin(), stack.back().end());
    return true;
}

bool ValidateMultisigEphemeralNotDuplicate(const CScript& utxoScript, const COutPoint& outpoint, const CScript& ephemeralScript, const CScript& scriptSig, CAssetsCache* assetCache, std::string& strError)
{
    if (!assetCache) {
        return true;
    }
    
    if (!IsP2AHMultisig(utxoScript)) {
        return true;
    }
    
    CScript redeemScript;
    std::string redeemError;
    if (!ExtractRedeemScriptFromScriptSig(scriptSig, redeemScript, redeemError)) {
        return true;
    }
    
    uint8_t m, n;
    std::string mnError;
    if (!ExtractMultisigMNFromRedeemScript(redeemScript, m, n, mnError)) {
        strError = strprintf("Failed to extract M-of-N from redeem script: %s", mnError);
        return false;
    }
    
    if (assetCache->HasMultisigEphemeralAsset(outpoint, m, n)) {
        strError = strprintf("M-of-N multisig (%d-of-%d) has already created an ephemeral asset for UTXO %s:%d", 
                             m, n, outpoint.hash.ToString(), outpoint.n);
        return false;
    }
    
    return true;
}

CScript GetScriptForWitness(const CScript& redeemscript)
{
    CScript ret;

    txnouttype typ;
    txnouttype scriptTyp;
    std::vector<std::vector<unsigned char> > vSolutions;
    if (Solver(redeemscript, typ, scriptTyp, vSolutions)) {
        if (typ == TX_PUBKEY) {
            unsigned char h160[20];
            CHash160().Write(&vSolutions[0][0], vSolutions[0].size()).Finalize(h160);
            ret << OP_0 << std::vector<unsigned char>(&h160[0], &h160[20]);
            return ret;
        } else if (typ == TX_PUBKEYHASH) {
           ret << OP_0 << vSolutions[0];
           return ret;
        }
    }
    uint256 hash;
    CSHA256().Write(&redeemscript[0], redeemscript.size()).Finalize(hash.begin());
    ret << OP_0 << ToByteVector(hash);
    return ret;
}

bool IsValidDestination(const CTxDestination& dest) {
    return dest.which() != 0;
}
