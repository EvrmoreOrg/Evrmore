// Copyright (c) 2022 The Evrmore Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "airdrop.h"
//#include "util.h"
#include "utilstrencodings.h"


std::vector<AirdropScriptItem> LoadAirdrop() {

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
}

std::vector<AirdropScriptItem> EmptyAirdrop() {
    std::vector<AirdropScriptItem> vAirdrop;
    return vAirdrop;
}
