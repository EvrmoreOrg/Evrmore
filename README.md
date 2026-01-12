
# This is the respository for Evrmore Core

### A note about branches

This repository contains (at least) 3 git branches which serve the following purposes:

    branch "release_v1.0.5.1"
        This branch contains the code used to generate the "official release" binary executables of the latest mainnet release of Evmore Core.

    branch "maintenance"
        This branch is for updates to the latest mainnet release which are low risk and which do not affect blockchain consensus. It will contain bug fixes and changes to support updated operating systems and dependencies. This branch is not the lowest risk most tested code for Core. Since it is work-in-process, it may not build properly. But if you want a particular bug fix or update, and you want to build Core binaries for youself, then this is the best code for you.

    branch "develop"
        This branch contains possible future changes to Evrmore Core which add major new features, which change blockchain consensus, or which may be considered risky. This code is generally deployed to the testnet network during development. You should not use this code unless you are doing Core development work.

### What is Evrmore?

#### Evrmore (EVR) is a blockchain DeFi (decentralized finance) platform with built-in asset and DeFi primitives. Evrmore is based on the Bitcoin (BTC) UTXO model, is mined publicly and transparently using Proof-of-Work, is free and open source and is open for use and development in any jurisdiction. 


### License

Evrmore Core is released under the terms of the MIT license. See [COPYING](COPYING) for more information or see https://opensource.org/licenses/MIT.


### Community

#### The primary website for Evrmore is at https://evrmorecoin.org

#### Please jon us on Discord at https://discord.gg/4csauGuvw3

#### Follow us on Twitter at https://twitter.com/EvrFoundation


### About Evrmore

Thank you to Satoshi Nakamoto and all the Bitcoin developers for Bitcoin. Thank you Bruce Fenton, Tron Black, BlondFrogs and all the Evrmore developers for Evrmore.

Evrmore (EVR) is a blockchain DeFi (decentralized finance) platform with built-in asset and DeFi primitives. Evrmore is based on the Bitcoin (BTC) UTXO model, is mined publicly and transparently using Proof-of-Work, is free and open source and is open for use and development in any jurisdiction. Evrmore is built as a code and UTXO fork of Evrmore (RVN) and grew out of DeFi discussions and research within the Evrmore community. Evrmore employs full replay protection and forks Evrmore’s coins but not assets. It takes additional measures to protect Evrmore because it seeks not to replace Evrmore, but to co-exist with a different purpose. Evrmore extended the Bitcoin UTXO model with asset primitives thereby eliminating errors commonly caused by implementing assets as "colored coins.” Evrmore similarly extends the Bitcoin UTXO model with DeFi primitives thereby eliminating errors commonly caused by implementing DeFi in general-purpose smart contracts – implementing DeFi functionality using powerful and general-purpose smart contracts is not the correct solution. In addition to DeFi primitives, Evrmore includes features that improve transaction speed, flexibility, zero-confirmation, minimal transaction fees, scaling and complex covenant financial derivatives. Evrmore also includes code development financing and uses the Ethash algo to keep hardware requirements affordable and competitive  

### Mining

#### Evrcore Mainnet uses evrprogpow GPU Proof-of-Work mining

#### See https://github.com/EvrmoreOrg/evrprogpowminer for the GPU solo miner to core (no straturm proxy needed).

