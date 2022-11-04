// Copyright (c) 2022 The Evrmore Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EVRMORE_AIRDROP_H
#define EVRMORE_AIRDROP_H

#include "primitives/transaction.h"

struct AirdropAddressItem {
    std::string hashaddr;
    CAmount amount;
};

struct AirdropScriptItem {
    CScript script;
    CAmount amount;
};

std::vector<AirdropScriptItem> LoadAirdrop();
std::vector<AirdropScriptItem> EmptyAirdrop();

#endif // EVRMORE_AIRDROP_H
