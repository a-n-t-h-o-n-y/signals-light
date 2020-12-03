#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>

#include <catch2/catch.hpp>

#include <signals_light/signal.hpp>
#include <type_traits>

TEST_CASE("Empty Signal returns std::nullopt or void", "[Signal]")
{
    sl::Signal<int()> si;
    sl::Signal<void()> sv;
    CHECK(si() == std::nullopt);
    static_assert(std::is_same_v<decltype(sv()), void>);
}

TEST_CASE("Single Slot connect and disconnect", "[Signal]")
{
    sl::Signal<int()> s;
    sl::Identifier id = s.connect([] { return 5; });
    CHECK(!s.is_empty());
    CHECK(s.slot_count() == 1);

    std::optional<int> result = s();
    REQUIRE(result.has_value());
    CHECK(*result == 5);
    s.disconnect(id);
    CHECK(s.is_empty());
    CHECK(s.slot_count() == 0);
    CHECK(s() == std::nullopt);
}

TEST_CASE("Multiple Slots connect and disconnect", "[Signal]")
{
    sl::Signal<int(char, int, bool)> s;
    sl::Identifier sum_id =
        s.connect([](char c, int i, bool b) { return c + i + b; });
    sl::Identifier product_id =
        s.connect([](char c, int i, bool b) { return c * i * b; });
    sl::Identifier difference_id =
        s.connect([](char c, int i, bool b) { return c - i - b; });

    CHECK(s.slot_count() == 3);
    CHECK(*s(5, 1, false) == 4);

    sl::Slot<int(char, int, bool)> difference_slot =
        s.disconnect(difference_id);
    CHECK(difference_slot(5, 1, false) == 4);
    CHECK(s.slot_count() == 2);
    CHECK(*s(4, 3, true) == 12);

    sl::Slot<int(char, int, bool)> sum_slot = s.disconnect(sum_id);
    CHECK(sum_slot(5, 5, false) == 10);
    CHECK(s.slot_count() == 1);
    CHECK(*s(5, 5, true) == 25);

    sl::Slot<int(char, int, bool)> product_slot = s.disconnect(product_id);
    CHECK(product_slot(5, 5, true) == 25);
    CHECK(s.slot_count() == 0);
    CHECK(s.is_empty());
    CHECK(s(5, 5, true) == std::nullopt);
}

TEST_CASE("void return type", "[Signal]")
{
    sl::Signal<void(int)> s;
    static_assert(std::is_same_v<decltype(s(5)), void>);
    s.connect([](int) {});
    static_assert(std::is_same_v<decltype(s(5)), void>);
}

TEST_CASE("Throw on disconnect of invalid Identifier", "[Signal]")
{
    sl::Signal<void()> s;
    auto id   = s.connect([] {});
    auto slot = s.disconnect(id);
    CHECK_THROWS_AS(s.disconnect(id), std::invalid_argument);
    CHECK_THROWS_AS(s.disconnect(sl::Identifier{}), std::invalid_argument);
}

TEST_CASE("Lifetime tracking of single object", "[Signal]")
{
    sl::Signal<int()> s;
    sl::Slot<int()> slot = [] { return 5; };
    auto life            = std::make_unique<sl::Lifetime>();
    slot.track(*life);
    auto id = s.connect(slot);
    CHECK(*s() == 5);
    life.reset();
    CHECK(s() == std::nullopt);
    s.disconnect(id);
    CHECK(s() == std::nullopt);
}

TEST_CASE("Lifetime tracking of multiple objects", "[Signal]")
{
    sl::Signal<int()> s;
    sl::Slot<int()> slot_5 = [] { return 5; };
    sl::Slot<int()> slot_3 = [] { return 3; };

    auto life_1 = std::make_unique<sl::Lifetime>();
    auto life_2 = std::make_unique<sl::Lifetime>();
    auto life_3 = std::make_unique<sl::Lifetime>();
    auto life_4 = std::make_unique<sl::Lifetime>();
    auto life_5 = std::make_unique<sl::Lifetime>();
    auto life_6 = std::make_unique<sl::Lifetime>();

    slot_5.track(*life_1);
    slot_5.track(*life_2);
    slot_5.track(*life_3);
    slot_5.track(*life_4);
    slot_3.track(*life_5);
    slot_3.track(*life_6);

    s.connect(slot_5);
    s.connect(slot_3);

    CHECK(*s() == 3);
    life_1.reset();
    CHECK(*s() == 3);
    life_5.reset();
    CHECK(s() == std::nullopt);
    life_6.reset();
    CHECK(s() == std::nullopt);
    life_2.reset();
    CHECK(s() == std::nullopt);
    life_3.reset();
    CHECK(s() == std::nullopt);
    life_4.reset();
    CHECK(s() == std::nullopt);
}
