# 1 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino"
# 1 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino"
/**

 * BMS.ino 

 * Temperature Calculations and ADC for the BMS

 * Authors: Alex Sternberg

 *          Thinh Le

 * Property of Spartan Racing Electric 2018

 */
# 9 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino"
/* GENERAL LIBRARIES */
# 11 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino" 2
# 12 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino" 2

# 14 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino" 2
/* PROJECT LIBRARIES */
# 16 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino" 2
# 17 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino" 2
# 18 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino" 2
# 19 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino" 2
# 20 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino" 2
# 21 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino" 2
/* PROJECT HEADERS */
# 23 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino" 2
/* CONSTANTS */







/* DEBUG */




float voltages[8][12];
float current;
float resistance[8][9];

void setup() {
  Serial.begin(115200);
  pinMode(6, 0x1);
  digitalWrite(6, 0x1); //setup fault

  // spi_enable(SPI_CLOCK_DIV16);
  // set_adc(MD_NORMAL, DCP_DISABLED, CELL_CH_ALL, AUX_CH_ALL);
  LTC6804_initialize(); //Initialize LTC6804 hardware

  init_cfg(); //initialize the 6804 configuration array to be written

  BmsPrint bp;
  // so you can build a binary without logging statements
  // since printing is the slowest operation one can do on a mcu
  // hence compiler dierctives to tell it to not build those lines in
  if (mcp2515_init(1 /* CAN speed at 500 kbps*/)) {

    Serial.println("CAN init OK");

  }

  else {
    Serial.println("CAN init FAIL");
  }

}

// Scope for void loop
int8_t read_cells(float cellv[8][12]) {
  //allocate temp measure memory 
  uint16_t cell_codes[8][12];
  // Wake isoSPI up from idle state
  wakeup_idle();
  //command cell measurement
  LTC6804_adcv();
  //read measured voltages
  int8_t error = LTC6804_rdcv(0, 12, cell_codes);
  if (error) {
    set_fault();
    Serial.println("Error!");
  }
  // Counter to see which line code steps into/over
  // int debug_count = 0;
  for (int current_ic = 0; current_ic < 8; current_ic++) {
    for (int i = 0; i < 12; i++) {
      // Serial.print("Iteration count: ");
      // Serial.print(current_ic);
      // Serial.println(i);

      //convert fixed point to floating point
      float voltage = cell_codes[current_ic][i] * 0.0001;
      // Fill in channels 10, 11, 12 with empty
      if(i > 9 /* Supposed to be 9*/)
      {
        cellv[current_ic][i] = 0;
      }
      cellv[current_ic][i] = voltage; // <- The issue

      // Serial.println(cellv[current_ic][i]);

      // For 20V to BMB, 2.35 for 35V
      if (voltage < 2.22 || voltage > 4.35) {
        //voltage out of range
        set_fault();
        Serial.println("Out of range");
      }
      if (!(i % 5)) i++; // If not 5th iterstion continue
      if (i == 11) i++; // Skip channel 11
    }
  }
}

void set_fault() {
  // Set pin 6 (CS) active low, BITCLR
  digitalWrite(6, 0x0); // Sent to BMS Master Fault pin 
}

float read_current(uint8_t sensor) {
  // Definition in wiring_analog.c 
  int measure = analogRead(sensor);
  return 200 * (measure / 820);
}

