// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2021 The Raven Core developers
// Copyright (c) 2022 The Evrmore Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"
#include "arith_uint256.h"

#include <assert.h>
#include "chainparamsseeds.h"
#include "airdrop.h"
#include "airdropitems.h"   // The two airdrop arrays are declared as global in airdrop.cpp but initialized here


static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint64_t nNonce64, 
    uint32_t nBits, int32_t nVersion, const CAmount& genesisReward, std::vector<AirdropScriptItem>& vAirdrop)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(vAirdrop.size() + 1);
// Note that the total scriptSig length in a coinbase transaction must be <= 100 per "if (tx.IsCoinBase())" in CheckTransaction() in consensus/tx_verify.cpp
    txNew.vin[0].scriptSig = CScript() << CScriptNum(0) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    for (unsigned int i = 0; i < vAirdrop.size(); ++i)
    {
        txNew.vout[i + 1].nValue = vAirdrop[i].amount;
        txNew.vout[i + 1].scriptPubKey = vAirdrop[i].script;
    }

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;

    // EVRPROGPOW
    genesis.nHeight  = 0;
    genesis.nNonce64 = nNonce64;
    genesis.mix_hash.SetNull();

    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint64_t nNonce64, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward, std::vector<AirdropScriptItem> vAirdrop)
{
    const char* pszTimestamp = "Bloomberg.com October 27 2022:  Hong Kong Plans to Legalize Retail Crypto Trading to Become Hub";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nNonce64, nBits, nVersion, genesisReward, vAirdrop);
}

