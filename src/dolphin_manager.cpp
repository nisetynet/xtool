#include <dme/DolphinProcess/DolphinAccessor.h>
#include <dolphin_manager.hpp>

DolphinManager::DolphinManager() { m_dolphin.init(); }

DolphinComm::DolphinAccessor const &DolphinManager::dolphin() const {
  this->hook_dolphin_if_not_hooked();
  return m_dolphin;
}

void DolphinManager::hook_dolphin_if_not_hooked() const {
  auto const status = m_dolphin.getStatus();
  if (status != DolphinComm::DolphinStatus::hooked) {
    m_dolphin.hook();
  }
}
