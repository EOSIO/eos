#include <eos/wallet_plugin/wallet_plugin.hpp>

namespace fc { class variant; }

namespace eos {

class wallet_plugin_impl {
public:
private:
};

wallet_plugin::wallet_plugin()
  : my(new wallet_plugin_impl()) {
}


} // namespace eos
