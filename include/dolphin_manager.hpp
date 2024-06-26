#pragma once
#include <dme/DolphinProcess/DolphinAccessor.h>

class DolphinManager {
public:
  DolphinManager();

  DolphinComm::DolphinAccessor const &dolphin() const;

private:
  void hook_dolphin_if_not_hooked() const;

private:
  DolphinComm::DolphinAccessor m_dolphin;
};