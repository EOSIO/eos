#include "eos.hpp"

/**
 *  Transfer requires that the sender and receiver be the first two
 *  accounts notified and that the sender has provided authorization.
 */
struct Transfer {
  AccountName to;
  AccountName from;
  uint64_t    amount = 0;
  char        memo[]; /// extra bytes are treated as a memo and ignored by logic
};
