#pragma once
#include <linux/uinput.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

class UInputDevice {
    int fd_ = -1;

public:
    UInputDevice() {
        fd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (fd_ < 0)
            throw std::runtime_error("Cannot open /dev/uinput (run as root or add user to input group)");

        ioctl(fd_, UI_SET_EVBIT, EV_KEY);
        ioctl(fd_, UI_SET_KEYBIT, BTN_LEFT);
        ioctl(fd_, UI_SET_KEYBIT, BTN_RIGHT);

        ioctl(fd_, UI_SET_EVBIT, EV_REL);
        ioctl(fd_, UI_SET_RELBIT, REL_X);
        ioctl(fd_, UI_SET_RELBIT, REL_Y);
        ioctl(fd_, UI_SET_RELBIT, REL_WHEEL);
        ioctl(fd_, UI_SET_RELBIT, REL_HWHEEL);

        ioctl(fd_, UI_SET_KEYBIT, BTN_MIDDLE);
        ioctl(fd_, UI_SET_KEYBIT, BTN_SIDE);
        ioctl(fd_, UI_SET_KEYBIT, BTN_EXTRA);

        ioctl(fd_, UI_SET_EVBIT, EV_SYN);

        struct uinput_setup setup{};
        std::strncpy(setup.name, "Ghast Clicker Virtual Mouse", UINPUT_MAX_NAME_SIZE - 1);
        setup.id.bustype = BUS_USB;
        setup.id.vendor = 0x1234;
        setup.id.product = 0x5678;

        if (ioctl(fd_, UI_DEV_SETUP, &setup) < 0 ||
            ioctl(fd_, UI_DEV_CREATE) < 0) {
            close(fd_);
            throw std::runtime_error("Failed to create uinput device");
        }

        usleep(100000);
    }

    ~UInputDevice() {
        if (fd_ >= 0) {
            ioctl(fd_, UI_DEV_DESTROY);
            close(fd_);
        }
    }

    UInputDevice(const UInputDevice&) = delete;
    UInputDevice& operator=(const UInputDevice&) = delete;

    void click(int button, int value) const {
        emit(EV_KEY, button, value);
        emit(EV_SYN, SYN_REPORT, 0);
    }

    void emit_raw(int type, int code, int val) const {
        emit(type, code, val);
    }

private:
    void emit(int type, int code, int val) const {
        struct input_event ev{};
        ev.type = type;
        ev.code = code;
        ev.value = val;
        write(fd_, &ev, sizeof(ev));
    }
};
