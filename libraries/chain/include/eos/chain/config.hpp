/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/types/types.hpp>

namespace eos { namespace config {
using types::UInt16;
using types::UInt32;
using types::UInt64;
using types::UInt128;
using types::ShareType;
using types::Asset;
using types::AccountName;
using types::PermissionName;

const static char KeyPrefix[] = "EOS";

const static AccountName EosContractName = N(eos);

const static AccountName NobodyAccountName = N(nobody);
const static AccountName AnybodyAccountName = N(anybody);
const static AccountName ProducersAccountName = N(producers);

const static PermissionName ActiveName = N(active);
const static PermissionName OwnerName = N(owner);

const static ShareType InitialTokenSupply = Asset::fromString("90000000.0000 EOS").amount;

const static int BlockIntervalSeconds = 3;

/** Percentages are fixed point with a denominator of 10,000 */
const static int Percent100 = 10000;
const static int Percent1 = 100;

const static UInt32 DefaultMaxBlockSize = 5 * 1024 * 1024;
const static UInt32 DefaultTargetBlockSize = 128 * 1024;
const static UInt64 DefaultMaxStorageSize = 10 * 1024;
const static ShareType DefaultElectedPay = Asset(100).amount;
const static ShareType DefaultRunnerUpPay = Asset(75).amount;
const static ShareType DefaultMinEosBalance = Asset(100).amount;
const static UInt32 DefaultMaxTrxLifetime = 60*60;
const static UInt16 DefaultAuthDepthLimit = 6;
const static UInt32 DefaultMaxTrxRuntime = 10*1000;
const static UInt16 DefaultInlineDepthLimit = 4;
const static UInt32 DefaultMaxInlineMsgSize = 4 * 1024;
const static UInt32 DefaultMaxGenTrxSize = 64 * 1024;
const static UInt32 ProducersAuthorityThreshold = 14;

const static int BlocksPerRound = 21;
const static int VotedProducersPerRound = 20;
const static int IrreversibleThresholdPercent = 70 * Percent1;
const static int MaxProducerVotes = 30;

const static UInt128 ProducerRaceLapLength = std::numeric_limits<UInt128>::max();

const static auto StakedBalanceCooldownSeconds = fc::days(3).to_seconds();
} } // namespace eos::config

template<typename Number>
Number EOS_PERCENT(Number value, int percentage) {
   return value * percentage / eos::config::Percent100;
}
