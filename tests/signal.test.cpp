#include <optional>
#include <stdexcept>
#include <type_traits>

#define TEST_MAIN
#include <zzz/test.hpp>

#include <signals_light/signal.hpp>

TEST(signal_no_slots)
{
    {  // void Signal return type will return void
        {
            auto sig = sl::Signal<void()>{};
            ASSERT((std::is_same_v<decltype(sig()), void>));
        }
        {
            auto sig = sl::Signal<void(int, char, float)>{};
            ASSERT((std::is_same_v<decltype(sig(1, 'a', 3.14f)), void>));
        }
    }

    {  // non-void Signal return type will return std::nullopt
        {
            auto sig = sl::Signal<int()>{};
            ASSERT((std::is_same_v<decltype(sig()), std::optional<int>>));
            ASSERT(sig() == std::nullopt);
        }
        {
            auto sig = sl::Signal<int(int, char, float)>{};
            ASSERT((std::is_same_v<decltype(sig(1, 'a', 3.14f)),
                                   std::optional<int>>));
            ASSERT(sig(1, 'a', 3.14f) == std::nullopt);
        }
    }
}

TEST(slot_connect_and_disconnect)
{
    {  // Returns the value of the connected Slot when only one Slot connected
        auto sig = sl::Signal<int()>{};
        ASSERT(sig.is_empty());
        ASSERT(sig.slot_count() == 0);
        ASSERT(sig() == std::nullopt);

        auto const id = sig.connect([] { return 5; });
        ASSERT((std::is_same_v<decltype(id), sl::Identifier const>));

        ASSERT(!sig.is_empty());
        ASSERT(sig.slot_count() == 1);

        auto const result = sig();
        ASSERT((std::is_same_v<decltype(result), std::optional<int> const>));
        ASSERT(result.has_value());
        ASSERT(*result == 5);

        sig.disconnect(id);
        ASSERT(sig.is_empty());
        ASSERT(sig.slot_count() == 0);
        ASSERT(sig() == std::nullopt);
    }

    {  // Last connected Slot's return value is returned.
        auto sig        = sl::Signal<int()>{};
        auto const id_1 = sig.connect([] { return 5; });
        ASSERT((std::is_same_v<decltype(id_1), sl::Identifier const>));

        sig.connect([] { return 3; });
        ASSERT(sig.slot_count() == 2);

        auto const result = sig();
        ASSERT((std::is_same_v<decltype(result), std::optional<int> const>));
        ASSERT(result.has_value());
        ASSERT(*result == 3);

        // Disconnecting Slot 1; will still return Slot 2's return value.
        sig.disconnect(id_1);
        ASSERT(!sig.is_empty());
        ASSERT(sig.slot_count() == 1);

        auto const result_2 = sig();
        ASSERT((std::is_same_v<decltype(result_2), std::optional<int> const>));
        ASSERT(result_2.has_value());
        ASSERT(*result_2 == 3);
    }

    {  // Last connected Slot's return value is returned.
        auto sig = sl::Signal<int()>{};
        sig.connect([] { return 5; });

        auto const id_2 = sig.connect([] { return 3; });
        ASSERT(sig.slot_count() == 2);

        auto const result = sig();
        ASSERT((std::is_same_v<decltype(result), std::optional<int> const>));
        ASSERT(result.has_value());
        ASSERT(*result == 3);

        // Disconnecting Slot 2; will now return Slot 1's return value.
        sig.disconnect(id_2);
        ASSERT(!sig.is_empty());
        ASSERT(sig.slot_count() == 1);

        auto const result_2 = sig();
        ASSERT((std::is_same_v<decltype(result_2), std::optional<int> const>));
        ASSERT(result_2.has_value());
        ASSERT(*result_2 == 5);
    }
}

TEST(emitting_signal_invokes_slot_fns)
{
    auto sig = sl::Signal<int(int, int, int)>{};

    sig.connect([](int a, int b, int c) { return a + b + c; });
    ASSERT(sig(5, 4, 3).value() == 12);

    sig.connect([](int a, int b, int c) { return a * b * c; });
    ASSERT(sig(5, 4, 3).value() == 60);
}

TEST(disconnecting_with_invalid_id_will_throw)
{
    auto sig      = sl::Signal<void()>{};
    auto const id = sig.connect([] {});
    sig.disconnect(id);

    ASSERT_THROWS(sig.disconnect(id), std::invalid_argument);
    ASSERT_THROWS(sig.disconnect(sl::Identifier{}), std::invalid_argument);
}

TEST(lifetime_tracking)
{
    {  // Destroying the only tracked object on a Slot will disable that Slot.
        auto sig  = sl::Signal<int()>{};
        auto slot = sl::Slot<int()>{[] { return 5; }};
        {
            auto life = sl::Lifetime{};
            slot.track(life);
            sig.connect(slot);
            ASSERT(*sig() == 5);
        }
        ASSERT(sig() == std::nullopt);
    }

    {  // Lifetime tracking only works if track() is called before connect().
        auto sig  = sl::Signal<int()>{};
        auto slot = sl::Slot<int()>{[] { return 5; }};
        sig.connect(slot);
        {
            auto life = sl::Lifetime{};
            slot.track(life);
            ASSERT(*sig() == 5);
        }
        ASSERT(*sig() == 5);
    }

    {  // Only a single tracked object needs to be destroyed to disable a Slot.
        auto sig    = sl::Signal<int()>{};
        auto slot   = sl::Slot<int()>{[] { return 5; }};
        auto life_1 = sl::Lifetime{};
        slot.track(life_1);
        {
            auto life_2 = sl::Lifetime{};
            slot.track(life_2);
            sig.connect(slot);
            ASSERT(*sig() == 5);
        }
        ASSERT(sig() == std::nullopt);
    }

    {  // Non-disabled Slots will still be run.
        auto sig    = sl::Signal<int()>{};
        auto slot_1 = sl::Slot<int()>{[] { return 5; }};
        sig.connect(slot_1);
        {
            auto slot_2 = sl::Slot<int()>{[] { return 3; }};
            auto life   = sl::Lifetime{};
            slot_2.track(life);
            sig.connect(slot_2);
            ASSERT(*sig() == 3);
        }
        ASSERT(*sig() == 5);
    }
}