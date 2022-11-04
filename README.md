Evrmore Core integration/staging tree
=====================================

Evrmore is a fork of Evrmore (https://github.com/EvrmoreOrg/Evrmore)

What is Evrmore?
------------------
Evrmore (EVR) is a blockchain DeFi (decentralized finance) platform with built-in asset and DeFi primitives. Evrmore is based on the Bitcoin (BTC) UTXO model, is mined publicly and transparently using Proof-of-Work, is free and open source and is open for use and development in any jurisdiction. 


License
-------
Evrmore Core is released under the terms of the MIT license. See [COPYING](COPYING) for more information or see https://opensource.org/licenses/MIT.


Development Process
-------------------
The `master` branch is regularly built and tested, but is not guaranteed to be completely stable. [Tags](https://github.com/EvrmoreOrg/Evrmore/tags) are created regularly to indicate new official, stable release versions of Evrmore Core.

Active development is done in the `develop` branch. 

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

Please join us soon on discord at https://discord.gg/gESeAEtA


Testing
-------
Testing and code review is the bottleneck for development; we get more pull requests than we can review and test on short notice. Please be patient and help out by testing other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

We are working to bring Testnet up and running and available to use during development.

### Automated Testing

Developers are strongly encouraged to write [unit tests](src/test/README.md) for new code, and to submit new unit tests for old code. Unit tests can be compiled and run (assuming they weren't disabled in configure) with: `make check`. Further details on running and extending unit tests can be found in [/src/test/README.md](/src/test/README.md).

There are also [regression and integration tests](/test), written in Python, that are run automatically on the build server. These tests can be run (if the [test dependencies](/test) are installed) with: `test/functional/test_runner.py`

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the code. This is especially important for large or high-risk changes. It is useful to add a test plan to the pull request description if testing the changes is not straightforward.


About Evrmore
-------------
Thank you to Satoshi Nakamoto and all the Bitcoin developers for Bitcoin. Thank you Bruce Fenton, Tron Black, BlondFrogs and all the Evrmore developers for Evrmore.

Evrmore (EVR) is a blockchain DeFi (decentralized finance) platform with built-in asset and DeFi primitives. Evrmore is based on the Bitcoin (BTC) UTXO model, is mined publicly and transparently using Proof-of-Work, is free and open source and is open for use and development in any jurisdiction. Evrmore is built as a code and UTXO fork of Evrmore (RVN) and grew out of DeFi discussions and research within the Evrmore community. Evrmore employs full replay protection and forks Evrmore’s coins but not assets. It takes additional measures to protect Evrmore because it seeks not to replace Evrmore, but to co-exist with a different purpose. Evrmore extended the Bitcoin UTXO model with asset primitives thereby eliminating errors commonly caused by implementing assets as "colored coins.” Evrmore similarly extends the Bitcoin UTXO model with DeFi primitives thereby eliminating errors commonly caused by implementing DeFi in general-purpose smart contracts – implementing DeFi functionality using powerful and general-purpose smart contracts is not the correct solution. In addition to DeFi primitives, Evrmore includes features that improve transaction speed, flexibility, zero-confirmation, minimal transaction fees, scaling and complex covenant financial derivatives. Evrmore also includes code development financing and uses the Ethash algo to keep hardware requirements affordable and competitive  

