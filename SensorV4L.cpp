#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <unistd.h>

#define _USE_POLL

#ifdef _USE_POLL
#include <sys/poll.h>
#endif

#include <iostream>
#include <algorithm>

#include "SensorV4L.h"

// Can configure the Format and resolution of the sensor.
// look at the optimom driver documentation for more informations
struct Sensor_modes
{
        unsigned int format;
        unsigned int width;
        unsigned int height;
};

static const struct Sensor_modes sensor_modes[4] = {
    {.format = V4L2_PIX_FMT_SBGGR8, .width = 1920, .height = 1080}, // GRINN
    {.format = V4L2_PIX_FMT_GREY, .width = 1920, .height = 1080}};


#define CLEAR(x) memset(&(x), 0, sizeof(x))

SensorV4L::SensorV4L() :
    m_fd(-1),  m_NbBuffersMax(2), m_NbBuffers(0)
{
    OpenDevice(SENSOR_VIDEO_DRIVER, SENSOR_VIDEO_CTRLS);
}

SensorV4L::~SensorV4L()
{
    DestroyStream();
    CloseDevice();
}

int SensorV4L::OpenDevice(const std::string & strVideoNodeName, const std::string & strCtrlNodeName)
{
    m_fd = open(strVideoNodeName.c_str(), O_RDWR | O_NONBLOCK, 0);   // O_NONBLOCK => none blocking on VIDIOC_DQBUF
    if (m_fd < 0)
    {
        std::cout << "SensorV4L::OpenV4L2Node: Cannot open video device" << strVideoNodeName << std::endl;
        return SENSOR_ERR_OPEN_V4L_DRIVER;
    }
    ListControls(m_fd);

    m_fd_ctrl = open(strCtrlNodeName.c_str(), O_RDWR | O_NONBLOCK, 0);   // O_NONBLOCK => none blocking on VIDIOC_DQBUF
    if (m_fd_ctrl < 0)
    {
        std::cout << "SensorV4L::OpenV4L2Node: Cannot open ctrl device" <<  strCtrlNodeName << std::endl;
        // return SENSOR_ERR_OPEN_V4L_DRIVER;
    } else {
        ListControls(m_fd_ctrl);
    }

    return SENSOR_ERR_SUCCESS;
}

int SensorV4L::CloseDevice()
{
    if (m_fd >=0)
            close(m_fd); // closing device.

    if (m_fd_ctrl >=0)
            close(m_fd_ctrl); // closing device.

    return SENSOR_ERR_SUCCESS;
}

int SensorV4L::InitializeFormat(int64_t i64SensorMode) // initialize format
{
    struct v4l2_format fmt; // struct containing
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = sensor_modes[i64SensorMode].width;
    fmt.fmt.pix.height = sensor_modes[i64SensorMode].height;
    fmt.fmt.pix.pixelformat = sensor_modes[i64SensorMode].format;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    int nCodeRet = xioctl((int)VIDIOC_S_FMT, &fmt);
    if (nCodeRet != SENSOR_ERR_SUCCESS)
    {
        std::cout << "SensorV4L::InitializeFormat: error ioctl for VIDIOC_S_FMT " << std::endl;
        return nCodeRet;
    }

    if (fmt.fmt.pix.pixelformat != sensor_modes[i64SensorMode].format)
    {
        std::cout << "SensorV4L::InitializeFormat: Libv4l didn't accept the current pixel format(" << fmt.fmt.pix.pixelformat << ")Can't proceed." << std::endl;
        return SENSOR_ERR_UNSUPPORTED_CONFIGURATION;
    }
    if ((fmt.fmt.pix.width != sensor_modes[i64SensorMode].width) || (fmt.fmt.pix.height != sensor_modes[i64SensorMode].height))
    {
        std::cout << "SensorV4L::InitializeFormat: Warning: driver is sending image at width=" << fmt.fmt.pix.width << " height=" << fmt.fmt.pix.height << std::endl;
        return SENSOR_ERR_UNSUPPORTED_CONFIGURATION;
    }

    return SENSOR_ERR_SUCCESS;
}

