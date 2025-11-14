#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <sstream>

#include "SensorV4L.h"

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

	std::cout << "CreateStream" << std::endl;
	int nCodeRet = sensor.CreateStream(SENSOR_DEFAULT_MODE);
	if (nCodeRet != SENSOR_ERR_SUCCESS) {
		std::cerr << "Error " << nCodeRet << " CreateStream" << std::endl;
		return;
	}

	std::cout << "StartStreaming" << std::endl;
	nCodeRet = sensor.StartStreaming();
	if (nCodeRet != SENSOR_ERR_SUCCESS) {
		std::cerr << "Error " << nCodeRet << " StartStreaming" << std::endl;
		return;
	}

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

	for (int count=1; count <= 10; count++) {
		std::cout << "WaitForBuffer" << std::endl;
		nCodeRet = sensor.WaitForBuffer(buf, planes[0], &data);
		if (nCodeRet != SENSOR_ERR_SUCCESS) {
			std::cerr << "Error " << nCodeRet << " WaitForBuffer" << std::endl;
			return;
		}

		std::stringstream file_base;
		file_base << "image" << std::setw(2) << std::setfill('0') << count
			<< "_" << sensor.getWidth() << "x" << sensor.getHeight()
			<< "_" << sensor.getFourCC();

		SaveToBinaryFile(file_base.str() + ".raw10", data, planes[0].length);
		if (sensor.isRAW10()) {
				unpack_image(data, raw16_dst, raw16_required_dst_size);
				SaveToBinaryFile(file_base.str() + ".raw16", raw16_dst, raw16_required_dst_size);
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

	std::cout << "DestroyStream" << std::endl;
	nCodeRet = sensor.DestroyStream();
	if (nCodeRet != SENSOR_ERR_SUCCESS) {
		std::cerr << "Error " << nCodeRet << " DestroyStream" << std::endl;
		return;
	}
}

int main(int argc, char* argv[])
{
	runSensorOnly();
	std::cout << "Exiting" << std::endl;

	return 0;
}
