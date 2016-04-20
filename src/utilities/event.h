#ifndef CYNNY_SIMPLE_EVENT_H
#define CYNNY_SIMPLE_EVENT_H

#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>


namespace cynny {
namespace utilities {

/**
 * @brief Offers a simple interface to a synchronizing mechanism.
 *
 * An Event, at any time of its life, has one of the following states:
 *
 * 1. *set*
 * 2. *not set*
 *
 * When the state of an Event is "set", the Event::wait_event member function returns immediatly.
 * When the state of an Event is "not set", the Event::wait_event function blocks the execution of the thread from
 * which it is invoked; then, if the state of the Event gets turned into "set" by any other thread calling the
 * Event::set_event member function, the thread blocked inside Event::wait_event is awaken and continues its execution.
 */
class Event {
public:
    /**
     * @brief Create a new event, setting its state to \p set.
     *
     * The default is an *unset* event, i.e. execution will block at Event::wait_event.
     *
     * @param set Tells whether the event will be initially set
     */
    Event(bool set = false);

    /**
     * @brief set_event sets the state of the event to *set*, i.e. unlock the event.
     *
     * Successive calls to Event::wait_event() won't block the execution, unless another call to Event::set_event() has been performed.
     */
    void set_event();

    /**
     * @brief reset_event sets the state of the event to *not set*, i.e. the event will block at Event::wait_event since now on.
     */
    void reset_event();

    /**
     * @brief wait_event blocks the caller thread until the state of the event gets set.
     *
     * If the event is already in *set* state, this function returns immediately.
     */
    void wait_event();

private:
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic_bool is_set;
};

} // namespace utilities
} // namespace cynny

#endif //SMUGGLER_EVENT_H
