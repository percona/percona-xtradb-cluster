#ifndef __WSREP_MASTER_KEY_MANAGER__
#define __WSREP_MASTER_KEY_MANAGER__

#include <functional>
#include <string>

#include "mysql/components/my_service.h"
#include "mysql/components/services/registry.h"

#include <mysql/components/services/keyring_generator.h>
#include <mysql/components/services/keyring_reader_with_status.h>
#include <mysql/components/services/keyring_writer.h>
#include <mysql/service_plugin_registry.h>

class MasterKeyManager {
 public:
  MasterKeyManager(std::function<void(const std::string &)> logger);
  bool Init();
  void DeInit();

  bool GenerateKey(const std::string &keyId);
  std::string GetKey(const std::string &keyId);

 private:
  SERVICE_TYPE(keyring_reader_with_status) * keyring_reader_service_{nullptr};
  SERVICE_TYPE(keyring_writer) * keyring_writer_service_{nullptr};
  SERVICE_TYPE(keyring_generator) * keyring_generator_service_{nullptr};

  std::function<void(const std::string &)> logger_;
};

#endif