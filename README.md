# Signals Light 🪶

This is a Signals and Slots library. The `Signal` class is an observer type, it
can have `Slots`(functions) registered to it, and when the Signal is emitted,
all registered Slots are invoked. It is written in C++17.

Slots can track the lifetime of particular objects and they will disable
themselves when one of the tracked objects is destroyed. This is useful if the
function you are registering with the Signal holds a reference to an object that
might be destroyed before the Signal is destroyed.

This library is 'light' in terms of the boost::Signal2 library, which is thread
safe, has ordered connections, and in general is more heavy-weight.

See the [design doc](docs/design.md) for more information.