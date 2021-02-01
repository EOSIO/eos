#include <eosio/tpm-helpers/tpm-helpers.hpp>
#include <fc/scoped_exit.hpp>
#include <fc/fwd_impl.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/process.hpp>

#include <random>

extern "C" {
#include <tss2_esys.h>
#include <tss2_rc.h>
#include <tss2_mu.h>
#include <tss2_tctildr.h>
}

namespace eosio::tpm {

static const TPM2B_PUBLIC primary_template = {
   .size = 0,
   .publicArea = {
      .type = TPM2_ALG_RSA,
      .nameAlg = TPM2_ALG_SHA256,
      .objectAttributes = (  TPMA_OBJECT_RESTRICTED|TPMA_OBJECT_DECRYPT
                            |TPMA_OBJECT_FIXEDTPM|TPMA_OBJECT_FIXEDPARENT
                            |TPMA_OBJECT_SENSITIVEDATAORIGIN|TPMA_OBJECT_USERWITHAUTH
                          ),
      .authPolicy = {
         .size = 0,
         .buffer = {0},
      },
      .parameters = {
         .rsaDetail = {
            .symmetric = {
               .algorithm = TPM2_ALG_AES,
               .keyBits = {.aes = 128},
               .mode = {.aes = TPM2_ALG_CFB},
            },
            .scheme = {
               .scheme = TPM2_ALG_NULL,
            },
            .keyBits = 2048,
            .exponent = 0,
         }
      }
   }
};

static const TPM2B_PUBLIC ecc_key_template = {
   .size = 0,
   .publicArea = {
      .type = TPM2_ALG_ECC,
      .nameAlg = TPM2_ALG_SHA256,
      .objectAttributes = (  TPMA_OBJECT_SIGN_ENCRYPT
                            |TPMA_OBJECT_FIXEDTPM|TPMA_OBJECT_FIXEDPARENT
                            |TPMA_OBJECT_SENSITIVEDATAORIGIN|TPMA_OBJECT_USERWITHAUTH
                          ),
      .parameters = {
         .eccDetail = {
            .symmetric = {
               .algorithm = TPM2_ALG_NULL,
               .mode = {.aes = TPM2_ALG_NULL},
            },
            .scheme = {
               .scheme = TPM2_ALG_NULL,
            },
            .curveID = TPM2_ECC_NIST_P256,
            .kdf = { .scheme = TPM2_ALG_NULL, .details = {} }
         }
      }
   }
};

class esys_context {
public:
   esys_context(const std::string& tcti) {
      TSS2_RC rc;

      if(!tcti.empty()) {
         rc = Tss2_TctiLdr_Initialize(tcti.c_str(), &tcti_ctx);
         FC_ASSERT(!rc, "Failed to initialize tss tcti \"${s}\": ${m}", ("s", tcti)("m", Tss2_RC_Decode(rc)));
      }

      TSS2_ABI_VERSION abi_version = TSS2_ABI_VERSION_CURRENT;
      rc = Esys_Initialize(&esys_ctx, tcti_ctx, &abi_version);
      if(rc) {
         if(tcti_ctx)
            Tss2_TctiLdr_Finalize(&tcti_ctx);
         FC_ASSERT(!rc, "Failed to initialize tss esys: ${m}", ("m", Tss2_RC_Decode(rc)));
      }
   }

   ~esys_context() {
      if(esys_ctx)
         Esys_Finalize(&esys_ctx);
      if(tcti_ctx)
         Tss2_TctiLdr_Finalize(&tcti_ctx);
   }

   ESYS_CONTEXT* ctx() const {
      return esys_ctx;
   }

