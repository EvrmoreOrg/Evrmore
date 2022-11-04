#!/usr/bin/env python3
# Copyright (c) 2022 The Evrmore developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.blocktools import (create_block, create_coinbase)
from test_framework.messages import to_hex
from test_framework.test_framework import EvrmoreTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
)

from decimal import Decimal

MINER_FUND_RATIO = 10
MINER_FUND_ADDR = '2MtafNz7A8yycP6eQ3tSP1Z7A5VmAxei894'
BLOCK_TIME = 1589544000

class MinerDevFundTest(EvrmoreTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        fund_address = MINER_FUND_ADDR
      # node = self.nodes[0]
      # address = node.get_deterministic_priv_key().address
      # node.generatetoaddress(1, address)
      # height = node.getblockcount()

        self.run_no_miner_fund_test()
        self.run_miner_fund_test(fund_address)

    def run_no_miner_fund_test(self):
        self.log.info("Testing without minerdevfund enabled")
        
        node = self.nodes[0]
#        address = node.get_deterministic_priv_key().address
#        node.generatetoaddress(1, address)
        node.generate(1)

        def get_best_coinbase():
            return node.getblock(node.getbestblockhash(), 2)['tx'][0]

        coinbase = get_best_coinbase()
        self.log.info("Checking that generate does not send anything to the fund")
        assert_equal(len(coinbase['vout']), 2)

    def run_miner_fund_test(self, fund_address):
        self.log.info("Testing with minerdevfund enabled")

        node = self.nodes[0]
        self.stop_node(0)
        self.start_node(0, extra_args=['-enableminerdevfund'])

#        address = node.get_deterministic_priv_key().address
#        node.generatetoaddress(1, address)
        node.generate(1)

        def get_best_coinbase():
            return node.getblock(node.getbestblockhash(), 2)['tx'][0]

        self.log.info("Checking that part of the coinbase is sent to the devfund addr")
        coinbase = get_best_coinbase()
        assert_equal(len(coinbase['vout']), 3)
        assert_equal(
            coinbase['vout'][1]['scriptPubKey']['addresses'][0],
            fund_address)

        total = Decimal()
        for o in coinbase['vout']:
            total += o['value']

        self.log.info("Checking that the devfund payment is large enough")
        assert_greater_than_or_equal(
            (total * MINER_FUND_RATIO)/100,
            coinbase['vout'][1]['value'])

        # Submit a custom block that does not send anything
        #     to the fund, and check it is rejected.
        
      # node.invalidateblock(node.getbestblockhash())

        block_height = node.getblockcount()
        block_hash = int(node.getbestblockhash(), 16)
        block_time = node.getblock(node.getbestblockhash())['time']

        block = create_block(
            block_hash, create_coinbase(block_height + 1), block_time + 60)
        block.nVersion = 805306368
        block.solve()

        self.log.info("Checking that blocks without minerdevfund payment are rejected")
        #assert_equal(node.submitblock(to_hex(block)), 'bad-cb-minerdevfund')
        assert node.submitblock(to_hex(block)) in ['bad-cb-minerdevfund-1', 'bad-cb-minerdevfund-2']

if __name__ == '__main__':
    MinerDevFundTest().main()
