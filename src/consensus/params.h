// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2021 The Raven Core developers
// Copyright (c) 2022 The Evrmore Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EVRMORE_CONSENSUS_PARAMS_H
#define EVRMORE_CONSENSUS_PARAMS_H

#include "uint256.h"
#include <amount.h>
#include <map>
#include <string>

namespace Consensus {

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    // DEPLOYMENT_ASSETS, // Deployment of RIP2
    // DEPLOYMENT_CSV, // Deployment of BIP68, BIP112, and BIP113.
    // DEPLOYMENT_SEGWIT, // Deployment of BIP141, BIP143, and BIP147.
    MAX_VERSION_BITS_DEPLOYMENTS
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
};

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout;
    /** Use to override the confirmation window on a specific BIP */
    uint32_t nOverrideMinerConfirmationWindow;
    /** Use to override the the activation threshold on a specific BIP */
    uint32_t nOverrideRuleChangeActivationThreshold;
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    int nSubsidyHalvingInterval; // EVR-TODO: eliminate this after adjusting the unit tests
    bool nBIP34Enabled;
    bool nBIP65Enabled;
    bool nBIP66Enabled;
    bool nCSVEnabled;
    bool nSegwitEnabled;

// EVR-TODO: clean up the mess at the end of src/validation.cpp and specify the following here:        
//     fAssetsIsActive = true;
//     fRip5IsActive = true;
//     fTransferScriptIsActive = true;
//     fEnforcedValuesIsActive = true;
//     fCheckCoinbaseAssetsIsActive = true;
//     fP2Active = true;

    /**
     * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargeting period,
     * (nPowTargetTimespan / nPowTargetSpacing) which is also used for BIP9 deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];

    // Enable or disable the miner fund by default 
    bool enableMinerDevFund;

    /** Proof of work parameters */
    uint256 powLimit;
    uint256 evrprogpowLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;
    int64_t nPowTargetTimespan;
    int64_t DifficultyAdjustmentInterval() const { return nPowTargetTimespan / nPowTargetSpacing; }
    uint256 nMinimumChainWork;
    uint256 defaultAssumeValid;
    //double rewardReductionRate;
    CAmount baseReward;
    int rewardReductionInterval;
};
} // namespace Consensus

#endif // EVRMORE_CONSENSUS_PARAMS_H
