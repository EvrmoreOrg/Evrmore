// Copyright (c) 2017-2017 The Bitcoin Core developers
// Copyright (c) 2017-2021 The Raven Core developers
// Copyright (c) 2022 The Evrmore Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assets/assets.h>
#include <script/standard.h>
#include <util.h>
#include <validation.h>
#include "tx_verify.h"
#include "chainparams.h"

#include "consensus.h"
#include "primitives/transaction.h"
#include "primitives/block.h"
#include "primitives/outpoint.h"
#include "script/interpreter.h"
#include "validation.h"
#include <cmath>
#include <wallet/wallet.h>
#include <base58.h>
#include <tinyformat.h>
#include <limits>

// TODO remove the following dependencies
#include "chain.h"
#include "coins.h"
#include "utilmoneystr.h"

bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime)
{
    if (tx.nLockTime == 0)
        return true;
    if ((int64_t)tx.nLockTime < ((int64_t)tx.nLockTime < LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
        return true;
    for (const auto& txin : tx.vin) {
        if (!(txin.nSequence == CTxIn::SEQUENCE_FINAL))
            return false;
    }
    return true;
}

std::pair<int, int64_t> CalculateSequenceLocks(const CTransaction &tx, int flags, std::vector<int>* prevHeights, const CBlockIndex& block)
{
    assert(prevHeights->size() == tx.vin.size());

    // Will be set to the equivalent height- and time-based nLockTime
    // values that would be necessary to satisfy all relative lock-
    // time constraints given our view of block chain history.
    // The semantics of nLockTime are the last invalid height/time, so
    // use -1 to have the effect of any height or time being valid.
    int nMinHeight = -1;
    int64_t nMinTime = -1;

    // tx.nVersion is signed integer so requires cast to unsigned otherwise
    // we would be doing a signed comparison and half the range of nVersion
    // wouldn't support BIP 68.
    bool fEnforceBIP68 = static_cast<uint32_t>(tx.nVersion) >= 2
                      && flags & LOCKTIME_VERIFY_SEQUENCE;

    // Do not enforce sequence numbers as a relative lock time
    // unless we have been instructed to
    if (!fEnforceBIP68) {
        return std::make_pair(nMinHeight, nMinTime);
    }

    for (size_t txinIndex = 0; txinIndex < tx.vin.size(); txinIndex++) {
        const CTxIn& txin = tx.vin[txinIndex];

        // Sequence numbers with the most significant bit set are not
        // treated as relative lock-times, nor are they given any
        // consensus-enforced meaning at this point.
        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG) {
            // The height of this input is not relevant for sequence locks
            (*prevHeights)[txinIndex] = 0;
            continue;
        }

        int nCoinHeight = (*prevHeights)[txinIndex];

        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG) {
            int64_t nCoinTime = block.GetAncestor(std::max(nCoinHeight-1, 0))->GetMedianTimePast();
            // NOTE: Subtract 1 to maintain nLockTime semantics
            // BIP 68 relative lock times have the semantics of calculating
            // the first block or time at which the transaction would be
            // valid. When calculating the effective block time or height
            // for the entire transaction, we switch to using the
            // semantics of nLockTime which is the last invalid block
            // time or height.  Thus we subtract 1 from the calculated
            // time or height.

            // Time-based relative lock-times are measured from the
            // smallest allowed timestamp of the block containing the
            // txout being spent, which is the median time past of the
            // block prior.
            nMinTime = std::max(nMinTime, nCoinTime + (int64_t)((txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK) << CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) - 1);
        } else {
            nMinHeight = std::max(nMinHeight, nCoinHeight + (int)(txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK) - 1);
        }
    }

    return std::make_pair(nMinHeight, nMinTime);
}

bool EvaluateSequenceLocks(const CBlockIndex& block, std::pair<int, int64_t> lockPair)
{
    assert(block.pprev);
    int64_t nBlockTime = block.pprev->GetMedianTimePast();
    if (lockPair.first >= block.nHeight || lockPair.second >= nBlockTime)
        return false;

    return true;
}

bool SequenceLocks(const CTransaction &tx, int flags, std::vector<int>* prevHeights, const CBlockIndex& block)
{
    return EvaluateSequenceLocks(block, CalculateSequenceLocks(tx, flags, prevHeights, block));
}

unsigned int GetLegacySigOpCount(const CTransaction& tx)
{
    unsigned int nSigOps = 0;
    for (const auto& txin : tx.vin)
    {
        nSigOps += txin.scriptSig.GetSigOpCount(false);
    }
    for (const auto& txout : tx.vout)
    {
        nSigOps += txout.scriptPubKey.GetSigOpCount(false);
    }
    return nSigOps;
}

unsigned int GetP2SHSigOpCount(const CTransaction& tx, const CCoinsViewCache& inputs)
{
    if (tx.IsCoinBase())
        return 0;

    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const Coin& coin = inputs.AccessCoin(tx.vin[i].prevout);
        assert(!coin.IsSpent());
        const CTxOut &prevout = coin.out;
        if (prevout.scriptPubKey.IsPayToScriptHash())
            nSigOps += prevout.scriptPubKey.GetSigOpCount(tx.vin[i].scriptSig);
    }
    return nSigOps;
}

int64_t GetTransactionSigOpCost(const CTransaction& tx, const CCoinsViewCache& inputs, int flags)
{
    int64_t nSigOps = GetLegacySigOpCount(tx) * WITNESS_SCALE_FACTOR;

    if (tx.IsCoinBase())
        return nSigOps;

    if (flags & SCRIPT_VERIFY_P2SH) {
        nSigOps += GetP2SHSigOpCount(tx, inputs) * WITNESS_SCALE_FACTOR;
    }

    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const Coin& coin = inputs.AccessCoin(tx.vin[i].prevout);
        assert(!coin.IsSpent());
        const CTxOut &prevout = coin.out;
        nSigOps += CountWitnessSigOps(tx.vin[i].scriptSig, prevout.scriptPubKey, &tx.vin[i].scriptWitness, flags);
    }
    return nSigOps;
}

