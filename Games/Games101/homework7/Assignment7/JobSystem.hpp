#ifndef JOBSYSTEM_H
#define JOBSYSTEM_H

#include <functional>
#include <pthread.h>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
//#include <sys/time.h>
//#include <time.h>
#include <unistd.h>

#include "global.hpp"

template<typename T, int SIZE>
struct Slot {

	~Slot() { destruct(); }
	T* construct(T* instance);
	void destruct();

	char storage[SIZE];
	std::atomic<uint64_t> turn = { 0 };
};

template<typename T, int SIZE>
T* Slot<T, SIZE>::construct(T* data) {
	return new (storage) T(*data);
}

template<typename T, int SIZE>
void Slot<T, SIZE>::destruct() {
	if (turn.load(std::memory_order_acquire) % 2 == 1) {
		((T*)storage)->~T();
	}
}

template<typename T, int BUFFER_SIZE, uint64_t MAX_LENGTH>
class RingBuffer {
public:
	void push(T* data);
	T* pop();

protected:
	uint64_t idx(uint64_t n) { return n % MAX_LENGTH; }
	uint64_t turn(uint64_t n) { return n / MAX_LENGTH; }

private:
	Slot<T, BUFFER_SIZE> m_slots[MAX_LENGTH];
	std::atomic<uint64_t> m_head = { 0 };
	std::atomic<uint64_t> m_tail = { 0 };
};

template<typename T, int BUFFER_SIZE, uint64_t MAX_LENGTH>
void RingBuffer<T, BUFFER_SIZE, MAX_LENGTH>::push(T* data) {
	uint64_t tail = m_tail.fetch_add(1, std::memory_order_seq_cst);
	uint64_t index = idx(tail);

	// different threads may concurrency here
	while (2 * turn(tail) != m_slots[index].turn.load(std::memory_order_acquire));
	m_slots[index].construct(data);
	m_slots[index].turn.fetch_add(1, std::memory_order_seq_cst);
}

template<typename T, int BUFFER_SIZE, uint64_t MAX_LENGTH>
T* RingBuffer<T, BUFFER_SIZE, MAX_LENGTH>::pop() {
	uint64_t head = m_head.load(std::memory_order_acquire);
	uint64_t index = idx(head);

	if (2 * turn(head) + 1 != m_slots[index].turn.load(std::memory_order_acquire)) {
		return nullptr;
	}

	// MUST copy, because some other thread may construct this slot in push soon later!
	T* ret = new T(*((T*)(m_slots[index].storage)));

	// different threads may concurrency here
	if (m_head.compare_exchange_strong(head, head + 1, std::memory_order_seq_cst)) {
		m_slots[index].turn.fetch_add(1, std::memory_order_seq_cst);
		return ret;
	}

	return nullptr;
}

typedef void* JobFunc(void*);
typedef void JobCallback(void*, void*);

struct alignas(64) Job {

	Job() : jobFunc(nullptr), callback(nullptr), data(nullptr) {}
	Job(JobFunc* _jobFunc, JobCallback* _callback, void* _data) : jobFunc(_jobFunc), callback(_callback), data(_data) {}

	JobFunc* jobFunc;
	JobCallback* callback;
	void* data;
};

class JobSystem {
public:
	JobSystem();
	~JobSystem();
	void createJob(JobFunc* jobFunc, JobCallback* callback, void* data);
	static void* thread_loop(void* self);
    void loop();
	void exit();

private:
	RingBuffer<Job, 128, MAX_JOB_NUM> m_jobs;
	pthread_t m_threads[MAX_THREAD_NUM];
	bool exited = { false };
	//pthread_cond_t cv;
	//pthread_mutex_t mtx;
};

JobSystem::JobSystem() {
    //pthread_cond_init(&cv, NULL);
    //pthread_mutex_init(&mtx, NULL);
	for (int i = 0; i < MAX_THREAD_NUM; ++i) {
        pthread_create(&m_threads[i], NULL, JobSystem::thread_loop, (void*)this);
	}
}

JobSystem::~JobSystem() {
	if (!exited) exit();
}

void JobSystem::exit() {
	exited = true;
	for (int i = 0; i < MAX_THREAD_NUM; ++i)
        pthread_join(m_threads[i],NULL);
    //pthread_mutex_destroy(&mtx);
    //pthread_cond_destroy(&cv);
}

void JobSystem::createJob(JobFunc* jobFunc, JobCallback* callback, void* data) {
	Job job(jobFunc, callback, data);
	m_jobs.push(&job);
    //pthread_mutex_lock(&mtx)
    //pthread_cond_signal(&cv);
    //pthread_mutex_unlock(&mtx);
}

void* JobSystem::thread_loop(void* self) {
    JobSystem *jobSystem = (JobSystem*)self;
    jobSystem->loop();
    return nullptr;
}

void JobSystem::loop() {
	while (!exited) {
		Job* job = m_jobs.pop();
		if (job) {
			job->callback(job->jobFunc(job->data), job);
		}
		else {
            usleep(5000); // sleep 5ms
            /*
			pthread_mutex_lock(&mtx);
            struct timespec abstime;
            struct timeval now;
            long timeout_ms = 10; // wait time 10ms
            gettimeofday(&now, NULL);
            long nsec = now.tv_usec * 1000 + (timeout_ms % 1000) * 1000000;
            abstime.tv_sec = now.tv_sec + nsec / 1000000000 + timeout_ms / 1000;
            abstime.tv_nsec = nsec % 1000000000;
			pthread_cond_timedwait(&cv, &mtx, &abstime);
            pthread_mutex_unlock(&mtx);
			//printf("wake thread: %lld\n", thread_id);
            */
		}
	}
}


#endif //JOBSYSTEM_H