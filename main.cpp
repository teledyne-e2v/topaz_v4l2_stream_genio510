#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include "SensorV4L.h"


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

	std::cout << "CreateStream" << std::endl;
	int nCodeRet = sensor.CreateStream(0);
	if (nCodeRet != SENSOR_ERR_SUCCESS) {
		std::cout << "Error " << nCodeRet << " CreateStream" << std::endl;
		return;
	}

	std::cout << "StartStreaming" << std::endl;
	nCodeRet = sensor.StartStreaming();
	if (nCodeRet != SENSOR_ERR_SUCCESS) {
		std::cout << "Error " << nCodeRet << " StartStreaming" << std::endl;
		return;
	}

	struct v4l2_buffer buf;
	void *data;

	for (int count=1; count <= 10; count++) {
		std::cout << "WaitForBuffer" << std::endl;
		nCodeRet = sensor.WaitForBuffer(buf, &data);
		if (nCodeRet != SENSOR_ERR_SUCCESS) {
			std::cout << "Error " << nCodeRet << " WaitForBuffer" << std::endl;
			return;
		}

		{
			std::stringstream ss;
			ss << "image" << count << "_width_" << 1920 << "_height_" << 1080 << "_Mono8bit.raw"; // GRINN
			SaveToBinaryFile(ss.str(), data, 1920 * 1080 * sizeof(uint8_t));   // GRINN    Mono8bit
		}

		std::cout << "RequeueBuffer" << std::endl;
		nCodeRet = sensor.RequeueBuffer(buf);
		if (nCodeRet != SENSOR_ERR_SUCCESS) {
			std::cout << "Error " << nCodeRet << " RequeueBuffer" << std::endl;
			return;
		}

		std::cout << "Grab image " << count << std::endl;
	}

	std::cout << "StopStreaming" << std::endl;
	nCodeRet = sensor.StopStreaming();
	if (nCodeRet != SENSOR_ERR_SUCCESS) {
		std::cout << "Error " << nCodeRet << " StopStreaming" << std::endl;
		return;
	}

	std::cout << "DestroyStream" << std::endl;
	nCodeRet = sensor.DestroyStream();
	if (nCodeRet != SENSOR_ERR_SUCCESS) {
		std::cout << "Error " << nCodeRet << " DestroyStream" << std::endl;
		return;
	}
}

int main(int argc, char* argv[])
{
	runSensorOnly();
	std::cout << "Exiting" << std::endl;

	return 0;
}