bool CheckTransaction(const CTransaction& tx, CValidationState &state, bool fCheckDuplicateInputs, bool fMempoolCheck, bool fBlockCheck)
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vin-empty");
    if (tx.vout.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vout-empty");
    // Size limits (this doesn't take the witness into account, as that hasn't been checked for malleability)
    if (::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * WITNESS_SCALE_FACTOR > GetMaxBlockWeight())
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-oversize");

    // Check for negative or overflow output values
    CAmount nValueOut = 0;
    std::set<std::string> setAssetTransferNames;
    std::map<std::pair<std::string, std::string>, int> mapNullDataTxCount; // (asset_name, address) -> int
    std::set<std::string> setNullGlobalAssetChanges;
    bool fContainsNewRestrictedAsset = false;
    bool fContainsRestrictedAssetReissue = false;
    bool fContainsNullAssetVerifierTx = false;
    int nCountAddTagOuts = 0;
    for (const auto& txout : tx.vout)
    {
        if (txout.nValue < 0)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-negative");
        if (txout.nValue > MAX_MONEY)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-toolarge");
        nValueOut += txout.nValue;
        if (!MoneyRange(nValueOut))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-txouttotal-toolarge");

        /** RVN START */
        // Find and handle all new OP_EVR_ASSET null data transactions
        if (txout.scriptPubKey.IsNullAsset(IsTollsActive())) {
            CNullAssetTxData data;
            std::string address;
            std::string strError = "";

            if (txout.scriptPubKey.IsNullAssetTxDataScript(IsTollsActive())) {
                if (!AssetNullDataFromScript(txout.scriptPubKey, data, address))
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-null-asset-data-serialization");

                if (!VerifyNullAssetDataFlag(data.flag, strError))
                    return state.DoS(100, false, REJECT_INVALID, strError);

                auto pair = std::make_pair(data.asset_name, address);
                if(!mapNullDataTxCount.count(pair)){
                    mapNullDataTxCount.insert(std::make_pair(pair, 0));
                }

                mapNullDataTxCount.at(pair)++;

                if (mapNullDataTxCount.at(pair) > 1)
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-null-data-only-one-change-per-asset-address");

                // For each qualifier that is added, there is a burn fee
                if (IsAssetNameAQualifier(data.asset_name)) {
                    if (data.flag == (int)QualifierType::ADD_QUALIFIER) {
                        nCountAddTagOuts++;
                    }
                }

            } else if (txout.scriptPubKey.IsNullGlobalRestrictionAssetTxDataScript()) {
                if (!GlobalAssetNullDataFromScript(txout.scriptPubKey, data))
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-null-global-asset-data-serialization");

                if (!VerifyNullAssetDataFlag(data.flag, strError))
                    return state.DoS(100, false, REJECT_INVALID, strError);

                if (setNullGlobalAssetChanges.count(data.asset_name)) {
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-null-data-only-one-global-change-per-asset-name");
                }

                setNullGlobalAssetChanges.insert(data.asset_name);

            } else if (txout.scriptPubKey.IsNullAssetVerifierTxDataScript()) {

                if (!CheckVerifierAssetTxOut(txout, strError))
                    return state.DoS(100, false, REJECT_INVALID, strError);

                if (fContainsNullAssetVerifierTx)
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-null-data-only-one-verifier-per-tx");

                fContainsNullAssetVerifierTx = true;
            }
        }
        /** RVN END */

        /** RVN START */
        bool isAsset = false;
        int nType;
        bool fIsOwner;
        if (txout.scriptPubKey.IsAssetScript(nType, fIsOwner))
            isAsset = true;
        
        // Check for transfers that don't meet the assets units only if the assetCache is not null
        if (isAsset) {
            // Get the transfer transaction data from the scriptPubKey
            if (nType == TX_TRANSFER_ASSET) {
                CAssetTransfer transfer;
                std::string address;
                if (!TransferAssetFromScript(txout.scriptPubKey, transfer, address))
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-asset-bad-deserialize");

                // insert into set, so that later on we can check asset null data transactions
                setAssetTransferNames.insert(transfer.strName);

                // Check asset name validity and get type
                AssetType assetType;
                if (!IsAssetNameValid(transfer.strName, assetType)) {
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-asset-name-invalid");
                }

                // If the transfer is an ownership asset. Check to make sure that it is OWNER_ASSET_AMOUNT
                if (IsAssetNameAnOwner(transfer.strName)) {
                    if (transfer.nAmount != OWNER_ASSET_AMOUNT)
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-owner-amount-was-not-1");
                }

                // If the transfer is a unique asset. Check to make sure that it is UNIQUE_ASSET_AMOUNT
                if (assetType == AssetType::UNIQUE) {
                    if (transfer.nAmount != UNIQUE_ASSET_AMOUNT)
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-unique-amount-was-not-1");
                }

                // If the transfer is a restricted channel asset.
                if (assetType == AssetType::RESTRICTED) {
                    // TODO add checks here if any
                }

                // If the transfer is a qualifier channel asset.
                if (assetType == AssetType::QUALIFIER || assetType == AssetType::SUB_QUALIFIER) {
                    if (transfer.nAmount < QUALIFIER_ASSET_MIN_AMOUNT || transfer.nAmount > QUALIFIER_ASSET_MAX_AMOUNT)
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-qualifier-amount-must be between 1 - 100");
                }
                
                // Specific check and error message to go with to make sure the amount is 0
                if (txout.nValue != 0)
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-asset-transfer-amount-isn't-zero");
            } else if (nType == TX_NEW_ASSET) {
                // Specific check and error message to go with to make sure the amount is 0
                if (txout.nValue != 0)
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-asset-issued-amount-isn't-zero");
            } else if (nType == TX_REISSUE_ASSET) {
                // Specific check and error message to go with to make sure the amount is 0
                if (AreEnforcedValuesDeployed()) {
                    // We only want to not accept these txes when checking them from CheckBlock.
                    // We don't want to change the behavior when reading transactions from the database
                    // when AreEnforcedValuesDeployed return true
                    if (fBlockCheck) {
                        if (txout.nValue != 0) {
                            return state.DoS(0, false, REJECT_INVALID, "bad-txns-asset-reissued-amount-isn't-zero");
                        }
                    }
                }

                if (fMempoolCheck) {
                    // Don't accept to the mempool no matter what on these types of transactions
                    if (txout.nValue != 0) {
                        return state.DoS(0, false, REJECT_INVALID, "bad-mempool-txns-asset-reissued-amount-isn't-zero");
                    }
                }
            } else {
                return state.DoS(0, false, REJECT_INVALID, "bad-asset-type-not-any-of-the-main-three");
            }
        }
    }

    // Check for Add Tag Burn Fee
    if (nCountAddTagOuts) {
        if (!tx.CheckAddingTagBurnFee(nCountAddTagOuts))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-tx-doesn't-contain-required-burn-fee-for-adding-tags");
    }

    for (auto entry: mapNullDataTxCount) {
        if (entry.first.first.front() == RESTRICTED_CHAR) {
            std::string ownerToken = entry.first.first.substr(1,  entry.first.first.size()); // $TOKEN into TOKEN
            if (!setAssetTransferNames.count(ownerToken + OWNER_TAG)) {
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-tx-contains-restricted-asset-null-tx-without-asset-transfer");
            }
        } else { // must be a qualifier asset QUALIFIER_CHAR
            if (!setAssetTransferNames.count(entry.first.first)) {
                return state.DoS(100, false, REJECT_INVALID,
                                 "bad-txns-tx-contains-qualifier-asset-null-tx-without-asset-transfer");
            }
        }
    }

    for (auto name: setNullGlobalAssetChanges) {
        if (name.size() == 0)
            return state.DoS(100, false, REJECT_INVALID,"bad-txns-tx-contains-global-asset-null-tx-with-null-asset-name");

        std::string rootName = name.substr(1,  name.size()); // $TOKEN into TOKEN
        if (!setAssetTransferNames.count(rootName + OWNER_TAG)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-tx-contains-global-asset-null-tx-without-asset-transfer");
        }
    }

    /** RVN END */

    if (fCheckDuplicateInputs) {
        std::set<COutPoint> vInOutPoints;
        for (const auto& txin : tx.vin)
        {
            if (!vInOutPoints.insert(txin.prevout).second)
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-duplicate");
        }
    }

    if (tx.IsCoinBase())
    {
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > 100)
            return state.DoS(100, false, REJECT_INVALID, "bad-cb-length");

        if (AreCoinbaseCheckAssetsDeployed()) {
            for (auto vout : tx.vout) {
                if (vout.scriptPubKey.IsAssetScript() || vout.scriptPubKey.IsNullAsset(IsTollsActive())) {
                    return state.DoS(0, error("%s: coinbase contains asset transaction", __func__),
                                     REJECT_INVALID, "bad-txns-coinbase-contains-asset-txes");
                }
            }
        }
    }
    else
    {
        for (const auto& txin : tx.vin)
            if (txin.prevout.IsNull())
                return state.DoS(10, false, REJECT_INVALID, "bad-txns-prevout-null");
    }

    /** RVN START */
    if (tx.IsNewAsset()) {
        /** Verify the reissue assets data */
        std::string strError = "";
        if(!tx.VerifyNewAsset(strError))
            return state.DoS(100, false, REJECT_INVALID, strError);

        CNewAsset asset;
        std::string strAddress;
        if (!AssetFromTransaction(tx, asset, strAddress))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-asset-from-transaction");

        // Validate the new assets information
        if (!IsNewOwnerTxValid(tx, asset.strName, strAddress, strError))
            return state.DoS(100, false, REJECT_INVALID, strError);

        if(!CheckNewAsset(asset, strError))
            return state.DoS(100, false, REJECT_INVALID, strError);

    } else if (tx.IsReissueAsset()) {

        /** Verify the reissue assets data */
        std::string strError;
        if (!tx.VerifyReissueAsset(strError))
            return state.DoS(100, false, REJECT_INVALID, strError);

        CReissueAsset reissue;
        std::string strAddress;
        if (!ReissueAssetFromTransaction(tx, reissue, strAddress))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-reissue-asset");

        if (!CheckReissueAsset(reissue, strError))
            return state.DoS(100, false, REJECT_INVALID, strError);

        // Get the assetType
        AssetType type;
        IsAssetNameValid(reissue.strName, type);

        // If this is a reissuance of a restricted asset, mark it as such, so we can check to make sure only valid verifier string tx are added to the chain
        if (type == AssetType::RESTRICTED) {
            CNullAssetTxVerifierString new_verifier;
            bool fNotFound = false;

            // Try and get the verifier string if it was changed
            if (!tx.GetVerifierStringFromTx(new_verifier, strError, fNotFound)) {
                // If it return false for any other reason besides not being found, fail the transaction check
                if (!fNotFound) {
                    return state.DoS(100, false, REJECT_INVALID,
                                     "bad-txns-reissue-restricted-verifier-" + strError);
                }
            }

            fContainsRestrictedAssetReissue = true;
        }

    } else if (tx.IsNewUniqueAsset()) {

        /** Verify the unique assets data */
        std::string strError = "";
        if (!tx.VerifyNewUniqueAsset(strError)) {
            return state.DoS(100, false, REJECT_INVALID, strError);
        }


        for (auto out : tx.vout)
        {
            if (IsScriptNewUniqueAsset(out.scriptPubKey))
            {
                CNewAsset asset;
                std::string strAddress;
                if (!AssetFromScript(out.scriptPubKey, asset, strAddress))
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-check-transaction-issue-unique-asset-serialization");

                if (!CheckNewAsset(asset, strError))
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-unique" + strError);
            }
        }
    } else if (tx.IsNewMsgChannelAsset()) {
        /** Verify the msg channel assets data */
        std::string strError = "";
        if(!tx.VerifyNewMsgChannelAsset(strError))
            return state.DoS(100, false, REJECT_INVALID, strError);

        CNewAsset asset;
        std::string strAddress;
        if (!MsgChannelAssetFromTransaction(tx, asset, strAddress))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-msgchannel-from-transaction");

        if (!CheckNewAsset(asset, strError))
            return state.DoS(100, error("%s: %s", __func__, strError), REJECT_INVALID, "bad-txns-issue-msgchannel" + strError);

    } else if (tx.IsNewQualifierAsset()) {
        /** Verify the qualifier channel assets data */
        std::string strError = "";
        if(!tx.VerifyNewQualfierAsset(strError))
            return state.DoS(100, false, REJECT_INVALID, strError);

        CNewAsset asset;
        std::string strAddress;
        if (!QualifierAssetFromTransaction(tx, asset, strAddress))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-qualifier-from-transaction");

        if (!CheckNewAsset(asset, strError))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-qualfier" + strError);

    } else if (tx.IsNewRestrictedAsset()) {
        /** Verify the restricted assets data. */
        std::string strError = "";
        if(!tx.VerifyNewRestrictedAsset(strError))
            return state.DoS(100, false, REJECT_INVALID, strError);

        // Get asset data
        CNewAsset asset;
        std::string strAddress;
        if (!RestrictedAssetFromTransaction(tx, asset, strAddress))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-restricted-from-transaction");

        if (!CheckNewAsset(asset, strError))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-restricted" + strError);

        // Get verifier string
        CNullAssetTxVerifierString verifier;
        if (!tx.GetVerifierStringFromTx(verifier, strError))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-restricted-verifier-search-" + strError);

        // Mark that this transaction has a restricted asset issuance, for checks later with the verifier string tx
        fContainsNewRestrictedAsset = true;
    }
    else {
        // Fail if transaction contains any non-transfer asset scripts and hasn't conformed to one of the
        // above transaction types.  Also fail if it contains OP_EVR_ASSET opcode but wasn't a valid script.
        for (auto out : tx.vout) {
            int nType;
            bool _isOwner;
            if (out.scriptPubKey.IsAssetScript(nType, _isOwner)) {
                if (nType != TX_TRANSFER_ASSET) {
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-bad-asset-transaction");
                }
            } else {
                if (out.scriptPubKey.Find(OP_EVR_ASSET)) {
                    if (out.scriptPubKey[0] != OP_EVR_ASSET) {
                        return state.DoS(100, false, REJECT_INVALID,
                                         "bad-txns-op-evr-asset-not-in-right-script-location");
                    }
                }
            }
        }
    }

    // Check to make sure that if there is a verifier string, that there is also a issue or reissuance of a restricted asset
    if (fContainsNullAssetVerifierTx && !fContainsRestrictedAssetReissue && !fContainsNewRestrictedAsset)
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-tx-cointains-verifier-string-without-restricted-asset-issuance-or-reissuance");

    // If there is a restricted asset issuance, verify that there is a verifier tx associated with it.
    if (fContainsNewRestrictedAsset && !fContainsNullAssetVerifierTx) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-tx-cointains-restricted-asset-issuance-without-verifier");
    }

    // we allow restricted asset reissuance without having a verifier string transaction, we don't force it to be update
    /** RVN END */

    return true;
}

