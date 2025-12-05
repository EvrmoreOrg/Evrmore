// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2022 The Evrmore Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EVRMORE_ASSETDB_H
#define EVRMORE_ASSETDB_H

#include "fs.h"
#include "serialize.h"

#include <string>
#include <map>
#include <dbwrapper.h>

const int8_t ASSET_UNDO_INCLUDES_VERIFIER_STRING = -1;
const int8_t ASSET_UNDO_INCLUDES_TOLL_FIELDS = -2;

class CNewAsset;
class uint256;
class COutPoint;
class CDatabasedAssetData;

struct CBlockAssetUndo
{
    bool fChangedIPFS;
    bool fChangedUnits;
    std::string strIPFS;
    int32_t nUnits;
    int8_t version;
    bool fChangedVerifierString;
    std::string verifierString;

    bool fChangedPermanentIPFSHash;
    bool fChangedTollAmountMutability;
    bool fChangedTollAmount;
    bool fChangedTollAddressMutability;
    bool fChangedTollAddress;
    std::string strPermanentIPFSHash;
    CAmount nTollAmount;
    std::string strTollAddress;

    bool fChangeReissuable;
    bool fChangedRemintable;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(fChangedUnits);
        READWRITE(fChangedIPFS);
        READWRITE(strIPFS);
        READWRITE(nUnits);
        if (ser_action.ForRead()) {
            if (!s.empty() and s.size() >= 1) {
                int8_t nVersionCheck;
                ::Unserialize(s, nVersionCheck);

                if (nVersionCheck == ASSET_UNDO_INCLUDES_VERIFIER_STRING || nVersionCheck == ASSET_UNDO_INCLUDES_TOLL_FIELDS) {
                    ::Unserialize(s, fChangedVerifierString);
                    ::Unserialize(s, verifierString);
                }

                // Check for the new version that includes toll fields
                if (nVersionCheck == ASSET_UNDO_INCLUDES_TOLL_FIELDS) {
                    ::Unserialize(s, fChangedPermanentIPFSHash);
                    ::Unserialize(s, fChangedTollAmountMutability);
                    ::Unserialize(s, fChangedTollAmount);
                    ::Unserialize(s, fChangedTollAddressMutability);
                    ::Unserialize(s, fChangedTollAddress);
                    ::Unserialize(s, strPermanentIPFSHash);
                    ::Unserialize(s, nTollAmount);
                    ::Unserialize(s, strTollAddress);
                    ::Unserialize(s, fChangeReissuable);
                    ::Unserialize(s, fChangedRemintable);
                }

                version = nVersionCheck;
            }
        } else {
            ::Serialize(s, version);
            if (version == ASSET_UNDO_INCLUDES_VERIFIER_STRING || version == ASSET_UNDO_INCLUDES_TOLL_FIELDS) {
                ::Serialize(s, fChangedVerifierString);
                ::Serialize(s, verifierString);
            }

            if (version == ASSET_UNDO_INCLUDES_TOLL_FIELDS) {
                ::Serialize(s, fChangedPermanentIPFSHash);
                ::Serialize(s, fChangedTollAmountMutability);
                ::Serialize(s, fChangedTollAmount);
                ::Serialize(s, fChangedTollAddressMutability);
                ::Serialize(s, fChangedTollAddress);
                ::Serialize(s, strPermanentIPFSHash);
                ::Serialize(s, nTollAmount);
                ::Serialize(s, strTollAddress);
                ::Serialize(s, fChangeReissuable);
                ::Serialize(s, fChangedRemintable);
            }
        }
    }
};

/** Access to the block database (blocks/index/) */
class CAssetsDB : public CDBWrapper
{
public:
    explicit CAssetsDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    CAssetsDB(const CAssetsDB&) = delete;
    CAssetsDB& operator=(const CAssetsDB&) = delete;

    // Write to database functions
    bool WriteAssetData(const CNewAsset& asset, const int nHeight, const uint256& blockHash);
    bool WriteAssetAddressQuantity(const std::string& assetName, const std::string& address, const CAmount& quantity);
    bool WriteAddressAssetQuantity( const std::string& address, const std::string& assetName, const CAmount& quantity);
    bool WriteBlockUndoAssetData(const uint256& blockhash, const std::vector<std::pair<std::string, CBlockAssetUndo> >& assetUndoData);
    bool WriteReissuedMempoolState();

    // Read from database functions
    bool ReadAssetData(const std::string& strName, CNewAsset& asset, int& nHeight, uint256& blockHash);
    bool ReadAssetAddressQuantity(const std::string& assetName, const std::string& address, CAmount& quantity);
    bool ReadAddressAssetQuantity(const std::string& address, const std::string& assetName, CAmount& quantity);
    bool ReadBlockUndoAssetData(const uint256& blockhash, std::vector<std::pair<std::string, CBlockAssetUndo> >& assetUndoData);
    bool ReadReissuedMempoolState();

    // Erase from database functions
    bool EraseAssetData(const std::string& assetName);
    bool EraseMyAssetData(const std::string& assetName);
    bool EraseAssetAddressQuantity(const std::string &assetName, const std::string &address);
    bool EraseAddressAssetQuantity(const std::string &address, const std::string &assetName);

    // Restricted P2AH address requirements methods (reserved for restricted P2AH addresses only)
    bool WriteRestrictedP2AHAddressRequirements(const uint160& assetHash, const CRestrictedP2AHAddressRequirements& requirements);
    bool ReadRestrictedP2AHAddressRequirements(const uint160& assetHash, CRestrictedP2AHAddressRequirements& requirements);
    bool EraseRestrictedP2AHAddressRequirements(const uint160& assetHash);
    
    // P2AH multisig signing requirements methods
    bool WriteP2AHMultisigSigningRequirements(const uint160& multisigAssetHash, const CP2AHMultisigSigningRequirements& requirements);
    bool ReadP2AHMultisigSigningRequirements(const uint160& multisigAssetHash, CP2AHMultisigSigningRequirements& requirements);
    bool EraseP2AHMultisigSigningRequirements(const uint160& multisigAssetHash);

    // Helper functions
    bool LoadAssets();
    bool AssetDir(std::vector<CDatabasedAssetData>& assets, const std::string filter, const size_t count, const long start);
    bool AssetDir(std::vector<CDatabasedAssetData>& assets);

    bool AddressDir(std::vector<std::pair<std::string, CAmount> >& vecAssetAmount, int& totalEntries, const bool& fGetTotal, const std::string& address, const size_t count, const long start);
    bool AssetAddressDir(std::vector<std::pair<std::string, CAmount> >& vecAddressAmount, int& totalEntries, const bool& fGetTotal, const std::string& assetName, const size_t count, const long start);
};


#endif //EVRMORE_ASSETDB_H
