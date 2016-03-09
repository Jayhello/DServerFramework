#ifndef _EVENT_LOOP_H
#define _EVENT_LOOP_H

#include <stdint.h>
#include <functional>
#include <vector>
#include <mutex>
#include <thread>
#include <memory>
#include <unordered_map>
#include <atomic>

#include "CurrentThread.h"
#include "SocketLibFunction.h"
#include "timer.h"

class Channel;

class EventLoop
{
public:
    typedef std::function<void(void)>           USER_PROC;

#ifdef PLATFORM_WINDOWS
    enum OLV_VALUE
    {
        OVL_RECV = 1,
        OVL_SEND,
    };

    struct ovl_ext_s
    {
        OVERLAPPED  base;
        int         OP;
    };
#endif

public:
    EventLoop();
    ~EventLoop();

    void                            loop(int64_t    timeout);

    bool                            wakeup();

    /*  Ͷ��һ���첽�ص�����EventLoop::loop�����Ѻ�ִ�� */
    void                            pushAsyncProc(const USER_PROC& f);
    void                            pushAsyncProc(USER_PROC&& f);

    /*  ���̰߳�ȫ:Ͷ�ݻص������ڵ���loop��βʱִ��   */
    void                            pushAfterLoopProc(const USER_PROC& f);
    void                            pushAfterLoopProc(USER_PROC&& f);

    TimerMgr&                       getTimerMgr();

    bool                            linkChannel(int fd, Channel* ptr);

#ifdef PLATFORM_WINDOWS
    HANDLE                          getIOCPHandle() const;
#else
    int                             getEpollHandle() const;
#endif

    bool                            isInLoopThread();
private:
    void                            reallocEventSize(int size);
    void                            processAfterLoopProcs();
    void                            processAsyncProcs();


private:
    size_t                          mEventEntriesNum;
#ifdef PLATFORM_WINDOWS
    typedef BOOL(WINAPI *sGetQueuedCompletionStatusEx) (HANDLE ,LPOVERLAPPED_ENTRY ,ULONG ,PULONG ,DWORD ,BOOL );

    OVERLAPPED_ENTRY*               mEventEntries;
    sGetQueuedCompletionStatusEx    mPGetQueuedCompletionStatusEx;
    HANDLE                          mIOCP;
    ovl_ext_s                       mWakeupOvl;
    Channel*                        mWakeupChannel;
#else
    int                             mEpollFd;
    epoll_event*                    mEventEntries;
    int                             mWakeupFd;
    Channel*                        mWakeupChannel;
#endif
    std::atomic_bool                mIsInBlock;
    std::atomic_bool                mIsAlreadyPostWakeup;

    std::vector<USER_PROC>          mAsyncProcs;                /*  Ͷ�ݵ���eventloop���첽function����    */

    std::vector<USER_PROC>          mAfterLoopProcs;            /*  eventloopÿ��ѭ����ĩβҪִ�е�һϵ�к���   */
    std::vector<USER_PROC>          copyAfterLoopProcs;         /*  ������loop�д���mAfterLoopProcs���б������������;���������Ԫ��  */

    std::mutex                      mAsyncProcsMutex;

    bool                            mIsInitThreadID;
    CurrentThread::THREAD_ID_TYPE   mSelfThreadid;              /*  ����loop��������thread��id */

    TimerMgr                        mTimer;
};

#endif