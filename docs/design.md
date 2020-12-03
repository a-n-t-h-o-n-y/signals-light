# Signals Light Design Document

## Name

signals-light🪶

## Namespace

`namespace sl {}`

## Files

`include/signals_light/signal.hpp`

## Description

This is a Signals and Slots library. The `Signal` class is an observer type, it
can have `Slots`(functions) registered to it, and when the Signal is emitted,
all registered Slots are invoked.

Slots can track the lifetime of particular objects and they will disable
themselves when one of the tracked objects is destroyed. This is useful if the
function you are registering with the Signal holds a reference to an object that
might be destroyed before the Signal is destroyed.

This library is 'light' in terms of the boost::Signal2 library, which is thread
safe, has ordered connections, and in general is more heavy-weight.

## Rationale

The current signals library is too heavy to have many signals within each
Widget, it is the largest source of stack size within the Widget class. If the
size of Widget can be brought down, more Widgets can fit in a single cache line,
potentially speeding up access to Widgets and speeding up UI code in CPPurses.

Most of the features of the current signals library are not being utilized,
thread safety is already guaranteed by the event system, therefore Signals do
not need to worry about Widget data being changed out from under them. Grouping
connected Slots and ordering is not used, as far as I know, execution order of
Slots should not matter, Slots should be independent, if they are dependent,
then they can be combined into a single slot.

Current `Signal` size: **168 Bytes**

Expected light `Signal` size: **24 Bytes**

A reduction of **144 Bytes**

There are a potential minimum of 36 Signals in a Widget, this number will go
down without the move event, to 34 Widgets, once all filter signals are added.
Extended Widgets may add more signals on top of this number. That is **6,048
Bytes** solely for Signals, this will go down to **864 Bytes**, a reduction of
**5,184 Bytes** per Widget. Depending on how many Widgets are in a single
application, this is considerable.

An L1 cache line is around 256 kB, in the current state that holds 42 Widgets,
with the new design that is 296 Widgets. These will not be the only objects in
the cache, but it leaves more space for others.

## Interfaces

### `class Lifetime_observer`

This is esentailly a wrapper around `std::weak_ptr<void>` that only allows
access to the `is_expired()` method. Any non-const `std::weak_ptr` can reset the
owned object, so using a `std::weak_ptr` is not ideal for the below
`Lifetime_tracker` class since it would allow a client to invalidate the
invariant that the `Lifetime_tracker` object should not be expired before it is
destroyed.

```cpp
/// Provides a const view of a std::weak_ptr, providing an is_expired() check.
/** Always contains a valid lifetime, unless moved from. */
class Lifetime_observer {
   public:
    Lifetime_observer() = delete;

    // Note: The below overloads are provided so a Lifetime_observer can be
    // implicitly constructed from std::..._ptr objects in a function parameter.

    /// Construct a view of the lifetime of the object pointed to by \p p.
    /** Throws std::invalid_argument if \p p is nullptr. */
    template <typename T>
    Lifetime_observer(std::weak_ptr<T> p);

    /// Construct a view of the lifetime of the object pointed to by \p p.
    /** Throws std::invalid_argument if \p p is nullptr. */
    template <typename T>
    Lifetime_observer(std::shared_ptr<T> p);

    /// Creates a new observer to the same object.
    Lifetime_observer(Lifetime_observer const&) = default;

    /// Moving will leave the moved from Lifetime_observer in an undefined state
    Lifetime_observer(Lifetime_observer&&) = default;

    /// Overwrites *this to track the same lifetime as the rhs.
    auto operator=(Lifetime_observer const&) -> Lifetime_observer& = default;

    /// Moving will leave the moved from Lifetime_observer in an undefined state
    auto operator=(Lifetime_observer&&) -> Lifetime_observer& = default;

   public:
    /// Return true if the tracked object has been deleted.
    auto is_expired() const -> bool;

    /// Return a unique id that is associated with the tracked object
    /** Returns zero if the tracked object has been destroyed. */
    auto get_id() const -> std::uintptr_t;

   private:
    std::weak_ptr<void> handle_;
};

```

