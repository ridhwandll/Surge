// Copyright (c) - SurgeTechnologies - All rights reserved
#include "ThreadPool.hpp"
#include "Pch.hpp"

namespace Surge
{
    ThreadPool::ThreadPool(Uint threadCount) : mThreadCount(std::max<Uint>(threadCount, 1)), mThreads(new std::thread[std::max<Uint>(threadCount, 1)])
    {
        Log<Severity::Info>("Creating ThreadPool with {0} threads...", threadCount);
        CreateThreads();
    }

    ThreadPool::~ThreadPool()
    {
        WaitForTasks();
        mRunning = false;
        DestroyThreads();
    }

    void ThreadPool::Reset(Uint threadCount)
    {
        WaitForTasks();
        mRunning = false;
        DestroyThreads();
        mThreadCount = std::max<Uint>(threadCount, 1);
        mThreads.reset(new std::thread[std::max<Uint>(threadCount, 1)]);
        mRunning = true;
        CreateThreads();
    }

    void ThreadPool::WaitForTasks()
    {
        while (mTasksWaiting != 0)
        {
            std::this_thread::yield();
        }
    }

    void ThreadPool::CreateThreads()
    {
        for (Uint i = 0; i < mThreadCount; i++)
        {
            mThreads[i] = std::thread(&ThreadPool::Worker, this);
        }
    }

    void ThreadPool::DestroyThreads()
    {
        for (std::uint_fast32_t i = 0; i < mThreadCount; i++)
        {
            mThreads[i].join();
        }
    }

    bool ThreadPool::PopTask(std::function<void()>& task)
    {
        const std::scoped_lock lock(mQueueMutex);
        if (mTasks.empty())
            return false;
        else
        {
            task = std::move(mTasks.front());
            mTasks.pop();
            return true;
        }
    }

    void ThreadPool::Worker()
    {
        while (mRunning)
        {
            std::function<void()> task;
            if (PopTask(task))
            {
                task();
                mTasksWaiting--;
            }
            else
            {
                std::this_thread::yield();
            }
        }
    }
} // namespace Surge