bool Consensus::CheckTxInputs(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, int nSpendHeight, CAmount& txfee)
{
    // are the actual inputs available?
    if (!inputs.HaveInputs(tx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-missingorspent", false,
                         strprintf("%s: inputs missing/spent", __func__), tx.GetHash());
    }

    CAmount nValueIn = 0;
    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin& coin = inputs.AccessCoin(prevout);
        assert(!coin.IsSpent());

        // If prev is coinbase, check that it's matured
        if (coin.IsCoinBase() && nSpendHeight - coin.nHeight < COINBASE_MATURITY) {
            return state.Invalid(false,
                REJECT_INVALID, "bad-txns-premature-spend-of-coinbase",
                strprintf("tried to spend coinbase at depth %d", nSpendHeight - coin.nHeight));
        }

        // EVR - If prev is airdrop, check if it's expired airdrop
        if (coin.IsCoinBase() && coin.nHeight == 0 && nSpendHeight > AIRDROP_EXPIRATION) {
            return state.Invalid(false,
                REJECT_INVALID, "bad-txns-spend-of-expired-airdrop",
                strprintf("tried to spend airdrop at chain height %d", nSpendHeight));
        }

        // Check for negative or overflow input values
        nValueIn += coin.out.nValue;
        if (!MoneyRange(coin.out.nValue) || !MoneyRange(nValueIn)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputvalues-outofrange", false, "", tx.GetHash());
        }
    }

    const CAmount value_out = tx.GetValueOut(AreEnforcedValuesDeployed());
    if (nValueIn < value_out) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-in-belowout", false,
            strprintf("value in (%s) < value out (%s)", FormatMoney(nValueIn), FormatMoney(value_out)), tx.GetHash());
    }

    // Tally transaction fees
    const CAmount txfee_aux = nValueIn - value_out;
    if (!MoneyRange(txfee_aux)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-fee-out-of-range", false, "", tx.GetHash());
    }

    txfee = txfee_aux;
    return true;
}

