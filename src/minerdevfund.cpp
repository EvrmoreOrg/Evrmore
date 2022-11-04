// Copyright (c) 2022 The Evrmore developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "minerdevfund.h"
#include "util.h"
#include "chainparams.h"
#include "base58.h"  // for DecodeDestination

// Both P2PKH and P2SH are supported. But these are for mainnet, so use P2SH Multisig
std::vector<std::string> vMainnetDevFundAddr = {
"eHNUGzw8ZG9PGC8gKtnneyMaQXQTtAUm98",
"e7Tkk3kjS9NjSYVX2Q8qzxXB1WKMRvea1j"
};

// Both P2PKH and P2SH are supported. These are for testnet, so P2PKH is ok
std::vector<std::string> vTestnetDevFundAddr = {
"n2rxh4SrVLkzASyxaP3jB6Zf2TEfsPXdMz",
"muFMkxZeEpptRMNK1zWvsi2aPRNULCin3s"
};

// Percentage of the block reward to be sent to the fund.
static constexpr int MINER_FUND_RATIO = 10;

CAmount GetMinerDevFundAmount(const CAmount &coinbaseValue) {
    return MINER_FUND_RATIO * coinbaseValue / 100;
}

std::vector<std::string> vMinerDevFundAddress;

//Returns a Hash160 format list of approved minerdevfund addresses
std::vector<CTxDestination> GetMinerDevFundWhitelist(const Consensus::Params &params) {
    if (!gArgs.GetBoolArg("-enableminerdevfund", params.enableMinerDevFund)) { 
        return {};
    } else {
		if (bNetwork.fOnTestnet || bNetwork.fOnRegtest) {
			vMinerDevFundAddress = vTestnetDevFundAddr ;
		} else {
			vMinerDevFundAddress = vMainnetDevFundAddr;
		}
		std::vector<CTxDestination> minerdevfundwhitelist{};
		for (const std::string& addr : vMinerDevFundAddress) {
			CEvrmoreAddress address(addr.c_str());
			assert(address.IsValid());
//			assert(address.IsScript());
			CTxDestination dest = DecodeDestination(addr);
			minerdevfundwhitelist.push_back(dest);
        }
		return minerdevfundwhitelist;
	}
}
