/**
 *  @file
 *  @copyright defined in LICENSE.txt; Parts of this file Copyright (c) 2009 The Go Authors
 */
#include <fc/network/platform_root_ca.hpp>
#include <fc/exception/exception.hpp>

#include <string>

#include <boost/container/flat_set.hpp>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#endif

namespace fc {

#if defined(__APPLE__)
static void add_macos_root_cas(boost::asio::ssl::context& ctx) {
   boost::container::flat_set<std::string> trusted_certs;
   boost::container::flat_set<std::string> untrusted_certs;

   SecTrustSettingsDomain domains[] = { kSecTrustSettingsDomainSystem,
                                        kSecTrustSettingsDomainAdmin,
                                        kSecTrustSettingsDomainUser };

   unsigned int number_domains = sizeof(domains)/sizeof(domains[0]);

   for (unsigned int i = 0; i < number_domains; i++) {
      CFArrayRef certs;
      OSStatus err = SecTrustSettingsCopyCertificates(domains[i], &certs);
      if(err != noErr)
         continue;

      for(CFIndex j = 0; j < CFArrayGetCount(certs); ++j) {
         CFArrayRef trustSettings = nullptr;
         SecCertificateRef cert = (SecCertificateRef)CFArrayGetValueAtIndex(certs, j);
         if(cert == nullptr)
            continue;

         bool untrusted{false}, trust_as_root{i == 0}, trust_root{false};
         if(i != 0) {
            for (unsigned int k = i; k < number_domains; k++) {
               CFArrayRef domainTrustSettings = nullptr;
               err = SecTrustSettingsCopyTrustSettings(cert, domains[k], &domainTrustSettings);
               if (err == errSecSuccess && domainTrustSettings != nullptr) {
                  if(trustSettings)
                     CFRelease(trustSettings);
                  trustSettings = domainTrustSettings;
               }
            }
            if(trustSettings == nullptr)
               continue;
            if(CFArrayGetCount(trustSettings) == 0)
               trust_as_root = true;
            else for(CFIndex k = 0; k < CFArrayGetCount(trustSettings); k++) {
               CFNumberRef cfNum;
               CFDictionaryRef tSetting = (CFDictionaryRef)CFArrayGetValueAtIndex(trustSettings, k);
               if(CFDictionaryGetValueIfPresent(tSetting, kSecTrustSettingsResult, (const void**)&cfNum)){
                  SInt32 result = 0;
                  CFNumberGetValue(cfNum, kCFNumberSInt32Type, &result);
                  if(result == kSecTrustSettingsResultDeny)
							untrusted = true;
						else if (result == kSecTrustSettingsResultTrustAsRoot)
							trust_as_root = true;
						else if (result == kSecTrustSettingsResultTrustRoot)
							trust_root = true;
					}
				}
            CFRelease(trustSettings);
         }

         //double check that these manually trusted ones are actually CAs
         if(trust_root) {
            CFErrorRef errRef = nullptr;
            CFDataRef subjectName = SecCertificateCopyNormalizedSubjectContent(cert, &errRef);
            if(errRef != nullptr) {
               CFRelease(errRef);
               continue;
            }
            CFDataRef issuerName = SecCertificateCopyNormalizedIssuerContent(cert, &errRef);
            if(errRef != nullptr) {
               CFRelease(subjectName);
               CFRelease(errRef);
               continue;
            }
            Boolean equal = CFEqual(subjectName, issuerName);
            CFRelease(subjectName);
            CFRelease(issuerName);
            if(!equal)
               continue;
         }

         CFDataRef certAsPEM;
         err = SecKeychainItemExport(cert, kSecFormatX509Cert, kSecItemPemArmour, nullptr, &certAsPEM);
         if(err != noErr)
            continue;
         if(certAsPEM) {
            if(!trust_root && !trust_as_root)
               untrusted_certs.emplace((const char*)CFDataGetBytePtr(certAsPEM), CFDataGetLength(certAsPEM));
            else
               trusted_certs.emplace((const char*)CFDataGetBytePtr(certAsPEM), CFDataGetLength(certAsPEM));
            CFRelease(certAsPEM);
         }
      }
      CFRelease(certs);
   }
   for(const auto& untrusted : untrusted_certs)
      trusted_certs.erase(untrusted);
   boost::system::error_code dummy;
   for(const auto& trusted : trusted_certs)
      ctx.add_certificate_authority(boost::asio::const_buffer(trusted.data(), trusted.size()), dummy);
}
#endif

void add_platform_root_cas_to_context(boost::asio::ssl::context& ctx) {
#if defined( __APPLE__ )
      add_macos_root_cas(ctx);
#elif defined( _WIN32 )
      FC_THROW("HTTPS on Windows not supported");
#else
      ctx.set_default_verify_paths();
#endif
}
}