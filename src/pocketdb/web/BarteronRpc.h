// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_BARTERON_RPC_H
#define SRC_BARTERON_RPC_H

#include "rpc/server.h"
#include "pocketdb/web/WebRpcUtils.h"

namespace PocketWeb::PocketWebRpc
{
    using namespace std;

    RPCHelpMan GetBarteronAccounts();
    RPCHelpMan GetBarteronOffersByRootTxHashes();
    RPCHelpMan GetBarteronOffersByAddress();
    RPCHelpMan GetBarteronFeed();
    RPCHelpMan GetBarteronDeals();
    RPCHelpMan GetBarteronOffersDetails();
    RPCHelpMan GetBarteronComplexDeals();
}

#endif //SRC_BARTERON_RPC_H
