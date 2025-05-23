#ifndef LINUX
    #define LINUX
#endif
#ifndef PJON_LINUX_SEPARATE_DEFINITION
    #define PJON_LINUX_SEPARATE_DEFINITION
#endif

#include "PJON_LINUX_Interface.h"
#include "asm-generic/termbits.h"
#include "asm-generic/ioctls.h"
#include <chrono>

inline auto& getMutableStartTime()
{
    static auto start_ts = std::chrono::high_resolution_clock::now();
    return start_ts;
}
inline auto getStartTime()
{
    static auto start_ts_ms = std::chrono::high_resolution_clock::now();
    return start_ts_ms;
}

inline uint32_t micros() {
  auto elapsed_usec =
  std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::high_resolution_clock::now() - getMutableStartTime()
  ).count();

  if(elapsed_usec >= UINT32_MAX) {
    getMutableStartTime() = std::chrono::high_resolution_clock::now();
    return 0;
  } else return elapsed_usec;
};

inline uint32_t millis() {
  return (uint32_t)
  std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::high_resolution_clock::now() - getStartTime()
  ).count();
};

inline void delayMicroseconds(uint32_t delay_value) {
  struct timeval tv;
  if (delay_value < 1000000){
    tv.tv_sec = 0;
    tv.tv_usec = delay_value;
  }
  else{
    tv.tv_sec = floor(delay_value / 1000000);
    tv.tv_usec = delay_value - tv.tv_sec * 1000000;
  }

  select(0, NULL, NULL, NULL, &tv);
};

inline void delay(uint32_t delay_value_ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(delay_value_ms));
};

/* Open serial port ----------------------------------------------------- */

inline int serialOpen(const char *device, const int baud) {
  speed_t bd;
  int fd;

  if((fd = open(device, O_NDELAY | O_NOCTTY | O_NONBLOCK | O_RDWR)) == -1)
  return -1;

  fcntl(fd, F_SETFL, O_RDWR);

  struct termios2 config;
  int state;
  int r = ioctl(fd, TCGETS2, &config);
  if(r) return -1;

  // sets the terminal to something like the "raw" mode of the old Version 7 terminal driver
  config.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                   | INLCR | IGNCR | ICRNL | IXON);
  config.c_cflag &= ~(CSTOPB | CSIZE | PARENB);
  config.c_cflag |= CS8;
  config.c_lflag &= ~(ECHO | ECHOE | ECHONL | ICANON | ISIG | IEXTEN);
  config.c_oflag &= ~OPOST;

  // sets the baudrate
  config.c_ispeed = config.c_ospeed = baud;
  config.c_cflag &= ~CBAUD;
  config.c_cflag |= BOTHER;

  config.c_cflag |= (CLOCAL | CREAD);
  config.c_cc [VMIN] = 0;
  config.c_cc [VTIME] = 50; // 5 seconds reception timeout

  r = ioctl(fd, TCSETS2, &config);
  if(r) return -1;

  r = ioctl(fd, TIOCMGET, &state);
  if(r) return -1;

  state |= (TIOCM_DTR | TIOCM_RTS);
  r = ioctl(fd, TIOCMSET, &state);
  if(r) return -1;

  delayMicroseconds(10000);	// Sleep for 10 milliseconds
  return fd;
};

/* Returns the number of bytes of data available to be read in the buffer */

inline int serialDataAvailable(const int fd) {
  int result = 0;
  ioctl(fd, FIONREAD, &result);
  return result;
};

/* Reads a character from the serial buffer ------------------------------- */

inline int serialGetCharacter(const int fd) {
  uint8_t result;
  if(read(fd, &result, 1) != 1) return -1;
  return ((int)result) & 0xFF;
};