// EVR - Start of code to mine the genesis block. Once found, we can use the nonce and block hash found to create a valid genesis block
void GenesisGenerator(Consensus::Params &consensus1, uint32_t nGenesisTime1, uint32_t nTarget1, std::vector<AirdropScriptItem> vAirdrop1) {
    uint32_t nNonce = 0;
	uint64_t nNonce64 = 0;
	arith_uint256 test;
	bool fNegative;
	bool fOverflow;
    uint256 hashgenesis;
	CBlock genesis1;        
	
	test.SetCompact(nTarget1, &fNegative, &fOverflow);
	std::cout << "Test threshold: " << test.GetHex() << "\n\n";
	
	uint256 BestBlockHash = uint256S("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

    if (fEvrprogpowAsMiningAlgo) {
        genesis1 = CreateGenesisBlock(nGenesisTime1, 0, nNonce64, nTarget1, 4, consensus1.baseReward, vAirdrop1);
    } else {
        genesis1 = CreateGenesisBlock(nGenesisTime1, nNonce, 0, nTarget1, 4, consensus1.baseReward, vAirdrop1);
    }

    while(true)
    {
	    if (fEvrprogpowAsMiningAlgo) {
	        hashgenesis = genesis1.GetEVRPROGPOWHash_OnlyMix();
	    } else {
	        hashgenesis = genesis1.GetSerializeHash();
	    }
	
	    arith_uint256 BestBlockHashArith = UintToArith256(BestBlockHash);
	    if (UintToArith256(hashgenesis) < BestBlockHashArith) {
	        BestBlockHash = hashgenesis;
    		if (fEvrprogpowAsMiningAlgo) {
        		std::cout << BestBlockHash.GetHex() << " Nonce64: " << genesis1.nNonce64 << "\n";
    		} else {
        		std::cout << BestBlockHash.GetHex() << " Nonce: " << genesis1.nNonce << "\n";
            }
	        std::cout << "   PrevBlockHash: " << genesis1.hashPrevBlock.GetHex() << "\n";
	    }
	    if (UintToArith256(BestBlockHash) < test) {
	        break;
	    }

	    if (fEvrprogpowAsMiningAlgo) {
	        ++genesis1.nNonce64;
	    } else {
	        ++genesis1.nNonce;
	    }

	    if (!fEvrprogpowAsMiningAlgo && genesis1.nNonce == 0) {    // Nonce passed 2^32=4294967295. Won't worry about Nonce64 passing 2^64
	        std::cout << "\n" << "NOTE: Nonce wrapped; incrementing block Time" << "\n";
            ++genesis1.nTime; // nNonce = 0;
	    }
	}
	
	std::cout << "\n"; std::cout << "\n"; std::cout << "\n";
	std::cout << "Test threshold: " << test.GetHex() << "\n";
	std::cout << "Set hashGenesisBlock to 0x" << BestBlockHash.GetHex() << std::endl;
	
	if (fEvrprogpowAsMiningAlgo) {
	    std::cout << "Set Genesis Nonce   to 0" << std::endl;
	    std::cout << "Set Genesis Nonce64 to " << genesis1.nNonce64 << std::endl;
	} else {
	    std::cout << "Set Genesis Nonce   to " << genesis1.nNonce << std::endl;
	    std::cout << "Set Genesis Nonce64 to 0" << std::endl;
	}
	
	std::cout << "Set Genesis Merkle to " << genesis1.hashMerkleRoot.GetHex() << std::endl;
    std::cout << "Set Genesis Time to " << genesis1.nTime << std::endl << std::endl;
	
	assert(0);	//return;
}
// EVR - End of code to mine the genesis block.

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

void CChainParams::TurnOffBIP34() {
	consensus.nBIP34Enabled = false;
}

void CChainParams::TurnOffBIP65() {
	consensus.nBIP65Enabled = false;
}

void CChainParams::TurnOffBIP66() {
	consensus.nBIP66Enabled = false;
}

void CChainParams::TurnOffCSV() {
	consensus.nCSVEnabled = false;
}

void CChainParams::TurnOffSegwit() {
	consensus.nSegwitEnabled = false;
}

bool CChainParams::BIP34() {
	return consensus.nBIP34Enabled;
}

bool CChainParams::BIP65() {
	return consensus.nBIP34Enabled;
}

bool CChainParams::BIP66() {
	return consensus.nBIP34Enabled;
}

bool CChainParams::CSVEnabled() const{
	return consensus.nCSVEnabled;
}


/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nCSVEnabled = true;
        consensus.nSegwitEnabled = true;

// EVR-TODO: clean up the mess at the end of src/validation.cpp and specify the following here:        
//      consensus.fAssetsIsActive = true;
//      consensus.fRip5IsActive = true;
//      consensus.fTransferScriptIsActive = true;
//      consensus.fEnforcedValuesIsActive = true;
//      consensus.fCheckCoinbaseAssetsIsActive = true;
//      consensus.fP2Active = true;

        consensus.baseReward = 2778 * COIN;
        consensus.rewardReductionInterval = 1648776; //~ 3.1 years at 1 min block time; needed for 21 billion max EVR assuming 2022-10-28 launch        
        // the series a +ar + ar^2 + ar^3 +... for |r|<1 converges to a/(1-r)
        consensus.powLimit = uint256S("000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // 0x1E00FFFF
        consensus.enableMinerDevFund = true;    // The miner dev fund is enabled by default on mainnet.
        consensus.evrprogpowLimit = uint256S("0000000000ffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // This is never actually used in Evrmore
        consensus.nPowTargetTimespan = 2016 * 60; // 1.4 days
        consensus.nPowTargetSpacing = 1 * 60;
		consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1613; // Approx 80% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // UTC: Tue January 01 2008 12:00:01
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // UTC: Wed Dec 31 2008 23:59:59
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nOverrideRuleChangeActivationThreshold = 1814;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nOverrideMinerConfirmationWindow = 2016; // Approx 90% of 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].bit = 29;
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].nStartTime = 1999145601; // TODO - Set this for mainnet
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].nTimeout = 2099145601; // TODO - Set this for mainnet
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].nOverrideRuleChangeActivationThreshold = 1814; // TODO - Set this for mainnet
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].nOverrideMinerConfirmationWindow = 2016; // Approx 90% of 2016 // TODO - Set this for mainnet
        // Removed the Ravencoin BIP9 parameters for ASSETS (RIP2), MSG_REST_ASSETS (RIP5), TRANSFER_SCRIPT_SIZE, 
        //     ENFORCE_VALUE, COINBASE_ASSETS, and P2SH_ASSETS

        // The best chain should have at least this much work.
        // per bitcoin-maintainer-tools release-prep.sh, just use "chainwork" as reported by getblockchaininfo on the current chain        
        // consensus.nMinimumChainWork = uint256S("000000000000000000000000000000000000000000000020d4ac871fb7009b63"); // Block 1186833
        consensus.nMinimumChainWork = uint256S("0x00");
        // EVR-TODO: Need to update this as we get more recent chain info

        // By default assume that the signatures in ancestors of this block are valid.
        // per bitcoin-maintainer-tools release-prep.sh, just use "bestblockhash" as reported by getblockchaininfo on the current chain        
        // consensus.defaultAssumeValid = uint256S("0x0000000000000d4840d4de1f7d943542c2aed532bd5d6527274fc0142fa1a410"); // Block 1186833
        consensus.defaultAssumeValid = uint256S("0x00");
        // EVR-TODO: Need to update this as we get more recent chain info

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x45; // E
        pchMessageStart[1] = 0x56; // V
        pchMessageStart[2] = 0x52; // R
        pchMessageStart[3] = 0x4d; // M
        nDefaultPort = 8820;	// currently unassigned by IANA
        nPruneAfterHeight = 100000;

        vAirdrop = LoadAirdrop();

        // Use SHA256 or EvrprogPow depending on this choice
        fEvrprogpowAsMiningAlgo = true;     // The value is set here but declared as global in primitives/block.h

        uint32_t nGenesisTime = 1667072172;  //  Saturday, October 29, 2022 19:36:12 UTC

        uint32_t nTarget = 0x1e00ffff;      //  bitcoin uses 0x1d00ffff
        // This variable is used here only for CreateGenesisBlock. "consensus.powLimit" is used for the periodic calculation

        // EVR-TODO: note that GetDifficulty in src/rpc/blockchain.cpp shows wrong "difficulty" in RPC getblock if nTarget != 0x1d00ffff

