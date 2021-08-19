// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

#include <thread>
#include <mutex>
#include <future>
#include <utility>
#include <queue>
#include <algorithm>
#include <atomic>
#include <functional>

namespace Surge
{
    class SURGE_API ThreadPool
    {
    public:
        ThreadPool(Uint threadCount = std::thread::hardware_concurrency());
        ~ThreadPool();

        Uint GetThreadCount() { return mThreadCount; }

		template <typename T, typename F>
		void ParallelizeLoop(T first_index, T last_index, const F& loop, Uint num_tasks = 0)
		{
			if (num_tasks == 0)
				num_tasks = mThreadCount;
			if (last_index < first_index)
				std::swap(last_index, first_index);
			size_t total_size = last_index - first_index + 1;
			size_t block_size = total_size / num_tasks;
			if (block_size == 0)
			{
				block_size = 1;
				num_tasks = std::max((Uint)1, (Uint)total_size);
			}
			std::atomic<Uint> blocks_running = 0;
			for (Uint t = 0; t < num_tasks; t++)
			{
				T start = (T)(t * block_size + first_index);
				T end = (t == num_tasks - 1) ? last_index : (T)((t + 1) * block_size + first_index - 1);
				blocks_running++;
				PushTask([&start, &end, &loop, &blocks_running] {
					for (T i = start; i <= end; i++)
						loop(i);
					blocks_running--;
				});
				while (blocks_running != 0)
				{
					std::this_thread::yield();
				}
			}
		}

		template <typename F>
		void PushTask(const F& task)
		{
			m_TasksWaiting++;
			{
				const std::scoped_lock lock(QueueMutex);
				mTasks.push(std::move(std::function<void()>(task)));
			}
		}

		template <typename F, typename... A>
		void PushTask(const F& task, const A &... args)
		{
			PushTask([task, args...]{ task(args...); });
		}

		void Reset(Uint threadCount = std::thread::hardware_concurrency());

		template <typename F, typename... A, typename E = std::enable_if_t<std::is_void_v<std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>>>>
		std::future<bool> Submit(const F& task, const A &... args)
		{
			std::shared_ptr<std::promise<bool>> promise(new std::promise<bool>);
			std::future<bool> future = promise->get_future();
			PushTask([task, args..., promise]{
				task(args...);
				promise->set_value(true);
				});
			return future;
		}

		template <typename F, typename... A, typename R = std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>, typename = std::enable_if_t<!std::is_void_v<R>>>
		std::future<R> Submit(const F& task, const A &... args)
		{
			std::shared_ptr<std::promise<R>> promise(new std::promise<R>);
			std::future<R> future = promise->get_future();
			PushTask([task, args..., promise]{
				promise->set_value(task(args...));
				});
			return future;
		}

		void WaitForTasks();
    private:
        void CreateThreads();
        void DestroyThreads();

        bool PopTask(std::function<void()>& task);
        void Worker();

        std::atomic<bool> mRunning = true;
        std::atomic<Uint> mTasksWaiting = 0;
        mutable std::mutex mQueueMutex;
        std::queue<std::function<void()>> mTasks;
        Uint mThreadCount;
        std::unique_ptr<std::thread[]> mThreads;
    };
}