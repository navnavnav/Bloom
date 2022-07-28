#pragma once

#include <csignal>
#include <cassert>

#include "SyncSafe.hpp"

namespace Bloom
{
    enum class ThreadState
    {
        UNINITIALISED,
        READY,
        STOPPED,
        STARTING,
        SHUTDOWN_INITIATED,
    };

    class Thread
    {
    public:
        Thread() = default;
        virtual ~Thread() = default;

        Thread(const Thread& other) = delete;
        Thread(Thread&& other) = delete;

        Thread& operator = (const Thread& other) = delete;
        Thread& operator = (Thread&& other) = delete;

        virtual ThreadState getThreadState() {
            auto lock = this->state.acquireLock();
            return this->state.getValue();
        }

    protected:
        virtual void setThreadState(ThreadState state) {
            this->state.setValue(state);
        }

        /**
         * Disables signal interrupts on current thread.
         */
        void blockAllSignals() {
            sigset_t set = {};
            sigfillset(&set);
            sigprocmask(SIG_SETMASK, &set, NULL);
        }

        void setName(const std::string& name) {
            // POSIX thread names cannot exceed 16 characters, including the terminating null byte.
            assert(name.size() <= 15);

            pthread_setname_np(pthread_self(), name.c_str());
        }

    private:
        SyncSafe<ThreadState> state = SyncSafe<ThreadState>(ThreadState::UNINITIALISED);
    };
}
