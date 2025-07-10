// Copyright (c) 2022 The Evrmore Core developers
// Copyright (c) 2025 The Echelon Technology Group developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EVRMORE_AIRDROP_H
#define EVRMORE_AIRDROP_H

#include "primitives/transaction.h"
#include <memory>

// Now in airdropitems.h
/*struct AirdropAddressItem {
    std::string hashaddr;
    CAmount amount;
};*/

struct AirdropScriptItem {
    CScript script;
    CAmount amount;
};

// Lazy loading singleton for airdrop/chainparams compile optimization
class AirdropDataManager {
public:
    static const std::vector<AirdropScriptItem>& GetMainnetAirdrop();
    static std::vector<AirdropScriptItem> GetEmptyAirdrop();

private:
    static std::unique_ptr<std::vector<AirdropScriptItem>> mainnetAirdrop;
    static void LoadMainnetAirdrop();
};

// Old Methods
//std::vector<AirdropScriptItem> LoadAirdrop();
//std::vector<AirdropScriptItem> EmptyAirdrop();

#endif // EVRMORE_AIRDROP_H