int SensorV4L::xioctl(int request, void *arg)
{
    int r;
    int nCodeRet;

    do
    {
        r = ioctl(m_fd, request, arg);
        nCodeRet = errno;
    } while (r == -1 && (nCodeRet == EINTR));   // O_NONBLOCK => none blocking if  not ready => returns EAGAIN

    if (r == -1)
    {
        if (nCodeRet != EAGAIN)
            std::cout << "SensorV4L::xioctl: error " << nCodeRet << " " <<  strerror(nCodeRet) << std::endl;

        return (nCodeRet == EAGAIN) ? SENSOR_ERR_NO_DATA_AVAILABLE : SENSOR_ERR_IOCTL;
    }

    return SENSOR_ERR_SUCCESS;
}

int SensorV4L::AllocateBuffers()
{
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers req;

    m_NbBuffers = 0;

    int nCodeRet;
    // initialize request buffer ask camera to allocate x buffers for camera
    CLEAR(req);
    req.count = m_NbBuffersMax;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    nCodeRet = xioctl((int)VIDIOC_REQBUFS, &req);
    if (nCodeRet != SENSOR_ERR_SUCCESS)
    {
        std::cout << "SensorV4L::AllocateBuffers: error ioctl for V4L2_BUF_TYPE_VIDEO_CAPTURE " << std::endl;
        return nCodeRet;
    }

    // create a FIFO to store the x buffers
    m_Buffers.reset(new struct buffer[req.count]);

    for (m_NbBuffers = 0; m_NbBuffers < req.count; m_NbBuffers++) // initialize buffers
    {
        CLEAR(buf);

        // get a pointer on each buffer
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = m_NbBuffers;

        nCodeRet = xioctl((int)VIDIOC_QUERYBUF, &buf);
        if (nCodeRet != SENSOR_ERR_SUCCESS)
        {
            std::cout << "SensorV4L::AllocateBuffers: error ioctl for VIDIOC_QUERYBUF " << std::endl;
            return nCodeRet;
        }

        // map each buffer into the FIFO
        m_Buffers[m_NbBuffers].length = buf.length;
        m_Buffers[m_NbBuffers].start = mmap(NULL, buf.length,
        PROT_READ | PROT_WRITE, MAP_SHARED,
        m_fd, buf.m.offset);

        if (MAP_FAILED == m_Buffers[m_NbBuffers].start)
        {
            std::cout << "SensorV4L::AllocateBuffers: error mmap for buffer " << m_NbBuffers << std::endl;
            return SENSOR_ERR_BUFFER_MAP;
        }
    }

    return SENSOR_ERR_SUCCESS;
}


int SensorV4L::FreeBuffers()
{
    for (unsigned int i = 0; i < m_NbBuffers; ++i)       // free buffers
         munmap(m_Buffers[i].start, m_Buffers[i].length);

    m_Buffers.reset(nullptr);

    m_NbBuffers = 0;

    return SENSOR_ERR_SUCCESS;
}

int SensorV4L::QueueBuffers()
{
    struct v4l2_buffer buf;

    int nCodeRet;
    // put the buffers in queue
    for (unsigned int i = 0; i < m_NbBuffers; ++i)
    {
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        nCodeRet = xioctl((int)VIDIOC_QBUF, &buf);
        if (nCodeRet != SENSOR_ERR_SUCCESS)
        {
            std::cout << "error QueueBuffers ioctl VIDIOC_QBUF" << std::endl;
            return nCodeRet;
        }
    }

    return SENSOR_ERR_SUCCESS;
}

int SensorV4L::WaitForBuffer(struct v4l2_buffer &buf, void ** data)
{
    if (! data)
        return SENSOR_ERR_INVALID_PARAMETER;

    int nCodeRet;

#ifdef _USE_POLL
    struct pollfd pfd = { m_fd, POLLIN, 0 };
    nCodeRet = poll( &pfd, 1, 1000);   // wait for 1000ms
    if (nCodeRet == -1) {
        std::cout << "SensorV4L::WaitForBuffer: Error polling: " << strerror(errno) << std::endl;
        return SENSOR_ERR_OPEN_V4L_POLLING;
    }
    if (nCodeRet == 0) {
        std::cout << "SensorV4L::WaitForBuffer: polling timeout" << std::endl;
        return SENSOR_ERR_NO_DATA_AVAILABLE;
    }
#endif

    CLEAR(buf); // clear the buffer
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    nCodeRet = xioctl((int)VIDIOC_DQBUF, &buf);

    switch (nCodeRet)
    {
        case SENSOR_ERR_SUCCESS:
            *data = m_Buffers[buf.index].start;
            break;

        case SENSOR_ERR_NO_DATA_AVAILABLE:
            return nCodeRet;

        default: {
            std::cout << "error WaitForBuffer ioctl VIDIOC_DQBUF" << std::endl;
            return nCodeRet;
    }
    }

    return SENSOR_ERR_SUCCESS;
}

