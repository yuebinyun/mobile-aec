#include "RtpAudioReceiver.h"
#include <unistd.h>
#include <android/log.h>

#define TAG "AEC-RtpAudioReceiver"

RtpAudioReceiver::RtpAudioReceiver(int socketFd) :
        socketFd(socketFd), sequenceCounter() {
}

int RtpAudioReceiver::init() {
    return 0;
}

RtpPacket *RtpAudioReceiver::receive(char *encodedData, int encodedDataLen) {

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10;
    setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    // UDP SOCKET 接收数据
    int received = recv(socketFd, encodedData, encodedDataLen, 0);
    __android_log_print(ANDROID_LOG_INFO, TAG, "recv() size %d", received);

    if (received == -1) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "recv() failed!");
        return NULL;
    }

    if (received < RtpPacket::getMinimumSize()) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "recveived malformed packet!");
        return NULL;
    }

    RtpPacket *packet = new RtpPacket(encodedData, received);

    __android_log_print(ANDROID_LOG_INFO,
                        TAG,
                        "recv sn %d, ts %d",
                        packet->getSequenceNumber(),
                        packet->getTimestamp()
    );

    return packet;


}