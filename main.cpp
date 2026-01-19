#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread> 

#include "SensorV4L.h"
#include "SensorI2C.hpp"
#include "unpack_raw10.h"


void SaveToBinaryFile(const std::string &strFilePath, void * buffer, size_t unImageSize) {
	std::ofstream file(strFilePath, std::ios::binary);
	if (!file) {
		std::cerr << "Error : impossible to open file " << strFilePath << std::endl;
		return;
	}

	if (!file.write(reinterpret_cast<char*>(buffer), unImageSize)) {
		std::cerr << "Error writing file" << std::endl;
		return;
	}

	file.close();
}

void runSensorOnly() {
	std::cout << "Sensor only mode" << std::endl;

	std::cout << "Instanciate SensorV4L" << std::endl;
	SensorV4L sensor;
	
	ModuleCtrl *moduleCtrl;
	moduleCtrl = new ModuleCtrl();
	moduleCtrl->ModuleControlInit(); // init ic2

	//READ CHIP ID
	int reg_addr = 0x007F;
	int reg_val = 0;
    moduleCtrl->readReg(reg_addr, &reg_val);	
	std::cout << "SensorI2C: 0x" 
	<< std::setw(4) << std::setfill('0') << std::hex << reg_addr 
	<< " = 0x" 
	<< std::setw(4) << std::setfill('0') << reg_val 
	<< std::dec << std::endl;

	// init config check
	moduleCtrl->createDump("init-config-dump.txt");
	moduleCtrl->print_sensor_feedback();
	moduleCtrl->save_sensor_feedback("init-feedback.txt");

	//initial setup
	std::string control = "exposure";
	int64_t value = 1000;
	std::cout << "Control: " << control << " = " << value << std::endl;
	sensor.SetControl(control, value);

	control = "analog_gain";
	value = 0;
	std::cout << "Control: " << control << " = " << value << std::endl;
	sensor.SetControl(control, value);

	control = "digtal_gain";
	value = 256;
	std::cout << "Control: " << control << " = " << value << std::endl;
	sensor.SetControl(control, value);

	control = "test_pattern";
	value = 0;
	std::cout << "Control: " << control << " = " << value << std::endl;
	sensor.SetControl(control, value);

	control = "image_offset";
	value = 0;
	std::cout << "Control: " << control << " = " << value << std::endl;
	sensor.SetControl(control, value);

	control = "trigger_output";
	value = 0;
	std::cout << "Control: " << control << " = " << value << std::endl;
	sensor.SetControl(control, value);

	control = "flash_delay_on";
	value = 0;
	std::cout << "Control: " << control << " = " << value << std::endl;
	sensor.SetControl(control, value);

	//parameter sweep setup
	control = "flash_delay_off";
	value = 0;
	int64_t step = 50;
	int nb_steps = 5;

	for (int sweep=1; sweep <= nb_steps; sweep++) {

		std::cout << "CreateStream" << std::endl;
		int nCodeRet = sensor.CreateStream(SENSOR_DEFAULT_MODE);
		if (nCodeRet != SENSOR_ERR_SUCCESS) {
			std::cerr << "Error " << nCodeRet << " CreateStream" << std::endl;
			return;
		}

		
		// int64_t getvalue = 0;
		// sensor.GetControl(control, getvalue);
		// std::cout << "Control read: " << control << " = " << getvalue << std::endl;
		
		std::cout << "Control: " << control << " = " << value << std::endl;
		sensor.SetControl(control, value);

		std::cout << "StartStreaming" << std::endl;
		nCodeRet = sensor.StartStreaming();
		if (nCodeRet != SENSOR_ERR_SUCCESS) {
			std::cerr << "Error " << nCodeRet << " StartStreaming" << std::endl;
			return;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100)); //wait for sensor startup

		struct v4l2_buffer buf;
		struct v4l2_plane planes[VIDEO_MAX_PLANES];
		void *data;

		void *raw16_dst = NULL;
		int raw16_required_dst_size;

		if (sensor.isRAW10()) {
			raw16_required_dst_size = sensor.getWidth() * sensor.getHeight() * 2;
			raw16_dst = malloc(raw16_required_dst_size);
			if (!raw16_dst) {
				std::cerr << "Error malloc" << std::endl;
				return;
			}
			std::cout << "Allocated " << raw16_required_dst_size << " buffer for converted frame" << std::endl;
		}

		std::stringstream file_base; // base name for file saving
		file_base << "image" 
			<< "_" << sensor.getWidth() << "x" << sensor.getHeight()
			<< "_" << sensor.getFourCC() 
			<< "_" << control 
			<< "_" << value;
		std::cout << file_base.str() << std::endl;

		// CHECK CONTROL
		moduleCtrl->print_sensor_feedback();
		moduleCtrl->save_sensor_feedback(file_base.str() + "_fb_reg.txt");
		moduleCtrl->createDump(file_base.str() + "_dump.txt");

		for (int count=1; count <= 2; count++) {
			std::cout << "WaitForBuffer" << std::endl;
			nCodeRet = sensor.WaitForBuffer(buf, planes[0], &data);
			if (nCodeRet != SENSOR_ERR_SUCCESS) {
				std::cerr << "Error " << nCodeRet << " WaitForBuffer" << std::endl;
				return;
			}
	
			std::stringstream file_image;
			file_image << file_base.str() << "_" << std::setw(2) << std::setfill('0') << count;

			SaveToBinaryFile(file_image.str() + ".raw", data, planes[0].length);
			if (sensor.isRAW10()) {
					unpack_image(data, raw16_dst, raw16_required_dst_size);
					SaveToBinaryFile(file_image.str() + ".raw16", raw16_dst, raw16_required_dst_size);
			}

			std::cout << "RequeueBuffer" << std::endl;
			nCodeRet = sensor.RequeueBuffer(buf);
			if (nCodeRet != SENSOR_ERR_SUCCESS) {
				std::cerr << "Error " << nCodeRet << " RequeueBuffer" << std::endl;
				return;
			}

			std::cout << "Grab image " << count << std::endl;
		}
		if (raw16_dst) {
			free(raw16_dst);
			raw16_dst = NULL;
		}

		std::cout << "StopStreaming" << std::endl;
		nCodeRet = sensor.StopStreaming();
		if (nCodeRet != SENSOR_ERR_SUCCESS) {
			std::cerr << "Error " << nCodeRet << " StopStreaming" << std::endl;
			return;
		}

		// std::cout << "FlushBuffer" << std::endl;
		// int fcount=1;
		// do{
		// 	std::cout << "WaitForBuffer " << fcount++ << std::endl;
		// 	nCodeRet = sensor.WaitForBuffer(buf, planes[0], &data);
		// 	if (nCodeRet == SENSOR_ERR_SUCCESS) {
		// 		std::cout << "RequeueBuffer" << std::endl;
		// 		int nCodeRetReq = sensor.RequeueBuffer(buf);
		// 	}
		// }while(nCodeRet == SENSOR_ERR_SUCCESS);


		std::cout << "DestroyStream" << std::endl;
		nCodeRet = sensor.DestroyStream();
		if (nCodeRet != SENSOR_ERR_SUCCESS) {
			std::cerr << "Error " << nCodeRet << " DestroyStream" << std::endl;
			return;
		}

		value = value + step; // inc for next image
	}

}

int main(int argc, char* argv[])
{
	runSensorOnly();
	std::cout << "Exiting" << std::endl;

	return 0;
}
