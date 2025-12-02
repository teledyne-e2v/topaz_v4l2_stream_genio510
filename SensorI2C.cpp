#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "SensorI2C.hpp"


#define DEBUG 0

/************************************************
 *Main fuction
 ************************************************/
ModuleCtrl::~ModuleCtrl()
{
#ifndef DEBUG_MODE
	ModuleControlClose(); // close IC2
#endif
}

/************************************************
 *Init I2C bus
 ************************************************/
void ModuleCtrl::ModuleControlInit()
{

	/* Open device node */
	fd_reg = open(SYSFS_PATH_REG, O_RDWR);
	if(fd_reg < 0)
	{
		fprintf(stderr, "SYSFS_PATH_REG open error\n");
	}
    std::cout << "open SYSFS_PATH_REG" << std::endl;

    fd_val = open(SYSFS_PATH_VAL, O_RDWR);
	if(fd_val < 0)
	{
		fprintf(stderr, "SYSFS_PATH_VAL open error\n");
	}
    std::cout << "open SYSFS_PATH_VAL" << std::endl;
    
}

/************************************************
 *Close I2C bus
 ************************************************/
void ModuleCtrl::ModuleControlClose()
{

	/* Close device node */
    close(fd_reg);
	close(fd_val);

	std::cout << "close SYSFS_PATH" << std::endl;

}

/************************************************
 *Read register
 ************************************************/
int ModuleCtrl::readReg(int regAddr, int *value)
{

	char buf[50] = {0};
	char read_buf[50] = {0};
	int ret;
	int error=0;
	unsigned int read_addr;
	unsigned int read_data;	
	
	// sprintf(buf, "%d 0x%02hx", E2VTOPAZ_READ_MODE, regAddr);

    sprintf(buf, "0x%04hx", regAddr);

	ret = write(fd_reg, buf, strlen(buf)+1);
	if(DEBUG==1) std::cout << "SensorI2C: return write: " << ret << std::endl;
	ret = lseek(fd_reg, 0, SEEK_SET);
	if(DEBUG==1) std::cout << "SensorI2C: return lseek: " << ret << std::endl;
	//write() change the fd pos, need use lseek to make pos point to the start of the file

	ret = read(fd_val, read_buf, sizeof(read_buf));
	if(DEBUG==1) std::cout << "SensorI2C: return read: " << ret << " buf: " << read_buf << std::endl;
	ret = lseek(fd_val, 0, SEEK_SET);
	if(DEBUG==1) std::cout << "SensorI2C: return lseek: " << ret << std::endl;
	//write() change the fd pos, need use lseek to make pos point to the start of the file

	if (sscanf(read_buf, "0x%04x", &read_data) == 1)
	{
		if(DEBUG==1) std::cout << "SensorI2C: read_addr= " << regAddr << "/ read_data= " <<  read_data << std::endl;
		*value=read_data;
	}
	else
	{
		if(DEBUG==1) std::cout << "SensorI2C: Failed to parse the read_buf: " << read_buf << std::endl;
		error=2;
	}
	

	return error;
}

/************************************************
 *Write register
 ************************************************/
int ModuleCtrl::writeReg(int regAddr, int value) //TODO: make this function
{

	int ret;
	char buf[30] = {0};
	int error=0;
	
	if(DEBUG==1) printf("reg_addr: 0x%x, reg_data: 0x%x\n", regAddr, value);

	//sprintf(buf, "%d 0x%02hx 0x%04hx", E2VTOPAZ_WRITE_MODE, regAddr, value);
	if(DEBUG==1) printf("buf val: %s\n", buf);

	// ret = write(fd, buf, strlen(buf)+1);
	if(DEBUG==1) printf("return write: %d\n", ret);
	
	return error;
}

/************************************************
 *Get the sensor state
 ************************************************/
int ModuleCtrl::read_sensor_state(int *state) //TODO: testing
{
	int regAddr;
	int value=0;
	int error=0;

	regAddr = REG_FB_STATE;

	error=this->readReg(regAddr, &value);
	*state=value;

	return error;
}

/************************************************
 *Set the integration time (ms)
 ************************************************/
