// Copyright (c) 2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2022 The Evrmore Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EVRMORE_CONSENSUS_VERSIONBITS
#define EVRMORE_CONSENSUS_VERSIONBITS

#include "chain.h"
#include <map>

/** What block version to use for new blocks (pre versionbits) */
static const int32_t VERSIONBITS_LAST_OLD_BLOCK_VERSION = 5;    // <== This was "4" in bitcoin and didn't need to change. 
    // It is only used in Boost tests. Formerly used in the routine to generate "Warning: Unknown block versions being mined!" msg
/** What bits to set in version for versionbits blocks */
static const int32_t VERSIONBITS_TOP_BITS = 0x20000000UL;
/** What bits to set in the version for versionbits blocks after assets is active */
static const int32_t VERSIONBITS_TOP_BITS_ASSETS = 0x30000000UL; //<== This was added by EVR. It is the only EVR BIP which changes
    // the block version number nVersion (something which bitcoin no longer does). It is used in validation.cpp to set & check 
    // block versions based on whether assets are active as determined by the BIP9 vote. It is redundant to the use of BIP9 bit 6 vote
    // but it makes it easy to check on inspection whether a block is after assets are valid. Note that Evrmore's use of 
    // nVersion = 3000 0000 would be in conflict with bitcoin BIP320 since it uses bit-28 which BIP320 specifies as one of the 16 
    // extraNone bits for the miners
/** What bitmask determines whether versionbits is in use */
static const int32_t VERSIONBITS_TOP_MASK = 0xE0000000UL;
/** Total bits available for versionbits */
static const int32_t VERSIONBITS_NUM_BITS = 29;

enum ThresholdState {
    THRESHOLD_DEFINED,
    THRESHOLD_STARTED,
    THRESHOLD_LOCKED_IN,
    THRESHOLD_ACTIVE,
    THRESHOLD_FAILED,
};

// A map that gives the state for blocks whose height is a multiple of Period().
// The map is indexed by the block's parent, however, so all keys in the map
// will either be nullptr or a block with (height + 1) % Period() == 0.
typedef std::map<const CBlockIndex*, ThresholdState> ThresholdConditionCache;

struct VBDeploymentInfo {
    /** Deployment name */
    const char *name;
    /** Whether GBT clients can safely ignore this rule in simplified usage */
    bool gbt_force;
};

struct BIP9Stats {
    int period;
    int threshold;
    int elapsed;
    int count;
    bool possible;
};

extern const struct VBDeploymentInfo VersionBitsDeploymentInfo[];

/**
 * Abstract class that implements BIP9-style threshold logic, and caches results.
 */
class AbstractThresholdConditionChecker {
protected:
    virtual bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const =0;
    virtual int64_t BeginTime(const Consensus::Params& params) const =0;
    virtual int64_t EndTime(const Consensus::Params& params) const =0;
    virtual int Period(const Consensus::Params& params) const =0;
    virtual int Threshold(const Consensus::Params& params) const =0;

public:
    BIP9Stats GetStateStatisticsFor(const CBlockIndex* pindex, const Consensus::Params& params) const;
    // Note that the functions below take a pindexPrev as input: they compute information for block B based on its parent.
    ThresholdState GetStateFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const;
    int GetStateSinceHeightFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const;
};

struct VersionBitsCache
{
    ThresholdConditionCache caches[Consensus::MAX_VERSION_BITS_DEPLOYMENTS];

    void Clear();
};

ThresholdState VersionBitsState(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos, VersionBitsCache& cache);
BIP9Stats VersionBitsStatistics(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos);
int VersionBitsStateSinceHeight(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos, VersionBitsCache& cache);
uint32_t VersionBitsMask(const Consensus::Params& params, Consensus::DeploymentPos pos);

#endif