//! Check to make sure that the inputs and outputs CAmount match exactly.
bool Consensus::CheckTxAssets(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, CAssetsCache* assetCache, bool fCheckMempool, std::vector<std::pair<std::string, uint256> >& vPairReissueAssets, const bool fRunningUnitTests, std::set<CMessage>* setMessages, int64_t nBlocktime,   std::vector<std::pair<std::string, CNullAssetTxData>>* myNullAssetData)
{
    // are the actual inputs available?
    if (!inputs.HaveInputs(tx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-missing-or-spent", false,
                         strprintf("%s: inputs missing/spent", __func__), tx.GetHash());
    }

    // Create map that stores the amount of an asset transaction input. Used to verify no assets are burned
    std::map<std::string, CAmount> totalInputs;
    std::map<std::string, std::string> mapAddresses;

    // Maps to store asset address tracking for tolls
    std::map<std::string, std::vector<std::pair<std::string, CAmount>>> assetFromAddresses;
    std::map<std::string, std::vector<std::pair<std::string, CAmount>>> assetToAddresses; // AssetName -> Vector<Address,Amount>

    // Map to track toll amounts required and paid
    std::map<std::string, CAmount> mapRequiredTolls; // Address -> Amount
    std::map<std::string, CAmount> mapEVRSentInTransaction; // Address -> Amount

    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin& coin = inputs.AccessCoin(prevout);
        assert(!coin.IsSpent());

        if (coin.IsAsset()) {
            CAssetOutputEntry data;
            if (!GetAssetData(coin.out.scriptPubKey, data))
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-failed-to-get-asset-from-script", false, "", tx.GetHash());

            // Check if P2AH UTXO is locked by ephemeral asset
            if (coin.out.scriptPubKey.IsP2AHAssetScript() && assetCache) {
                std::string lockError;
                // Check if locked (expired ephemeral assets don't lock the UTXO)
                if (IsP2AHUTXOLocked(prevout, assetCache, lockError, 0, nBlocktime)) {
                    // UTXO is locked by a non-expired ephemeral asset
                    // Check if ephemeral assets are being spent to authorize it
                    // This enables M-of-N multisig workflows
                    std::string authError;
                    if (!ValidateEphemeralAssetsAuthorizeSpending(tx, prevout, assetCache, authError, &inputs)) {
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-p2ah-utxo-locked-by-ephemeral", false, authError, tx.GetHash());
                    }
                    // Ephemeral assets authorize spending - allow it
                }
                
                // Check if this is an M-of-N multisig UTXO and if that specific M has already issued an ephemeral asset
                if (IsP2AHMultisig(coin.out.scriptPubKey)) {
                    // Extract M-of-N from the redeem script in scriptSig
                    const CScript& scriptSig = tx.vin[i].scriptSig;
                    CScript redeemScript;
                    std::string redeemError;
                    if (ExtractRedeemScriptFromScriptSig(scriptSig, redeemScript, redeemError)) {
                        uint8_t m, n;
                        std::string mnError;
                        if (ExtractMultisigMNFromRedeemScript(redeemScript, m, n, mnError)) {
                            // For M=1, proof-only ephemeral assets don't lock the UTXO, so allow direct spending
                            // For M>1, if a UTXO ephemeral asset exists, require ephemeral assets to authorize spending
                            if (m > 1) {
                                // Check if this M-of-N has already created a UTXO ephemeral asset for this UTXO
                                if (assetCache->HasMultisigEphemeralAsset(prevout, m, n)) {
                                    std::string duplicateError = strprintf("M-of-N multisig (%d-of-%d) has already created an ephemeral asset for UTXO %s:%d. Cannot spend UTXO directly - must use ephemeral assets to authorize.", 
                                                                          m, n, prevout.hash.ToString(), prevout.n);
                                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-p2ah-multisig-ephemeral-already-exists", false, duplicateError, tx.GetHash());
                                }
                            }
                            // For M=1, proof-only ephemeral assets are allowed and don't prevent direct spending
                        }
                    }
                }
            }

            // Add to the total value of assets in the inputs
            if (totalInputs.count(data.assetName))
                totalInputs.at(data.assetName) += data.nAmount;
            else
                totalInputs.insert(make_pair(data.assetName, data.nAmount));

            if (AreMessagesDeployed()) {
                mapAddresses.insert(make_pair(data.assetName,EncodeDestination(data.destination)));
            }

            if (IsAssetNameAnRestricted(data.assetName)) {
                if (assetCache->CheckForAddressRestriction(data.assetName, EncodeDestination(data.destination), true)) {
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-restricted-asset-transfer-from-frozen-address", false, "", tx.GetHash());
                }
            }

            // Track the addresses the asset is coming from - except for owners. We don't care about owner assets
            if (!IsAssetNameAnOwner(data.assetName)) {
                auto& fromAddressList = assetFromAddresses[data.assetName]; // Get or create the vector for this asset name

                // Check if the address already exists in the vector
                bool addressFound = false;
                for (auto& entry : fromAddressList) {
                    if (entry.first == EncodeDestination(data.destination)) {
                        // Address exists, sum up the amounts
                        entry.second += data.nAmount;
                        addressFound = true;
                        break;
                    }
                }

                if (!addressFound) {
                    // If address is not found, add a new entry
                    fromAddressList.emplace_back(EncodeDestination(data.destination), data.nAmount);
                }
            }
        }
    }

    // Create map that stores the amount of an asset transaction output. Used to verify no assets are burned
    std::map<std::string, CAmount> totalOutputs;
    int index = 0;
    int64_t currentTime = GetTime();
    std::string strError = "";
    int i = 0;
    for (const auto& txout : tx.vout) {
        i++;

        // Values are subject to change, by isAssetScript a few lines down.
        int nType = 0;
        int nScriptType = 0;
        int nStart = 0;
        bool fIsOwner = false;

        // False until BIP9 consensus activates P2SH for Assets.
        bool fP2Active = AreP2SHAssetsAllowed();

        bool fIsAsset = txout.scriptPubKey.IsAssetScript(nType, nScriptType, fIsOwner, nStart, fP2Active);
        bool fIsNullAssetData = txout.scriptPubKey.IsNullAsset(IsTollsActive());
        bool fIsUnspendable = txout.scriptPubKey.IsUnspendable();

        // Track every EVR being sent in the outputs so we have the data for validating tolls later.
        if (!fIsAsset && !fIsNullAssetData && !fIsUnspendable && txout.nValue > 0) {
            CTxDestination destTracker;
            ExtractDestination(txout.scriptPubKey, destTracker);
            if (!IsValidDestination(destTracker)) {
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-evr-tracker-address-verification", false, "", tx.GetHash());
            }
            std::string toAddress = EncodeDestination(destTracker);
            // TODO - remove after testing
            LogPrintf("Found a EVR send: %s - %d\n", toAddress, txout.nValue);
            if (mapEVRSentInTransaction.count(toAddress)) {
                mapEVRSentInTransaction[toAddress] += txout.nValue;
            } else {
                mapEVRSentInTransaction[toAddress] = txout.nValue;
            }
        }

        // Check for P2AH restricted addresses in outputs
        if (assetCache && IsP2AHRestricted(txout.scriptPubKey)) {
            if (!AreRestrictedAssetsDeployed())
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-p2ah-restricted-before-restricted-assets-activated", false, "", tx.GetHash());
            // P2AH restricted addresses require additional validation (basic check complete)
        }

        // Check for P2AH chain signing addresses in outputs
        if (IsP2AHChainSigning(txout.scriptPubKey)) {
            // Validate chain signing: check if signing asset is spending target asset's UTXO
            if (assetCache) {
                // Check all inputs to see if any contain assets that match chain signing pattern
                for (unsigned int inputIdx = 0; inputIdx < tx.vin.size(); ++inputIdx) {
                    const COutPoint &prevout = tx.vin[inputIdx].prevout;
                    const Coin& coin = inputs.AccessCoin(prevout);
                    if (coin.IsAsset()) {
                        CAssetOutputEntry inputData;
                        if (GetAssetData(coin.out.scriptPubKey, inputData)) {
                            // Check if input asset script is also a chain signing script
                            if (coin.out.scriptPubKey.IsP2AHAssetScript() && IsP2AHChainSigning(coin.out.scriptPubKey)) {
                                std::string strError;
                                if (!ValidateP2AHChainSigning(tx, coin.out.scriptPubKey, txout.scriptPubKey, assetCache, nBlocktime, strError, &inputs)) {
                                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-p2ah-chain-signing-validation", false, strError, tx.GetHash());
                                }
                            }
                        }
                    }
                }
            }
        }

        // Check for P2AH ephemeral addresses in outputs
        if (IsP2AHEphemeral(txout.scriptPubKey)) {
            // Validate ephemeral asset expiration and burn fee
            if (assetCache) {
                std::string strError;
                // Note: nBlockHeight not available in CheckTxAssets, using 0 as placeholder
                // Full block height validation should be done at block validation level
                // Proof-only ephemeral assets are validated but don't create UTXOs
                if (!ValidateP2AHEphemeralAsset(txout.scriptPubKey, tx, assetCache, nBlocktime, 0, strError, &inputs)) {
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-p2ah-ephemeral-validation", false, strError, tx.GetHash());
                }
                
                // Check if this ephemeral asset is for an M-of-N multisig UTXO
                // Validate that this M-of-N hasn't already created an ephemeral asset for the parent UTXO
                // Check all inputs to find the parent UTXO
                for (unsigned int inputIdx = 0; inputIdx < tx.vin.size(); ++inputIdx) {
                    const COutPoint &prevout = tx.vin[inputIdx].prevout;
                    const Coin& coin = inputs.AccessCoin(prevout);
                    if (coin.IsAsset() && coin.out.scriptPubKey.IsP2AHAssetScript()) {
                        // Check if this is a multisig UTXO
                        if (IsP2AHMultisig(coin.out.scriptPubKey)) {
                            // Extract M-of-N from the redeem script in scriptSig
                            const CScript& scriptSig = tx.vin[inputIdx].scriptSig;
                            CScript redeemScript;
                            std::string redeemError;
                            if (ExtractRedeemScriptFromScriptSig(scriptSig, redeemScript, redeemError)) {
                                uint8_t m, n;
                                std::string mnError;
                                if (ExtractMultisigMNFromRedeemScript(redeemScript, m, n, mnError)) {
                                    // For M=1 (1-of-N), allow proof-only ephemeral assets
                                    // For M>1, require UTXO ephemeral assets
                                    bool isProofOnly = IsEphemeralAssetProofOnly(txout.scriptPubKey, assetCache);
                                    bool isUTXO = IsEphemeralAssetUTXO(txout.scriptPubKey, assetCache);
                                    
                                    if (m == 1) {
                                        // M=1: Allow proof-only ephemeral assets
                                        // No need to register or check duplicates for proof-only (they don't lock UTXOs)
                                        if (isProofOnly) {
                                            // Proof-only ephemeral assets for 1-of-N are allowed
                                            // They don't create UTXOs or lock the parent UTXO
                                            continue;
                                        }
                                    } else {
                                        // M>1: Require UTXO ephemeral assets
                                        if (isProofOnly) {
                                            std::string error = strprintf("M-of-N multisig (%d-of-%d) requires UTXO ephemeral assets, but proof-only ephemeral asset provided for UTXO %s:%d", 
                                                                         m, n, prevout.hash.ToString(), prevout.n);
                                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-p2ah-multisig-ephemeral-proof-only-invalid", false, error, tx.GetHash());
                                        }
                                        if (!isUTXO) {
                                            std::string error = strprintf("M-of-N multisig (%d-of-%d) requires UTXO ephemeral assets for UTXO %s:%d", 
                                                                         m, n, prevout.hash.ToString(), prevout.n);
                                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-p2ah-multisig-ephemeral-utxo-required", false, error, tx.GetHash());
                                        }
                                    }
                                    
                                    // For UTXO ephemeral assets (M>1 or M=1 with UTXO), check for duplicates
                                    if (isUTXO) {
                                        // Check if this M-of-N has already created an ephemeral asset for this UTXO
                                        if (assetCache->HasMultisigEphemeralAsset(prevout, m, n)) {
                                            std::string duplicateError = strprintf("M-of-N multisig (%d-of-%d) has already created an ephemeral asset for UTXO %s:%d", 
                                                                                  m, n, prevout.hash.ToString(), prevout.n);
                                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-p2ah-multisig-ephemeral-duplicate", false, duplicateError, tx.GetHash());
                                        }
                                        
                                        // Extract ephemeral asset hash
                                        uint160 ephemeralHash;
                                        if (ExtractAssetHashFromP2AH(txout.scriptPubKey, ephemeralHash)) {
                                            // Register this M-of-N ephemeral asset
                                            assetCache->RegisterMultisigEphemeralAsset(prevout, m, n, ephemeralHash);
                                        }
                                    }
                                }
                            }
                        }
                        
                        // Also check for duplicate using the general validation
                        std::string multisigError;
                        const CScript& scriptSig = tx.vin[inputIdx].scriptSig;
                        if (!ValidateMultisigEphemeralNotDuplicate(coin.out.scriptPubKey, prevout, txout.scriptPubKey, scriptSig, assetCache, multisigError)) {
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-p2ah-multisig-ephemeral-duplicate", false, multisigError, tx.GetHash());
                        }
                    }
                }
                
                // Only UTXO ephemeral assets need to be tracked for burn fees
                // Proof-only ephemeral assets are validated but don't create UTXOs or incur fees
            }
        }
        
        // Validate address requirements for RESTRICTED P2AH outputs only (receiving assets)
        // Basic P2AH addresses have no requirements - keep it simple
        if (IsP2AHRestricted(txout.scriptPubKey) && assetCache) {
            // Check if this output contains a restricted P2AH asset that needs address requirement validation
            CAssetOutputEntry outputData;
            if (GetAssetData(txout.scriptPubKey, outputData)) {
                std::string strError;
                if (!ValidateRestrictedP2AHAddressRequirements(txout.scriptPubKey, outputData.assetName, outputData.nAmount, assetCache, strError)) {
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-restricted-p2ah-address-requirements", false, strError, tx.GetHash());
                }
            }
        }

        if (assetCache) {
            if (fIsAsset && !AreAssetsDeployed())
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-is-asset-and-asset-not-active");

            if (txout.scriptPubKey.IsNullAsset(IsTollsActive())) {
                if (!AreRestrictedAssetsDeployed())
                    return state.DoS(100, false, REJECT_INVALID,
                                     "bad-tx-null-asset-data-before-restricted-assets-activated");

                if (txout.scriptPubKey.IsNullAssetTxDataScript(IsTollsActive())) {
                    if (!ContextualCheckNullAssetTxOut(txout, assetCache, strError, myNullAssetData))
                        return state.DoS(100, false, REJECT_INVALID, strError, false, "", tx.GetHash());
                } else if (txout.scriptPubKey.IsNullGlobalRestrictionAssetTxDataScript()) {
                    if (!ContextualCheckGlobalAssetTxOut(txout, assetCache, strError))
                        return state.DoS(100, false, REJECT_INVALID, strError, false, "", tx.GetHash());
                } else if (txout.scriptPubKey.IsNullAssetVerifierTxDataScript()) {
                    if (!ContextualCheckVerifierAssetTxOut(txout, assetCache, strError))
                        return state.DoS(100, false, REJECT_INVALID, strError, false, "", tx.GetHash());
                } else {
                    return state.DoS(100, false, REJECT_INVALID, "bad-tx-null-asset-data-unknown-type", false, "", tx.GetHash());
                }
            }
        }

        if (nType == TX_TRANSFER_ASSET) {
            CAssetTransfer transfer;
            std::string address = "";
            if (!TransferAssetFromScript(txout.scriptPubKey, transfer, address))
                return state.DoS(100, false, REJECT_INVALID, "bad-tx-asset-transfer-bad-deserialize", false, "",
                                 tx.GetHash());

            if (!ContextualCheckTransferAsset(assetCache, transfer, address, strError))
                return state.DoS(100, false, REJECT_INVALID, strError, false, "", tx.GetHash());

            // Add to the total value of assets in the outputs
            if (totalOutputs.count(transfer.strName))
                totalOutputs.at(transfer.strName) += transfer.nAmount;
            else
                totalOutputs.insert(make_pair(transfer.strName, transfer.nAmount));

            // Track the address the asset is going to for tolls
            if (!IsAssetNameAnOwner(transfer.strName)) {
                assetToAddresses[transfer.strName].push_back({address, transfer.nAmount});
            }

            if (!fRunningUnitTests) {
                if (IsAssetNameAnOwner(transfer.strName)) {
                    if (transfer.nAmount != OWNER_ASSET_AMOUNT)
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-owner-amount-was-not-1", false, "", tx.GetHash());
                } else {
                    // For all other types of assets, make sure they are sending the right type of units
                    CNewAsset asset;
                    if (!assetCache->GetAssetMetaDataIfExists(transfer.strName, asset))
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-asset-not-exist", false, "", tx.GetHash());

                    if (asset.strName != transfer.strName)
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-asset-database-corrupted", false, "", tx.GetHash());

                    if (!CheckAmountWithUnits(transfer.nAmount, asset.units))
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-asset-amount-not-match-units", false, "", tx.GetHash());
                }
            }

            /** Get messages from the transaction, only used when getting called from ConnectBlock **/
            // Get the messages from the Tx unless they are expired
            if (AreMessagesDeployed() && fMessaging && setMessages) {
                if (IsAssetNameAnOwner(transfer.strName) || IsAssetNameAnMsgChannel(transfer.strName)) {
                    if (!transfer.message.empty()) {
                        if (transfer.nExpireTime == 0 || transfer.nExpireTime > currentTime) {
                            if (mapAddresses.count(transfer.strName)) {
                                if (mapAddresses.at(transfer.strName) == address) {
                                    COutPoint out(tx.GetHash(), index);
                                    CMessage message(out, transfer.strName, transfer.message,
                                                     transfer.nExpireTime, nBlocktime);
                                    setMessages->insert(message);
                                    LogPrintf("Got message: %s\n", message.ToString()); // TODO remove after testing
                                }
                            }
                        }
                    }
                }
            }
        } else if (nType == TX_REISSUE_ASSET) {
            CReissueAsset reissue;
            std::string address;
            if (!ReissueAssetFromScript(txout.scriptPubKey, reissue, address))
                return state.DoS(100, false, REJECT_INVALID, "bad-tx-asset-reissue-bad-deserialize", false, "", tx.GetHash());

            if (mapReissuedAssets.count(reissue.strName)) {
                if (mapReissuedAssets.at(reissue.strName) != tx.GetHash())
                    return state.DoS(100, false, REJECT_INVALID, "bad-tx-reissue-chaining-not-allowed", false, "", tx.GetHash());
            } else {
                vPairReissueAssets.emplace_back(std::make_pair(reissue.strName, tx.GetHash()));
            }
        }
        index++;
    }

    if (assetCache) {
        if (tx.IsNewAsset()) {
            // Get the asset type
            CNewAsset asset;
            std::string address;
            if (!AssetFromScript(tx.vout[tx.vout.size() - 1].scriptPubKey, asset, address)) {
                error("%s : Failed to get new asset from transaction: %s", __func__, tx.GetHash().GetHex());
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-serialzation-failed", false, "", tx.GetHash());
            }

            AssetType assetType;
            IsAssetNameValid(asset.strName, assetType);

            if (!ContextualCheckNewAsset(assetCache, asset, strError, fCheckMempool))
                return state.DoS(100, false, REJECT_INVALID, strError);

        } else if (tx.IsReissueAsset()) {
            CReissueAsset reissue_asset;
            std::string address;
            if (!ReissueAssetFromScript(tx.vout[tx.vout.size() - 1].scriptPubKey, reissue_asset, address)) {
                error("%s : Failed to get new asset from transaction: %s", __func__, tx.GetHash().GetHex());
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-reissue-serialzation-failed", false, "", tx.GetHash());
            }

            if (!ContextualCheckReissueAsset(assetCache, reissue_asset, strError, tx))
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-reissue-contextual-" + strError, false, "", tx.GetHash());
        } else if (tx.IsNewUniqueAsset()) {
            if (!ContextualCheckUniqueAssetTx(assetCache, strError, tx))
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-unique-contextual-" + strError, false, "", tx.GetHash());
        } else if (tx.IsNewMsgChannelAsset()) {
            if (!AreMessagesDeployed())
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-msgchannel-before-messaging-is-active", false, "", tx.GetHash());

            CNewAsset asset;
            std::string strAddress;
            if (!MsgChannelAssetFromTransaction(tx, asset, strAddress))
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-msgchannel-serialzation-failed", false, "", tx.GetHash());

            if (!ContextualCheckNewAsset(assetCache, asset, strError, fCheckMempool))
                return state.DoS(100, error("%s: %s", __func__, strError), REJECT_INVALID,
                                 "bad-txns-issue-msgchannel-contextual-" + strError);
        } else if (tx.IsNewQualifierAsset()) {
            if (!AreRestrictedAssetsDeployed())
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-qualifier-before-it-is-active", false, "", tx.GetHash());

            CNewAsset asset;
            std::string strAddress;
            if (!QualifierAssetFromTransaction(tx, asset, strAddress))
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-qualifier-serialzation-failed", false, "", tx.GetHash());

            if (!ContextualCheckNewAsset(assetCache, asset, strError, fCheckMempool))
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-qualfier-contextual" + strError, false, "", tx.GetHash());

        } else if (tx.IsNewRestrictedAsset()) {
            if (!AreRestrictedAssetsDeployed())
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-restricted-before-it-is-active", false, "", tx.GetHash());

            // Get asset data
            CNewAsset asset;
            std::string strAddress;
            if (!RestrictedAssetFromTransaction(tx, asset, strAddress))
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-restricted-serialzation-failed", false, "", tx.GetHash());

            if (!ContextualCheckNewAsset(assetCache, asset, strError, fCheckMempool))
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-restricted-contextual" + strError, false, "", tx.GetHash());

            // Get verifier string
            CNullAssetTxVerifierString verifier;
            if (!tx.GetVerifierStringFromTx(verifier, strError))
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-restricted-verifier-search-" + strError, false, "", tx.GetHash());

            // Check the verifier string against the destination address
            if (!ContextualCheckVerifierString(assetCache, verifier.verifier_string, strAddress, strError))
                return state.DoS(100, false, REJECT_INVALID, strError, false, "", tx.GetHash());

        } else {
            for (auto out : tx.vout) {
                int nType;
                int nScriptType;
                bool _isOwner;
                if (out.scriptPubKey.IsAssetScript(nType, nScriptType, _isOwner)) {
                    if (nType != TX_TRANSFER_ASSET) {
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-bad-asset-transaction", false, "", tx.GetHash());
                    }
                } else {
                    if (out.scriptPubKey.Find(OP_EVR_ASSET)) {
                        if (AreRestrictedAssetsDeployed()) {
                            if (out.scriptPubKey[0] != OP_EVR_ASSET) {
                                return state.DoS(100, false, REJECT_INVALID,
                                                 "bad-txns-op-evr-asset-not-in-right-script-location", false, "", tx.GetHash());
                            }
                        } else {
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-bad-asset-script", false, "", tx.GetHash());
                        }
                    }
                }
            }
        }
    }

    for (const auto& outValue : totalOutputs) {
        if (!totalInputs.count(outValue.first)) {
            std::string errorMsg;
            errorMsg = strprintf("Bad Transaction - Trying to create outpoint for asset that you don't have: %s", outValue.first);
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-inputs-outputs-mismatch " + errorMsg, false, "", tx.GetHash());
        }

        if (totalInputs.at(outValue.first) != outValue.second) {
            std::string errorMsg;
            errorMsg = strprintf("Bad Transaction - Assets would be burnt %s", outValue.first);
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-inputs-outputs-mismatch " + errorMsg, false, "", tx.GetHash());
        }
    }

    // Check the input size and the output size
    if (totalOutputs.size() != totalInputs.size()) {
        return state.DoS(100, false, REJECT_INVALID, "bad-tx-asset-inputs-size-does-not-match-outputs-size", false, "", tx.GetHash());
    }


    if (IsTollsActive()) {
        // Ensure the toll is paid for each asset transferred to a new address
        for (const auto& assetPair : assetToAddresses) {
            const std::string& assetName = assetPair.first;
            const std::vector<std::pair<std::string, CAmount>>& toAddresses = assetPair.second;

            // Retrieve asset metadata to check for toll amount
            CNewAsset asset;
            if (!assetCache->GetAssetMetaDataIfExists(assetName, asset))
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-asset-not-exist", false, "", tx.GetHash());

            if (asset.nTollAmount > 0) {
                CAmount tollAmount = asset.nTollAmount;  // Assume tollAmount is part of asset metadata
                std::string tollAddress = asset.strTollAddress;

                // Check for each "to" address
                for (const auto& toAddressPair : toAddresses) {
                    const std::string& toAddress = toAddressPair.first;
                    CAmount sentAmount = toAddressPair.second;

                    // Allow assets to be sent to the global burn address without paying a toll.
                    if (toAddress == GetParams().GlobalBurnAddress()) {
                        continue;
                    }

                    // Get the amount previously sent from the address
                    CAmount amountPreviouslySent = 0;
                    auto fromAddresses = assetFromAddresses.find(assetName);
                    if (fromAddresses != assetFromAddresses.end()) {
                        for (const auto& fromAddressPair : fromAddresses->second) {
                            if (fromAddressPair.first == toAddress) {
                                amountPreviouslySent = fromAddressPair.second;
                                break;
                            }
                        }
                    }

                    // Calculate tollable amount: sentAmount - amountPreviouslySent
                    CAmount tollableAmount = sentAmount - amountPreviouslySent;

                    if (tollableAmount > 0) { // Toll is only applied to excess amounts
                        CAmount totalToll = CalculateToll(tollableAmount, tollAmount);
                        if (totalToll < 0 || totalToll > MAX_MONEY) {
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-toll-calculation-max-money", false, "", tx.GetHash());
                        }

                        // Log for debugging purposes
                        LogPrintf("Asset: %s, To Address: %s, Sent Amount: %d, Previously Sent: %d, Tollable Amount: %d, Toll: %d\n",
                                  assetName, toAddress, sentAmount, amountPreviouslySent, tollableAmount, totalToll);

                        // Update the required tolls map
                        if (mapRequiredTolls.count(tollAddress)) {
                            mapRequiredTolls[tollAddress] += totalToll;
                        } else {
                            mapRequiredTolls[tollAddress] = totalToll;
                        }
                    }
                }
            }
        }

        // Summary of tolls paid vs required
        LogPrintf("Toll Verification Summary:\n");
        for (const auto& requiredToll : mapRequiredTolls) {
            const std::string& tollAddress = requiredToll.first;
            CAmount totalRequiredToll = requiredToll.second;
            CAmount totalPaidToll = mapEVRSentInTransaction.count(tollAddress) ? mapEVRSentInTransaction[tollAddress] : 0;

            if (totalPaidToll < totalRequiredToll) {
                LogPrintf("Summary: Address: %s, Required Toll: %d, Paid Toll: %d (INSUFFICIENT)\n",
                          tollAddress, totalRequiredToll, totalPaidToll);
            } else {
                LogPrintf("Summary: Address: %s, Required Toll: %d, Paid Toll: %d (SUFFICIENT)\n",
                          tollAddress, totalRequiredToll, totalPaidToll);
            }
        }

        // Finally, compare the required tolls to the actual tolls paid for each toll address
        for (const auto& requiredToll : mapRequiredTolls) {
            const std::string& tollAddress = requiredToll.first;
            CAmount totalRequiredToll = requiredToll.second;

            // Check if the toll paid to this address is sufficient
            if (mapEVRSentInTransaction.count(tollAddress) == 0 || mapEVRSentInTransaction[tollAddress] < totalRequiredToll) {
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-insufficient-toll-paid", false, "", tx.GetHash());
            }
        }
    }
    return true;
}

