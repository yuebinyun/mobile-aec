#include "RtpAudioSender.h"
#include "RtpPacket.h"

#include <android/log.h>
#include <errno.h>

#define TAG "AEC-RtpAudioSender"

RtpAudioSender::RtpAudioSender(int socketFd, struct sockaddr *_sockAddr, int sockAddrLen) :
        socketFd(socketFd), sockAddr(_sockAddr), sockAddrLen(sockAddrLen), sequenceNumber(0) {
}

int RtpAudioSender::init() {
    return 0;
}

int RtpAudioSender::sendRTP(int timestamp, char *encodedData, int encodedDataLen) {

    RtpPacket packet(encodedData, encodedDataLen, sequenceNumber, timestamp);

    char *serializedPacket = packet.getSerializedPacket();
    int len = packet.getSerializedPacketLen();

    if (sendto(socketFd, serializedPacket, len, 0, sockAddr, sockAddrLen) == -1) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "sendto() failed: %s, data size : %d\n",
                            strerror(errno), len);
        return -1;
    } else {
//        __android_log_print(ANDROID_LOG_WARN, TAG, "sendto() data size : %d\n", len);
    }

    return 0;
}