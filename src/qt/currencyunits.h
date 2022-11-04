// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2021 The Raven Core developers
// Copyright (c) 2022 The Evrmore Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EVRMORE_QT_CURRENCYUNITS_H
#define EVRMORE_QT_CURRENCYUNITS_H

#include <QString>
#include <array>

/** Currency unit definitions. Stores basic title and symbol for a evr swap asset,
 * as well as how many decimals to format the dispaly with.
*/
struct CurrencyUnitDetails
{
    const char* Header;
    const char* Ticker;
    float Scalar;
    int Decimals;
};

class CurrencyUnits
{
public:
    static std::array<CurrencyUnitDetails, 6> CurrencyOptions;

    static int count() {
        return CurrencyOptions.size();
    }
};

#endif // EVRMORE_QT_CURRENCYUNITS_H
