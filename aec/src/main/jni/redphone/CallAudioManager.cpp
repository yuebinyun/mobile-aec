#include <arpa/inet.h>
#include <netdb.h>
#include "AudioCodec.h"
#include "MicrophoneReader.h"
#include "JitterBuffer.h"
#include "RtpAudioReceiver.h"
#include "AudioPlayer.h"
#include "CallAudioManager.h"
#include "NetworkUtil.h"

#define TAG "AEC-CallAudioManager"

CallAudioManager::CallAudioManager(int androidSdkVersion, int socketFd,
                                   struct sockaddr *_sockAddr, int sockAddrLen)
        : running(0),
          finished(1),
          engineObject(NULL),
          engineEngine(NULL),
          audioCodec(),
          socketFd(socketFd),
          audioSender(socketFd, _sockAddr, sockAddrLen),
          audioReceiver(socketFd),
          webRtcJitterBuffer(audioCodec),
          clock(),
          microphoneReader(androidSdkVersion, audioCodec, audioSender, clock),
          audioPlayer(webRtcJitterBuffer, audioCodec),
          sockAddr(sockAddr) {

    __android_log_print(ANDROID_LOG_WARN, TAG, "Temp : %s", _sockAddr->sa_data);

}

int CallAudioManager::init() {

    if (pthread_mutex_init(&mutex, NULL) != 0) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "Failed to create mutex!");
        return -1;
    }

    if (pthread_cond_init(&condition, NULL) != 0) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "Failed to create condition!");
        return -1;
    }

    return 0;
}

CallAudioManager::~CallAudioManager() {
    __android_log_print(ANDROID_LOG_WARN, TAG, "Shutting down...");

    microphoneReader.stop();

    __android_log_print(ANDROID_LOG_WARN, TAG, "Stopping audio player...");
    audioPlayer.stop();

    __android_log_print(ANDROID_LOG_WARN, TAG, "Stopping jitter buffer...");
    webRtcJitterBuffer.stop();

    __android_log_print(ANDROID_LOG_WARN, TAG, "Freeing resources...");

    if (sockAddr != NULL) {
        free(sockAddr);
    }

    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
    }

    if (socketFd != -1) {
        close(socketFd);
    }

    __android_log_print(ANDROID_LOG_WARN, TAG, "Shutdown complete....");
}

int CallAudioManager::start() {

    running = 1;
    finished = 0;

    if (slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL) != SL_RESULT_SUCCESS) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "Failed to create engineObject!");
        return -1;
    }

    if ((*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "Failed to realize engineObject!");
        return -1;
    }

    if ((*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine) !=
        SL_RESULT_SUCCESS) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "Failed to get engine interface!");
        return -1;
    }

    if (audioCodec.init() != 0) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "Failed to initialize codec!");
        return -1;
    }

    if (audioSender.init() != 0) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "Failed to initialize RTP sender!");
        return -1;
    }

    if (audioReceiver.init() != 0) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "Failed to initialize RTP receiver!");
        return -1;
    }

    if (webRtcJitterBuffer.init() != 0) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "Failed to initialize jitter buffer!");
        return -1;
    }

    __android_log_print(ANDROID_LOG_WARN, TAG, "Starting MicrophoneReader...");

    if (microphoneReader.start(&engineEngine) == -1) {
        __android_log_print(ANDROID_LOG_WARN, TAG,
                            "ERROR -- MicrophoneReader::start() returned -1!");
        return -1;
    }

    __android_log_print(ANDROID_LOG_WARN, TAG, "Starting AudioPlayer...");

    if (audioPlayer.start(&engineEngine) == -1) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "AudioPlayer::start() returned -1!");
        return -1;
    }

    char buffer[4096];

    while (running) {

        // 开始收取数据
        RtpPacket *packet = audioReceiver.receive(buffer, sizeof(buffer));

        if (packet != NULL) {

            if (packet->getTimestamp() == 0) {
                packet->setTimestamp(clock.getImprovisedTimestamp(packet->getPayloadLen()));
            }

            webRtcJitterBuffer.addAudio(packet, clock.getTickCount());

            delete packet;
        }
    }

    if (pthread_mutex_lock(&mutex) != 0) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "Failed to acquire mutex!");
        return 0;
    }

    finished = 1;

    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);

    return 0;
}