   esys_context(esys_context&) = delete;
   esys_context& operator=(const esys_context&) = delete;
private:
   TSS2_TCTI_CONTEXT* tcti_ctx = nullptr;
   ESYS_CONTEXT* esys_ctx = nullptr;
};

TPML_PCR_SELECTION pcr_selection_for_pcrs(const std::vector<unsigned>& pcrs) {
   constexpr unsigned max_pcr_value = 23u;  ///maybe query this from the TPM? at least simulator is angry if too large
   TPML_PCR_SELECTION pcr_selection = {1u, {{TPM2_ALG_SHA256, (max_pcr_value+7)/8}}};
   FC_ASSERT(pcrs.size() < 8, "Max number of PCRs is 8");
   for(const unsigned& pcr : pcrs) {
      FC_ASSERT(pcr <= max_pcr_value, "PCR value must be less than or equal to ${m}", ("m",max_pcr_value));
      pcr_selection.pcrSelections[0].pcrSelect[pcr/8u] |= (1u<<(pcr%8u));
   }
   return pcr_selection;
}

fc::sha256 current_pcr_hash_for_pcrs(esys_context& esys_ctx, const TPML_PCR_SELECTION& pcr_selection) {
   UINT32 pcr_update_counter;
   TPML_DIGEST* pcr_digests;

   TSS2_RC rc = Esys_PCR_Read(esys_ctx.ctx(), ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, &pcr_selection, &pcr_update_counter, NULL, &pcr_digests);
   FC_ASSERT(!rc, "Failed to read current PCR digests: ${m}", ("m", Tss2_RC_Decode(rc)));
   auto cleanup_pcr_digests = fc::make_scoped_exit([&]() {free(pcr_digests);});

   fc::sha256::encoder enc;
   for(unsigned i = 0; i < pcr_digests->count; ++i)
      enc.write((const char*)pcr_digests->digests[i].buffer, pcr_digests->digests[i].size);
   return enc.result();
}

class session_with_pcr_policy {
public:
   session_with_pcr_policy(esys_context& esys_ctx, const std::vector<unsigned>& pcrs, bool trial = false) : esys_ctx(esys_ctx) {
      TSS2_RC rc;

      TPMT_SYM_DEF symmetric = {TPM2_ALG_NULL};
      rc = Esys_StartAuthSession(esys_ctx.ctx(), ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, NULL,
                                 trial ? TPM2_SE_TRIAL : TPM2_SE_POLICY, &symmetric, TPM2_ALG_SHA256, &session_handle);
      FC_ASSERT(!rc, "Failed to create TPM auth session: ${m}", ("m", Tss2_RC_Decode(rc)));
      auto cleanup_auth_session = fc::make_scoped_exit([&]() {Esys_FlushContext(esys_ctx.ctx(), session_handle);});

      TPM2B_DIGEST pcr_digest = {sizeof(fc::sha256)};
      TPML_PCR_SELECTION pcr_selection = pcr_selection_for_pcrs(pcrs);
      fc::sha256 read_pcr_digest = current_pcr_hash_for_pcrs(esys_ctx, pcr_selection);
      memcpy(pcr_digest.buffer, read_pcr_digest.data(), sizeof(fc::sha256));

      rc = Esys_PolicyPCR(esys_ctx.ctx(), session_handle, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, &pcr_digest, &pcr_selection);
      FC_ASSERT(!rc, "Failed to set PCR policy on session: ${m}", ("m", Tss2_RC_Decode(rc)));

      cleanup_auth_session.cancel();
   }

   fc::sha256 policy_digest() {
      TPM2B_DIGEST* policy_digest;
      TSS2_RC rc = Esys_PolicyGetDigest(esys_ctx.ctx(), session_handle, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, &policy_digest);
      FC_ASSERT(!rc, "Failed to get policy digest: ${m}", ("m", Tss2_RC_Decode(rc)));
      auto cleanup_policy_digest = fc::make_scoped_exit([&]() {free(policy_digest);});
      FC_ASSERT(policy_digest->size == sizeof(fc::sha256), "policy digest size isn't expected");

      return fc::sha256((const char*)policy_digest->buffer, policy_digest->size);
   }

   ~session_with_pcr_policy() {
      Esys_FlushContext(esys_ctx.ctx(), session_handle);
   }

