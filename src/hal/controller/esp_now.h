#pragma once
#include <vector>

#include "lib/worker.h"

namespace toaster {

class ESPNowRemote : public Worker {
public:
  ESPNowRemote();

public:
  virtual bool beginLocal();
  virtual bool beginI2C(uint8_t addr);

public:
  void clearWhitelist();
  void addWhitelist(const uint8_t* mac);
  bool checkWhitelist(const uint8_t* mac) const;

protected:
  void onReceive(const uint8_t* mac, const uint8_t* incomingData, int len);

protected:
  virtual bool work();

protected:
  bool _init{false};
  uint8_t _i2c_addr{0};
  bool _i2c_found{false};
  uint8_t _recent_data{0};
  uint8_t _recent_msgid{0};
  timer_ms_t _recent_tick_ms{0};

protected:
  typedef struct _MAC_ADDRESS {
    uint8_t mac[6];
  } MAC_ADDRESS;
  std::vector<MAC_ADDRESS> _whitelist;

protected:
  static uint8_t calcChecksum(const uint8_t* data, size_t len);

protected:
  uint8_t commReadLength();
  void commReadData(uint8_t len);
};

};  // namespace toaster
