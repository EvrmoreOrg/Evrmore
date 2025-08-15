// Copyright (c) 2022 The Evrmore Core developers
// Copyright (c) 2025 The Echelon Technology Group developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EVRMORE_AIRDROPITEMS_H
#define EVRMORE_AIRDROPITEMS_H

#include "amount.h"

// Use a POD structure with no std::string to reduce compile overhead
struct AirdropAddressItem {
    const char* hashaddr;  // Just a pointer to string literal
    CAmount amount;
};

// Moved these to a .cpp file, NO LONGER in the header!
extern const AirdropAddressItem airdrop_items_p2pkh[49003];
extern const AirdropAddressItem airdrop_items_p2sh[999];

#endif // EVRMORE_AIRDROPITEMS_H