int ModuleCtrl::setTint(float tint) //TODO: testing
{
	int regAddr, tline;
	struct solution init;
	int error=0;

	tint = tint * 1000000; // convert to nanoseconds
	tint = tint / 20; // convert to clock cycle 50MHz

	// read the line length
	regAddr = REG_LINE_LENGTH;
	error=this->readReg(regAddr, &tline);
	
	if (error != 0)
	{
		fprintf(stderr, "READ ERROR: 0x%x\n", regAddr);
		error=-1;
		return error;
	}
	
	init.tintLL = (int)tint / tline;
	init.tintCK = (int)(tint - init.tintLL * tline);

	// write in reg_tint_ll
	regAddr = REG_TINT_LL;
	error=this->writeReg(regAddr, init.tintLL);
	if (error != 0)
	{
		fprintf(stderr, "WRITE ERROR: 0x%x\n", regAddr);
		error=-2;
		return error;
	}

	// write in reg_tint_ck
	regAddr = REG_TINT_CK;
	error=this->writeReg(regAddr, init.tintCK);
	if (error != 0)
	{
		fprintf(stderr, "WRITE ERROR: 0x%x\n", regAddr);
		error=-3;
		return error;
	}

	return error;
}

/************************************************
 *Set the analog gain
 ************************************************/
int ModuleCtrl::setAnalogGain(float again) //TODO: testing
{
	int regAddr, value;
	int error=0;

	value = (int)(again * 100);

	switch (value)
	{
	case 100:
		value = 0;
		break;
	case 120:
		value = 1;
		break;
	case 145:
		value = 2;
		break;
	case 150:
		value = 2;
		break;
	case 171:
		value = 3;
		break;
	case 200:
		value = 4;
		break;
	case 240:
		value = 5;
		break;
	case 300:
		value = 6;
		break;
	case 343:
		value = 7;
		break;
	case 400:
		value = 8;
		break;
	case 480:
		value = 9;
		break;
	case 500:
		value = 9;
		break;
	case 600:
		value = 10;
		break;
	case 686:
		value = 11;
		break;
	case 700:
		value = 11;
		break;
	case 800:
		value = 12;
		break;
	case 900:
		value = 13;
		break;
	case 960:
		value = 13;
		break;
	case 1000:
		value = 13;
		break;
	case 1200:
		value = 14;
		break;
	case 1600:
		value = 15;
		break;
	default:
		printf("Forbidden value, couldn't set analog gain\n");
		return 0;
	}

	// write in register
	regAddr = REG_ANA_GAIN;
	error=this->writeReg(regAddr, value);
	if (error != 0)
	{
		fprintf(stderr, "WRITE ERROR: 0x%x\n", regAddr);
		error=-1;
		return error;
	}

	return 0;
}

/************************************************
 *Set the digital gain
 ************************************************/
int ModuleCtrl::setDigitalGain(float dgain) //TODO: testing
{
	int regAddr, value;
	int error=0;
	regAddr = REG_DIG_GAIN;

	//from x0.004 to x16 (decimal ex: x1=256, x1.5=384...)
	value = (int)(dgain * 256);

	if (value > 4095)
	{
		value = 4095;
	}
	
	if (value < 1)
	{
		value = 1;
	}

	// write in register
	regAddr = REG_DIG_GAIN;
	error=this->writeReg(regAddr, value);
	if (error != 0)
	{
		fprintf(stderr, "WRITE ERROR: 0x%x\n", regAddr);
		error=-1;
		return error;
	}
		
	return error;
}

/************************************************
 *Set the global gain
 ************************************************/