   ESYS_TR session() const {return session_handle;}

private:
   esys_context& esys_ctx;
   ESYS_TR session_handle;
};

fc::crypto::public_key tpm_pub_to_pub(const TPM2B_PUBLIC* tpm_pub) {
   FC_ASSERT(tpm_pub->publicArea.type == TPM2_ALG_ECC, "Not an ECC key");
   FC_ASSERT(tpm_pub->publicArea.parameters.eccDetail.curveID == TPM2_ECC_NIST_P256, "ECC key is not p256 curve");
   FC_ASSERT(tpm_pub->publicArea.unique.ecc.x.size == 32 && tpm_pub->publicArea.unique.ecc.y.size == 32, "p256 key points not expected size");

   fc::crypto::public_key key;
   char serialized_public_key[1 + sizeof(fc::crypto::r1::public_key_data)] = {fc::get_index<fc::crypto::public_key::storage_type, fc::crypto::r1::public_key_shim>()};
   memcpy(serialized_public_key + 2, tpm_pub->publicArea.unique.ecc.x.buffer, 32);
   serialized_public_key[1] = 0x02u + (tpm_pub->publicArea.unique.ecc.y.buffer[31] & 1u);

   fc::datastream<const char*> ds(serialized_public_key, sizeof(serialized_public_key));
   fc::raw::unpack(ds, key);

   return key;
}

std::set<TPM2_HANDLE> persistent_handles(esys_context& esys_ctx) {
   TPMI_YES_NO more_data;
   TSS2_RC rc;
   std::set<TPM2_HANDLE> handles;

   UINT32 prop = TPM2_PERSISTENT_FIRST;

   do {
      TPMS_CAPABILITY_DATA* capability_data = nullptr;
      rc = Esys_GetCapability(esys_ctx.ctx(), ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, TPM2_CAP_HANDLES, prop, TPM2_MAX_CAP_HANDLES, &more_data, &capability_data);
      FC_ASSERT(!rc, "Failed to query persistent handles: ${m}", ("m", Tss2_RC_Decode(rc)));
      auto cleanup_capability_data = fc::make_scoped_exit([&]() {free(capability_data);});

      FC_ASSERT(capability_data->capability == TPM2_CAP_HANDLES, "TPM returned non-handle reply");

      for(unsigned i = 0; i < capability_data->data.handles.count; ++i)
         if(capability_data->data.handles.handle[i] < TPM2_PLATFORM_PERSISTENT)
            handles.emplace(capability_data->data.handles.handle[i]);

      if(capability_data->data.handles.count)
         prop = capability_data->data.handles.handle[capability_data->data.handles.count-1] + 1;
   } while(more_data == TPM2_YES && prop < TPM2_PLATFORM_PERSISTENT);

   return handles;
}

std::map<fc::crypto::public_key, TPM2_HANDLE> usable_persistent_keys_and_handles(esys_context& esys_ctx) {
   std::map<fc::crypto::public_key, TPM2_HANDLE> ret;
   TSS2_RC rc;

   for(const TPM2_HANDLE& handle : persistent_handles(esys_ctx)) {
      ESYS_TR object;
      rc = Esys_TR_FromTPMPublic(esys_ctx.ctx(), handle, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, &object);
      if(rc) {
         wlog("Failed to load TPM persistent handle: ${m}", ("m", Tss2_RC_Decode(rc)));
         continue;
      }
      auto cleanup_tr_object = fc::make_scoped_exit([&]() {Esys_TR_Close(esys_ctx.ctx(), &object);});

      TPM2B_PUBLIC* pub = nullptr;
      auto cleanup_pub = fc::make_scoped_exit([&]() {free(pub);});

      rc = Esys_ReadPublic(esys_ctx.ctx(), object, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, &pub, NULL, NULL);
      if(rc)
         continue;
      if((pub->publicArea.objectAttributes & TPMA_OBJECT_SIGN_ENCRYPT) == 0)
         continue;
      if(pub->publicArea.objectAttributes & TPMA_OBJECT_RESTRICTED)
         continue;

      try {
         ret[tpm_pub_to_pub(pub)] = handle;
      } catch(...) {}
   }

   return ret;
}

struct tpm_key::impl {
   impl(const std::string& tcti, const fc::crypto::public_key& pubkey, const std::vector<unsigned>& pcrs) : pubkey(pubkey), esys_ctx(tcti), pcrs(pcrs) {}

   ~impl() {
      if(key_object != ESYS_TR_NONE)
         Esys_TR_Close(esys_ctx.ctx(), &key_object);
   }

