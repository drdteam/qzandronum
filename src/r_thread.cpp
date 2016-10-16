
#include <stddef.h>
#include "templates.h"
#include "doomdef.h"
#include "i_system.h"
#include "w_wad.h"
#include "r_local.h"
#include "v_video.h"
#include "doomstat.h"
#include "st_stuff.h"
#include "g_game.h"
#include "g_level.h"
#include "r_thread.h"

CVAR(Bool, r_multithreaded, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG);

void R_BeginDrawerCommands()
{
	DrawerCommandQueue::Begin();
}

void R_EndDrawerCommands()
{
	DrawerCommandQueue::End();
}

/////////////////////////////////////////////////////////////////////////////

DrawerCommandQueue *DrawerCommandQueue::Instance()
{
	static DrawerCommandQueue queue;
	return &queue;
}

DrawerCommandQueue::DrawerCommandQueue()
{
}

DrawerCommandQueue::~DrawerCommandQueue()
{
	StopThreads();
}

void* DrawerCommandQueue::AllocMemory(size_t size)
{
	// Make sure allocations remain 16-byte aligned
	size = (size + 15) / 16 * 16;

	auto queue = Instance();
	if (queue->memorypool_pos + size > memorypool_size)
		return nullptr;

	void *data = queue->memorypool + queue->memorypool_pos;
	queue->memorypool_pos += size;
	return data;
}

void DrawerCommandQueue::Begin()
{
	auto queue = Instance();
	queue->Finish();
	queue->threaded_render++;
}

void DrawerCommandQueue::End()
{
	auto queue = Instance();
	queue->Finish();
	if (queue->threaded_render > 0)
		queue->threaded_render--;
}

void DrawerCommandQueue::WaitForWorkers()
{
	Instance()->Finish();
}

void DrawerCommandQueue::Finish()
{
	auto queue = Instance();
	if (queue->commands.empty())
		return;

	// Give worker threads something to do:

	std::unique_lock<std::mutex> start_lock(queue->start_mutex);
	queue->active_commands.swap(queue->commands);
	queue->run_id++;
	start_lock.unlock();

	queue->StartThreads();
	queue->start_condition.notify_all();

	// Do one thread ourselves:

	DrawerThread thread;
	thread.core = 0;
	thread.num_cores = (int)(queue->threads.size() + 1);

	struct TryCatchData
	{
		DrawerCommandQueue *queue;
		DrawerThread *thread;
		size_t command_index;
	} data;

	data.queue = queue;
	data.thread = &thread;
	data.command_index = 0;
	VectoredTryCatch(&data,
	[](void *data)
	{
		TryCatchData *d = (TryCatchData*)data;

		for (int pass = 0; pass < d->queue->num_passes; pass++)
		{
			d->thread->pass_start_y = pass * d->queue->rows_in_pass;
			d->thread->pass_end_y = (pass + 1) * d->queue->rows_in_pass;
			if (pass + 1 == d->queue->num_passes)
				d->thread->pass_end_y = MAX(d->thread->pass_end_y, MAXHEIGHT);

			size_t size = d->queue->active_commands.size();
			for (d->command_index = 0; d->command_index < size; d->command_index++)
			{
				auto &command = d->queue->active_commands[d->command_index];
				command->Execute(d->thread);
			}
		}
	},
	[](void *data)
	{
		TryCatchData *d = (TryCatchData*)data;
		ReportFatalError(d->queue->active_commands[d->command_index], true);
	});

	// Wait for everyone to finish:

	std::unique_lock<std::mutex> end_lock(queue->end_mutex);
	queue->end_condition.wait(end_lock, [&]() { return queue->finished_threads == queue->threads.size(); });

	if (!queue->thread_error.IsEmpty())
		I_FatalError("Fatal drawer error: %s", queue->thread_error.GetChars());

	// Clean up batch:

	for (auto &command : queue->active_commands)
		command->~DrawerCommand();
	queue->active_commands.clear();
	queue->memorypool_pos = 0;
	queue->finished_threads = 0;
}

void DrawerCommandQueue::StartThreads()
{
	if (!threads.empty())
		return;

	int num_threads = std::thread::hardware_concurrency();
	if (num_threads == 0)
		num_threads = 4;

	threads.resize(num_threads - 1);

	for (int i = 0; i < num_threads - 1; i++)
	{
		DrawerCommandQueue *queue = this;
		DrawerThread *thread = &threads[i];
		thread->core = i + 1;
		thread->num_cores = num_threads;
		thread->thread = std::thread([=]()
		{
			int run_id = 0;
			while (true)
			{
				// Wait until we are signalled to run:
				std::unique_lock<std::mutex> start_lock(queue->start_mutex);
				queue->start_condition.wait(start_lock, [&]() { return queue->run_id != run_id || queue->shutdown_flag; });
				if (queue->shutdown_flag)
					break;
				run_id = queue->run_id;
				start_lock.unlock();

				// Do the work:

				struct TryCatchData
				{
					DrawerCommandQueue *queue;
					DrawerThread *thread;
					size_t command_index;
				} data;

				data.queue = queue;
				data.thread = thread;
				data.command_index = 0;
				VectoredTryCatch(&data,
				[](void *data)
				{
					TryCatchData *d = (TryCatchData*)data;

					for (int pass = 0; pass < d->queue->num_passes; pass++)
					{
						d->thread->pass_start_y = pass * d->queue->rows_in_pass;
						d->thread->pass_end_y = (pass + 1) * d->queue->rows_in_pass;
						if (pass + 1 == d->queue->num_passes)
							d->thread->pass_end_y = MAX(d->thread->pass_end_y, MAXHEIGHT);

						size_t size = d->queue->active_commands.size();
						for (d->command_index = 0; d->command_index < size; d->command_index++)
						{
							auto &command = d->queue->active_commands[d->command_index];
							command->Execute(d->thread);
						}
					}
				},
				[](void *data)
				{
					TryCatchData *d = (TryCatchData*)data;
					ReportFatalError(d->queue->active_commands[d->command_index], true);
				});

				// Notify main thread that we finished:
				std::unique_lock<std::mutex> end_lock(queue->end_mutex);
				queue->finished_threads++;
				end_lock.unlock();
				queue->end_condition.notify_all();
			}
		});
	}
}

void DrawerCommandQueue::StopThreads()
{
	std::unique_lock<std::mutex> lock(start_mutex);
	shutdown_flag = true;
	lock.unlock();
	start_condition.notify_all();
	for (auto &thread : threads)
		thread.thread.join();
	threads.clear();
	lock.lock();
	shutdown_flag = false;
}

void DrawerCommandQueue::ReportFatalError(DrawerCommand *command, bool worker_thread)
{
	if (worker_thread)
	{
		std::unique_lock<std::mutex> end_lock(Instance()->end_mutex);
		if (Instance()->thread_error.IsEmpty())
			Instance()->thread_error = command->DebugInfo();
	}
	else
	{
		I_FatalError("Fatal drawer error: %s", command->DebugInfo().GetChars());
	}
}

#ifndef WIN32

void VectoredTryCatch(void *data, void(*tryBlock)(void *data), void(*catchBlock)(void *data))
{
	tryBlock(data);
}

#endif
