
#include <detail/custom_tracing.h>
#include <detail/dispatch_queue.h>

using namespace sdk;

namespace detail
{
dispatch_queue dispatch;

void dispatch_queue::spawn()
{
	// locate tls initializer entry of the game.
	allocate_thread_id = allocate_thread_id_t(game->tier0.at(functions::tls::allocate_thread_id));
	free_thread_id = free_thread_id_t(game->tier0.at(functions::tls::free_thread_id));

	// determine slots.
	const auto slots = min(std::thread::hardware_concurrency() - 1, calculate_remaining_tls_slots());

	// print warning, if the user was dumb again.
	if (slots < 1)
	{
		MessageBoxA(nullptr, XOR_STR("Please remove the -threads launch option and try again."), XOR_STR("Attention"),
					MB_ICONERROR | MB_SETFOREGROUND);
		RUNTIME_ERROR("Threads could not be spawned.");
	}

	// create enough threads to maintain the tracer.
	for (auto i = 0u; i < slots; i++)
		threads.emplace_back(&dispatch_queue::process, this, i);
}

// Caller is responsible for making sure this is synchronized!
// NOTE: This function is blocking.
void dispatch_queue::evaluate(std::vector<fn> &f)
{
	if (f.empty())
		return;

	// add new calls to dispatch.
	std::unique_lock lock(queue_mtx);
	queue = &f;
	to_run = queue->size();

	// notify threads about new calls.
	for (;;)
	{
		if (queue->empty())
		{
			auto val = 1;
			do
			{
				to_run.wait(val);
				val = to_run.load(std::memory_order_relaxed);
			} while (val > 0);
			break;
		}

		const auto current = std::move(queue->back());
		queue->pop_back();
		lock.unlock();

		queue_cond.notify_all();
		current();

		to_run.fetch_sub(1, std::memory_order_relaxed);
		lock.lock();
	}

	queue = nullptr;
}

void dispatch_queue::decomission()
{
	std::unique_lock lock(queue_mtx);

	if (decomissioned)
		return;

	decomissioned = true;
	lock.unlock();
	queue_cond.notify_all();

	for (auto &thread : threads)
		thread.join();
}

void dispatch_queue::process(const size_t index)
{
	// set affinity.
	SetThreadAffinityMask(GetCurrentThread(), 1 << index);

	// allocate tls storage for the current thread.
	allocate_thread_id();

	// process the actual queue.
	for (;;)
	{
		// retrieve a new job.
		std::unique_lock lock(queue_mtx);
		queue_cond.wait(lock, [this] { return queue && !queue->empty() || decomissioned; });

		// check if we are still running.
		if (decomissioned)
			break;

		const auto current = std::move(queue->back());
		queue->pop_back();
		lock.unlock();

		// process the current job.
		queue_cond.notify_all();
		current();

		// decrease remaining calls.
		--to_run;
		to_run.notify_one();
	}

	free_thread_id();
}

size_t dispatch_queue::calculate_remaining_tls_slots()
{
	static constexpr auto max_threads = 32;

	auto highest_index = 1;
	while (highest_index < 128 && reinterpret_cast<uint8_t *>(game->tier0.at(functions::tls::tls_slots))[highest_index])
		highest_index++;

	return std::clamp(max_threads - highest_index, 0, max_threads);
}
} // namespace detail
