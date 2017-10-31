#define BOOST_TEST_MODULE incremental_merkle
#include <boost/test/unit_test.hpp>

#include <eosio/blockchain/incremental_merkle.hpp>
#include <eosio/blockchain/merkle.hpp>

using namespace eosio::blockchain;

static auto test_digests = vector<digest_type>({
   digest_type("f4cbab621176d3b6ad31dba075c9fe4e97a1139a0e890f6aa852c01b76a2db63"), digest_type("8c18afa57e070e0dbbc67fe5f21e3be1de4d41b05eb8d4579c93f23df6fa90f9"),
   digest_type("1834d0a56d1321ce1012fde26b3062394a9d9f66522abc6063b17447d7440c58"), digest_type("0f63d26ee027ddbee1438ceaa6e894f7d0d40c97a0ef24d1b2c0cce425d0486b"),
   digest_type("1e32501716ff8717e95d00d477d886b0ccea347e7380d40c7fed4ab31b778bdb"), digest_type("d4a8a81f62b1afeb8e47c75facc72d85305014174d2afdc8e3108af4ccd25613"),
   digest_type("ce8eaea4d0094cbaf24b19a9fda97f25c3aebacaddfff002f1a9f155bffde498"), digest_type("442b9795b4e918ceed5ff128084d8bf4560960b1e8df6e74719bf296ab65c5ad"),
   digest_type("2cba24633e718a8e3a02786eb40e57e3f9b4808c6f8b95f1cebede51b92cc120"), digest_type("8e478d9b5ab6c74354c13f0f046148f72817a6769eda63b350cd93880ec4e764"),
   digest_type("a479ae07a7398774ccefeb3d010a42edb42959136e7aa99048d28060e8c98801"), digest_type("11a131976985b119fa2cc965ea72e59ba9745a32d4611afcadf8fe505a726bc8"),
   digest_type("6d06d67a3acb04f13a8f912d9870c8d81f5f56f9b760cc11a76aacf607897c76"), digest_type("d3b7f3fad59faec5ead5fa6c8584dd8247207fc7b181ba890d4cf55a71b74074"),
   digest_type("463ea6422359c5468b32d2f446ac8db1d9c0fed989b4e7ae7cabed4de3bb43bd"), digest_type("5da8a26cff5ba32f5a4a603658461a7bab968b6eaaa91d779f631bf4aec10eed"),
   digest_type("1d526ddc9a9396673c2a2aca71b01f810015f6c4d73777ade2fdd73210dd416e"), digest_type("eacda74e7a2c741f7da01c8948d5eed07ce6a6d280379efe9be7e9d871607ca6"),
   digest_type("6de94359f2b53211d596dd1ca5000f50dd2f6604ba055ba32c4688f736164293"), digest_type("ae7987ee1171d7aeb82305ef2f31dc8fab4304032459be415f9d542c17e16245"),
   digest_type("0ee517ec4900cd02d6685e689b350525e851289f85be9b9b19e70b0388e147e6"), digest_type("a5f92d3dc2b691d9a7e6dddf448511e8392e80b5ef27f011754b8df70099ac0f"),
   digest_type("6a466ca314c0b0665bb8d05c0991c569d25e07396b9efc83936e51895de13e4e"), digest_type("d8d47c34a6b45614c2c1aca7d35b252118c554409efc0a43e4a2510b2d4aacd8"),
   digest_type("d751d319cfbbb9f14dbbf3a3900fa7fe0c015a5bc89d996f25b40934576a2f15"), digest_type("e232fa56b9475b777c1050d0d9282131a1d8e2f2b86a000140ec9781dd0797b5"),
   digest_type("00dab0db43916b17a57b5b57171ae5a73b52a388045b9fcf24a18eb5c19f0a45"), digest_type("d32c3e83fc5224b4af7b0f0e5ca9f70beb432adf1e649a31b0ee1791afd10558"),
   digest_type("2658e077f6d8b56e84a4ffa0358ea7750000b116c4228e77e453f36f8bcb40b6"), digest_type("d4cff98dd2586b62e081b697c8ea7fa0e854ca1350a5663459a20c5719644c02"),
   digest_type("9776cc1055a512dfcea8a96e852a38ffa36ad5e03d2f4d3398d28930c94e0f19"), digest_type("2f94b560bcca6eba37121cc0b0439b78772f8863070677db93d8152e8b80dd5f"),
   digest_type("7ca48a9dd840a25f262a454b5e5ddbd5262c0df53d4c9ef603e630a1b65e7be6"), digest_type("92056d822fef96caccee8fa8bfa57692c43d11f9f23256f42a95de31655cb0eb"),
   digest_type("c076ba5090b4a6683d020c5b98e13fe50c471482b8d54b163297638f416aab19"), digest_type("0ee782cdc3e8a394ffbbde073c157d7159198249dea32178f3a275fe705274fd"),
   digest_type("31088db72ff38d7fea6545df004ad19a514c581d42a12443871aed07010fc363"), digest_type("beeb0e174bbe72252272143d2c0047e32d09b70c449e1f3bb055c8e58a6e1fe5"),
   digest_type("1233bb0fb16784e24b3eeeb4860436d93f86ebe52973c3865ee7f8ca2488e196"), digest_type("7289794e302e69d272871bfe3b65f78476c0ffee817c41fb36ae1b5a724ba3ac"),
   digest_type("37378cd8c1bc7d5b45e4ff14ba064469f5e116db346164581dde88cd277accf2"), digest_type("3d3b1e60f6d33ae6d4575cb3cb02abab027c40b84ebde57a2fdd1530589b4688"),
   digest_type("782628657cb20769d73fde6f46aa7523f4ecb0886f9b09bd0f6d8557ec9235cf"), digest_type("64eb239063013fd7edf93134625d3a591d952be27325091eb517a543e0783de5"),
   digest_type("f35d3c728d5f0d6fdde3aa7a01f1064e155acff01c885f67fcfbbfffa8dc3327"), digest_type("83e1d6b6b4f0a77853cf29901a292f1beac699da27d905a27e165c62374e9e13"),
   digest_type("841c9727c7531ec1750dce91f184f76d5c3df41e31bfb99eb913381d7f8c3abd"), digest_type("5b76c6192260d4229528fe4c1a9707d06402e22a950d2f8938cb9cdd2550df6a"),
   digest_type("95d0a6228524e321f7a9a9c9b9ce8c9350c7c4f1938c31974bbd2d2824a61d3b"), digest_type("4b0a151c34326fd5630540877b93daa18dbc3977584261b11e77cd5059c33566"),
   digest_type("a87a1857a48a52813f3463e2828e2aa466b1a0820de31a63dd83afceb68ccd85"), digest_type("10933a75ea665c08c02252a15b795843493031e974b49ec8697e0cc6478257fc"),
   digest_type("fff516f38edba53b9869edd1a31056ec05fee08310483bdd152b05151d0012dc"), digest_type("e96b81d6bbdeac4625be9019592247e86b923cc9844d771f0382af2dcc0085e5"),
   digest_type("9abe2f774b58b3d14cb2d68208bf15dd7f2034258d7d445c124fafa4c56b90ed"), digest_type("b09b4a87f228fd5fea45612dbae8e267ce446a46169b75b9ada50b7596f04256"),
   digest_type("3b14795a75d5bdbd302b490d3dcbcfe59b058a778889990055b5de2ba26c6562"), digest_type("50ed6afd30611aad55d9e67eb709b552a34f3adc10b6b8228ca073969f50bebb"),
   digest_type("be173d106343e2c6bd63993f726e5b4ec77b39b11c5897e79756da6b6e779b97"), digest_type("ee09d95f1d9b982eea8fe566d619fff2065c88a685c2782541c1b08bfb43a728"),
   digest_type("2a42930de29bf02209ee81f5e32dab2ac4ec09779bc0e306f653e09842a58658"), digest_type("57a00f805d4376d8ff4d42d1edfb4eec810ccad9a756bb7fa703643a8810ae5b"),
   digest_type("ec7199f8148647a044e897f7d3a569dcf7c5019f88ef8ed4e551290a2e8f0e9d"), digest_type("90daaf67afb3383b1de49e3a73fcd132c8d9c4ddd3a36c29454a3a4ea0be9f06"),
});

BOOST_AUTO_TEST_SUITE(incremental_merkle_tests)
BOOST_AUTO_TEST_CASE(test_against_merkle_iterative) {
   auto incremental = incremental_merkle();
   auto digests = vector<digest_type>();
   digests.reserve( test_digests.size() );

   for(const auto& digest : test_digests) {
      digests.emplace_back(digest);
      auto expected = merkle(digests);
      auto root =incremental.append(digest);
      BOOST_TEST_INFO( "Node Count = " << digests.size() );
      BOOST_REQUIRE_EQUAL(root.str(), expected.str());
   }
}

BOOST_AUTO_TEST_SUITE_END()