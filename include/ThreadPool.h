//sgn
#pragma once

#define BS_THREAD_POOL_VERSION "v3.3.0 (2022-08-03)"

#include <atomic>             // std::atomic
#include <chrono>             // std::chrono
#include <condition_variable> // std::condition_variable
#include <exception>          // std::current_exception
#include <functional>         // std::bind, std::function, std::invoke
#include <future>             // std::future, std::promise
#include <iostream>           // std::cout, std::endl, std::flush, std::ostream
#include <memory>             // std::make_shared, std::make_unique, std::shared_ptr, std::unique_ptr
#include <mutex>              // std::mutex, std::scoped_lock, std::unique_lock
#include <queue>              // std::queue
#include <thread>             // std::thread
#include <type_traits>        // std::common_type_t, std::conditional_t, std::decay_t, std::invoke_result_t, std::is_void_v
#include <utility>            // std::forward, std::move, std::swap
#include <vector>             // std::vector

namespace util {
using concurrency_t = std::invoke_result_t<decltype(std::thread::hardware_concurrency)>;

template <typename T>
class [[nodiscard]] multi_future {
public:
    multi_future(const size_t num_futures_ = 0) : futures(num_futures_) {}
    [[nodiscard]] std::conditional_t<std::is_void_v<T>, void, std::vector<T>> get()     {
        if constexpr (std::is_void_v<T>) {
            for (size_t i = 0; i < futures.size(); ++i)
                futures[i].get();
            return;
        } else {
            std::vector<T> results(futures.size());
            for (size_t i = 0; i < futures.size(); ++i)
                results[i] = futures[i].get();
            return results;
        }
    }

    [[nodiscard]] std::future<T>& operator[](const size_t i) {
        return futures[i];
    }
    void push_back(std::future<T> future) {
        futures.push_back(std::move(future));
    }
    [[nodiscard]] size_t size() const {
        return futures.size();
    }
    void wait() const {
        for (size_t i = 0; i < futures.size(); ++i)
            futures[i].wait();
    }
private:
    std::vector<std::future<T>> futures;
};


template <typename T1, typename T2, typename T = std::common_type_t<T1, T2>>
class [[nodiscard]] blocks {
public:
    blocks(const T1 first_index_, const T2 index_after_last_, const size_t num_blocks_) : first_index(static_cast<T>(first_index_)), index_after_last(static_cast<T>(index_after_last_)), num_blocks(num_blocks_)
    {
        if (index_after_last < first_index)
            std::swap(index_after_last, first_index);
        total_size = static_cast<size_t>(index_after_last - first_index);
        block_size = static_cast<size_t>(total_size / num_blocks);
        if (block_size == 0)
        {
            block_size = 1;
            num_blocks = (total_size > 1) ? total_size : 1;
        }
    }
    [[nodiscard]] T start(const size_t i) const {
        return static_cast<T>(i * block_size) + first_index;
    }
    [[nodiscard]] T end(const size_t i) const {
        return (i == num_blocks - 1) ? index_after_last : (static_cast<T>((i + 1) * block_size) + first_index);
    }
    [[nodiscard]] size_t get_num_blocks() const {
        return num_blocks;
    }

    [[nodiscard]] size_t get_total_size() const {
        return total_size;
    }

private:

    size_t block_size = 0;
    T first_index = 0;
    T index_after_last = 0;
    size_t num_blocks = 0;
    size_t total_size = 0;
};


class [[nodiscard]] ThreadPool {
public:
    typedef std::shared_ptr<ThreadPool> Ptr;
    ThreadPool(const concurrency_t thread_count_ = 0) : thread_count(determine_thread_count(thread_count_)), threads(std::make_unique<std::thread[]>(determine_thread_count(thread_count_)))
    {
        create_threads();
    }

    ~ThreadPool() {
        wait_for_tasks();
        destroy_threads();
    }


    [[nodiscard]] size_t get_tasks_queued() const {
        const std::scoped_lock tasks_lock(tasks_mutex);
        return tasks.size();
    }


    [[nodiscard]] size_t get_tasks_running() const {
        const std::scoped_lock tasks_lock(tasks_mutex);
        return tasks_total - tasks.size();
    }

    [[nodiscard]] size_t get_tasks_total() const {
        return tasks_total;
    }

    [[nodiscard]] concurrency_t get_thread_count() const {
        return thread_count;
    }


    [[nodiscard]] bool is_paused() const {
        return paused;
    }

    template <typename F, typename T1, typename T2, typename T = std::common_type_t<T1, T2>, typename R = std::invoke_result_t<std::decay_t<F>, T, T>>
    [[nodiscard]] multi_future<R> parallelize_loop(const T1 first_index, const T2 index_after_last, F&& loop, const size_t num_blocks = 0)
    {
        blocks blks(first_index, index_after_last, num_blocks ? num_blocks : thread_count);
        if (blks.get_total_size() > 0)
        {
            multi_future<R> mf(blks.get_num_blocks());
            for (size_t i = 0; i < blks.get_num_blocks(); ++i)
                mf[i] = submit(std::forward<F>(loop), blks.start(i), blks.end(i));
            return mf;
        }
        else
        {
            return multi_future<R>();
        }
    }


    template <typename F, typename T, typename R = std::invoke_result_t<std::decay_t<F>, T, T>>
    [[nodiscard]] multi_future<R> parallelize_loop(const T index_after_last, F&& loop, const size_t num_blocks = 0)
    {
        return parallelize_loop(0, index_after_last, std::forward<F>(loop), num_blocks);
    }