void CallAudioManager::stop() {
    running = 0;
    microphoneReader.stop();
    audioPlayer.stop();
    webRtcJitterBuffer.stop();

    pthread_mutex_lock(&mutex);
    while (finished == 0) {
        pthread_cond_wait(&condition, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    usleep(40000); // Duration of microphone frame.
}

void CallAudioManager::setMute(int muteEnabled) {
    microphoneReader.setMute(muteEnabled);
}

jlong JNICALL Java_com_aec_learn_CallAudioManager_create(JNIEnv *env,
                                                         jobject obj,
                                                         jint androidSdkVersion,
                                                         jstring serverIpString,
                                                         jint serverPort,
                                                         jstring session_
) {

    const char *serverIp = env->GetStringUTFChars(serverIpString, 0);
    __android_log_print(ANDROID_LOG_DEBUG, TAG,
                        "[serverIp: %s, serverPort: %d]", serverIp, serverPort);

    int sfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sfd < 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to get sfd!");
        env->ReleaseStringUTFChars(serverIpString, serverIp);
        env->ThrowNew(env->FindClass("java/io/IOException"),
                      "Failed to get sfd!");
        return -1;
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(serverPort);

    if (bind(sfd, (struct sockaddr *) &local_addr, sizeof(local_addr)) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "bind failed: %s\n", strerror(errno));
        env->ReleaseStringUTFChars(serverIpString, serverIp);
        env->ThrowNew(env->FindClass("java/io/IOException"),
                      "Failed to bind!");
        return -1;
    }

    struct sockaddr_in *remote_addr = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
    memset(remote_addr, 0, sizeof(struct sockaddr_in));

    remote_addr->sin_family = AF_INET;
    remote_addr->sin_port = htons(serverPort);
    remote_addr->sin_addr.s_addr = inet_addr(serverIp);

    CallAudioManager *manager = new CallAudioManager(androidSdkVersion, sfd,
                                                     (struct sockaddr *) remote_addr,
                                                     sizeof(struct sockaddr_in));

    if (manager->init() != 0) {
        delete manager;
        env->ReleaseStringUTFChars(serverIpString, serverIp);
        env->ThrowNew(env->FindClass("java/io/IOException"),
                      "Failed to initialize native audio");
        return -1;
    }

    env->ReleaseStringUTFChars(serverIpString, serverIp);
    return (jlong) manager;
}

void JNICALL Java_com_aec_learn_CallAudioManager_start(JNIEnv *env,
                                                       jobject obj,
                                                       jlong handle) {


    __android_log_print(ANDROID_LOG_ERROR, TAG, "Java_com_aec_learn_CallAudioManager_start");

    CallAudioManager *manager = reinterpret_cast<CallAudioManager *>(handle);
    int result = manager->start();

    if (result == -1) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to start native audio");
        env->ThrowNew(env->FindClass("java/io/IOException"),
                      "Failed to start native audio");
    }
}

void JNICALL Java_com_aec_learn_CallAudioManager_setMute(JNIEnv *env,
                                                         jobject obj,
                                                         jlong handle,
                                                         jboolean muteEnabled) {

    CallAudioManager *manager = reinterpret_cast<CallAudioManager *>(handle);
    manager->setMute(muteEnabled);

}

void JNICALL Java_com_aec_learn_CallAudioManager_stop(JNIEnv *env,
                                                      jobject obj,
                                                      jlong handle) {

    CallAudioManager *manager = reinterpret_cast<CallAudioManager *>(handle);
    manager->stop();

}

void JNICALL Java_com_aec_learn_CallAudioManager_dispose(JNIEnv *env,
                                                         jobject obj,
                                                         jlong handle) {

    CallAudioManager *manager = reinterpret_cast<CallAudioManager *>(handle);
    delete manager;
}