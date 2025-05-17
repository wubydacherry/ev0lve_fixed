
#ifndef DETAIL_DISPATCH_QUEUE_H
#define DETAIL_DISPATCH_QUEUE_H

#include <atomic>
#include <detail/custom_tracing.h>
#include <shared_mutex>
#include <thread>

namespace detail
{
class dispatch_queue
{
	typedef int32_t(__cdecl *allocate_thread_id_t)();
	typedef void(__cdecl *free_thread_id_t)();

public:
	typedef std::function<void()> fn;

	dispatch_queue() = default;
	virtual ~dispatch_queue() { decomission(); }

	void evaluate(std::vector<fn> &f);
	void spawn();
	void decomission();

private:
	void process(size_t index);
	size_t calculate_remaining_tls_slots();

	allocate_thread_id_t allocate_thread_id;
	free_thread_id_t free_thread_id;

	std::vector<fn> *queue = nullptr;
	std::shared_mutex queue_mtx{};
	std::condition_variable_any queue_cond{};
	std::atomic<size_t> to_run{};
	std::vector<std::thread> threads{};
	std::atomic<bool> decomissioned{};
};

extern dispatch_queue dispatch;
} // namespace detail

#endif // DETAIL_DISPATCH_QUEUE_H
