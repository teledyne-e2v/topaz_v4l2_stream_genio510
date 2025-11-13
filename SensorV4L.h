#pragma once

#include <vector>
#include <list>
#include <sstream>
#include <memory>
#include <linux/videodev2.h>
#include <libv4l2.h>

// This is a vendor-custom define available only in vendor kernel.
// Why cannot modify 'linux-libc-headers.bb' explained here:
// https://docs.yoctoproject.org/dev-manual/new-recipe.html#using-headers-to-interface-with-devices
// and
// https://github.com/yoctoproject/poky/blob/b33a8abe77081a2bdda0d89c61736473b2f9bb8b/meta/recipes-kernel/linux-libc-headers/linux-libc-headers.inc#L7
//
// It is also possible to create own recipe for mediatek kernel headers and set
// PREFERRED_PROVIDER_linux-libc-headers = "linux-libc-headers-genio"

#define V4L2_PIX_FMT_MTISP_SBGGR10  v4l2_fourcc('M', 'B', 'B', 'A') /*  Packed 10-bit  */


#define SENSOR_VIDEO_DRIVER   "/dev/video-topaz1"   // GRINN
#define SENSOR_VIDEO_CTRLS    "/dev/v4l-subdev-topaz1"
#define SENSOR_DEFAULT_MODE   1  // 0 - RAW8 / 1 - RAW10

#define SENSOR_ERR_SUCCESS							 0
#define SENSOR_ERR_NO_DATA_AVAILABLE					-1
#define SENSOR_ERR_BUFFER_MAP						-2
#define SENSOR_ERR_INVALID_PARAMETER					-3
#define SENSOR_ERR_IOCTL								-4
#define SENSOR_ERR_OPEN_V4L_DRIVER					-5
#define SENSOR_ERR_UNSUPPORTED_CONFIGURATION			-6
#define SENSOR_ERR_AIRY_3D_NO_TDM_FOUND				-7
#define SENSOR_ERR_AIRY_3D_NO_CALIBRATION_FOUND		-8
#define SENSOR_ERR_ACCESS_DENIED						-9
#define SENSOR_ERR_AIRY3D_CONFIGURATION				-10
#define SENSOR_ERR_OPEN_V4L_POLLING					-11

struct buffer
{
        void *start;
        size_t length;
};

enum class V4LCtrlFlag { // Warning !!! can be ored !!
	eDisable = V4L2_CTRL_FLAG_DISABLED, // control currently disableed and cannot be used
	eGrabbed = V4L2_CTRL_FLAG_GRABBED, // currently beinused  by another application and cannot be modified
	eReadOnly = V4L2_CTRL_FLAG_READ_ONLY, // control is Read only
	eUpdate = V4L2_CTRL_FLAG_UPDATE, // control automatically updated by the hardware
	eInactive = V4L2_CTRL_FLAG_INACTIVE, // contole is inactive ans should'nt be displayed to the user
	eSlider = V4L2_CTRL_FLAG_SLIDER, // suggests control can be represented as a slider in user interface
	eWriteOnly = V4L2_CTRL_FLAG_WRITE_ONLY, // control is Write only
	eVolatile = V4L2_CTRL_FLAG_VOLATILE, // Control value can changed without user intervention, often due to hardware or driver updates
	eHasPayLoad = V4L2_CTRL_FLAG_HAS_PAYLOAD,
	eExecuteOnWrite = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	eModifyLayout = V4L2_CTRL_FLAG_MODIFY_LAYOUT
};

class V4LCtrl
{
	public:
		V4LCtrl(const std::string &strName, const struct v4l2_queryctrl &Ctrl);
		V4LCtrl(const std::string &strName, const struct v4l2_query_ext_ctrl &Ctrl);
		void AddEnumEntry(const struct v4l2_querymenu &Menu);
		uint32_t GetId() const;
		enum v4l2_ctrl_type GetType() const;
		std::string	GetName() const;
		int64_t	GetMinimum() const;
		int64_t	GetMaximum() const;
		uint64_t GetStep() const;
		int64_t GetDefaultValue() const;
		V4LCtrlFlag GetFlag() const;
		void GetEnumEntries(std::vector<std::string> &EnumVector) const;

	private:
		uint32_t m_u32Id;
		enum v4l2_ctrl_type m_eType;
		V4LCtrlFlag m_eFlag;
		std::string m_strName;
		int64_t	m_i64Minimum;
		int64_t	m_i64Maximum;
		uint64_t m_u64Step;
		int64_t	m_i64DefaultValue;
		std::vector<std::string> m_EnumEntries;
};


class SensorV4L
{
public:
	SensorV4L();
	~SensorV4L();
	int CreateStream(int64_t i64SensorMode);
	int DestroyStream();
	int WaitForBuffer(struct v4l2_buffer &buf, struct v4l2_plane &planes, void ** data);
	int RequeueBuffer(struct v4l2_buffer &buf);
	int StartStreaming();
	int StopStreaming();
	int GetControl(int64_t code, int64_t &value); // get control by code
	int SetControl(int64_t code, int64_t value); // set control by code
	int GetControl(const std::string & strName, int64_t &value); // get control by name
	int SetControl(const std::string & strName, int64_t value); // set control by name
	const std::list<std::unique_ptr<V4LCtrl>> & GetControlList() const;
	uint32_t getWidth() const { return m_width; }
	uint32_t getHeight() const { return m_height; }
	uint32_t getPixelformat() const { return m_pixelformat; }
	const std::string getFourCC();

private:
	int OpenDevice(const std::string & strVideoNodeName, const std::string & strCtrlNodeName);
	int CloseDevice();
	int InitializeFormat(int64_t i64SensorMode);
	int xioctl(int request, void *arg);
	int AllocateBuffers();
	int FreeBuffers();
	int QueueBuffers();
	int ListControls(int fd);
	const std::string GetFourCCString(uint32_t fmt);

private:
	int m_fd;
	int m_fd_ctrl;
	const unsigned int m_NbBuffersMax;
	unsigned int m_NbBuffers;
	std::unique_ptr<struct buffer []> m_Buffers;
	std::list<std::unique_ptr<V4LCtrl>> m_CtrlList;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_pixelformat;
};