int ModuleCtrl::setGain(float gain) //TODO: testing
{
	float again, dgain;
	int againVal, dgainVal, regAddr, value;
	int error=0;

	regAddr = REG_DIG_GAIN;

	
	//RANGE: x0.004 to x256 (decimal ex: x1=256, x1.5=384...)

	//again = (int)(gain * 100);

	if(gain < 1.2) 
	{
		again=1;	
		againVal = 0; //x1
	}
	else if(gain < 1.45) 
	{
		again=1.2;
		againVal = 1; //x1.2
	}
	else if(gain < 1.71) 
	{
		again=1.45;
		againVal = 2; //x1.45
	}
	else if(gain < 2.00) 
	{
		again=1.71;
		againVal = 3; //x1.71
	}
	else if(gain < 2.40) 
	{
		again=2;
		againVal = 4; //x2.0	
	}
	else if(gain < 3.00) 
	{
		again=2.4;
		againVal = 5; //x2.4
	}
	else if(gain < 3.43) 
	{
		again=3.0;
		againVal = 6; //x3.0	
	}
	else if(gain < 4.00) 
	{
		again=3.43;
		againVal = 7; //x3.43
	}
	else if(gain < 4.80)  
	{
		again=4.0;
		againVal = 8; //x4.0	
	}
	else if(gain < 6.00)  
	{
		again=4.80;
		againVal = 9; //x4.8	
	}
	else if(gain < 6.86)  
	{
		again=6.0;
		againVal = 10; //x6.0	
	}
	else if(gain < 8.00)  
	{
		again=6.86;
		againVal = 11; //x6.86
	}
	else if(gain < 9.60)  
	{
		again=8.0;
		againVal = 12; //x8	
	}
	else if(gain < 12.00)  
	{
		again=9.60;
		againVal = 13; //x9.6	
	}
	else if(gain < 16.00)  
	{
		again=12.0;
		againVal = 14; //x12	
	}
	else
	{
		again=16.0;
		againVal = 15; //x16	
	}
		
	//from x0.004 to x16 (decimal ex: x1=256, x1.5=384...)
	dgain=gain/again;
	dgainVal = (int)(dgain * 256);

	if (dgainVal > 4095)
	{
		dgainVal = 4095;
	}
	
	if (dgainVal < 1)
	{
		dgainVal = 1;
	}

	printf("GAIN: %4.2lf => AGAIN:  %4.2lf (%d) / DGAIN: %4.2lf (%d)\n",gain, again, againVal, dgain, dgainVal);

	// write in register
	regAddr = REG_ANA_GAIN;
	error=this->writeReg(regAddr, againVal);
	if (error != 0)
	{
		fprintf(stderr, "WRITE ERROR: 0x%x\n", regAddr);
		error=-1;
		return error;
	}
	
	// write in register
	regAddr = REG_DIG_GAIN;
	error=this->writeReg(regAddr, dgainVal);
	if (error != 0)
	{
		fprintf(stderr, "WRITE ERROR: 0x%x\n", regAddr);
		error=-2;
		return error;
	}
		
	return error;
}

/************************************************
 *Set the frame period (ms) for frame rate control
 ************************************************/
int ModuleCtrl::setFramePeriod(float tperiod) //TODO: testing
{
	int regAddr, regVal, tline;
	int error=0;

	tperiod = tperiod * 1000000; // convert to nanoseconds
	tperiod = tperiod / 20; // convert to clock cycle 50MHz

	// read the line length
	regAddr = REG_LINE_LENGTH;
	error=this->readReg(regAddr, &tline);
	
	if (error != 0)
	{
		fprintf(stderr, "READ ERROR: 0x%x\n", regAddr);
		error=-1;
		return error;
	}
	
	regVal = (int)tperiod / tline;
	
	// write in reg_frame_period
	regAddr = REG_FRAME_PERIOD;
	error=this->writeReg(regAddr, regVal);
	if (error != 0)
	{
		fprintf(stderr, "WRITE ERROR: 0x%x\n", regAddr);
		error=-2;
		return error;
	}

	return error;
}

/************************************************
 *Create dump file
 ************************************************/
int ModuleCtrl::createDump(const std::string& filename)
{
    int start_addr = 0x02;
    int stop_addr = 0x7F;
    int reg_addr=start_addr, reg_val=0;

    std::ofstream file(filename);

    if (!file) {
        std::cerr << "Error : fail to create dump file" << std::endl;
        return 1;
    }

    file << "ADDR;VALUE" << std::endl;
      do{
        this->readReg(reg_addr, &reg_val);
        file << std::hex 
        << "0x" 
        << std::setw(4) << std::setfill('0') << reg_addr 
        << ";0x" 
        << std::setw(4) << std::setfill('0') << reg_val 
        << std::dec << std::endl;

        reg_addr ++;
    }while(reg_addr <= stop_addr);

    file.close();

    return 0;
}

/************************************************
 *Read sensor feedback
 ************************************************/
