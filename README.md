# Signals Light 🪶

This is a Signals and Slots library. The `Signal` class is an observer type, it
can have `Slots`(functions) registered to it, and when the Signal is emitted,
all registered Slots are invoked. It is written in C++17 and is header only.

Slots can track the lifetime of particular objects and they will disable
themselves when one of the tracked objects is destroyed. This is useful if the
function you are registering with the Signal holds a reference to an object that
might be destroyed before the Signal is destroyed.

This library is 'light' in terms of the boost::Signal2 library, which is thread
safe, has ordered connections, and in general is more feature-heavy.

```cpp
#include <signals_light/signal.hpp>

{
    auto signal = sl::Signal<void(int)>{};
    signal.connect([](int i){ std::cout << i << '\n'; });
    signal.connect([](int i){ std::cout << i * 2 << '\n'; });

    signal(4);  // prints "4\n8\n" to standard output.
}
```

## Getting Started

The `CMakeLists.txt` file will give you a `signals-light` library target to link against in your own project.

- `#include <signals_light/signal.hpp>`
- namespace `sl`.

## CMake Options

- `SIGNALS_LIGHT_BUILD_TESTS` (default: `OFF`): builds test targets under `tests/`.
- With default options, configuring `signals-light` does not fetch test dependencies (such as `zzz`).
- To enable tests: `cmake -S . -B build -DSIGNALS_LIGHT_BUILD_TESTS=ON` (and keep `BUILD_TESTING=ON`).