   ESYS_TR key_object = ESYS_TR_NONE;
   fc::ec_key sslkey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
   fc::crypto::public_key pubkey;
   esys_context esys_ctx;
   std::vector<unsigned> pcrs;
};

tpm_key::tpm_key(const std::string& tcti, const fc::crypto::public_key& pubkey, const std::vector<unsigned>& pcrs) : my(tcti, pubkey, pcrs) {
   std::map<fc::crypto::public_key, TPM2_HANDLE> keys = usable_persistent_keys_and_handles(my->esys_ctx);
   FC_ASSERT(keys.find(pubkey) != keys.end(), "Unable to find persistent key ${k} in TPM via tcti ${t}", ("k", pubkey)("t", tcti));

   TSS2_RC rc = Esys_TR_FromTPMPublic(my->esys_ctx.ctx(), keys.find(pubkey)->second, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, &my->key_object);
   FC_ASSERT(!rc, "Failed to get handle to key ${k}: ${m}", ("k", pubkey)("m", Tss2_RC_Decode(rc)));
}

tpm_key::~tpm_key() = default;

static fc::crypto::signature tpm_signature_to_fc_signature(const fc::ec_key& sslkey, const fc::crypto::public_key& pub_key, const fc::sha256& digest, const TPMT_SIGNATURE* sig) {
   FC_ASSERT(sig->signature.ecdsa.signatureR.size == 32 && sig->signature.ecdsa.signatureS.size == 32, "Signature size from TPM not as expected");

   fc::ecdsa_sig sslsig = ECDSA_SIG_new();
   FC_ASSERT(sslsig.obj, "Failed to ECDSA_SIG_new");
   BIGNUM *r = BN_new(), *s = BN_new();
   FC_ASSERT(BN_bin2bn(sig->signature.ecdsa.signatureR.buffer,32,r) && BN_bin2bn(sig->signature.ecdsa.signatureS.buffer,32,s), "Failed to BN_bin2bn");
   FC_ASSERT(ECDSA_SIG_set0(sslsig, r, s), "Failed to ECDSA_SIG_set0");

   char serialized_signature[sizeof(fc::crypto::r1::compact_signature) + 1] = {fc::get_index<fc::crypto::signature::storage_type, fc::crypto::r1::signature_shim>()};

   fc::crypto::r1::compact_signature* compact_sig = (fc::crypto::r1::compact_signature*)(serialized_signature + 1);
   *compact_sig = fc::crypto::r1::signature_from_ecdsa(sslkey, std::get<fc::crypto::r1::public_key_shim>(pub_key._storage)._data, sslsig, digest);

   fc::crypto::signature final_signature;
   fc::datastream<const char*> ds(serialized_signature, sizeof(serialized_signature));
   fc::raw::unpack(ds, final_signature);
   return final_signature;
}

fc::crypto::signature tpm_key::sign(const fc::sha256& digest) {
   TPM2B_DIGEST d = {sizeof(fc::sha256)};
   memcpy(d.buffer, digest.data(), sizeof(fc::sha256));
   TPMT_SIG_SCHEME scheme = {TPM2_ALG_ECDSA};
   scheme.details.ecdsa.hashAlg = TPM2_ALG_SHA256;
   TPMT_TK_HASHCHECK validation = {TPM2_ST_HASHCHECK, TPM2_RH_NULL};

   std::optional<session_with_pcr_policy> session;
   if(my->pcrs.size())
      session.emplace(my->esys_ctx, my->pcrs);

   TPMT_SIGNATURE* sig;
   TSS2_RC rc = Esys_Sign(my->esys_ctx.ctx(), my->key_object, session ? session->session() : ESYS_TR_PASSWORD, ESYS_TR_NONE, ESYS_TR_NONE, &d, &scheme, &validation, &sig);
   FC_ASSERT(!rc, "Failed TPM sign on key ${k}: ${m}", ("k", my->pubkey)("m", Tss2_RC_Decode(rc)));
   auto cleanup_sig = fc::make_scoped_exit([&]() {free(sig);});

   return tpm_signature_to_fc_signature(my->sslkey, my->pubkey, digest, sig);
}

boost::container::flat_set<fc::crypto::public_key> get_all_persistent_keys(const std::string& tcti) {
   esys_context esys_ctx(tcti);

   boost::container::flat_set<fc::crypto::public_key> keys;
   boost::copy(usable_persistent_keys_and_handles(esys_ctx) | boost::adaptors::map_keys, std::inserter(keys, keys.end()));
   return keys;
}

attested_key create_key_attested(const std::string& tcti, const std::vector<unsigned>& pcrs, uint32_t certifying_key_handle) {
   esys_context esys_ctx(tcti);
   attested_key returned_key;

   TSS2_RC rc;
   const TPM2B_SENSITIVE_CREATE empty_sensitive_create = {};
   const TPM2B_DATA empty_data = {};
   const TPML_PCR_SELECTION empty_pcr_selection = {};

   ESYS_TR primary_handle;
   ESYS_TR created_handle;

   TPM2B_PUBLIC* created_pub = nullptr;
   TPM2B_DIGEST* creation_hash = nullptr;
   TPMT_TK_CREATION* creation_ticket = nullptr;
   auto cleanup_stuff = fc::make_scoped_exit([&]() {free(created_pub); free(creation_hash); free(creation_ticket);});

   TPM2B_PUBLIC key_creation_template = ecc_key_template;

   rc = Esys_CreatePrimary(esys_ctx.ctx(), ESYS_TR_RH_OWNER, ESYS_TR_PASSWORD, ESYS_TR_NONE, ESYS_TR_NONE,
                                   &empty_sensitive_create, &primary_template, &empty_data, &empty_pcr_selection,
                                   &primary_handle, NULL, NULL, NULL, NULL);
   FC_ASSERT(!rc, "Failed to create TPM primary key: ${m}", ("m", Tss2_RC_Decode(rc)));

   {
      auto cleanup_primary = fc::make_scoped_exit([&]() {Esys_FlushContext(esys_ctx.ctx(), primary_handle);});

      if(pcrs.size()) {
         session_with_pcr_policy trial_policy_session(esys_ctx, pcrs, true);
         key_creation_template.publicArea.authPolicy.size = sizeof(fc::sha256);
         memcpy(key_creation_template.publicArea.authPolicy.buffer, trial_policy_session.policy_digest().data(), sizeof(fc::sha256));
         key_creation_template.publicArea.objectAttributes &= ~TPMA_OBJECT_USERWITHAUTH;
      }

      TPM2B_PRIVATE* created_priv;
      rc = Esys_Create(esys_ctx.ctx(), primary_handle, ESYS_TR_PASSWORD, ESYS_TR_NONE, ESYS_TR_NONE, &empty_sensitive_create,
                       &key_creation_template, &empty_data, &empty_pcr_selection, &created_priv, &created_pub, NULL, &creation_hash, &creation_ticket);
      FC_ASSERT(!rc, "Failed to create key: ${m}", ("m", Tss2_RC_Decode(rc)));

      auto cleanup_created_priv = fc::make_scoped_exit([&]() {free(created_priv);});

      rc = Esys_Load(esys_ctx.ctx(), primary_handle, ESYS_TR_PASSWORD, ESYS_TR_NONE, ESYS_TR_NONE, created_priv, created_pub, &created_handle);
      FC_ASSERT(!rc, "Failed to load created key: ${m}", ("m", Tss2_RC_Decode(rc)));
   }

   auto cleanup_created_handle = fc::make_scoped_exit([&]() {Esys_FlushContext(esys_ctx.ctx(), created_handle);});

   returned_key.pub_key = tpm_pub_to_pub(created_pub);

   if(certifying_key_handle) {
      //The marshal code for TPM2B types always prepends a 2byte size header. Eliminate this from the returned vector as
      // it's duplicate information. Besides, hashes do not include this header.
      returned_key.public_area.data.resize(created_pub->size+sizeof(uint16_t));
      rc = Tss2_MU_TPM2B_PUBLIC_Marshal(created_pub, (uint8_t*)returned_key.public_area.data.data(), returned_key.public_area.data.size(), NULL);
      FC_ASSERT(!rc, "Failed to serialize created public area: ${m}", ("m", Tss2_RC_Decode(rc)));
      FC_ASSERT(returned_key.public_area.data.size() > 2, "Unexpected public area size");
      returned_key.public_area.data.erase(returned_key.public_area.data.begin(), returned_key.public_area.data.begin()+2);

      ESYS_TR certifying_key;
      rc = Esys_TR_FromTPMPublic(esys_ctx.ctx(), certifying_key_handle, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, &certifying_key);
      FC_ASSERT(!rc, "Failed to get handle to key performing attestation: ${m}", ("m", Tss2_RC_Decode(rc)));
      auto cleanup_certifying_object = fc::make_scoped_exit([&]() {Esys_TR_Close(esys_ctx.ctx(), &certifying_key);});

      TPM2B_PUBLIC* certifying_pub = nullptr;
      auto cleanup_pub = fc::make_scoped_exit([&]() {free(certifying_pub);});
      rc = Esys_ReadPublic(esys_ctx.ctx(), certifying_key, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, &certifying_pub, NULL, NULL);
      FC_ASSERT(!rc, "Failed to get information about key performing attestation: ${m}", ("m", Tss2_RC_Decode(rc)));
      fc::crypto::public_key certifying_public_key = tpm_pub_to_pub(certifying_pub);

      TPMT_SIG_SCHEME scheme = {TPM2_ALG_ECDSA};
      scheme.details.ecdsa.hashAlg = TPM2_ALG_SHA256;

      TPM2B_ATTEST* certification_info;
      TPMT_SIGNATURE* certification_signature;
      rc = Esys_CertifyCreation(esys_ctx.ctx(), certifying_key, created_handle, ESYS_TR_PASSWORD, ESYS_TR_NONE, ESYS_TR_NONE,
                                &empty_data, creation_hash, &scheme, creation_ticket, &certification_info, &certification_signature);
      FC_ASSERT(!rc, "Failed to attest: ${m}", ("m", Tss2_RC_Decode(rc)));
      auto cleanup_certification_objs = fc::make_scoped_exit([&]() {free(certification_info); free(certification_signature);});

      returned_key.creation_certification.data.resize(certification_info->size+sizeof(uint16_t));
      rc = Tss2_MU_TPM2B_ATTEST_Marshal(certification_info, (uint8_t*)returned_key.creation_certification.data.data(), returned_key.creation_certification.data.size(), NULL);
      FC_ASSERT(!rc, "Failed to serialize attestation: ${m}", ("m", Tss2_RC_Decode(rc)));
      FC_ASSERT(returned_key.creation_certification.data.size() > 2, "Unexpected public area size");
      returned_key.creation_certification.data.erase(returned_key.creation_certification.data.begin(), returned_key.creation_certification.data.begin()+2);


      returned_key.certification_signature = tpm_signature_to_fc_signature(EC_KEY_new_by_curve_name(NID_X9_62_prime256v1),
                                                                           certifying_public_key,
                                                                           fc::sha256::hash(returned_key.creation_certification.data.data(), returned_key.creation_certification.data.size()),
                                                                           certification_signature);
   }

   std::set<TPM2_HANDLE> currrent_persistent_handles = persistent_handles(esys_ctx);
   TPMI_DH_PERSISTENT persistent_handle_id = TPM2_PERSISTENT_FIRST;
   const TPMI_DH_PERSISTENT past_last_owner_persistent = TPM2_PLATFORM_PERSISTENT;
   for(; persistent_handle_id < past_last_owner_persistent; persistent_handle_id++)
      if(currrent_persistent_handles.find(persistent_handle_id) == currrent_persistent_handles.end())
         break;
   FC_ASSERT(persistent_handle_id != past_last_owner_persistent, "Couldn't find unused persistent handle");

   ESYS_TR persistent_handle;
   rc = Esys_EvictControl(esys_ctx.ctx(), ESYS_TR_RH_OWNER, created_handle, ESYS_TR_PASSWORD, ESYS_TR_NONE, ESYS_TR_NONE,
                          persistent_handle_id, &persistent_handle);
   FC_ASSERT(!rc, "Failed to persist TPM key: ${m}", ("m", Tss2_RC_Decode(rc)));
   Esys_TR_Close(esys_ctx.ctx(), &persistent_handle);

   return returned_key;
}

fc::crypto::public_key create_key(const std::string& tcti, const std::vector<unsigned>& pcrs) {
   return create_key_attested(tcti, pcrs, 0).pub_key;
}

fc::crypto::public_key verify_attestation(const attested_key& ak, const std::map<unsigned, fc::sha256>& pcr_policy) {
   TSS2_RC rc;
   TPM2B_PUBLIC public_area = {0};
   TPM2B_ATTEST attest = {0};
   TPMS_ATTEST tpms_attest = {0};

   std::vector<char> public_area_bytes = ak.public_area.data;
   public_area_bytes.insert(public_area_bytes.begin(), 2, 0);
   *((uint16_t*)public_area_bytes.data()) = htons(ak.public_area.data.size());
   rc = Tss2_MU_TPM2B_PUBLIC_Unmarshal((const uint8_t*)public_area_bytes.data(), public_area_bytes.size(), NULL, &public_area);
   FC_ASSERT(!rc, "Failed to deserialize public area: ${m}", ("m", Tss2_RC_Decode(rc)));

   std::vector<char> creation_certification_bytes = ak.creation_certification.data;
   creation_certification_bytes.insert(creation_certification_bytes.begin(), 2, 0);
   *((uint16_t*)creation_certification_bytes.data()) = htons(ak.creation_certification.data.size());
   rc = Tss2_MU_TPM2B_ATTEST_Unmarshal((const uint8_t*)creation_certification_bytes.data(), creation_certification_bytes.size(), NULL, &attest);
   FC_ASSERT(!rc, "Failed to deserialize attest structure: ${m}", ("m", Tss2_RC_Decode(rc)));

   rc = Tss2_MU_TPMS_ATTEST_Unmarshal(attest.attestationData, attest.size, NULL, &tpms_attest);
   FC_ASSERT(!rc, "Failed to deserialize tpms attest structure: ${m}", ("m", Tss2_RC_Decode(rc)));

   //ensure that the public key inside the public area matches the eos PUB_ key
   fc::crypto::public_key attested_key = tpm_pub_to_pub(&public_area);
   FC_ASSERT(ak.pub_key == attested_key, "Attested key ${a} does not match ${k} in json", ("a", attested_key)("k", ak.pub_key));

   //verify a few obvious things about the attest statement
   FC_ASSERT(tpms_attest.type == TPM2_ST_ATTEST_CREATION, "attestation is not a creation certification");
   FC_ASSERT(tpms_attest.attested.creation.objectName.size == sizeof(fc::sha256)+sizeof(uint16_t), "public name is not expected size");
   FC_ASSERT(tpms_attest.attested.creation.objectName.name[0] == 0x00 &&
             tpms_attest.attested.creation.objectName.name[1] == TPM2_ALG_SHA256, "public name isn't sha256 based");

   //ensure that the name (which is hash over public area) in the attest statement matches given name blob
   FC_ASSERT(*((fc::sha256*)(tpms_attest.attested.creation.objectName.name+sizeof(uint16_t))) ==
               fc::sha256::hash(ak.public_area.data.data(), ak.public_area.data.size()), "Public name hash does not match that in certification");

   const fc::sha256* const auth_policy_digest = (fc::sha256*)public_area.publicArea.authPolicy.buffer;

   //ensure the policy digest is expected if PCR values have been specified
   if(pcr_policy.size()) {
      fc::sha256::encoder pcr_hash_encoder;
      for(const auto& pcr_entry : pcr_policy)
         fc::raw::pack(pcr_hash_encoder, pcr_entry.second);

      fc::sha256::encoder policy_hash_encoder;
      fc::raw::pack(policy_hash_encoder, fc::sha256());
      fc::raw::pack(policy_hash_encoder, (uint32_t)htonl(TPM2_CC_PolicyPCR));

      std::vector<unsigned> pcrs;
      boost::copy(pcr_policy | boost::adaptors::map_keys, std::inserter(pcrs, pcrs.end()));
      TPML_PCR_SELECTION pcr_selection = pcr_selection_for_pcrs(pcrs);
      char buff[512];
      size_t off = 0;
      Tss2_MU_TPML_PCR_SELECTION_Marshal(&pcr_selection, (uint8_t*)buff, sizeof(buff), &off);
      policy_hash_encoder.write(buff, off);

      fc::raw::pack(policy_hash_encoder, pcr_hash_encoder.result());

      FC_ASSERT(policy_hash_encoder.result() == *auth_policy_digest, "policy digest not expected given specified PCR values");
   }
   else
      FC_ASSERT(fc::sha256() == *auth_policy_digest, "Key has non-zero policy when zero policy expected");

   const fc::sha256 attest_hash = fc::sha256::hash(ak.creation_certification.data.data(), ak.creation_certification.data.size());
   return fc::crypto::public_key(ak.certification_signature, attest_hash);
}

struct nv_data::impl {
   impl(const std::string& tcti, const std::vector<unsigned>& pcrs) : esys_ctx(tcti), pcrs(pcrs) {}