//      EVR - This is used to mine the genesis block. Once found, we can use the nonce and block hash found to create a valid genesis block
//      GenesisGenerator(consensus, nGenesisTime, nTarget, vAirdrop);

        genesis = CreateGenesisBlock(nGenesisTime, 0, 1777396, nTarget, 4, consensus.baseReward, vAirdrop);   // snapshot 2510000
        consensus.hashGenesisBlock = genesis.GetEVRPROGPOWHash_OnlyMix();
        assert(consensus.hashGenesisBlock == uint256S("0000007b11d0481b2420a7c656ef76775d54ab5b29ee7ea250bc768535693b05"));  //  snapshot 2510000
        assert(genesis.hashMerkleRoot == uint256S("c191c775b10d2af1fcccb4121095b2a018f1bee84fa5efb568fcddd383969262"));  // snapshot 2510000

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        // EVR-TODO: maintain DNS seeders
        vSeeds.emplace_back("seed-mainnet-evr.evrmorecoin.org", false);
        
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,33);	// produces 'E' as first char of address after base58 encoding
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,92);	// produces 'e' as first char of address after base58 encoding
        base58Prefixes[ASSET_ADDRESS] = std::vector<unsigned char>(1,23);      // mainnet asset address prefix: 'A' 
        base58Prefixes[ASSETHASH_ADDRESS] = std::vector<unsigned char>(1,83); // mainnet assethash address prefix: 'a' 
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128); // after base58 encoding, produces a first char of '5' for uncompressed
                                                                            //      WIF, 'K' or 'L' for compressed WIF (as for Bitcoin)
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        // Raven BIP44 cointype in mainnet is '175'
        nExtCoinType = 175;
        // EVR-TODO: need to appy for a BIP44 type from satoshilabs and put it here after the airdrop is ancient history

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fMiningRequiresPeers = true;

        /**
         * What makes a good checkpoint block?
         * + Is surrounded by blocks with reasonable timestamps
         *   (no blocks before with a timestamp after, none after with
         *    timestamp before)
         * + Contains no strange transactions
         */
        checkpointData = (CCheckpointData) {
            {
            //    { 1186833, uint256S("0x0000000000000d4840d4de1f7d943542c2aed532bd5d6527274fc0142fa1a410")},
            //    { 1610000, uint256S("0x000000000001f1a67604ace3320cf722039f1b706b46a4d95e1d8502729b3046")}
            }
        };

        // EVR-TODO: Update the checkpointData and chainTxData as we get more recent data

        // This is just used for reporting progress % on updates, so approximation is ok
        // Per bitcoin's usage, get the data from RPC: getchaintxstats 4096 <blockhash>
        chainTxData = ChainTxData{
            // Update as we know more about the contents of the Evrmore chain
            // Stats as of block 927,995
            0,  // * UNIX timestamp of last known number of transactions
            0,  // * total number of transactions between genesis and that timestamp
            0.1 // * estimated number of transactions per second after that timestamp
        };

        // Burn Amounts
        nIssueAssetBurnAmount = 500 * COIN;
        nReissueAssetBurnAmount = 100 * COIN;
        nIssueSubAssetBurnAmount = 100 * COIN;
        nIssueUniqueAssetBurnAmount = 5 * COIN;
        nIssueMsgChannelAssetBurnAmount = 100 * COIN;
        nIssueQualifierAssetBurnAmount = 1000 * COIN;
        nIssueSubQualifierAssetBurnAmount = 100 * COIN;
        nIssueRestrictedAssetBurnAmount = 1500 * COIN;
        nAddNullQualifierTagBurnAmount = .1 * COIN;
        nEphemeralAssetBurnAmount = .1 * COIN;  // 0.1 EVR - small burn fee for unused ephemeral assets

        nReissueMetaDataOnly = 1 * COIN;
        nRemintingOnly = 0.1 * COIN;

        strIssueAssetBurnAddress = "EXissueAssetXXXXXXXXXXXXXXXXYiYRBD";
        strReissueAssetBurnAddress = "EXReissueAssetXXXXXXXXXXXXXXY1ANQH";
        strIssueSubAssetBurnAddress = "EXissueSubAssetXXXXXXXXXXXXXWW1ASo";
        strIssueUniqueAssetBurnAddress = "EXissueUniqueAssetXXXXXXXXXXTZjZJ5";
        strIssueMsgChannelAssetBurnAddress = "EXissueMsgChanneLAssetXXXXXXXD3mRa";
        strIssueQualifierAssetBurnAddress = "EXissueQuaLifierXXXXXXXXXXXXW5Zxyf";
        strIssueSubQualifierAssetBurnAddress = "EXissueSubQuaLifierXXXXXXXXXUgTjtu";
        strIssueRestrictedAssetBurnAddress = "EXissueRestrictedXXXXXXXXXXXZZMynb";
        strAddNullQualifierTagBurnAddress = "EXaddTagBurnXXXXXXXXXXXXXXXXb5HLXh";

            //Global Burn Address
        strGlobalBurnAddress = "EXBurnXXXXXXXXXXXXXXXXXXXXXXZ8ZjfN";

        // Burn Mint Assets Address
        strBurnMintAddress = "EXBurnMintXXXXXXXXXXXXXXXXXXXbdK5E";

        nMaxReorganizationDepth = 60; // 60 at 1 minute block timespan is +/- 60 minutes.
        nMinReorganizationPeers = 4;
        nMinReorganizationAge = 60 * 60 * 12; // 12Hrs- Disable DGW during sync (if GetTime() - chainActive.Tip()->nTime  > nMinReorgAge) in seconds

    }
};