/** Structure to track ephemeral assets created in a block */
bool TrackEphemeralAssetsInBlock(const CBlock& block, CAssetsCache* assetCache, 
                                  const CCoinsViewCache& view, const CAmount& nBurnFee,
                                  std::vector<EphemeralAssetInfo>& vEphemeralAssetsCreated,
                                  CAmount& totalBurnFee, CValidationState& state)
{
    // Phase 1: Identify ephemeral assets created in this block
    // IMPORTANT: Only track UTXO ephemeral assets (proof-only don't incur fees)
    vEphemeralAssetsCreated.clear();
    std::map<uint160, size_t> mapEphemeralAssetIndex;  // hash -> index in vector
    
    for (unsigned int i = 0; i < block.vtx.size(); ++i) {
        const CTransaction& tx = *block.vtx[i];
        for (unsigned int j = 0; j < tx.vout.size(); ++j) {
            const CTxOut& txout = tx.vout[j];
            
            // Check if this is an ephemeral asset UTXO (not proof-only)
            if (IsP2AHEphemeral(txout.scriptPubKey) && IsEphemeralAssetUTXO(txout.scriptPubKey, assetCache)) {
                uint160 ephemeralHash;
                if (!ExtractAssetHashFromP2AH(txout.scriptPubKey, ephemeralHash)) {
                    continue;  // Skip if we can't extract hash
                }
                
                EphemeralAssetInfo info;
                info.assetHash = ephemeralHash;
                info.creationTxHash = tx.GetHash();
                info.nCreationOutputIndex = j;
                info.fSpentInBlock = false;
                info.fIsUTXO = true;
                info.nBurnFee = nBurnFee;
                
                // Resolve asset name if possible
                ResolveAssetNameFromHash(ephemeralHash, assetCache, info.assetName);
                
                // Find parent UTXO that this ephemeral asset locks
                // Check transaction inputs for P2AH assets
                for (unsigned int inputIdx = 0; inputIdx < tx.vin.size(); ++inputIdx) {
                    const COutPoint &prevout = tx.vin[inputIdx].prevout;
                    const Coin& coin = view.AccessCoin(prevout);
                    if (coin.IsAsset() && coin.out.scriptPubKey.IsP2AHAssetScript()) {
                        info.parentOutpoint = prevout;
                        break;  // Use first P2AH asset input as parent
                    }
                }
                
                vEphemeralAssetsCreated.push_back(info);
                mapEphemeralAssetIndex[ephemeralHash] = vEphemeralAssetsCreated.size() - 1;
            }
            // Note: Proof-only ephemeral assets are validated but not tracked for fees
        }
    }
    
    // Phase 2: Check if ephemeral assets are spent in this block
    for (unsigned int i = 0; i < block.vtx.size(); ++i) {
        const CTransaction& tx = *block.vtx[i];
        for (const auto& vin : tx.vin) {
            // Check if this input spends an ephemeral asset created in this block
            COutPoint prevout = vin.prevout;
            for (auto& info : vEphemeralAssetsCreated) {
                if (info.creationTxHash == prevout.hash && 
                    info.nCreationOutputIndex == prevout.n) {
                    info.fSpentInBlock = true;
                    break;
                }
            }
        }
    }
    
    // Phase 3: Calculate total burn fee for unused ephemeral assets
    totalBurnFee = 0;
    for (const auto& info : vEphemeralAssetsCreated) {
        if (!info.fSpentInBlock) {
            totalBurnFee += info.nBurnFee;
        }
    }
    
    // Phase 4: Validate burn fee is paid (if any)
    // Check all transactions in the block for burn outputs to GlobalBurnAddress
    if (totalBurnFee > 0) {
        CAmount totalBurned = 0;
        std::string globalBurnAddress = GetParams().GlobalBurnAddress();
        
        for (const auto& tx : block.vtx) {
            for (const auto& txout : tx->vout) {
                // Extract destination
                CTxDestination destination;
                if (!ExtractDestination(txout.scriptPubKey, destination)) {
                    continue;
                }
                
                // Check if this is a burn address
                std::string strDestination = EncodeDestination(destination);
                if (strDestination == globalBurnAddress) {
                    totalBurned += txout.nValue;
                }
            }
        }
        
        // Validate that burn fee requirement is met
        if (totalBurned < totalBurnFee) {
            std::string errorMsg = strprintf("Ephemeral asset burn fee not paid: required %d, found %d", 
                                            totalBurnFee, totalBurned);
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-ephemeral-burn-fee-insufficient", 
                           false, errorMsg);
        }
    }
    
    return true;
}

