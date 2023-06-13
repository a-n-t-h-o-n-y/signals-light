#include <optional>
#include <stdexcept>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

#include <signals_light/signal.hpp>

TEST_CASE("Signal with no Slots", "[Signal]")
{
    SECTION("void Signal return type will return void")
    {
        {
            auto sig = sl::Signal<void()>{};
            REQUIRE(std::is_same_v<decltype(sig()), void>);
        }
        {
            auto sig = sl::Signal<void(int, char, float)>{};
            REQUIRE(std::is_same_v<decltype(sig(1, 'a', 3.14f)), void>);
        }
    }
    SECTION("non-void Signal return type will return std::nullopt")
    {
        {
            auto sig = sl::Signal<int()>{};
            REQUIRE(std::is_same_v<decltype(sig()), std::optional<int>>);
            REQUIRE(sig() == std::nullopt);
        }
        {
            auto sig = sl::Signal<int(int, char, float)>{};
            REQUIRE(std::is_same_v<decltype(sig(1, 'a', 3.14f)),
                                   std::optional<int>>);
            REQUIRE(sig(1, 'a', 3.14f) == std::nullopt);
        }
    }
}

TEST_CASE("Slot connect and disconnectd", "[Signal]")
{
    auto sig = sl::Signal<int()>{};

    SECTION(
        "Returns the value of the connected Slot when only one Slot connected")
    {
        REQUIRE(sig.is_empty());
        REQUIRE(sig.slot_count() == 0);
        REQUIRE(sig() == std::nullopt);

        auto const id = sig.connect([] { return 5; });
        REQUIRE(std::is_same_v<decltype(id), sl::Identifier const>);

        REQUIRE(!sig.is_empty());
        REQUIRE(sig.slot_count() == 1);

        auto const result = sig();
        REQUIRE(std::is_same_v<decltype(result), std::optional<int> const>);
        REQUIRE(result.has_value());
        REQUIRE(*result == 5);

        sig.disconnect(id);
        REQUIRE(sig.is_empty());
        REQUIRE(sig.slot_count() == 0);
        REQUIRE(sig() == std::nullopt);
    }

    SECTION(
        "Returns the return value of the last connected Slot when multiple "
        "Slots connected")
    {
        REQUIRE(sig.is_empty());
        REQUIRE(sig.slot_count() == 0);
        REQUIRE(sig() == std::nullopt);

        auto const id_1 = sig.connect([] { return 5; });
        REQUIRE(std::is_same_v<decltype(id_1), sl::Identifier const>);

        REQUIRE(!sig.is_empty());
        REQUIRE(sig.slot_count() == 1);

        auto const id_2 = sig.connect([] { return 3; });
        REQUIRE(std::is_same_v<decltype(id_2), sl::Identifier const>);

        REQUIRE(!sig.is_empty());
        REQUIRE(sig.slot_count() == 2);

        auto const result = sig();
        REQUIRE(std::is_same_v<decltype(result), std::optional<int> const>);
        REQUIRE(result.has_value());
        REQUIRE(*result == 3);

        SECTION("Disconnecting Slot 1 will still return Slot 2's return value.")
        {
            sig.disconnect(id_1);
            REQUIRE(!sig.is_empty());
            REQUIRE(sig.slot_count() == 1);

            auto const result_2 = sig();
            REQUIRE(
                std::is_same_v<decltype(result_2), std::optional<int> const>);
            REQUIRE(result_2.has_value());
            REQUIRE(*result_2 == 3);
        }

        SECTION("Disconnecting Slot 2 will return Slot 1's return value.")
        {
            sig.disconnect(id_2);
            REQUIRE(!sig.is_empty());
            REQUIRE(sig.slot_count() == 1);

            auto const result_2 = sig();
            REQUIRE(
                std::is_same_v<decltype(result_2), std::optional<int> const>);
            REQUIRE(result_2.has_value());
            REQUIRE(*result_2 == 5);
        }
    }
}

TEST_CASE("Emitting a Signal actually invokes it's Slot functions.", "[Signal]")
{
    auto sig = sl::Signal<int(int, int, int)>{};

    sig.connect([](int a, int b, int c) { return a + b + c; });
    REQUIRE(sig(5, 4, 3).value() == 12);

    sig.connect([](int a, int b, int c) { return a * b * c; });
    REQUIRE(sig(5, 4, 3).value() == 60);
}

TEST_CASE("Disconnecting with an invalid Identifier will throw", "[Signal]")
{
    auto sig      = sl::Signal<void()>{};
    auto const id = sig.connect([] {});
    REQUIRE_NOTHROW(sig.disconnect(id));

    REQUIRE_THROWS_AS(sig.disconnect(id), std::invalid_argument);
    REQUIRE_THROWS_AS(sig.disconnect(sl::Identifier{}), std::invalid_argument);
}

TEST_CASE("Lifetime Tracking", "[Signal]")
{
    SECTION(
        "Destroying the only tracked object on a Slot will disable that Slot")
    {
        auto sig  = sl::Signal<int()>{};
        auto slot = sl::Slot<int()>{[] { return 5; }};
        {
            auto life = sl::Lifetime{};
            slot.track(life);
            sig.connect(slot);
            REQUIRE(*sig() == 5);
        }
        REQUIRE(sig() == std::nullopt);
    }

    SECTION(
        "Lifetime tracking only works if track() is called before connect()")
    {
        auto sig  = sl::Signal<int()>{};
        auto slot = sl::Slot<int()>{[] { return 5; }};
        sig.connect(slot);
        {
            auto life = sl::Lifetime{};
            slot.track(life);
            REQUIRE(*sig() == 5);
        }
        REQUIRE(*sig() == 5);
    }

    SECTION(
        "Only a single tracked object needs to be destroyed to disable a Slot")
    {
        auto sig    = sl::Signal<int()>{};
        auto slot   = sl::Slot<int()>{[] { return 5; }};
        auto life_1 = sl::Lifetime{};
        slot.track(life_1);
        {
            auto life_2 = sl::Lifetime{};
            slot.track(life_2);
            sig.connect(slot);
            REQUIRE(*sig() == 5);
        }
        REQUIRE(sig() == std::nullopt);
    }

    SECTION("Non-disabled Slots will still be run")
    {
        auto sig    = sl::Signal<int()>{};
        auto slot_1 = sl::Slot<int()>{[] { return 5; }};
        sig.connect(slot_1);
        {
            auto slot_2 = sl::Slot<int()>{[] { return 3; }};
            auto life   = sl::Lifetime{};
            slot_2.track(life);
            sig.connect(slot_2);
            REQUIRE(*sig() == 3);
        }
        REQUIRE(*sig() == 5);
    }
}
