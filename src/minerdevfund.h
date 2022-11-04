// Copyright (c) 2022 The Evrmore developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EVRMORE_MINERFUND_H
#define EVRMORE_MINERFUND_H

#include "script/standard.h"

#include <vector>

namespace Consensus {
struct Params;
}

CAmount GetMinerDevFundAmount(const CAmount &coinbaseValue);

std::vector<CTxDestination> 
GetMinerDevFundWhitelist(const Consensus::Params &params);

#endif // EVRMORE_MINERFUND_H
