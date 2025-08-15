// Copyright (c) 2022 The Evrmore Core developers
// Copyright (c) 2025 The Echelon Technology Group developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "airdrop.h"
#include "airdropitems.h"
//#include "util.h"
#include "utilstrencodings.h"
#include <mutex>

// New Code For Compile Efficiency
std::unique_ptr<std::vector<AirdropScriptItem>> AirdropDataManager::mainnetAirdrop;
static std::mutex airdropMutex;

void AirdropDataManager::LoadMainnetAirdrop() {
    auto data = std::make_unique<std::vector<AirdropScriptItem>>();
    data->reserve(49003 + 999);

    // Process P2PKH
    for (size_t i = 0; i < 49003; ++i) {
        const auto& item = airdrop_items_p2pkh[i];
        std::vector<unsigned char> hash = ParseHex(item.hashaddr);

        CScript script;
        script << OP_DUP << OP_HASH160 << hash << OP_EQUALVERIFY << OP_CHECKSIG;

        data->emplace_back(AirdropScriptItem{std::move(script), item.amount});
    }

    // Process P2SH
    for (size_t i = 0; i < 999; ++i) {
        const auto& item = airdrop_items_p2sh[i];
        std::vector<unsigned char> hash = ParseHex(item.hashaddr);

        CScript script;
        script << OP_HASH160 << hash << OP_EQUAL;

        data->emplace_back(AirdropScriptItem{std::move(script), item.amount});
    }

    mainnetAirdrop = std::move(data);
}

const std::vector<AirdropScriptItem>& AirdropDataManager::GetMainnetAirdrop() {
    std::lock_guard<std::mutex> lock(airdropMutex);
    if (!mainnetAirdrop) {
        LoadMainnetAirdrop();
    }
    return *mainnetAirdrop;
}

std::vector<AirdropScriptItem> AirdropDataManager::GetEmptyAirdrop() {
    return std::vector<AirdropScriptItem>();
}

// Old, less memory efficient way
/*std::vector<AirdropScriptItem> LoadAirdrop() {

    // The two airdrop arrays are declared as global here but initialized in "chainparams.cpp" via the "airdrop.h" include file there
    extern const AirdropAddressItem airdrop_items_p2pkh[49003]; // number of p2pkh addresses
    extern const AirdropAddressItem airdrop_items_p2sh[999];  // number of p2sh addresses

    AirdropScriptItem newentry; 

    std::vector<AirdropScriptItem> vAirdrop_p2pkh;
	for (const AirdropAddressItem& i: airdrop_items_p2pkh) {
        // sanity checks
        assert(i.hashaddr.length() == 40);
        assert(i.amount > 0);
        // encode script for P2PKH
		newentry.script = (CScript() << OP_DUP << OP_HASH160 << ParseHex(i.hashaddr) << OP_EQUALVERIFY << OP_CHECKSIG);
		newentry.amount = i.amount;
		vAirdrop_p2pkh.push_back(newentry);
	}

    std::vector<AirdropScriptItem> vAirdrop_p2sh;
	for (const AirdropAddressItem& i: airdrop_items_p2sh) {
        // sanity checks
        assert(i.hashaddr.length() == 40);
        assert(i.amount > 0);        
        // encode script for P2SH
		newentry.script = (CScript() << OP_HASH160 << ParseHex(i.hashaddr) << OP_EQUAL);
		newentry.amount = i.amount;
		vAirdrop_p2sh.push_back(newentry);
    }

    std::vector<AirdropScriptItem> vAirdrop;
    vAirdrop = vAirdrop_p2pkh;
    vAirdrop.insert(vAirdrop.end(), vAirdrop_p2sh.begin(), vAirdrop_p2sh.end());

    return vAirdrop;
}*/


/*std::vector<AirdropScriptItem> EmptyAirdrop() {
    std::vector<AirdropScriptItem> vAirdrop;
    return vAirdrop;
}*/
