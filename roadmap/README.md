# Evrmore Functionality and Roadmap

### Core Asset Functionality

Evrmore (EVR) inherits its core asset functionality from the software codebase of Ravencoin (RVN).
Evrmore builds upon that base with additional protocol-level Distributed Finance capability.

#### Asset Support

Evrmore extends the bitcoin UTXO architecture to include the ability to issue, reissue, and transfer assets. Assets can be reissuable or limited to a set supply at the point of issuance. There is a cost in EVRs to create any qty of an asset. Each asset name must be unique. 

Examples of valid assets:  
THE_GAME  
A.TOKEN  
123  

Examples of invalid assets:  
_TOKEN  
THEEND.  
A..B (consecutive punctuation)  
AB  
12  
.FIRST
apple

The EVR used to issue assets are sent to a burn address, which reduces the amount of EVR available. 

Asset transfers require the standard EVR transaction fees for transfer from one address to another.

#### Metadata

Metadata about the token can be stored in IPFS.

#### Rewards

It is easy to issue EVR rewards (similar to stock dividends) to all holders of an asset. Payments of EVR can be distributed to all asset holders pro rata simply by querying an EVR node for all addresses holding a particular asset, and then paying EVRs to those same addresses.

Example: A small software company issues an asset GAMECO that represents a share of the project. GAMECO tokens can be traded with others. Once the software company profits, those profits can be distributed to all holders of GAMECO by sending the profits (via EVR) to all holders of GAMECO.

#### Block Size

Evrmore currently uses a blocksize of 2 MB. In the future, Evrmore plans to follow the Bitcoin-Cash code extension to increase capacity and to add some of the BCH- inspired capabilties.


### Unique Assets

Once created, assets can be made unique for a cost of an EVR fee. Only non-divisible assets can be made unique. This moves an asset to a UTXO and associates a unique identifier with the txid. From this point the asset can be moved from one address to another and can be traced back to its origin. Only the issuer of the original asset can make an asset unique.  
The costs to make unique assets will be sent to a burn address.  

Some examples of unique assets:  
*  Imagine that an art dealer issues the asset named ART. The dealer can then make unique ART assets by attaching a name or a serialized number to each piece of art. These unique tokens can be transferred to the new owner along with the artwork as a proof of authenticity. The tokens ART#MonaLisa and ART#VenusDeMilo are not fungible and represent distinct pieces of art.
*  A software developer can issue the asset with the name of their software ABCGAME, and then assign each ABCGAME token a unique id or license key. The game tokens could be transferred as the license transfers. Each token ABCGAME#398222 and ABCGAME#398223 are unique tokens.
*  In game assets. A game ZYX_GAME could create unique limited edition in-game assets that are owned and used by the game player. Example: ZYX_GAME#Sword005 and ZYX_GAME#Purse
*  EVR based unique assets can be tied to real world assets. Create an asset named GOLDVAULT. Each gold coin or gold bar in a vault can be serialized and audited. Associated unique assets GOLDVAULT#444322 and GOLDVAULT#555994 can be created to represent the specific assets in the physical gold vault. The public nature of the chain allows for full transparency.

Messaging

Messages can be attached to any Evrmore transaction by attaching an IPFS URL to that transaction. Furthermore, an IPFS URL can be attached to any asset type to point to further information or content related to that asset.

### Voting

Voting can be accomplished by creating and distributing parallel tokens to token holders. These tokens can be sent to EVR addresses to record a vote. In the future, a new asset type which expires will be created, so that expired votes need not continue being tracked as part of the UTXO if so desired.

### Restricted Assets

Restricted assets allow the easy implementation of applications which require certain approvals in order to hold those tokens. Further, the token issuer maintains a certain level of control over those tokens even after they have been transferred to an addresses controlled by another party. A restricted asset is always associated with one or more Qualifier strings. Only addresses which satisfy the logic of the qualifiers may receive a restricted asset during a transfer. Thus, an address can be required to prove a requirement such as a club membership or a KYC requirement to limit which addresses may receive the associated asset. Moreover, assets may be frozen at certain addresses or globally to enforce compliance with the appropriate requirements.


### RPC commands for assets

For a list of RPC commands currently implemented by Evrmore, please see:
	"https://hans-schmidt.github.io/mastering_evrmore/reference/All defined Ravencoin evrmore-qt RPC calls.html"
	
### ROADMAP: Evrmore additional protocol-level Distributed Finance capability

For a detailed description of the Defi capability planned for Evrmore, please see:
	"https://hans-schmidt.github.io/mastering_evrmore/evermore_defi_roadmap/The_Evermore_Defi_Roadmap_for_Ravencoin.html"
	


