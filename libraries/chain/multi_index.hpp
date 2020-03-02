#pragma

#define GENERATE_PRED_CONSTANT(X, VALUE)      \
   namespace eosio { namespace multi_index {  \
      static inline constexpr bool X = VALUE; \
   }}

#ifdef EOS_USE_MIRA
   GENERATE_PRED_CONSTANT(use_mira_adaptor, true)
#else
   GENERATE_PRED_CONSTANT(use_mira_adaptor, false)
#endif

namespace eosio { namespace multi_index {
   namespace detail {
      template <typename... Args>
      struct which_container {
         using container_type  = std::conditional_t<use_mira_adaptor, mira::multi_index_adaptor<Args...>, boost::multi_index::multi_index_container>;
         using indexed_by_type = std::conditional_t<use_mira_adaptor, mira::multi_index::indexed_by, boost::multi_index::indexed_by>;
         using ordered_unique_type = std::conditional_t<use_mira_adaptor, mira::multi_index::ordered_unique, boost::multi_index::ordered_unique>;
         using tag_type = std::conditional_t<use_mira_adaptor, mira::multi_index::tag, boost::multi_index::tag>;
         using member_type = std::conditional_t<use_mira_adaptor, mira::multi_index::tag, boost::multi_index::tag>;
      };
   } // ns detail

   template <typename... Args>
   using container = detail::which_container<Args...>::container_type;
}} // ns eosio::chain
