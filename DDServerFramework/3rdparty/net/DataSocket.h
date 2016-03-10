#ifndef _DATASOCKET_H
#define _DATASOCKET_H

#include <memory>
#include <functional>
#include <deque>

#include "SocketLibFunction.h"
#include "Channel.h"
#include "timer.h"
#include "EventLoop.h"

#ifdef USE_OPENSSL

#ifdef  __cplusplus
extern "C" {
#endif
#include "openssl/ssl.h"
#ifdef  __cplusplus
}
#endif

#endif

class EventLoop;
struct buffer_s;

class DataSocket : public Channel
{
public:
    typedef DataSocket*                                                         PTR;

    typedef std::function<void(PTR)>                                            ENTER_CALLBACK;
    typedef std::function<int(PTR, const char* buffer, size_t len)>             DATA_CALLBACK;
    typedef std::function<void(PTR)>                                            DISCONNECT_CALLBACK;
    typedef std::shared_ptr<std::function<void(void)>>                          PACKED_SENDED_CALLBACK;

    typedef std::shared_ptr<std::string>                                        PACKET_PTR;

public:
    explicit DataSocket(int fd, int maxRecvBufferSize);
    ~DataSocket();

    bool                            onEnterEventLoop(EventLoop* el);

    void                            send(const char* buffer, size_t len, const PACKED_SENDED_CALLBACK& callback = nullptr);

    void                            sendPacketInLoop(const PACKET_PTR&, const PACKED_SENDED_CALLBACK& callback = nullptr);
    void                            sendPacketInLoop(PACKET_PTR&&, const PACKED_SENDED_CALLBACK& callback = nullptr);

    void                            sendPacket(const PACKET_PTR&, const PACKED_SENDED_CALLBACK& callback = nullptr);
    void                            sendPacket(PACKET_PTR&&, const PACKED_SENDED_CALLBACK& callback = nullptr);

    void                            setEnterCallback(ENTER_CALLBACK cb);
    void                            setDataCallback(DATA_CALLBACK cb);
    void                            setDisConnectCallback(DISCONNECT_CALLBACK cb);

    void                            setCheckTime(int overtime);
    /*����(Ͷ��)�Ͽ�����,�ᴥ���Ͽ��ص�*/
    void                            postDisConnect();

    void                            setUserData(int64_t value);
    int64_t                         getUserData() const;

#ifdef USE_OPENSSL
    void                            setupAcceptSSL(SSL_CTX*);
    void                            setupConnectSSL();
#endif

    static  PACKET_PTR              makePacket(const char* buffer, size_t len);
private:
    void                            growRecvBuffer();

    void                            PingCheck();
    void                            startPingCheckTimer();

    void                            canRecv() override;
    void                            canSend() override;

    bool                            checkRead();
    bool                            checkWrite();

    void                            recv();
    void                            flush();
    void                            normalFlush();
    void                            quickFlush();

    void                            onClose() override;
    void                            closeSocket();
    void                            procCloseInLoop();

    void                            runAfterFlush();
#ifdef PLATFORM_LINUX
    void                            removeCheckWrite();
#endif

private:

#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    struct EventLoop::ovl_ext_s     mOvlRecv;
    struct EventLoop::ovl_ext_s     mOvlSend;

    bool                            mPostRecvCheck;     /*  �Ƿ�Ͷ���˿ɶ����   */
    bool                            mPostWriteCheck;    /*  �Ƿ�Ͷ���˿�д���   */
#endif

    int                             mFD;
    bool                            mIsPostFinalClose;  /*  �Ƿ�Ͷ�������յ�close����    */

    bool                            mCanWrite;          /*  socket�Ƿ��д  */

    EventLoop*                      mEventLoop;
    buffer_s*                       mRecvBuffer;
    size_t                          mMaxRecvBufferSize;

    struct pending_packet
    {
        PACKET_PTR  packet;
        size_t      left;
        PACKED_SENDED_CALLBACK  mCompleteCallback;
    };

    typedef std::deque<pending_packet>   PACKET_LIST_TYPE;
    PACKET_LIST_TYPE                mSendList;          /*  ������Ϣ�б�  */

    ENTER_CALLBACK                  mEnterCallback;
    DATA_CALLBACK                   mDataCallback;
    DISCONNECT_CALLBACK             mDisConnectCallback;

    bool                            mIsPostFlush;       /*  �Ƿ��Ѿ�����flush��Ϣ�Ļص�    */

    int64_t                         mUserData;          /*  ���ӵ��û��Զ�������  */

#ifdef USE_OPENSSL
    SSL_CTX*                        mSSLCtx;
    SSL*                            mSSL;
#endif

    bool                            mRecvData;
    int                             mCheckTime;
    Timer::WeakPtr                  mTimer;
};

#endif