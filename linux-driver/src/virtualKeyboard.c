/*
Taken and adopted from: https://www.kernel.org/doc/html/v4.12/input/uinput.html
as well as: https://www.froglogic.com/blog/tip-of-the-week/using-linux-uinput-from-a-test-script/
*/
#include <errno.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <wiringPi.h>
#include <wiringSerial.h>

//#define SERIAL_PORT "/dev/serial0"
#define SERIAL_PORT "/dev/ttyAMA0"
//#define SERIAL_PORT "/dev/ttyS0"

#define BAUD_RATE 115200

static void emit(int fd, int type, int code, int val)
{
    struct input_event ie;

    ie.type = type;
    ie.code = code;
    ie.value = val;
    /* timestamp values below are ignored */
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    write(fd, &ie, sizeof(ie));
}

static volatile int run = 1;

static void signalHandler(int i)
{
    run = 0;
}

static const int keyMapping[] = {
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    // a button
    KEY_J,
    // b button
    KEY_K,
    // start button
    KEY_ENTER,
    // select button
    KEY_ESC,
    // L button
    KEY_L,
    // R button
    KEY_P,
};

int main(void)
{
    int serialPort = serialOpen(SERIAL_PORT, BAUD_RATE);
    if (serialPort < 0) {
        fprintf(stderr, "Failed to open serial port: %s\n", SERIAL_PORT);
        return 1;
    }
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "WiringPi Setup failed!\n", SERIAL_PORT);
        return 2;
    }

    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    if (fd < 0) {
        fprintf(stderr, "Failed to open device, error: %s\n", strerror(errno));
        return 3;
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN);

    /*
    * The ioctls below will enable the device that is about to be
    * created, to pass key events, in this case the space key.
    */
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (int i = 0; i < sizeof(keyMapping) / sizeof(keyMapping[0]); ++i) {
        if (ioctl(fd, UI_SET_KEYBIT, keyMapping[i])) {
            fprintf(stderr, "Failed to set key bit, error: %s\n", strerror(errno));
            return 4;
        }
    }

    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x4242;  /* sample vendor */
    usetup.id.product = 0x4242; /* sample product */
    strcpy(usetup.name, "ESHO1_20 UART2Key");

    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);

    for (; run && serialDataAvail(serialPort) >= 0;) {
        char c = serialGetchar(serialPort);
        // the messages are interpreted as follow: up to 7 bits for the index and the LSB bit for the key state
        // Key press is encoded as 1 & key release is encoded as 0
        int index = c >> 1;
        if (index < sizeof(keyMapping) / sizeof(keyMapping[0])) {
            // update key state
            emit(fd, EV_KEY, keyMapping[index], c & 1);
            // report the event
            emit(fd, EV_SYN, SYN_REPORT, 0);
        }
    }

    serialClose(serialPort);

    ioctl(fd, UI_DEV_DESTROY);
    close(fd);

    return 0;
}