   ~impl() {
      if(nv_handle != ESYS_TR_NONE)
         Esys_TR_Close(esys_ctx.ctx(), &nv_handle);
   }

   ESYS_TR nv_handle = ESYS_TR_NONE;
   bool has_data;
   unsigned size;
   unsigned max_read_write_buff_size;

   esys_context esys_ctx;
   std::vector<unsigned> pcrs;

   static constexpr unsigned create_size = 2048;
};

nv_data::nv_data(const std::string& tcti, unsigned nv_index, const std::vector<unsigned>& pcrs) : my(tcti, pcrs) {
   TSS2_RC rc;

   rc = Esys_TR_FromTPMPublic(my->esys_ctx.ctx(), TPM2_HR_NV_INDEX + nv_index, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, &my->nv_handle);
   if(rc) {
      //let's try to create it...
      TPM2B_NV_PUBLIC nv_definition = {
         .nvPublic = {
            .nvIndex = TPM2_HR_NV_INDEX + nv_index,
            .nameAlg = TPM2_ALG_SHA256,
            .attributes = TPMA_NV_OWNERWRITE | TPMA_NV_OWNERREAD,
            .authPolicy = {},
            .dataSize = impl::create_size
         }
      };

      if(pcrs.size()) {
         session_with_pcr_policy trial_policy_session(my->esys_ctx, pcrs, true);
         nv_definition.nvPublic.authPolicy.size = sizeof(fc::sha256);
         memcpy(nv_definition.nvPublic.authPolicy.buffer, trial_policy_session.policy_digest().data(), sizeof(fc::sha256));
         nv_definition.nvPublic.attributes = TPMA_NV_POLICYWRITE | TPMA_NV_POLICYREAD;
      }

      const TPM2B_AUTH auth = {};
      rc = Esys_NV_DefineSpace(my->esys_ctx.ctx(), ESYS_TR_RH_OWNER, ESYS_TR_PASSWORD, ESYS_TR_NONE, ESYS_TR_NONE, &auth, &nv_definition, &my->nv_handle);
   }
   FC_ASSERT(!rc, "Failed to get esys handle to NVindex: ${m}", ("m", Tss2_RC_Decode(rc)));

   TPM2B_NV_PUBLIC* nvpub;
   rc = Esys_NV_ReadPublic(my->esys_ctx.ctx(), my->nv_handle, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, &nvpub, NULL);
   FC_ASSERT(!rc, "Failed to get NV public area: ${m}", ("m", Tss2_RC_Decode(rc)));
   auto cleanup_readpublic = fc::make_scoped_exit([&]() {free(nvpub);});

   my->has_data = nvpub->nvPublic.attributes & TPMA_NV_WRITTEN;
   my->size = nvpub->nvPublic.dataSize;

   TPMS_CAPABILITY_DATA* cap_data;
   rc = Esys_GetCapability(my->esys_ctx.ctx(), ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE,
                           TPM2_CAP_TPM_PROPERTIES, TPM2_PT_NV_BUFFER_MAX, 1, NULL, &cap_data);
   FC_ASSERT(!rc, "Failed to get max NV read/write size: ${m}", ("m", Tss2_RC_Decode(rc)));
   my->max_read_write_buff_size = cap_data->data.tpmProperties.tpmProperty[0].value;
   free(cap_data);
}

nv_data::~nv_data() = default;

std::optional<std::vector<char>> nv_data::data() {
   if(!my->has_data)
      return std::optional<std::vector<char>>();

   std::vector<char> ret(my->size);

   unsigned offset = 0;
   while(offset < ret.size()) {
      std::optional<session_with_pcr_policy> policy_session;
      if(my->pcrs.size())
         policy_session.emplace(my->esys_ctx, my->pcrs);

      unsigned thisiteration = std::min(my->size - offset, my->max_read_write_buff_size);
      TPM2B_MAX_NV_BUFFER* nv_contents;
      TSS2_RC rc = Esys_NV_Read(my->esys_ctx.ctx(), policy_session ? my->nv_handle : ESYS_TR_RH_OWNER, my->nv_handle, policy_session ? policy_session->session() : ESYS_TR_PASSWORD, ESYS_TR_NONE, ESYS_TR_NONE, thisiteration, offset, &nv_contents);
      FC_ASSERT(!rc, "Failed to read NV data: ${m}", ("m", Tss2_RC_Decode(rc)));
      memcpy(ret.data()+offset, nv_contents->buffer, thisiteration);
      free(nv_contents);
      offset += thisiteration;
   }

   return ret;
}

void nv_data::set_data(const std::vector<char>& data) {
   FC_ASSERT(data.size() <= my->size, "Setting NV data of size ${s} but NV data area max is {m}", ("s", data.size())("m", my->size));

   //pad it up to the public area defined size; makes it easier to know what to read back
   std::vector<char> padded_data = data;
   padded_data.resize(my->size);

   unsigned offset = 0;
   while(offset < padded_data.size()) {
      std::optional<session_with_pcr_policy> policy_session;
      if(my->pcrs.size())
         policy_session.emplace(my->esys_ctx, my->pcrs);

      unsigned thisiteration = std::min(my->size - offset, my->max_read_write_buff_size);
      TPM2B_MAX_NV_BUFFER nv_contents_write = {(uint16_t)thisiteration};
      memcpy(nv_contents_write.buffer, padded_data.data()+offset, thisiteration);
      TSS2_RC rc = Esys_NV_Write(my->esys_ctx.ctx(), policy_session ? my->nv_handle : ESYS_TR_RH_OWNER, my->nv_handle, policy_session ? policy_session->session() : ESYS_TR_PASSWORD, ESYS_TR_NONE, ESYS_TR_NONE, &nv_contents_write, offset);
      FC_ASSERT(!rc, "Failed to write NV data: ${m}", ("m", Tss2_RC_Decode(rc)));
      offset += thisiteration;
   }

   my->has_data = true;
}

struct swtpm::impl {
   uint16_t port = 2222;
   uint16_t ctl_port;

   fc::temp_directory state_dir;
};

swtpm::swtpm() {
   //try a few times to find an available tcp port pair
   std::random_device rd;
   std::mt19937 gen(rd());
   std::uniform_int_distribution dist(2000, 8000);

   for(unsigned i = 0; i < 5; ++i) {
      my->port = dist(gen);
      my->ctl_port = my->port + 1;

      std::stringstream ss;
      ss << "swtpm socket -p " << std::to_string(my->port) << " --tpm2 --tpmstate dir=" << my->state_dir.path().generic_string()
         << " --ctrl type=tcp,port=" << std::to_string(my->ctl_port) << " --flags startup-clear -d";

      if(boost::process::system(ss.str()) == 0)
         return;
   }

   FC_ASSERT(false, "Failed to start swtpm");
}

swtpm::~swtpm() {
   std::stringstream ss;
   ss << "swtpm_ioctl -s --tcp :" << std::to_string(my->ctl_port);
   boost::process::system(ss.str());
}

std::string swtpm::tcti() const {
   std::stringstream ss;
   ss << "swtpm:port=" << std::to_string(my->port);
   return ss.str();
}

}