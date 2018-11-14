#ifndef BMS_PRINT_H
#define BMS_PRINT_H

#include <SoftwareSerial.h>
#include <Arduino.h>
#include <stdint.h>
#include <SPI.h>

#define TOTAL_IC          8

class BmsPrint {
public:
    BmsPrint();
    ~BmsPrint();
    void print_txdata(uint8_t tx_cfg[][6]);
    void print_rxdata(uint8_t rx_cfg[][8]);
    void serial_print_hex(uint8_t data);
private:
};

#endif /* BMS_PRINT_H */