/**
 * Testnet (v1)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nCSVEnabled = true;
        consensus.nSegwitEnabled = true;

// EVR-TODO: clean up the mess at the end of src/validation.cpp and specify the following here:        
//      consensus.fAssetsIsActive = true;
//      consensus.fRip5IsActive = true;
//      consensus.fTransferScriptIsActive = true;
//      consensus.fEnforcedValuesIsActive = true;
//      consensus.fCheckCoinbaseAssetsIsActive = true;
//      consensus.fP2Active = true;

        consensus.baseReward = 2778 * COIN;
        consensus.rewardReductionInterval = 1648776; //~ 3.1 years at 1 min block time; needed for 21 billion max EVR assuming 2022-10-28 launch        
        // the series a +ar + ar^2 + ar^3 +... for |r|<1 converges to a/(1-r)
        consensus.powLimit = uint256S("000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // 0x1E00FFFF
        consensus.enableMinerDevFund = true;
        consensus.evrprogpowLimit = uint256S("000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");   // This is never actually used in Evrmore
        consensus.nPowTargetTimespan = 2016 * 60; // 1.4 days
        consensus.nPowTargetSpacing = 1 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1310; // Approx 65% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // UTC: Tue January 01 2008 12:00:01
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // UTC: Wed Dec 31 2008 23:59:59
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nOverrideRuleChangeActivationThreshold = 1814;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nOverrideMinerConfirmationWindow = 2016; // Approx 90% of 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].bit = 29;
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].nStartTime = 1739836801; // UTC: Tue February 18 2025 00:00:01
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].nTimeout = 1771372799; // UTC: Tue February 17 2026 23:59:59
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].nOverrideRuleChangeActivationThreshold = 1411; // Approx 70% of 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].nOverrideMinerConfirmationWindow = 2016;
        // Removed the Ravencoin BIP9 parameters for ASSETS (RIP2), MSG_REST_ASSETS (RIP5), TRANSFER_SCRIPT_SIZE, 
        //     ENFORCE_VALUE, COINBASE_ASSETS, and P2SH_ASSETS

        // The best chain should have at least this much work.
        // per bitcoin-maintainer-tools release-prep.sh, just use "chainwork" as reported by getblockchaininfo on the current chain
        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000000000e5e1012de97");  // Block 928065
        // EVR-TODO: Need to update this as we get more recent chain info

        // By default assume that the signatures in ancestors of this block are valid.
        // per bitcoin-maintainer-tools release-prep.sh, just use "bestblockhash" as reported by getblockchaininfo on the current chain        
        consensus.defaultAssumeValid = uint256S("0x000000a1479d0cfd788b0312c7b2453ead0c53401186c49dffbf256af6e874f6");  // Block 928065
        // EVR-TODO: Need to update this as we get more recent chain info


        pchMessageStart[0] = 0x45; // E
        pchMessageStart[1] = 0x56; // V
        pchMessageStart[2] = 0x52; // R
        pchMessageStart[3] = 0x54; // T
        nDefaultPort = 18820;	// currently unassigned by IANA
        nPruneAfterHeight = 1000;

        vAirdrop = EmptyAirdrop();
        vAirdrop = LoadAirdrop();

        // Use SHA256 or EvrprogPow depending on this choice
        fEvrprogpowAsMiningAlgo = false;     // The value is set here but declared as global in primitives/block.h

        uint32_t nGenesisTime = 1667073378;  //  Saturday, October 29, 2022 19:56:18 UTC

        uint32_t nTarget = 0x1e00ffff;      //  bitcoin uses 0x1d00ffff
        // This variable is used here only for CreateGenesisBlock. "consensus.powLimit" is used for the periodic calculation

//      EVR - This is used to mine the genesis block. Once found, we can use the nonce and block hash found to create a valid genesis block
//      GenesisGenerator(consensus, nGenesisTime, nTarget, vAirdrop);

        genesis = CreateGenesisBlock(nGenesisTime, 331572, 0, nTarget, 4, consensus.baseReward, vAirdrop); // snapshot 2510000
        consensus.hashGenesisBlock = genesis.GetSerializeHash();
        assert(consensus.hashGenesisBlock == uint256S("00000044bc03f8460e64bc07b080f4929b1cb96fda46b8bd806e57bfb9db82f4")); // snapshot 2510000
        assert(genesis.hashMerkleRoot == uint256S("c191c775b10d2af1fcccb4121095b2a018f1bee84fa5efb568fcddd383969262"));  // snapshot 2510000

        vFixedSeeds.clear();
        vSeeds.clear();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        // EVR-TODO: Maintain DNS seeders
        vSeeds.emplace_back("seed-testnet-evr.evrmorecoin.org", false);
 
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);  // produces 'm' or 'n' as first char of address after base58 encoding
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);  // produces '2' as first char of address after base58 encoding
        base58Prefixes[ASSET_ADDRESS] = std::vector<unsigned char>(1,80);      // testnet asset address prefix: 'Z' 
        base58Prefixes[ASSETHASH_ADDRESS] = std::vector<unsigned char>(1,142); // testnet assethash address prefix: 'z' 
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);  // after base58 encoding, produces a first char of '9' for uncompressed
                                                                            //      WIF, 'c' for compressed WIF (as for Bitcoin)
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        // Evrmore BIP44 cointype in testnet
        nExtCoinType = 1;

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fMiningRequiresPeers = true;

        checkpointData = (CCheckpointData) {
            {
                {197836, uint256S("0x000000a09a45d43f8fa06c12bb377d85fe34a9c378da6e8e089aa8b5d7a04fb2")},   // 2023-05-15
                {395673, uint256S("0x000000e461cb6a4e40fa71a3778f4473836a81ced2d0d92b148f121783bc09b3")},   // 2023-10-27
                {593509, uint256S("0x0000008ee346bbee34d58fd282ca9075763f8598e973eceb647f437ba2166e5d")},   // 2024-05-03
                {791345, uint256S("0x00000009903eda37c5c6d03de1e8e38162dcb754987518e10785635e3423a2e9")}    // Block 791345 on 2024-10-31
            }
        };
        
        // EVR-TODO: Update the checkpointData and chainTxData as we get more recent data

        // This is just used for reporting progress % on updates, so approximation is ok
        // Per bitcoin's usage, get the data from RPC: getchaintxstats 4096 <blockhash>
        chainTxData = ChainTxData{
            // Update as we know more about the contents of the Evrmore chain
            // Stats as of block 927,995
            1739833683, // * UNIX timestamp of last known number of transactions
            928284,     // * total number of transactions between genesis and that timestamp
            0.014       // * estimated number of transactions per second after that timestamp
        };

        // Burn Amounts
        nIssueAssetBurnAmount = 500 * COIN;
        nReissueAssetBurnAmount = 100 * COIN;
        nIssueSubAssetBurnAmount = 100 * COIN;
        nIssueUniqueAssetBurnAmount = 5 * COIN;
        nIssueMsgChannelAssetBurnAmount = 100 * COIN;
        nIssueQualifierAssetBurnAmount = 1000 * COIN;
        nIssueSubQualifierAssetBurnAmount = 100 * COIN;
        nIssueRestrictedAssetBurnAmount = 1500 * COIN;
        nAddNullQualifierTagBurnAmount = .1 * COIN;
        nEphemeralAssetBurnAmount = .1 * COIN;  // 0.1 EVR - small burn fee for unused ephemeral assets

        nReissueMetaDataOnly = 1 * COIN;
        nRemintingOnly = 0.1 * COIN;

        // Burn Addresses
        strIssueAssetBurnAddress = "n1issueAssetXXXXXXXXXXXXXXXXWdnemQ";
        strReissueAssetBurnAddress = "n1ReissueAssetXXXXXXXXXXXXXXWG9NLd";
        strIssueSubAssetBurnAddress = "n1issueSubAssetXXXXXXXXXXXXXbNiH6v";
        strIssueUniqueAssetBurnAddress = "n1issueUniqueAssetXXXXXXXXXXS4695i";
        strIssueMsgChannelAssetBurnAddress = "n1issueMsgChanneLAssetXXXXXXT2PBdD";
        strIssueQualifierAssetBurnAddress = "n1issueQuaLifierXXXXXXXXXXXXUysLTj";
        strIssueSubQualifierAssetBurnAddress = "n1issueSubQuaLifierXXXXXXXXXYffPLh";
        strIssueRestrictedAssetBurnAddress = "n1issueRestrictedXXXXXXXXXXXXZVT9V";
        strAddNullQualifierTagBurnAddress = "n1addTagBurnXXXXXXXXXXXXXXXXX5oLMH";

        // Global Burn Address
        strGlobalBurnAddress = "n1BurnXXXXXXXXXXXXXXXXXXXXXXU1qejP";

        // Burn Mint Assets Address
        strBurnMintAddress = "n1BurnMintXXXXXXXXXXXXXXXXXXbVTQiY";

        nMaxReorganizationDepth = 60; // 60 at 1 minute block timespan is +/- 60 minutes.
        nMinReorganizationPeers = 4;
        nMinReorganizationAge = 60 * 60 * 12; // 12Hrs- Disable DGW during sync (if GetTime() - chainActive.Tip()->nTime  > nMinReorgAge) in seconds

    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nCSVEnabled = true;
        consensus.nSegwitEnabled = true;

// EVR-TODO: clean up the mess at the end of src/validation.cpp and specify the following here:        
//      consensus.fAssetsIsActive = true;
//      consensus.fRip5IsActive = true;
//      consensus.fTransferScriptIsActive = true;
//      consensus.fEnforcedValuesIsActive = true;
//      consensus.fCheckCoinbaseAssetsIsActive = true;
//      consensus.fP2Active = true;

        consensus.baseReward = 2778 * COIN;
        consensus.rewardReductionInterval = 150; 
        // the series a +ar + ar^2 + ar^3 +... for |r|<1 converges to a/(1-r)
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.enableMinerDevFund = false;
        consensus.evrprogpowLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");   // This is never actually used in Evrmore
        consensus.nPowTargetTimespan = 2016 * 60; // 1.4 days
        consensus.nPowTargetSpacing = 1 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for regtest
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // UTC: Tue January 01 2008 12:00:01
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // UTC: Wed Dec 31 2008 23:59:59
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nOverrideRuleChangeActivationThreshold = 1814;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nOverrideMinerConfirmationWindow = 2016; // Approx 90% of 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].nOverrideRuleChangeActivationThreshold = 400;
        consensus.vDeployments[Consensus::DEPLOYMENT_TOLL].nOverrideMinerConfirmationWindow = 500;
        // Removed the Ravencoin BIP9 parameters for ASSETS (RIP2), MSG_REST_ASSETS (RIP5), TRANSFER_SCRIPT_SIZE, 
        //     ENFORCE_VALUE, COINBASE_ASSETS, and P2SH_ASSETS

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfa;  // all the same as bitcoin
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;   // same as bitcoin
        nPruneAfterHeight = 1000;

        vAirdrop = EmptyAirdrop();
        vAirdrop = LoadAirdrop();

        // Use SHA256 or EvrprogPow depending on this choice
        fEvrprogpowAsMiningAlgo = false;     // The value is set here but declared as global in primitives/block.h

        uint32_t nGenesisTime = 1667074564;  //  Saturday, October 29, 2022 20:16:04 UTC

        uint32_t nTarget = 0x207fffff;      //  bitcoin uses 0x207fffff
        // This variable is used here only for CreateGenesisBlock. "consensus.powLimit" is used for the periodic calculation

//      EVR - This is used to mine the genesis block. Once found, we can use the nonce and block hash found to create a valid genesis block
//      GenesisGenerator(consensus, nGenesisTime, nTarget, vAirdrop);

        genesis = CreateGenesisBlock(nGenesisTime, 0, 0, nTarget, 4, consensus.baseReward, vAirdrop); // snapshot 2510000
        consensus.hashGenesisBlock = genesis.GetSerializeHash();
        assert(consensus.hashGenesisBlock == uint256S("5177b521d358ee45b83abbe2597a01511846c1bb3c08c14dc762a4649a7d2fc9"));  // snapshot 2510000
        assert(genesis.hashMerkleRoot == uint256S("c191c775b10d2af1fcccb4121095b2a018f1bee84fa5efb568fcddd383969262"));  // snapshot 2510000

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = (CCheckpointData) {
            {
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);  // produces 'm' or 'n' as first char of address after base58 encoding
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);  // produces '2' as first char of address after base58 encoding
        base58Prefixes[ASSET_ADDRESS] = std::vector<unsigned char>(1,80);     // regtest asset address prefix: 'Z' (matches testnet, verified: 100% accuracy)
        base58Prefixes[ASSETHASH_ADDRESS] = std::vector<unsigned char>(1,142); // regtest assethash address prefix: 'z' (matches testnet, verified: 100% accuracy)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);  // after base58 encoding, produces a first char of '9' for uncompressed
                                                                            //      WIF, 'c' for compressed WIF (as for Bitcoin)
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        // Evrmore BIP44 cointype in regtest
        nExtCoinType = 1;

        // Burn Amounts
        nIssueAssetBurnAmount = 500 * COIN;
        nReissueAssetBurnAmount = 100 * COIN;
        nIssueSubAssetBurnAmount = 100 * COIN;
        nIssueUniqueAssetBurnAmount = 5 * COIN;
        nIssueMsgChannelAssetBurnAmount = 100 * COIN;
        nIssueQualifierAssetBurnAmount = 1000 * COIN;
        nIssueSubQualifierAssetBurnAmount = 100 * COIN;
        nIssueRestrictedAssetBurnAmount = 1500 * COIN;
        nAddNullQualifierTagBurnAmount = .1 * COIN;
        nEphemeralAssetBurnAmount = .1 * COIN;  // 0.1 EVR - small burn fee for unused ephemeral assets

        nReissueMetaDataOnly = 1 * COIN;
        nRemintingOnly = 0.1 * COIN;

        // Burn Addresses
        strIssueAssetBurnAddress = "n1issueAssetXXXXXXXXXXXXXXXXWdnemQ";
        strReissueAssetBurnAddress = "n1ReissueAssetXXXXXXXXXXXXXXWG9NLd";
        strIssueSubAssetBurnAddress = "n1issueSubAssetXXXXXXXXXXXXXbNiH6v";
        strIssueUniqueAssetBurnAddress = "n1issueUniqueAssetXXXXXXXXXXS4695i";
        strIssueMsgChannelAssetBurnAddress = "n1issueMsgChanneLAssetXXXXXXT2PBdD";
        strIssueQualifierAssetBurnAddress = "n1issueQuaLifierXXXXXXXXXXXXUysLTj";
        strIssueSubQualifierAssetBurnAddress = "n1issueSubQuaLifierXXXXXXXXXYffPLh";
        strIssueRestrictedAssetBurnAddress = "n1issueRestrictedXXXXXXXXXXXXZVT9V";
        strAddNullQualifierTagBurnAddress = "n1addTagBurnXXXXXXXXXXXXXXXXX5oLMH";

        // Global Burn Address
        strGlobalBurnAddress = "n1BurnXXXXXXXXXXXXXXXXXXXXXXU1qejP";

        // Burn Mint Assets Address
        strBurnMintAddress = "n1BurnMintXXXXXXXXXXXXXXXXXXbVTQiY";

        nMaxReorganizationDepth = 60; // 60 at 1 minute block timespan is +/- 60 minutes.
        nMinReorganizationPeers = 4;
        nMinReorganizationAge = 60 * 60 * 12; // 12Hrs- Disable DGW during sync (if GetTime() - chainActive.Tip()->nTime  > nMinReorgAge) in seconds

    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &GetParams() {
    assert(globalChainParams);
    return *globalChainParams;
}

// EVR - Beginning of support for evrmore-cli -addrconvertrvntoevr
class RvncoinAddressChainParam : public CMainParams
{
public:
    RvncoinAddressChainParam()
    {
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,60);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,122);
    }
};

static RvncoinAddressChainParam chainParamsForAddressConversion;

const CChainParams &RvncoinAddressFormatParams()
{
    return chainParamsForAddressConversion;
}
// EVR - Ending of support for evrmore-cli -addrconvertrvntoevr

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network, bool fForceBlockNetwork)
{
    SelectBaseParams(network);
    if (fForceBlockNetwork) {
        bNetwork.SetNetwork(network);
    }
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}

void TurnOffBIP34() {
	globalChainParams->TurnOffBIP34();
}

void TurnOffBIP65() {
	globalChainParams->TurnOffBIP65();
}

void TurnOffBIP66() {
	globalChainParams->TurnOffBIP66();
}

void TurnOffCSV() {
	globalChainParams->TurnOffCSV();
}

void TurnOffSegwit(){
	globalChainParams->TurnOffSegwit();
}