// Our execution of functions
void loop() {
  char input;
  // 1. Initiate state machines
  // 2. Verify hardware for SCK, CSB, ISOMD, WDT, DRIVE 
  // 3. Get data transmission going 
  // 4. Read cell voltages
  // Serial.println("Test");
  uint16_t cells[8][12];
  uint16_t aux[8][6];
  uint8_t * pec_data; // Store pec calcs here
  wakeup_sleep(); // Sleep
  Serial.print(0);
  // BMS Declaration and Constructor
  // BatteryManagementSystem* bms = BatteryManagementSystem_new();  
  do
  {
    // input = Serial.read();
    LTC6804_initialize(); // Standby 
    // Serial.println(1); // debug #1
    LTC6804_adcv(); // Measure 
    // Serial.println(2); // debug #2    
    // Serial.println("cell voltage");
    LTC6804_rdcv(0, 8, cells); // Reads all IC registers and stores into 2D matrix
    for(int current_ic = 0; current_ic < 8; current_ic++) {
      for(int current_cell = 0; current_cell < 8; current_cell++) {
        if(current_cell == 6)
        {
          uint16_t pec_temp = pec15_calc(2, pec_data);
          cells[current_ic][current_cell] = pec_temp;
          cells[current_ic][current_cell++] = (pec_temp << 8);
        }
        // Serial.println(cells[current_ic][current_cell]);
      }
    }
    LTC6804_clrcell();
    // Serial.println(3); // debug #3
    // Serial.println("aux registers");
    LTC6804_rdaux(0, 8, aux); // Reads all IC GPIO and stores into 2D matrix
    for(int current_ic = 0; current_ic < 8; current_ic++) {
      for(int current_cell = 0; current_cell < 6; current_cell++) {
        // Serial.println(aux[current_ic][current_cell]);
      }
    }
    LTC6804_clraux();
    // Serial.println(4); // debug #4
    // TODO: Write Configuration A for REFON 
    uint8_t wrcfga[8][6] = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
                                   {0x00, 0x00, 0x00, 0x00, 0x01, 0x00},
                                   {0x00, 0x00, 0x00, 0x01, 0x00, 0x00},
                                   {0x00, 0x00, 0x01, 0x00, 0x00, 0x00},
                                   {0x00, 0x01, 0x00, 0x00, 0x00, 0x00},
                                   {0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
                                   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    uint8_t read_cells[8][6];
    uint8_t rx_cfg1[8][8] = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}; // test with print rx                                   
    uint8_t rx_cfg2[8][8] = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}; // test with print rx                                   
    // Serial.println("before write");
    LTC6804_rdcfg(8, rx_cfg1);
    // bp.print_rxdata(rx_cfg1);
    // Serial.println("write");
    LTC6804_wrcfg(8, wrcfga);
    // bp.print_txdata(wrcfga); // REFUP
    // Serial.println("after write");
    LTC6804_rdcfg(8, rx_cfg2);
    // bp.print_rxdata(rx_cfg2);


    read_cells(bms->voltages); // TODO        
    // read_current(current);
    // test 
    bp.print_txdata(bms->tx_cfg);
    bp.print_rxdata(bms->rx_cfg);



    /* Test */
    uint8_t *store_data;
    int size = 0;
    //This will guarantee that the LTC6804 isoSPI port is awake.
    wakeup_sleep(); // TODO 
    bms->wakeup_sleep = 1;
    // // WRCFGA: 00000000001
    // LTC6804_wrcfg(TOTAL_IC, bms->tx_cfg);
    // Read register A voltage 
    LTC6804_rdcv_reg(1, 8, store_data);
    size = sizeof(store_data) / sizeof(uint8_t);
    for(int i = 0; i < size; i++)
    {
      if(store_data[i] != __null)
      {
        Serial.println(store_data[i]);
      }
      else{
        Serial.println("NULL Terminated");
      }
    }

  } while(input != 'q');
}


/*!***********************************

\brief Initializes the configuration array

**************************************/
# 243 "c:\\Users\\Timothy_Le\\Documents\\SRE\\src\\BMS_\\BMS\\BMS\\BMS.ino"
void init_cfg()
{
  uint8_t tx_cfg[8][6] = {{0xF8, 0x19, 0x16, 0xA4, 0x00, 0x00},
                                 {0xF8, 0x19, 0x16, 0xA4, 0x00, 0x00},
                                 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

  LTC6804_wrcfg(8, tx_cfg);
}
