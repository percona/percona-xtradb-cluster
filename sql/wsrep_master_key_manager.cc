#include "wsrep_master_key_manager.h"
#include <scope_guard.h>
#include <string.h>

constexpr char key_type[] = "AES";
static constexpr size_t KEY_LEN = 32;

MasterKeyManager::MasterKeyManager(
    std::function<void(const std::string &)> logger)
    : logger_(logger) {}

bool MasterKeyManager::Init() {
  SERVICE_TYPE(registry) *reg_svc = mysql_plugin_registry_acquire();

  if (reg_svc == nullptr) {
    logger_("Failed to get plugin registry");
    return true;
  }

  my_h_service h_keyring_reader_service = nullptr;
  my_h_service h_keyring_writer_service = nullptr;
  my_h_service h_keyring_generator_service = nullptr;

  auto cleanup = [&]() {
    if (h_keyring_reader_service) {
      reg_svc->release(h_keyring_reader_service);
    }
    if (h_keyring_writer_service) {
      reg_svc->release(h_keyring_writer_service);
    }
    if (h_keyring_generator_service) {
      reg_svc->release(h_keyring_generator_service);
    }
    mysql_plugin_registry_release(reg_svc);

    keyring_reader_service_ = nullptr;
    keyring_writer_service_ = nullptr;
    keyring_generator_service_ = nullptr;

    logger_("Failed initializing keyring services");
  };

  if (reg_svc->acquire("keyring_reader_with_status",
                       &h_keyring_reader_service) ||
      reg_svc->acquire_related("keyring_writer", h_keyring_reader_service,
                               &h_keyring_writer_service) ||
      reg_svc->acquire_related("keyring_generator", h_keyring_reader_service,
                               &h_keyring_generator_service)) {
    cleanup();
    return true;
  }

  keyring_reader_service_ =
      reinterpret_cast<SERVICE_TYPE(keyring_reader_with_status) *>(
          h_keyring_reader_service);
  keyring_writer_service_ = reinterpret_cast<SERVICE_TYPE(keyring_writer) *>(
      h_keyring_writer_service);
  keyring_generator_service_ =
      reinterpret_cast<SERVICE_TYPE(keyring_generator) *>(
          h_keyring_generator_service);

  mysql_plugin_registry_release(reg_svc);
  return false;
}

void MasterKeyManager::DeInit() {
  SERVICE_TYPE(registry) *reg_svc = mysql_plugin_registry_acquire();

  if (reg_svc == nullptr) {
    logger_("Failed to get plugin registry");
    return;
  }

  using keyring_reader_t = SERVICE_TYPE_NO_CONST(keyring_reader_with_status);
  using keyring_writer_t = SERVICE_TYPE_NO_CONST(keyring_writer);
  using keyring_generator_t = SERVICE_TYPE_NO_CONST(keyring_generator);

  if (keyring_reader_service_) {
    reg_svc->release(reinterpret_cast<my_h_service>(
        const_cast<keyring_reader_t *>(keyring_reader_service_)));
  }
  if (keyring_writer_service_) {
    reg_svc->release(reinterpret_cast<my_h_service>(
        const_cast<keyring_writer_t *>(keyring_writer_service_)));
  }
  if (keyring_generator_service_) {
    reg_svc->release(reinterpret_cast<my_h_service>(
        const_cast<keyring_generator_t *>(keyring_generator_service_)));
  }

  keyring_reader_service_ = nullptr;
  keyring_writer_service_ = nullptr;
  keyring_generator_service_ = nullptr;

  mysql_plugin_registry_release(reg_svc);
}

bool MasterKeyManager::GenerateKey(const std::string &keyId) {
  if (keyring_generator_service_->generate(keyId.c_str(), nullptr, key_type,
                                           KEY_LEN) == true) {
    logger_(
        "MasterKey generation FAILED. Check if Keyring component "
        "is installed.");
    return true;
  }
  return false;
}

std::string MasterKeyManager::GetKey(const std::string &keyId) {
  size_t secret_length = 0;
  size_t secret_type_length = 0;
  my_h_keyring_reader_object reader_object = nullptr;
  bool retval =
      keyring_reader_service_->init(keyId.c_str(), nullptr, &reader_object);

  /* Keyring error */
  if (retval == true) {
    logger_("Failed to initialize keyring reader service");
    return std::string();
  }
  /* Key absent */
  if (reader_object == nullptr) return std::string();

  auto cleanup_guard = create_scope_guard([&] {
    if (reader_object != nullptr)
      (void)keyring_reader_service_->deinit(reader_object);
    reader_object = nullptr;
  });

  /* Fetch length */
  if (keyring_reader_service_->fetch_length(reader_object, &secret_length,
                                            &secret_type_length) == true) {
    logger_("Failed to fetch secret length from keyring reader service");
    return std::string();
  }

  if (secret_length == 0 || secret_type_length == 0) return std::string();

  /* Allocate requried memory for key and secret_type */
  std::vector<unsigned char> secret_v;
  std::vector<char> secret_type_v;
  secret_v.reserve(secret_length);
  secret_type_v.reserve(secret_type_length + 1);
  memset(secret_v.data(), 0, secret_length);
  memset(secret_type_v.data(), 0, secret_type_length + 1);

  if (keyring_reader_service_->fetch(reader_object, secret_v.data(),
                                     secret_length, &secret_length,
                                     secret_type_v.data(), secret_type_length,
                                     &secret_type_length) == true) {
    logger_("Failed to fetch secret from keyring reader service");
    return std::string();
  }

  return std::string((char *)(secret_v.data()), secret_length);
}
