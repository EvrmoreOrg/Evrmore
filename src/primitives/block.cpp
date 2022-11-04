// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2020 The Raven Core developers
// Copyright (c) 2022 The Evrmore Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"

#include <hash.h>
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "crypto/common.h"

bool fEvrprogpowAsMiningAlgo;

BlockNetwork bNetwork = BlockNetwork();

BlockNetwork::BlockNetwork()
{
    fOnTestnet = false;
    fOnRegtest = false;
}

void BlockNetwork::SetNetwork(const std::string& net)
{
    if (net == "test") {
        fOnTestnet = true;
    } else if (net == "regtest") {
        fOnRegtest = true;
    }
}

uint256 CBlockHeader::GetHash() const
{
        if (fEvrprogpowAsMiningAlgo) {
            return EVRPROGPOWHash_OnlyMix(*this);
        } else {
            return SerializeHash(*this);        // Use standard SHA256 mining
        }
}


uint256 CBlockHeader::GetHashFull(uint256& mix_hash) const
{
        if (fEvrprogpowAsMiningAlgo) {
            return EVRPROGPOWHash(*this, mix_hash);
        } else {
            return SerializeHash(*this);        // Use standard SHA256 mining
        }
}


uint256 CBlockHeader::GetEVRPROGPOWHash_OnlyMix() const
{
    return EVRPROGPOWHash_OnlyMix(*this);
}

uint256 CBlockHeader::GetSerializeHash() const
{
    return SerializeHash(*this);        // Use standard SHA256 mining
}

/**
 * @brief This takes a block header, removes the nNonce64 and the mixHash. Then performs a serialized hash of it SHA256D.
 * This will be used as the input to the KAAAWWWPOW hashing function
 * @note Only to be called and used on KAAAWWWPOW block headers
 */
uint256 CBlockHeader::GetEVRPROGPOWHeaderHash() const
{
    CEVRPROGPOWInput input{*this};

    return SerializeHash(input);
}

std::string CBlockHeader::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, nNonce64=%u, nHeight=%u)\n",
                   nVersion,
                   hashPrevBlock.ToString(),
                   hashMerkleRoot.ToString(),
                   nTime, nBits, nNonce, nNonce64, nHeight);
    return s.str();
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, nNonce64=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce, nNonce64,
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}