### `class Lifetime`

This class creates an object that can easily have its lifetime tracked by a
`Lifetime_observer`. This is esentially a wrapper around a
`std::shared_ptr<...>` that holds some arbitrary data that can be tracked via
the `std::weak_ptr` type.

This can be placed as a member inside any class to give it the ability to be
tracked by a `Slot`, for instance, if the slot function will be holding a
reference to some object, it'd be a good idea to track the lifetime of the
object that is referenced to avoid dangling references.

`sizeof(Lifetime) == 16 Bytes`

```cpp
/// A class to keep track of an object's lifetime.
/** A Lifetime_observer can check if a Lifetime has ended. */
class Lifetime {
   public:
    /// Create a new lifetime to track.
    Lifetime();

    /// Create a new lifetime to track.
    /** Tracking does not split across multiple Lifetime objects. */
    Lifetime(Lifetime const&);

    /// Transfers the lifetime tracking to the new instance.
    /** Existing trackers will now track the newly constructed lifetime. */
    Lifetime(Lifetime&&) = default;

    /// Create a new lifetime to track, destroying the existing lifetime.
    /** Tracking does not split across multiple Lifetime objects. */
    auto operator=(Lifetime const& rhs) -> Lifetime&;

    /// Transfers the lifetime of the moved from object to *this.
    /** Anything tracking the moved from object will now be tracking *this. */
    auto operator=(Lifetime&& rhs) -> Lifetime&;

   public:
    /// Return a Lifetime_observer, to check if *this has been destroyed.
    /** The returned object is valid even after *this i\ destroyed. Even though
     *  this should never throw an exception, if a memory allocation fails at
     *  construction, it might not throw std::bad_alloc, returning nullptr. */
    auto track() const -> Lifetime_observer;

   private:
    std::shared_ptr<bool> life_;
};
```

### `class Slot`

A `Slot` is a wrapper around a `std::function` object that can condition its
invocation on the lifetime of other objects. If any object that is tracked by
the `Slot` is destroyed, then the call operator fails by throwing an exception.
The `is_expired()` and `slot_function()` methods can be used in tandem to avoid
exceptions.

`sizeof(Slot) == 56 Bytes`

```cpp
template <typename Signature>
class Slot;

/// An invocable type with object lifetime tracking.
/** An object of this type always holds a valid std::function object.
 *  The call operator will throw if any of the tracked objects are destroyed. */
template <typename R, typename... Args>
class Slot<R(Args...)> {
   public:
    using Signature_t = R(Args...);
    using Function_t  = std::function<Signature_t>;

    /// Exception thrown if any object being tracked has been destroyed.
    class Expired {};

   public:
    Slot() = delete;

    Slot(std::nullptr_t) = delete;

    /// Construct a Slot with the slot function \p f and no tracked objects.
    /** Throws std::invalid_argument if the given function is empty. */
    template <typename F>
    Slot(F f);

    /// Copies the slot function and the tracked list into the new Slot.
    /** Tracked objects by *this are tracked by the new Slot instance. */
    Slot(Slot const&) = default;

    /// Moves the slot function and the tracked list into the new Slot.
    Slot(Slot&&) = default;

    /// Copies the slot function and the tracked object reference container.
    auto operator=(Slot const&) -> Slot& = default;

    /// Moves the slot function and the tracked object reference container.
    auto operator=(Slot&&) -> Slot& = default;

   public:
    /// Track any object owned by a std::shared_ptr<...>.
    /** Returns *this. Tracking the same item multiple times is not checked. */
    auto track(Lifetime_observer const& x) -> Slot&;

    /// Track Lifetime object. Convenience.
    /** Returns *this. Tracking the same item multiple times is not checked. */
    auto track(Lifetime const& x) -> Slot&;

    /// Remove the observed object from the tracked objects list.
    /** Removes on Lifetime_observer::get_id() equality. Returns *this. Throws
     *  std::invalid_argument if \p x is not being tracked by *this. */
    auto untrack(Lifetime_observer const& x) -> Slot&;

    /// Remove the given Lifetime object from the tracked objects list.
    /** Removes on Lifetime_observer::get_id() equality. Returns *this. Throws
     *  std::invalid_argument if \p x is not being tracked by *this.*/
    auto untrack(Lifetime const& x) -> Slot&;

    /// Invokes the internally held function
    /** Throws Expired if any tracked object has been destroyed. */
    template <typename... Arguments>
    auto operator()(Arguments&&... args) const -> R;

    /// Check whether any of the tracked lifetimes has expired.
    /** Returns true if no lifetimes are being tracked. */
    auto is_expired() const -> bool;

    /// Return a const reference to the internal std::function.
    /** Always returns a valid Function_t object that can be called. */
    auto slot_function() const -> Function_t const&;

   private:
    Function_t f_;
    std::vector<Lifetime_observer> observers_;
};

```

