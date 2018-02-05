/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos.hpp>

/**
 *  The purpose of this contract is to implement something like Steem on EOS, this
 *  means this contract defines its own currency, allows people to create posts, vote
 *  on posts, and stake their voting power.
 *
 *  Unlike Steem, the goal is to enable maximum parallelism and enable the currency to
 *  be easily integrated with an exchange contract.
 */

struct post_action {
   account_name author;
   post_name    postid;
   account_name reply_to_author;
   int32_t      reply_to_id;
   uint8_t      author; /// index in notify list
   char[]       data; /// ignored, title is embedded within

   account_name get_author()const { return get_notify(author); }
};

struct vote_action {
   account_name voter;
   account_name author;
   post_name    postid;
   int32_t      vote_power;
};

struct post_record {
   uint64_t total_votes   = 0;
   uint64_t claimed_votes = 0;
   uint32_t created;

   post_record( uint32_t c = now() ):created(c){}
   static Name table_id() { return Name("post"); }
};

struct account {
   uint64_t social       = 0;
   uint64_t social_power = 0;
   int32_t  vote_power   = 0;
   uint32_t last_vote    = 0;

   static Name table_id() { return Name("account"); }
};


/**
 * When a user posts we create a record that tracks the total votes and the time it
 * was created. A user can submit this action multiple times, but subsequent calls do
 * nothing.
 *
 * This method only does something when called in the context of the author, if
 * any other contexts are notified 
 */
void apply_social_post() {
   const auto& post   = current_action<post_action>();
   require_auth( post.author );

   assert( current_context() == post.author, "cannot call from any other context" );
   
   static post_record& existing;
   if( !Db::get( post.postid, existing ) )
      Db::store( post.postid, post_record( now() ) );
}

/**
 * This action is called when a user casts a vote, it requires that this code is executed
 * in the context of both the voter and the author. When executed in the author's context it
 * updates the vote total.  When executed 
 */
void apply_social_vote() {
   const auto& vote  = current_action<vote_action>();
   require_recipient( vote.voter, vote.author );
   disable_context_code( vote.author() ); /// prevent the author's code from rejecting the potentially negative vote

   auto context = current_context();
   auto voter = vote.getVoter();

   if( context == vote.author ) {
      static post_record post;
      assert( Db::get( vote.postid, post ) > 0, "unable to find post" );
      assert( now() - post.created < days(7), "cannot vote after 7 days" );
      post.votes += vote.vote_power;
      Db::store( vote.postid, post );
   } 
   else if( context == vote.voter ) {
      static account vote_account;
      Db::get( "account", vote_account );
      auto abs_vote = abs(vote.vote_power);
      vote_account.vote_power = min( vote_account.social_power,
                                     vote_account.vote_power + (vote_account.social_power * (now()-last_vote)) / days(7));
      assert( abs_vote <= vote_account.vote_power, "insufficient vote power" );
      post.votes += vote.vote_power;
      vote_account.vote_power -= abs_vote;
      vote_account.last_vote  = now();
      Db::store( "account", vote_account );
   } else {
      assert( false, "invalid context for execution of this vote" );
   }
}

