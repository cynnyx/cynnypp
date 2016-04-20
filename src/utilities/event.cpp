#include "event.h"

namespace cynny{
namespace cynnypp{
namespace utilities{

Event::Event(bool set /* = false */)
        : is_set{set}
{}

void Event::set_event()
{
    std::lock_guard<std::mutex> lck{mtx};
    is_set = true;
    cv.notify_all();
}

void Event::reset_event()
{
    std::lock_guard<std::mutex> lck{mtx};
    is_set = false;
}

void Event::wait_event()
{
    std::unique_lock<std::mutex> lck{mtx};
    if( is_set )
        return;

    // the lambda is a condition to handle spurious wake-up's of the cv
    auto s = std::cref(is_set);
    cv.wait(lck, [s]()->bool { return s.get();} );
}


} // namespace utilities
} // namespace cynny
}