    void pause()
    {
        paused = true;
    }

    template <typename F, typename T1, typename T2, typename T = std::common_type_t<T1, T2>>
    void push_loop(const T1 first_index, const T2 index_after_last, F&& loop, const size_t num_blocks = 0)
    {
        blocks blks(first_index, index_after_last, num_blocks ? num_blocks : thread_count);
        if (blks.get_total_size() > 0)
        {
            for (size_t i = 0; i < blks.get_num_blocks(); ++i)
                push_task(std::forward<F>(loop), blks.start(i), blks.end(i));
        }
    }

    template <typename F, typename T>
    void push_loop(const T index_after_last, F&& loop, const size_t num_blocks = 0)
    {
        push_loop(0, index_after_last, std::forward<F>(loop), num_blocks);
    }

    template <typename F, typename... A>
    void push_task(F&& task, A&&... args)
    {
        std::function<void()> task_function = std::bind(std::forward<F>(task), std::forward<A>(args)...);
        {
            const std::scoped_lock tasks_lock(tasks_mutex);
            tasks.push(task_function);
        }
        ++tasks_total;
        task_available_cv.notify_one();
    }

    void reset(const concurrency_t thread_count_ = 0)
    {
        const bool was_paused = paused;
        paused = true;
        wait_for_tasks();
        destroy_threads();
        thread_count = determine_thread_count(thread_count_);
        threads = std::make_unique<std::thread[]>(thread_count);
        paused = was_paused;
        create_threads();
    }


    template <typename F, typename... A, typename R = std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>>
    [[nodiscard]] std::future<R> submit(F&& task, A&&... args)
    {
        std::function<R()> task_function = std::bind(std::forward<F>(task), std::forward<A>(args)...);
        std::shared_ptr<std::promise<R>> task_promise = std::make_shared<std::promise<R>>();
        push_task(
            [task_function, task_promise]
            {
                try
                {
                    if constexpr (std::is_void_v<R>)
                    {
                        std::invoke(task_function);
                        task_promise->set_value();
                    }
                    else
                    {
                        task_promise->set_value(std::invoke(task_function));
                    }
                }
                catch (...)
                {
                    try
                    {
                        task_promise->set_exception(std::current_exception());
                    }
                    catch (...)
                    {
                    }
                }
            });
        return task_promise->get_future();
    }

    void unpause()
    {
        paused = false;
    }

    void wait_for_tasks()
    {
        waiting = true;
        std::unique_lock<std::mutex> tasks_lock(tasks_mutex);
        task_done_cv.wait(tasks_lock, [this] { return (tasks_total == (paused ? tasks.size() : 0)); });
        waiting = false;
    }

private:
    void create_threads()
    {
        running = true;
        for (concurrency_t i = 0; i < thread_count; ++i)
        {
            threads[i] = std::thread(&ThreadPool::worker, this);
        }
    }

    void destroy_threads()
    {
        running = false;
        task_available_cv.notify_all();
        for (concurrency_t i = 0; i < thread_count; ++i)
        {
            threads[i].join();
        }
    }

    [[nodiscard]] concurrency_t determine_thread_count(const concurrency_t thread_count_)
    {
        if (thread_count_ > 0)
            return thread_count_;
        else
        {
            if (std::thread::hardware_concurrency() > 0)
                return std::thread::hardware_concurrency();
            else
                return 1;
        }
    }

    void worker()
    {
        while (running)
        {
            std::function<void()> task;
            std::unique_lock<std::mutex> tasks_lock(tasks_mutex);
            task_available_cv.wait(tasks_lock, [this] { return !tasks.empty() || !running; });
            if (running && !paused)
            {
                task = std::move(tasks.front());
                tasks.pop();
                tasks_lock.unlock();
                task();
                tasks_lock.lock();
                --tasks_total;
                if (waiting)
                    task_done_cv.notify_one();
            }
        }
    }

    std::atomic<bool> paused = false;
    std::atomic<bool> running = false;
    std::condition_variable task_available_cv = {};
    std::condition_variable task_done_cv = {};
    std::queue<std::function<void()>> tasks = {};
    std::atomic<size_t> tasks_total = 0;
    mutable std::mutex tasks_mutex = {};
    concurrency_t thread_count = 0;
    std::unique_ptr<std::thread[]> threads = nullptr;
    std::atomic<bool> waiting = false;
};


class [[nodiscard]] synced_stream
{
public:

    synced_stream(std::ostream& out_stream_ = std::cout) : out_stream(out_stream_) {}
    template <typename... T>
    void print(T&&... items)
    {
        const std::scoped_lock lock(stream_mutex);
        (out_stream << ... << std::forward<T>(items));
    }
    template <typename... T>
    void println(T&&... items)
    {
        print(std::forward<T>(items)..., '\n');
    }

    inline static std::ostream& (&endl)(std::ostream&) = static_cast<std::ostream& (&)(std::ostream&)>(std::endl);
    inline static std::ostream& (&flush)(std::ostream&) = static_cast<std::ostream& (&)(std::ostream&)>(std::flush);

private:

    std::ostream& out_stream;
    mutable std::mutex stream_mutex = {};
};


class [[nodiscard]] timer
{
public:

    void start()
    {
        start_time = std::chrono::steady_clock::now();
    }
    void stop()
    {
        elapsed_time = std::chrono::steady_clock::now() - start_time;
    }
    [[nodiscard]] std::chrono::milliseconds::rep ms() const
    {
        return (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time)).count();
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_time = std::chrono::duration<double>::zero();
};

} // namespace util
