//
// Created by thomas on 20.01.23.
//
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
#include "Network.h"

#ifndef MAV_SERIAL_H
#define MAV_SERIAL_H

namespace mav {
    class Serial : public NetworkInterface {
    private:
        int _fd;

    public:
        Serial(const std::string &device, int baud, bool flow_control = true) {
            _fd = open(device.c_str(), O_RDWR | O_NOCTTY);
            if (_fd == -1) {
                throw NetworkError(StringFormat() << "Failed to open " << device << " error " << _fd << StringFormat::end);
            }
            struct termios tc{};
            if (tcgetattr(_fd, &tc) != 0) {
                close(_fd);
                throw NetworkError(StringFormat() << "Failed to get tc attrs" << StringFormat::end);
            }

            tc.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
            tc.c_oflag &= ~(OCRNL | ONLCR | ONLRET | ONOCR | OFILL | OPOST);
            tc.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG | TOSTOP);
            tc.c_cflag &= ~(CSIZE | PARENB | CRTSCTS);
            tc.c_cflag |= CS8 | CLOCAL;

            tc.c_cc[VMIN] = 0; // We are ok with 0 bytes.
            tc.c_cc[VTIME] = 10; // Timeout after 1 second.

            if (flow_control) {
                tc.c_cflag |= CRTSCTS;
            }

            if (cfsetspeed(&tc, baud) != 0) {
                close(_fd);
                throw NetworkError(StringFormat() << "Failed to set baud rate to " << baud << StringFormat::end);
            }

            if (tcsetattr(_fd, TCSANOW, &tc) != 0) {
                close(_fd);
                throw NetworkError(StringFormat() << "Failed to set TCSANOW" << StringFormat::end);
            }
        }


        void stop() const {
            close(_fd);
        }

        void send(const uint8_t* data, uint32_t size) const override {
            uint32_t sent = 0;
            while (sent < size) {
                auto ret = write(_fd, data, size - sent);
                if (ret < 0) {
                    stop();
                    throw NetworkError("Serial send failed");
                }
                sent += static_cast<uint32_t>(ret);
            }
        };

        void receive(uint8_t* data, uint32_t size) const override {
            uint32_t received = 0;
            while (received < size) {
                auto ret = read(_fd, data, size - received);
                if (ret < 0) {
                    stop();
                    throw NetworkError("Serial read failed");
                }
                received += ret;
            }
        };


        virtual ~Serial() {
            stop();
        }
    };

}





#endif //MAV_SERIAL_H