int SensorV4L::RequeueBuffer(struct v4l2_buffer &buf)
{
    // Requeue the buffer
    int nCodeRet = xioctl((int)VIDIOC_QBUF, &buf);
    if (nCodeRet != SENSOR_ERR_SUCCESS)
        return nCodeRet;

    return SENSOR_ERR_SUCCESS;
}


int SensorV4L::CreateStream(int64_t i64SensorMode)
{
    int nCodeRet = InitializeFormat(i64SensorMode);
    if (nCodeRet != SENSOR_ERR_SUCCESS)
        return nCodeRet;

    nCodeRet = AllocateBuffers();
    if (nCodeRet != SENSOR_ERR_SUCCESS)
        return nCodeRet;

    nCodeRet = QueueBuffers();
    if (nCodeRet != SENSOR_ERR_SUCCESS)
        return nCodeRet;

    return SENSOR_ERR_SUCCESS;
}

int SensorV4L::DestroyStream()
{
    return FreeBuffers();
}

int SensorV4L::StartStreaming()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // Start stream (fill the buffers)
    return xioctl((int)VIDIOC_STREAMON, &type);

}

int SensorV4L::StopStreaming()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    return xioctl((int)VIDIOC_STREAMOFF, &type); // stream off
}


int SensorV4L::GetControl(int64_t code, int64_t &value) // get control by code
{
    struct v4l2_ext_controls ecs;
    struct v4l2_ext_control ec;
    memset(&ecs, 0, sizeof(ecs));
    memset(&ec, 0, sizeof(ec));
    ec.id = code; // this code can be obtain using the command v4l2-ctl -l
    ecs.controls = &ec;
    ecs.count = 1;
    ecs.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    int nCodeRet = xioctl((int)VIDIOC_G_EXT_CTRLS, &ecs);
    if (nCodeRet != SENSOR_ERR_SUCCESS)
        return nCodeRet;

    value = ec.value64;
    return SENSOR_ERR_SUCCESS;
}

int SensorV4L::SetControl(int64_t code, int64_t value) // set control by code
{
    struct v4l2_ext_controls ecs;
    struct v4l2_ext_control ec;
    memset(&ecs, 0, sizeof(ecs));
    memset(&ec, 0, sizeof(ec));
    ec.id = code; // this code can be obtain using the command v4l2-ctl -l
    ecs.controls = &ec;
    ecs.count = 1;
    ecs.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ec.value64 = value;
    ec.size = 0;
    return xioctl((int)VIDIOC_S_EXT_CTRLS, &ecs);
}

int SensorV4L::GetControl(const std::string & strName, int64_t &value) // get control by name
{
    for (const auto & item : m_CtrlList) {
            if (item->GetName() == strName) {
        return GetControl((int64_t)item->GetId(), value);
        }
    }

    return SENSOR_ERR_INVALID_PARAMETER;
}

int SensorV4L::SetControl(const std::string & strName, int64_t value) // set control by name
{
    for (const auto & item : m_CtrlList) {
            if (item->GetName() == strName) {
        return SetControl((int64_t)item->GetId(), value);
        }
    }

    return SENSOR_ERR_INVALID_PARAMETER;
}

const std::list<std::unique_ptr<V4LCtrl>> & SensorV4L::GetControlList() const {
    return m_CtrlList;
}