bool ProcessEphemeralAssetUTXOLocks(const CBlock& block, CAssetsCache* assetCache,
                                     const CCoinsViewCache& view, CValidationState& state)
{
    if (!assetCache) {
        return true;  // Can't process without cache
    }
    
    // Phase 1: Lock UTXOs when ephemeral assets are created
    for (unsigned int i = 0; i < block.vtx.size(); ++i) {
        const CTransaction& tx = *block.vtx[i];
        for (unsigned int j = 0; j < tx.vout.size(); ++j) {
            const CTxOut& txout = tx.vout[j];
            
            // Check if this is a UTXO ephemeral asset
            if (IsP2AHEphemeral(txout.scriptPubKey) && IsEphemeralAssetUTXO(txout.scriptPubKey, assetCache)) {
                uint160 ephemeralHash;
                if (!ExtractAssetHashFromP2AH(txout.scriptPubKey, ephemeralHash)) {
                    continue;
                }
                
                // Find parent UTXO and lock it
                for (unsigned int inputIdx = 0; inputIdx < tx.vin.size(); ++inputIdx) {
                    const COutPoint &prevout = tx.vin[inputIdx].prevout;
                    const Coin& coin = view.AccessCoin(prevout);
                    if (coin.IsAsset() && coin.out.scriptPubKey.IsP2AHAssetScript()) {
                        // Lock the parent UTXO
                        if (!assetCache->LockUTXOForEphemeral(prevout, ephemeralHash)) {
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-ephemeral-utxo-lock-failed", false, 
                                           strprintf("Failed to lock UTXO %s:%d for ephemeral asset", 
                                                    prevout.hash.ToString(), prevout.n), tx.GetHash());
                        }
                        break;  // Only lock first parent UTXO
                    }
                }
            }
        }
    }
    
    // Phase 2: Unlock UTXOs when ephemeral assets are spent
    // When an ephemeral asset is spent, it unlocks the parent UTXO and that parent UTXO should be spent in the same transaction
    for (unsigned int i = 0; i < block.vtx.size(); ++i) {
        const CTransaction& tx = *block.vtx[i];
        
        // First, collect all ephemeral assets being spent in this transaction
        std::vector<std::pair<uint160, COutPoint>> vEphemeralAssetsSpent;  // (ephemeralHash, ephemeralOutpoint)
        for (const auto& vin : tx.vin) {
            const Coin& coin = view.AccessCoin(vin.prevout);
            if (coin.IsAsset() && IsP2AHEphemeral(coin.out.scriptPubKey)) {
                uint160 ephemeralHash;
                if (ExtractAssetHashFromP2AH(coin.out.scriptPubKey, ephemeralHash)) {
                    vEphemeralAssetsSpent.push_back(std::make_pair(ephemeralHash, vin.prevout));
                }
            }
        }
        
        // For each ephemeral asset being spent, find and unlock its parent UTXO
        // The parent UTXO must exist and must be spent in this transaction
        for (const auto& ephemeralPair : vEphemeralAssetsSpent) {
            const uint160& ephemeralHash = ephemeralPair.first;
            COutPoint parentOutpoint;
            
            // Find the parent UTXO that was locked by this ephemeral asset
            // The parent UTXO MUST exist - if it doesn't, this is an error
            if (!assetCache->FindParentUTXOByEphemeralHash(ephemeralHash, parentOutpoint)) {
                // Parent UTXO not found - this is an error
                // The ephemeral asset must have locked a parent UTXO that still exists
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-ephemeral-parent-not-found", false,
                               strprintf("Ephemeral asset spent but parent UTXO not found (ephemeral hash: %s)",
                                        ephemeralHash.ToString()), tx.GetHash());
            }
            
            // Verify that the parent UTXO is being spent in this transaction
            bool parentSpentInTx = false;
            for (const auto& vin : tx.vin) {
                if (vin.prevout == parentOutpoint) {
                    parentSpentInTx = true;
                    break;
                }
            }
            
            if (parentSpentInTx) {
                // Parent UTXO is being spent - unlock it
                assetCache->UnlockUTXOForEphemeral(parentOutpoint);
            } else {
                // Ephemeral asset is spent but parent UTXO is not - this is an error
                // The ephemeral asset should authorize spending the parent UTXO in the same transaction
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-ephemeral-spent-without-parent", false,
                               strprintf("Ephemeral asset spent but parent UTXO %s:%d not spent in same transaction",
                                        parentOutpoint.hash.ToString(), parentOutpoint.n), tx.GetHash());
            }
        }
    }
    
    return true;
}