### `class Identifier`

```cpp
/// Objects of this type can be unique and compared against other identifiers.
class Identifier {
   public:
    using Underlying_int = std::uint32_t;

   public:
    /// Construct the initial value.
    Identifier();

    /// Generate the next identifier value, this is an increment.
    static auto next(Identifier x) -> Identifier;

   public:
    /// Return true if both Identifiers have the same internal value.
    friend auto operator==(Identifier x, Identifier y) -> bool;

    /// Return true if both Identifiers do not have the same internal value.
    friend auto operator!=(Identifier x, Identifier y) -> bool;

   private:
    /// Used by next(...).
    Identifier(Underlying_int value);

   private:
    Underlying_int value_;
};
```

### `class Signal`

A `Signal<R(Args...)>` object acts as an observer, notifying registered `Slots`
whenever the `Signal` is emitted. The `Signal` is able to pass on arguments to
its registered `Slots`, and the return value of emitting a `Signal` is a
`std::optional<R>` containing the result of the last `Slot` called.

`sizeof(Signal) == 24 Bytes`

```cpp
template <typename Signature>
class Signal;

/// An observer type that calls registered callbacks(Slots) when emitted.
template <typename R, typename... Args>
class Signal<R(Args...)> {
   public:
    using Signature_t = R(Args...);
    using Emit_result_t =
        std::conditional_t<std::is_same_v<void, R>, void, std::optional<R>>;

   public:
    /// Construct a Signal with no connected Slots.
    Signal() = default;

    /// Create a Signal with the same Slots connected, and the same Identifiers.
    Signal(Signal const&) = default;

    /// Move the connected Slots from the existing Signal to the new one.
    /** The moved from Signal will be empty afterwards. */
    Signal(Signal&&) = default;

    /// Overwrite the existing Signal with the Slots and Identifiers of the rhs.
    auto operator=(Signal const&) -> Signal& = default;

    /// Overwrite the existing Signal with the Slots and Identifiers of the rhs.
    /** The moved from Signal will be empty afterwards. */
    auto operator=(Signal&&) -> Signal& = default;

   public:
    /// Invoke all non-expired Slots.
    /** Returns the return value of the last connected Slot or std::nullopt if
     *  none. Expired Slots are ignored, rather than throwing an exception. */
    auto emit(Args const&... args) const -> Emit_result_t;

    /// Alternative notation for Signal::emit.
    auto operator()(Args const&... args) const -> Emit_result_t;

    /// Register a Slot with *this, will be invoked when *this is emitted.
    /** Returns a unique Identifier, to be used with Signal::disconnect. */
    auto connect(Slot<Signature_t> s) -> Identifier;

    /// Removes and returns the Slot associated with the given Identifier.
    /** Throws std::invalid_argument if no connected Slot is found with id. */
    auto disconnect(Identifier id) -> Slot<Signature_t>;

    /// Return the number of connected Slots.
    auto slot_count() const -> std::size_t;

    /// Return true if there are no connected Slots.
    auto is_empty() const -> bool;

   private:
    std::vector<std::pair<Identifier, Slot<R(Args...)>>> slots_;
};
```

## Test Code

