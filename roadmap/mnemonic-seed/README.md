# Mnemonic Seed

For all cryptocurrencies and crypto-assets, the greatest difficulty is securing your private keys.  

Over the last ten years, three standards have emerged which are implememted in the Evrmore core and mobile wallets:
	-BIP-32 (Hierarchical Deterministic wallets) generate all their keys from an initial seed, so that only that seed needs to be kept in order to maintain control and access to all wallet keys.
	-BIP-44 implements BIP-32 in a standardized ways with defined coin types, standardized ways of putting receiving addresses and change addresses into their own trees, and support for multiple accounts in a wallet. This makes it possible to use other wallet vendors if your wallet sotware becomes obsoleted and unavailable.
	-BIP-39 (Mnemonic code generation) provides a standardized way of generating a BIP-32 initial seed from a set of natural-language words and optional extra security words. (Evrmore uses a 12-word mnemonic seed and an option 13th word for more security).

Implementing these three standards in Evrmore ensures an optimal level of security and minimal risk for the storage of Evrmore keys.

The advantage to this method of key generation is that the 12-word mnemonic seed can be written down, or stamped into stainless steel and safely stored offline without the risk of a thumb-drive or hard drive backup failure. Also, the same 12-word seed can be entered into an online or offline webwallet in order to sign transactions using the derived keys. This allows simple online web wallets for assets combined with the security of having the keys held by each owner rather than by a centralized custodian.

### Technical

The BIP-32/BIP-44/BIP-39 implementation in Evrmore is done using the following parameters:
	-The BIP-44 coinid for Evrmore is 175
	-The BIP-44 derivation path is m/44'/175'/0'/0

Note that is the user imports keys into their Evrmore wallet, then those keys cannot be automatically generated from the initial mnemonic seed words. In that case, a copy of the wallet.dat for the Evrmore core wallet must be kept in order to keep access to the keys.

