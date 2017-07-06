#include <eos.hpp>

/**
 *  The purpose of this contract is to implement something like Steem on EOS, this
 *  means this contract defines its own currency, allows people to create posts, vote
 *  on posts, and stake their voting power.
 *
 *  Unlike Steem, the goal is to enable maximum parallelism and enable the currency to
 *  be easily integrated with an exchange contract.
 */

struct PostAction {
   AccountName author;
   PostName    postid;
   AccountName reply_to_author;
   int32_t     reply_to_id;
   uint8_t     author; /// index in notify list
   char[]      data; /// ignored, title is embedded within

   AccountName getAuthor()const { return getNotify(author); }
};

struct VoteAction {
   AccountName voter;
   AccountName author;
   PostName    postid;
   int32_t     vote_power;
};

struct PostRecord {
   uint64_t total_votes   = 0;
   uint64_t claimed_votes = 0;
   uint32_t created;

   PostRecord( uint32_t c = now() ):created(c){}
   static Name tableId() { return Name("post"); }
};

struct Account {
   uint64_t social       = 0;
   uint64_t social_power = 0;
   int32_t  vote_power   = 0;
   uint32_t last_vote    = 0;

   static Name tableId() { return Name("account"); }
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
   const auto& post   = currentMessage<PostAction>();
   requireAuth( post.author );

   assert( currentContext() == post.author, "cannot call from any other context" );
   
   static PostRecord& existing;
   if( !Db::get( post.postid, existing ) )
      Db::store( post.postid, PostRecord( now() ) );
}

/**
 * This action is called when a user casts a vote, it requires that this code is executed
 * in the context of both the voter and the author. When executed in the author's context it
 * updates the vote total.  When executed 
 */
void apply_social_vote() {
   const auto& vote  = currentMessage<VoteAction>();
   requireNotice( vote.voter, vote.author );
   disableContextCode( vote.author() ); /// prevent the author's code from rejecting the potentially negative vote

   auto context = currentContext();
   auto voter = vote.getVoter();

   if( context == vote.author ) {
      static PostRecord post;
      assert( Db::get( vote.postid, post ) > 0, "unable to find post" );
      assert( now() - post.created < days(7), "cannot vote after 7 days" );
      post.votes += vote.vote_power;
      Db::store( vote.postid, post );
   } 
   else if( context == vote.voter ) {
      static Account account;
      Db::get( "account", account );
      auto abs_vote = abs(vote.vote_power);
      account.vote_power = max( account.social_power, 
                                account.vote_power + (account.social_power * (now()-last_vote)) / days(7));
      assert( abs_vote <= account.vote_power, "insufficient vote power" );
      post.votes += vote.vote_power;
      account.vote_power -= abs_vote;
      account.last_vote  = now();
      Db::store( "account", account );
   } else {
      assert( false, "invalid context for execution of this vote" );
   }
}

