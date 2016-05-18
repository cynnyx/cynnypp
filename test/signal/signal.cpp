#include <signal/bus.hpp>
#include <signal/signal.hpp>
#include "catch.hpp"


using namespace cynny::cynnypp::signal;


TEST_CASE( "Signal", "To be honest: signal, bus, everything..." ) {
    struct EventA {
        EventA(unsigned int x, unsigned int y): value{x+y} { }
        unsigned int value;
    };

    struct EventB { };

    struct MyListener {
        MyListener(unsigned int *AP, unsigned int *BP): APtr{AP}, BPtr{BP} { }
        void receive(const EventA &) { (*APtr)++; }
        void listen(const EventB &) { (*BPtr)++; }
        void reset() { *APtr = 0; *BPtr = 0; }
        unsigned int *APtr{nullptr};
        unsigned int *BPtr{nullptr};
    };

    using MyBus = Bus<EventA, EventB>;

    unsigned int ACnt = 0;
    unsigned int BCnt = 0;

    SECTION("Add/Remove/Emit") {
        std::shared_ptr<MyListener> listener =
                std::make_shared<MyListener>(&ACnt, &BCnt);
        MyBus bus{};

        listener->reset();
        bus.publish<EventA>(40, 2);
        bus.publish<EventB>();

        REQUIRE(bus.size() == 0);
        REQUIRE(ACnt == 0);
        REQUIRE(BCnt == 0);

        listener->reset();
        bus.add<EventA>(listener);
        bus.publish<EventA>(40, 2);
        bus.publish<EventB>();

        REQUIRE(bus.size() == 1);
        REQUIRE(ACnt == 1);
        REQUIRE(BCnt == 0);

        listener->reset();
        bus.remove<EventA>(listener);
        bus.add<EventA>(listener);
        bus.add<EventA>(listener);
        bus.add<EventA>(listener);
        bus.add<EventA>(listener);
        bus.publish<EventA>(40, 2);
        bus.publish<EventB>();

        REQUIRE(bus.size() == 1);
        REQUIRE(ACnt == 1);
        REQUIRE(BCnt == 0);

        listener->reset();
        bus.add<EventB, MyListener, &MyListener::listen>(listener);
        bus.publish<EventA>(40, 2);
        bus.publish<EventB>();

        REQUIRE(bus.size() == 2);
        REQUIRE(ACnt == 1);
        REQUIRE(BCnt == 1);

        listener->reset();
        bus.remove<EventA>(listener);
        bus.publish<EventA>(40, 2);
        bus.publish<EventB>();

        REQUIRE(bus.size() == 1);
        REQUIRE(ACnt == 0);
        REQUIRE(BCnt == 1);

        listener->reset();
        bus.remove<EventB, MyListener, &MyListener::listen>(listener);
        bus.publish<EventA>(40, 2);
        bus.publish<EventB>();

        REQUIRE(bus.size() == 0);
        REQUIRE(ACnt == 0);
        REQUIRE(BCnt == 0);
    }

    SECTION("Expired listeners") {
        std::shared_ptr<MyListener> listener =
                std::make_shared<MyListener>(&ACnt, &BCnt);
        MyBus bus{};

        listener->reset();
        bus.add<EventA>(listener);
        bus.publish<EventA>(40, 2);
        bus.publish<EventB>();

        REQUIRE(bus.size() == 1);
        REQUIRE(ACnt == 1);
        REQUIRE(BCnt == 0);

        listener->reset();
        listener = nullptr;

        REQUIRE(bus.size() == 1);

        bus.publish<EventA>(40, 2);
        bus.publish<EventB>();

        REQUIRE(bus.size() == 0);
        REQUIRE(ACnt == 0);
        REQUIRE(BCnt == 0);
    }
}
