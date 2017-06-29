#include "WebRtcJitterBuffer.h"
//#include <time.h>

#define TAG "WebRtcJitterBuffer"

static volatile int running = 0;

WebRtcJitterBuffer::WebRtcJitterBuffer(AudioCodec &codec) :
  neteq(NULL), webRtcCodec(codec)
{
  running = 1;
}

int WebRtcJitterBuffer::init() {

  webrtc::NetEq::Config config;
  config.sample_rate_hz = 8000;

  neteq = webrtc::NetEq::Create(config);

  if (neteq == NULL) {
    __android_log_print(ANDROID_LOG_WARN, TAG, "Failed to construct NetEq!");
    return -1;
  }

  if (neteq->RegisterExternalDecoder(&webRtcCodec, webrtc::kDecoderPCMu, 0) != 0) {
    __android_log_print(ANDROID_LOG_WARN, TAG, "Failed to register external codec!");
    return -1;
  }

  return 0;
}

WebRtcJitterBuffer::~WebRtcJitterBuffer() {
  if (neteq != NULL) {
    delete neteq;
  }
}

void WebRtcJitterBuffer::addAudio(RtpPacket *packet, uint32_t tick) {
  webrtc::WebRtcRTPHeader header;

  header.header.payloadType    = packet->getPayloadType();
  header.header.sequenceNumber = packet->getSequenceNumber();
  header.header.timestamp      = packet->getTimestamp();
  header.header.ssrc           = packet->getSsrc();

  uint8_t *payload = (uint8_t*)malloc(packet->getPayloadLen());
  memcpy(payload, packet->getPayload(), packet->getPayloadLen());

  if (neteq->InsertPacket(header, payload, packet->getPayloadLen(), tick) != 0) {
    __android_log_print(ANDROID_LOG_WARN, TAG, "neteq->InsertPacket() failed!");
  }
}

int WebRtcJitterBuffer::getAudio(short *rawData, int maxRawData) {
  int samplesPerChannel = 0;
  int numChannels       = 0;

  if (neteq->GetAudio(maxRawData, rawData, &samplesPerChannel, &numChannels, NULL) != 0) {
    __android_log_print(ANDROID_LOG_WARN, TAG, "neteq->GetAudio() failed!");
  }

  return samplesPerChannel;
}

void WebRtcJitterBuffer::stop() {
  running = 0;
}