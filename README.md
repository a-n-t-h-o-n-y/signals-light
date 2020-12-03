# Signals Light 🪶

This is a Signals and Slots library. The `Signal` class is an observer type, it
can have `Slots`(functions) registered to it, and when the Signal is emitted,
all registered Slots are invoked. It is written in C++17 and is header only.

Slots can track the lifetime of particular objects and they will disable
themselves when one of the tracked objects is destroyed. This is useful if the
function you are registering with the Signal holds a reference to an object that
might be destroyed before the Signal is destroyed.

This library is 'light' in terms of the boost::Signal2 library, which is thread
safe, has ordered connections, and in general is more heavy-weight.

```cpp
#include <signals_light/signal.hpp>

{
    auto signal = sl::Signal<void(int)>{};
    signal.connect([](int i){ std::cout << i << '\n'; });
    signal.connect([](int i){ std::cout << i * 2 << '\n'; });

    signal(4);  // prints "4\n8\n" to standard output.
}
```

See the [design doc](docs/design.md) for more information.

## Getting Started

There is a `CMakeLists.txt` in the project root which contains a library target
`signals-light`. This can be imported to your CMake project with
`add_subdirectory`, then use `signals-light` with `target_link_libraries` in
your project to get access to the header(`#include <signals_light/signal.hpp>`).
