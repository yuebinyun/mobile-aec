#ifndef __RTP_AUDIO_SENDER_H__
#define __RTP_AUDIO_SENDER_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
//#include "srtp.h"

class RtpAudioSender {
private:
  int      socketFd;
  uint32_t sequenceNumber;

    struct sockaddr *sockAddr;
    int             sockAddrLen;

public:
  RtpAudioSender(int socketFd, struct sockaddr *sockAddr, int sockAddrLen);

//  ~RtpAudioSender();

  int init();
  int sendRTP(int timestamp, char *encodedData, int encodedDataLen);

};


#endif