int SensorV4L::ListControls(int fd)
{
    struct v4l2_queryctrl queryctrl = {};
    struct v4l2_querymenu querymenu = {};
    std::string strName;
    memset(&queryctrl, 0, sizeof(queryctrl));

    std::cout << std::endl << "Controls on fd: " << fd << std::endl;

    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    while (ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
        if (!(queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {
            strName = std::string((const char *)queryctrl.name);
            std::replace(strName.begin(), strName.end(), ' ', '_');
            std::cout << "Control: " << strName << std::endl;
            std::cout << "  Id: " << std::hex << queryctrl.id << std::dec << std::endl;

            int64_t i64Minimum = queryctrl.minimum;
            int64_t i64Maximum = queryctrl.maximum;
            uint64_t u64Step = (uint64_t)queryctrl.step;
            int64_t i64DefaultValue = queryctrl.default_value;

            std::cout << std::dec << "  Type:" << queryctrl.type;
            switch(queryctrl.type) {
            case V4L2_CTRL_TYPE_INTEGER:
                std::cout << " (V4L2_CTRL_TYPE_INTEGER)" << std::endl;
                m_CtrlList.push_back(std::make_unique<V4LCtrl>(strName, queryctrl));
                break;
            case V4L2_CTRL_TYPE_BOOLEAN:
                std::cout << " (V4L2_CTRL_TYPE_BOOLEAN)" << std::endl;
                m_CtrlList.push_back(std::make_unique<V4LCtrl>(strName, queryctrl));
                break;
            case V4L2_CTRL_TYPE_MENU: {
                std::cout << " (V4L2_CTRL_TYPE_MENU)" << std::endl;
                m_CtrlList.push_back(std::make_unique<V4LCtrl>(strName, queryctrl));

                memset(&querymenu, 0, sizeof(querymenu));
                querymenu.id = queryctrl.id;
                for (querymenu.index = queryctrl.minimum; querymenu.index <= queryctrl.maximum; querymenu.index++) {
                        if (ioctl(m_fd, VIDIOC_QUERYMENU, &querymenu) == 0) {
                                std::cout << "    Menu item: " <<  (const char *)querymenu.name << std::endl;
                                m_CtrlList.back()->AddEnumEntry(querymenu);
                        }
                }
                break;
            }
            case V4L2_CTRL_TYPE_BUTTON: // Not supported: TODO
                std::cout << " (V4L2_CTRL_TYPE_BUTTON)" << std::endl;
                m_CtrlList.push_back(std::make_unique<V4LCtrl>(strName, queryctrl));
                break;
            case V4L2_CTRL_TYPE_INTEGER64: {
                std::cout << " (V4L2_CTRL_TYPE_INTEGER64)" << std::endl;

                struct v4l2_query_ext_ctrl queryctrlext = {};
                queryctrlext.id = queryctrl.id;
                if (ioctl(m_fd, VIDIOC_QUERY_EXT_CTRL, &queryctrlext) == 0) {
                      if (!(queryctrlext.flags & V4L2_CTRL_FLAG_DISABLED)) {
                        i64Minimum = queryctrlext.minimum;
                        i64Maximum = queryctrlext.maximum;
                        u64Step = queryctrlext.step;
                        i64DefaultValue = queryctrlext.default_value;
                        m_CtrlList.push_back(std::make_unique<V4LCtrl>(strName, queryctrlext));
                      }
                }
                else {
                        if (errno != EINVAL) {
                                perror("Querying control");
                                return SENSOR_ERR_IOCTL;
                        }
                }
                break;
            }
            case V4L2_CTRL_TYPE_STRING:
                std::cout << " (V4L2_CTRL_TYPE_STRING)" << std::endl;
                m_CtrlList.push_back(std::make_unique<V4LCtrl>(strName, queryctrl));
                break;
            case V4L2_CTRL_TYPE_BITMASK:
                std::cout << " (V4L2_CTRL_TYPE_BITMASK)" << std::endl;
                break;
            case V4L2_CTRL_TYPE_INTEGER_MENU:
                std::cout << " (V4L2_CTRL_TYPE_INTEGER_MENU)" << std::endl;

                memset(&querymenu, 0, sizeof(querymenu));
                querymenu.id = queryctrl.id;
                for (querymenu.index = queryctrl.minimum; querymenu.index <= queryctrl.maximum; querymenu.index++) {
                        if (ioctl(m_fd, VIDIOC_QUERYMENU, &querymenu) == 0) {
                                std::cout << "    Menu item: " << querymenu.value << "(" << std::hex << querymenu.value << ")" << std::dec << std::endl;
                                m_CtrlList.back()->AddEnumEntry(querymenu);
                        }
                }
                break;

            // Compound types are >= 0x0100
            case V4L2_CTRL_TYPE_U8:
                std::cout << " (V4L2_CTRL_TYPE_U8)" << std::endl;
                break;
            case V4L2_CTRL_TYPE_U16:
                std::cout << " (V4L2_CTRL_TYPE_U16)" << std::endl;
                break;
            case V4L2_CTRL_TYPE_U32:
                std::cout << " (V4L2_CTRL_TYPE_U32)" << std::endl;
                break;
            case V4L2_CTRL_TYPE_CTRL_CLASS: // Not a control
                std::cout << " (V4L2_CTRL_TYPE_CTRL_CLASS)" << std::endl;
                break;
            default:
                std::cout << " (unknown)" << std::endl;
                break;
            }

            std::cout << "  flag: " << std::hex << queryctrl.flags << std::dec << std::endl;

            std::cout << "  Minimum: " << i64Minimum << std::endl;
            std::cout << "  Maximum: " << i64Maximum << std::endl;
            std::cout << "  Step: " << u64Step << std::endl;
            std::cout << "  Default: " << i64DefaultValue << std::endl;
        }
        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    if (errno != EINVAL) {
        perror("Querying control");
        return SENSOR_ERR_IOCTL;
    }

    return SENSOR_ERR_SUCCESS;
}

V4LCtrl::V4LCtrl(const std::string &strName, const struct v4l2_queryctrl &Ctrl) :
        m_u32Id(Ctrl.id),
        m_eType((enum v4l2_ctrl_type)Ctrl.type),
        m_eFlag((V4LCtrlFlag)Ctrl.flags),
        m_strName(strName),
        m_i64Minimum(Ctrl.minimum),
        m_i64Maximum(Ctrl.maximum),
        m_u64Step(Ctrl.step),
        m_i64DefaultValue(Ctrl.default_value) {
}

V4LCtrl::V4LCtrl(const std::string &strName, const struct v4l2_query_ext_ctrl &Ctrl) :
        m_u32Id(Ctrl.id),
        m_eType((enum v4l2_ctrl_type)Ctrl.type),
        m_eFlag((V4LCtrlFlag)Ctrl.flags),
        m_strName(strName),
        m_i64Minimum(Ctrl.minimum),
        m_i64Maximum(Ctrl.maximum),
        m_u64Step(Ctrl.step),
        m_i64DefaultValue(Ctrl.default_value) {
}

void V4LCtrl::AddEnumEntry(const struct v4l2_querymenu &Menu) {
        if (m_eType == V4L2_CTRL_TYPE_MENU)  {
                m_EnumEntries.push_back((const char *)Menu.name);
        } else if (m_eType == V4L2_CTRL_TYPE_INTEGER_MENU) {
                std::stringstream Stream;
                Stream << Menu.value << " (" << std::hex << Menu.value << ")";
                m_EnumEntries.push_back(Stream.str());
        }
}

uint32_t V4LCtrl::GetId() const {
        return m_u32Id;
}
enum v4l2_ctrl_type V4LCtrl::GetType() const {
        return m_eType;
}

V4LCtrlFlag V4LCtrl::GetFlag() const {
        return m_eFlag;
}

std::string V4LCtrl::GetName() const {
        return m_strName;
}
int64_t	V4LCtrl::GetMinimum() const {
        return m_i64Minimum;
}
int64_t	V4LCtrl::GetMaximum() const {
        return m_i64Maximum;
}
uint64_t V4LCtrl::GetStep() const {
        return m_u64Step;
}

int64_t V4LCtrl::GetDefaultValue() const {
        return m_i64DefaultValue;
}

void V4LCtrl::GetEnumEntries(std::vector<std::string> &EnumVector) const{
        std::copy(m_EnumEntries.begin(), m_EnumEntries.begin(), EnumVector.begin());
}