```cpp
#include <signals_light/signal.hpp>

// Empty Signal returns std::nullopt or void.
{
    sl::Signal<int()> si;
    sl::Signal<void()> sv;
    assert(si() == std::nullopt);
    sv();
}

// Single Slot connect and disconnect.
{
    sl::Signal<int()> s;
    sl::Identifier id = s.connect([]{ return 5; });
    assert(!s.is_empty());
    assert(s.slot_count() == 1)

    std::optional<int> result = s();
    assert(result.is_valid() && *result == 5);
    s.disconnect(id);
    assert(s.is_empty());
    assert(s.slot_count() == 0);
    assert(s() == std::nullopt);
}

// Multiple Slots connected and disconnected.
{
    sl::Signal<int(char, int, bool)> s;
    sl::Identifier sum_id = s.connect([](char c, int i, bool b) { return c + i + b; });
    sl::Identifier product_id = s.connect([](char c, int i, bool b) { return c * i * b; });
    sl::Identifier difference_id = s.connect([](char c, int i, bool b) { return c - i - b; }); 

    assert(s.slot_count() == 3);
    assert(*s(5, 1, false) == 4);

    sl::Slot<int(char, int, bool)> difference_slot = s.disconnect(difference_id);
    assert(difference_slot(5, 1, false) == 4);
    assert(s.slot_count() == 2);
    assert(*s(4, 3, true) == 12);

    sl::Slot<int(char, int, bool)> sum_slot = s.disconnect(sum_id);
    assert(sum_slot(5, 5, false) == 10);
    assert(s.slot_count() == 1);
    assert(*s(5, 5, true) == 25);

    sl::Slot<int(char, int, bool)> produce_slot = s.disconnect(product_id);
    assert(product_slot(5, 5, true) == 25);
    assert(s.slot_count() == 0);
    assert(s.is_empty());
    assert(s(5, 5, true) == std::nullopt);
}

// void return type Slot/Signal.
{
    sl::Signal<void(int)> s;
    assert(std::is_same_v<decltype(s(5)), void>);
    s.connect([](int){});
    assert(std::is_same_v<decltype(s(5)), void>);
}

// Throwing on disconnect of invalid Identifier.
{
    sl::Signal<void()> s;
    auto id = s.connect([]{});
    auto slot = s.disconnect(id);
    expect_throw(s.disconnect(id), std::invalid_argument);
    expect_throw(s.disconnect(sl::Identifier{}), std::invalid_argument);
}

//  Lifetime tracking of single object.
{
    sl::Signal<> s;
    sl::Slot<int()> slot = []{ return 5; };
    auto life = std::make_unique<Lifetime>();
    slot.track(*life);
    auto id = s.connect(slot);
    assert(*s() == 5);
    life.reset();
    assert(s() == std::nullopt);
}

// Lifetime tracking of multiple objects.
{
    sl::Signal<int()> s;
    sl::Slot<int()> slot_5 = []{ return 5; }
    sl::Slot<int()> slot_3 = []{ return 3; }

    auto life_1 = std::make_unique<Lifetime>();
    auto life_2 = std::make_unique<Lifetime>();
    auto life_3 = std::make_unique<Lifetime>();
    auto life_4 = std::make_unique<Lifetime>();
    auto life_5 = std::make_unique<Lifetime>();
    auto life_6 = std::make_unique<Lifetime>();

    slot_5.track(*life_1);
    slot_5.track(*life_2);
    slot_5.track(*life_3);
    slot_5.track(*life_4);
    slot_3.track(*life_5);
    slot_3.track(*life_6);

    s.connect(slot_5);
    s.connect(slot_3);

    assert(*s() == 3);
    life_1.reset();
    assert(*s() == 3);
    life_5.reset();
    assert(s() == std::nullopt);
    life_6.reset();
    assert(s() == std::nullopt);
    life_2.reset();
    assert(s() == std::nullopt);
    life_3.reset();
    assert(s() == std::nullopt);
    life_4.reset();
    assert(s() == std::nullopt);
}
```