int ModuleCtrl::read_sensor_feedback()
{

	// PARAMETERS
	int Address;
	int Value;
    int fill = 18;
    std::ostringstream oss;

	//READ CHIP ID
	Address = REG_FB_CHIP_ID;
	Value = 0;
    this->readReg(Address, &Value);	

    oss << std::setw(fill) << std::setfill(' ') << std::left << "CHIP-ID:" << std::right
    << "0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Address  << std::dec 
    << "=0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Value << std::dec;

	if (Value>0x8000) oss << " => TOPAZ 2M" << std::endl;
	else if (Value>0x6000) oss << std::endl;
	else if (Value>0x5200) oss << " => SNAPPY 5M"  << std::endl;
	else if (Value>0x5100) oss << " => SNAPPY 2M"  << std::endl;
	else if (Value>0x4200) oss << " => EMERALD 10M/8.9M" << std::endl;
	else if (Value>0x4100) oss << " => EMERALD 16M/12M"  << std::endl;
	else oss << std::endl;

	//READ SENSOR STATE
	Address = REG_FB_STATE;
	Value = 0;
	this->readReg(Address, &Value);
    oss << std::setw(fill) << std::setfill(' ') << std::left << "FB_STATE:" << std::right 
    << "0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Address  << std::dec 
    << "=0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Value << std::dec;

    Value=Value/256;

    oss << " | global_state=0x" << Value 
    << " (" << Value << ")";
	if (Value==0x100) oss << " => SDTBY"<< std::endl;
	else if (Value==0x02) oss << " => WAKE_UP_IF" << std::endl;
	else if (Value==0x04) oss << " => IDLE_IF" << std::endl;
	else if (Value==0x08) oss << " => WAKE_UP_ALL" << std::endl;
	else if (Value==0x10) oss << " => IDLE" << std::endl;
	else if (Value==0x20) oss << " => ACQUISITION" << std::endl;
	else if (Value==0x40) oss << " => STOP_STD" << std::endl;
	else if (Value==0x80) oss << " => WAIT_END_CHAIN" << std::endl;
	else oss << std::endl;
	

	//Read line length fb
	Address = REG_FB_LINE_LENGTH;
	Value = 0;
	this->readReg(Address, &Value);
    oss << std::setw(fill) << std::setfill(' ') << std::left << "FB_LINE_LENGTH:" << std::right 
    << "0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Address  << std::dec 
    << "=0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Value  << std::dec
    << " (" << Value << ")"
    << std::endl;

	//Read error fb
	Address = REG_FB_ERROR;
	Value = 0;
	this->readReg(Address, &Value);
    oss << std::setw(fill) << std::setfill(' ') << std::left << "FB_ERROR:" << std::right 
    << "0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Address << std::dec 
    << "=0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Value << std::dec 
    << " (" << Value << ")";

	if (Value==0x0001) oss << " => trigger error" << std::endl;
	else if (Value==0x0002) oss << " => not(fb_fuse_supply_ena_pi)" << std::endl;
	else if (Value==0x0004) oss << " => frame period error in video mode" << std::endl;
	else if (Value==0x0008) oss << " => error_ll_overflow" << std::endl;
	else if (Value==0x0010) oss << " => error_ll_read" << std::endl;
	else if (Value==0x0020) oss << " => error_ll_conv" << std::endl;
	else if (Value==0x0040) oss << " => error_ll_mipi" << std::endl;
	else oss << std::endl;

	//Read tint fb
	Address = REG_FB_TINT;
	Value = 0;
	this->readReg(Address, &Value);
    oss << std::setw(fill) << std::setfill(' ') << std::left << "FB_TINT:" << std::right 
    << "0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Address << std::dec 
    << "=0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Value << std::dec 
    << " (" << Value << ")"
    << std::endl;

	//Read spy_tint_1_ll fb
	Address = REG_FB_SPY_TINT_LL;
	Value = 0;
	this->readReg(Address, &Value);
    oss << std::setw(fill) << std::setfill(' ') << std::left << "FB_SPY_TINT_LL:" << std::right 
    << "0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Address << std::dec 
    << "=0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Value << std::dec 
    << " (" << Value << ")"
    << std::endl;

    
	//Read spy_tint_1_ck fb
	Address = REG_FB_SPY_TINT_CK;
	Value = 0;
	this->readReg(Address, &Value);
    oss << std::setw(fill) << std::setfill(' ') << std::left  << "FB_SPY_TINT_CK:" << std::right 
    << "0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Address << std::dec 
    << "=0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Value << std::dec 
    << " (" << Value << ")"
    << std::endl;

	//Read frame_period fb
	Address = REG_FB_FRAME_PERIOD;
	Value = 0;
	this->readReg(Address, &Value);
    oss << std::setw(fill) << std::setfill(' ') << std::left << "FB_FRAME_PERIOD:" << std::right 
    << "0x"
    << std::hex << std::setw(4) << std::setfill('0') << Address << std::dec 
    << "=0x" 
    << std::hex << std::setw(4) << std::setfill('0') << Value << std::dec 
    << " (" << Value << ")"
    << std::endl;


    std::string data = oss.str();

    std::cout << data;

    std::ofstream file("feedback.txt");
    if (file) {
        file << data;
    }
    file.close();


	return 0;
}