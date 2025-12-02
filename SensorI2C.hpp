#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <iomanip>

#define REG_LINE_LENGTH 0x06
#define REG_TINT_LL 0x0B
#define REG_TINT_CK 0x0C
#define REG_ANA_GAIN 0x0D
#define REG_DIG_GAIN 0x0E
#define REG_FRAME_PERIOD 0x07
#define REG_FB_CHIP_ID 0x7F
#define REG_FB_STATE 0x53
#define REG_FB_LINE_LENGTH 0x52
#define REG_FB_ERROR 0x54
#define REG_FB_TINT 0x55
#define REG_FB_SPY_TINT_LL 0x57
#define REG_FB_SPY_TINT_CK 0x58
#define REG_FB_FRAME_PERIOD 0x56

#define SYSFS_PATH_REG  "/sys/bus/i2c/drivers/topaz/3-0010/reg"
#define SYSFS_PATH_VAL  "/sys/bus/i2c/drivers/topaz/3-0010/val"

class ModuleCtrl
{
private:
    /**
     * @brief 
     * Device node
     * 
     */
    // int fd;
    int fd_reg;
    int fd_val;

public:
    /**
     * @brief 
     * Initialise IC2 connection
     */
    void ModuleControlInit();

    ~ModuleCtrl();

    /**
     * @brief 
     * Close IC2 connection
     * 
     */
    void ModuleControlClose();

    /**
     * @brief write specified value in the specified register
     * 
     * @param register address 
     * @param value to write
     * @return int 
     */
    int writeReg(int regAddr, int value);

    /**
     * @brief 
     * Read specified register
     * 
     * @param register address 
     * @param pointer to store the read value
     * @return int 
     */
    int readReg(int regAddr, int *value);

    /**
     * @brief Read the state of the sensor
     * 
     * @return int The sensor state value
     */
    int read_sensor_state(int *state);

    /**
     * @brief Set the exposition time
     * 
     * @param tint in milisecond 
     * @return int 
     */
    int setTint(float tint);

    /**
     * @brief Set the Analog Gain
     * 
     * @param again 
     * @return int 
     */
    int setAnalogGain(float again);

    /**
     * @brief Set the Digital Gain
     * 
     * @param dgain 
     * @return int 
     */
    int setDigitalGain(float dgain);
    
     /**
     * @brief Set the Gain
     * 
     * @param gain 
     * @return int 
     */   
    int setGain(float gain);
    
    /**
     * @brief Set the period time for frame rate control
     * 
     * @param tperiod in milisecond 
     * @return int 
     */
    int setFramePeriod(float tperiod);

    /**
     * @brief 
     * Creates dump state file
     * 
     * @param moduleCtrl i2c device 
     * @param filename name of the txt file for the dump result
     * @return int 
     */
    // int createDump(ModuleCtrl *moduleCtrl, const std::string& filename);
    int createDump(const std::string& filename);


    int read_sensor_feedback();
    
};

struct solution
{
    int tintLL;
    int tintCK;
};