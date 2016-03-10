#ifndef _RWLIST_H
#define _RWLIST_H

#include <mutex>
#include <condition_variable>
#include <deque>

template<typename T>
class MsgQueue
{
public:
    typedef std::deque<T>   Container;

    MsgQueue()
    {
    }

    void    Push(const T& t)
    {
        mWriteList.push_back(t);
    }

    void    Push(T&& t)
    {
        mWriteList.push_back(std::move(t));
    }

    /*  ͬ��д���嵽�������(������б���Ϊ��)    */
    void    TrySyncWrite()
    {
        if (!mWriteList.empty() && mSharedList.empty())
        {
            mMutex.lock();

            if (!mWriteList.empty() && mSharedList.empty())
            {
                mSharedList.swap(mWriteList);
                mCond.notify_one();
            }

            mMutex.unlock();
        }
    }

    /*  ǿ��ͬ��    */
    void    ForceSyncWrite()
    {
        if (!mWriteList.empty())
        {
            if (mSharedList.empty())
            {
                /*  ����������Ϊ�գ�����н���  */
                TrySyncWrite();
            }
            else
            {
                mMutex.lock();

                if (!mWriteList.empty())
                {
                    /*  ǿ��д��    */
                    if (mWriteList.size() > mSharedList.size())
                    {
                        for (auto& x : mSharedList)
                        {
                            mWriteList.push_front(std::move(x));
                        }

                        mSharedList.clear();
                        mSharedList.swap(mWriteList);
                    }
                    else
                    {
                        for (auto& x : mWriteList)
                        {
                            mSharedList.push_back(std::move(x));
                        }

                        mWriteList.clear();
                    }

                    mCond.notify_one();
                }

                mMutex.unlock();
            }
        }
    }

    bool      PopFront(T* data)
    {
        bool ret = false;

        if (!mReadList.empty())
        {
            T& tmp = mReadList.front();
            *data = std::move(tmp);
            mReadList.pop_front();
            ret = true;
        }

        return ret;
    }

    bool      PopBack(T* data)
    {
        bool ret = false;

        if (!mReadList.empty())
        {
            T& tmp = mReadList.back();
            *data = std::move(tmp);
            mReadList.pop_back();
            ret = true;
        }

        return ret;
    }

    /*  �ӹ������ͬ������������(�����������Ϊ��ʱ) */
    void    SyncRead(int waitMicroSecond)
    {
        if (mReadList.empty())
        {
            if (waitMicroSecond > 0)
            {
                std::unique_lock<std::mutex>    tmp(mMutex);
                mCond.wait_for(tmp, std::chrono::microseconds(waitMicroSecond));
            }

            mMutex.lock();

            if (mReadList.empty() && !mSharedList.empty())
            {
                mSharedList.swap(mReadList);
            }

            mMutex.unlock();
        }
    }

    size_t  SharedListSize() const
    {
        return mSharedList.size();
    }

    size_t  ReadListSize() const
    {
        return mReadList.size();
    }

    size_t  WriteListSize() const
    {
        return mWriteList.size();
    }

private:
    std::mutex                      mMutex;
    std::condition_variable         mCond;

    /*  д���� */
    Container                       mWriteList;
    /*  �������    */
    Container                       mSharedList;
    /*  ��������    */
    Container                       mReadList;
};

#endif