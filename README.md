# Onyx

Modern general-purpose weakly typed language for development of arbitrary programs running on mainstream machine CPUs, such as modern Linux, Windows, MacOS, iOS and Android devices.

## Features:

* Generics;
* ARC;
* Built-in concurrency and parallelism;
* Modern built-in types, including half floats and tenzors;
* Easy C interoperability;
* Pointer arithmetics;
* Safety modifiers;
* Interface, class, struct, enum and unit types;
* Convenient module import system;

## Design goals

Onyx is designed to run on modern machines where MCUs, FPUs and even NPUs are ubiquitous and C standard library is implemented.
This allows to safely assume that programs written in Onyx can make use of dynamic memory, floating point operations, tensor arithmetic and common OS routines such as signals and events.

## Examples

Examples use mixed syntax to demonstrate its flexibility.

```nx
# Import similar to that in EcmaScript 2016+.
import { puts } from "io"

def sum(a, b) {
  a + b
}

let result = sum(1, 2)
puts("Result: #{result}") # -> Result: 3
```

```nx
# Struct members are public by default.
export default struct Point
  let x, y : Float

  # A struct is const by default, so are its members.
  def length() -> (this.x ^ 2 + this.y ^ 2).sqrt()
end

# The by-default-public constructor with
# arguments in the order of declaration.
final point = const Point(3, 4)

if (point.length() != 5) then throw "Something's wrong!"
```

```nx
export interface HasFullName {
  decl full_name() : StringBuf
}

# Class members are private by default.
# Also, a class type is mutable by default.
export class User implements HasFullName
  public get first_name, last_name : StringBuf

  # The default constructor is private, thus have to define a custom one.
  public static def new(*args, **kwargs) -> self(*args, **kwargs)

  impl ~HasFullName:full_name()
    return this.first_name + " " + this.last_name
  end
end

final printer = (obj : HasFullName) -> puts(obj.full_name())
final user = User.new(first_name: "John", last_name: "Doe")
printer(user) # -> John Doe
```

## Intrinsics

Intrinsics are executed in runtime.

```nx
# The lambda argument implicitly inherits the containing scope's safety.
decl @async(lambda : Lambda<(Void) : T>) : Promise<T>

# The lambda is argument implicitly fragile.
decl @parallel(lambda : Lambda<fragile (Void) : T>) : Promise<T>

decl @await(promise : Promise<T>) : T
decl @await(promises : Enumerable<Promise<T>>) : T

decl @mutexlock(klass : T) : Void
decl @mutexunlock(klass : T) : Void

decl @upcast(descendant : T) : U

# Throws if the *expr* is falsey.
decl @assert(expr) : Void
```

## Type system

`:` is the type operator, which ensures a type both in runtime and during compilation.
For example, `let x : Int32 = (42 + 1) : Int32`.

Built-in scalar types:

* `Int*` -- a signed binary integer with variable bitsize;
* `UInt*` -- an unsigned binary integer with variable bitsize;
* `Float*` -- a binary floating point number with variable bitsize;
* `Dec*` -- a decimal floating point number with variable bitsize;
* `Char` -- a 4-byte Unicode character;
* `Byte` -- alias to `UInt8`;
* `Bool` -- either `true` or `false`;
* `Void` -- always equals to `void`;
* `Pointer<T> : T*` -- a typed pointer with unsafe access;
* `Variant<T>` -- contains one or more types;

Built-in structs:

* `Ratio<T>` -- an integer ratio, e.g. `1/2 : Ratio<Int32>`;
* `Range<T>` -- a numeric range, e.g. `1..10 : Range<Int32>`;
* `Array<T, Z>` -- an on-stack container of fixed length, e.g. `[1, 2] : Array<Int32, 2> : Int32[2]`;
* `Tuple<*T>` -- an anonymous ordered struct, e.g. `("foo", 42) : Tuple<CString, Int32>`;
* `NamedTuple<**T>` -- an anonymous named struct, e.g. `(foo: 42, bar: "baz") : NamedTuple<foo: Int, bar: CString>`;
* `Map<K, V, Z>` -- an on-stack associative array of fixed size, e.g. `["foo" => 42] : Map<CString, Int32, 1>`;
* `Vector<T, Z>` -- an on-stack numeric vector of fixed size, e.g. `|1, 2, 3, 4| == %|1 2 3 4| : Vector<Int32, 4> : Tensor<Int32, 4>`;
* `Matrix<T, Z>` -- an on-stack square matrix of fixed size, e.g. `|[1, 2], [3, 4]| == %|[1 2][3 4]| : Matrix<Int32, 2> : Tensor<Int32, 2, 2>`
* `Tensor<T, *D>` -- an on-stack arbitrary-dimensioned tensor, e.g. `%|[[1 2][3 4]][[5 6][7 8]]| : Tensor<Int32, 2, 2, 2>`;

Built-in classes:

* `String` -- a mutable UTF-8 encoded string;
* `List<T>` -- a dynamic container, e.g. `List.new([1, 2]) : List<Int32>`;
* `Set<T>` -- a list with hash-defined uniquiness of its elements;
* `Hash<K, V>` -- a dynamic hashtable, e.g. `Hash.new(["foo" => 42]) : Hash<CString, Int32>`;
* `Box<T>` -- wraps scalar values;
* `Promise<T>` -- resolves at some later point of execution;
* `Lambda<safety [Closure](*Args) -> Return>` -- a lambda expression;

Custom `namespace`, `interface`, `struct`, `class`, `enum`, `unit` and `annotation` types may be defined.

### Scalars

`Int*`, `UInt*`, `Float*`, `Char`, `Bool`, `Void`, `Pointer<T>` and `CString` types are scalar: they are stored on the stack and are passed by value.

A scalar type doesn't have any fields, thus it can not have a mutability modifier, i.e. neither `mut Int32` nor `const Int32` is legal.

#### `Int`

`Int*` is a binary signed integer with built-in specializations of `Int8`, `Int16`, `Int32`, `Int64` and `Int128`.

Usually, an untyped whole number type defaults to `Int32`.

The literal symbol for signed integers is `i(size = 32)`, e.g. `%i[1 2 3] == %i32[1 2 3] == [1, 2, 3] : Int32[3]`.

#### `UInt`

`UInt*` is a binary unsigned integer with built-in specializations of `UInt8`, `UInt16`, `UInt32`, `UInt64` and `UInt128`.

The `Byte` type always aliases to `UInt8`.

The literal symbol for unsigned integers is `u(size = 32)`, e.g. `%u[1 2 3] == %u32[1 2 3] == [1u, 2u, 3u] : UInt32[3]`.

#### `Float`

`Float*` is a binary floating point number with built-in specializations of `Float16` to `Float128`, matching IEEE 754 `binary16` to `binary128`.

Usually, an untyped number with a decimal point type defaults to `Float32`.

The literal symbol for binary floats is `f(size = 32)`, e.g. `1.0 == 1.0f == 1.0f32 : Float32`, `%f[1 2 3] == %f32[1 2 3] == [1.0, 2.0, 3.0] : Float32[3]`.

#### `Dec`

`Dec*` is a decimal floating point number with built-in specializations of `Dec32` to `Dec128`, matching IEEE 754 `decimal32` to `decimal128`.

The literal symbol for decimal floats is `d(size = 32)`, e.g. `%d[1 2 3] == %d32[1 2 3] == [1d, 2d, 3d] : Dec32[3]`.

```nx
# Decimal floats allow precise decimal arithmetics.
@assert(1.1f + 2.2f != 3.3f)
@assert(1.1d + 2.2d == 3.3d)
```

#### `Char`

A `Char` is a single Unicode character fitting in 4 bytes.
It therefore can be safely cast to `UInt32` (but not vice versa due to some codepoints being invalid).

A char literal allows explicit Unicode codepoint in hexadecimal format, e.g. `'a' == '\u61'`.

The literal symbol for chars is `c`, e.g. `%c[hi !] == ['h', 'i', ' ', '!'] : Char[4]`.
A magic char string literal can be easily converted to a desired integer format by appending its literal symbol after `c`, for example: `%cu[Hello] == %u[72 101 108 108 111] : UInt32[5]`.

A C char literal obeys the according C rules.

```nx
$'a' : $char      # Usually one byte
$L'🌎' : $wchar_t # A wide character
```

#### `Bool`

The `Bool` internals are undefined, but it can be safely cast to any integer, resulting in either `0` or `1`.

`true` and `false` are the only `Bool` literals.

#### `Void`

The `Void` doesn't occupy any space in memory and indicates that there is no actual value.
It is commonly used in variants to indicate the null-ish option.

`Void` is a unit type, thus `Void` type and `void` literal are interchangeable.
However, it's common to use the former as a type restriction, and the latter as a "value".

#### `Pointer`

A `Pointer<T>` instance can be safely cast to any C pointer and vice-versa.
For example, both `user_ptr as $void*` and `void_ptr as Pointer<User>` are safe.
A `Pointer<T>` instance access is itself unsafe, however, e.g. `unsafe! user_ptr[0]`.

### Namespaces

A `namespace`, `interface`, `struct`, `class`, `enum` and `unit` types are all namespaces.

A namespace is accessed using the `::` delimeter.
A namespace in the top-level may have (and usually has) its preceding namespace delimeter omitted.
For example, it's usually `StringBuf`, not `::StringBuf`.

A `namespace` (thus any type) may be declared `private`, which would make it inaccessible from the outside.

### Interfaces

An `interface` type declares a list of `public` `instance` methods for a type to implement.

An interface member is implcitly `const` by default.

An `interface`, `struct`, `class`, `enum` or `unit` type may implement one or more `interface`s.

### Structs

A `struct` instance is stored on the stack (if possible) and is always passed by value.

A `struct` instance member (variable or method) and its constructor is `public` by default.

A `struct` type instance is implicitly `mut`: you can modify its fields.
However, a struct's method is always implicitly `const`, because a struct instance is passed by value to it.
That said, a mutability modifier only affects access to the struct's fields.
You can declare a struct instance explicitly `const`: this would disallow modifying its fields.

It is impossible to directly cast a `const` struct to a `mut` struct.
However, one can always copy-assign a `const T` struct to a `mut T` variable: `final mutable : T = constant : const T`.

```nx
final point1 = Point(1, 2)
point1.x = 3 # Ok, `point1` is implicitly `mut Point`

final point2 : const Point = point1
# point2.x = 3 # Panic! `point2` is `const`
```

A `struct` may only extend another `struct`.
A `struct` marked `abstract` can not be neither initialized nor safely cast to, but can contain unimplemented `decl`s.
Extending a struct guarantees that the extended struct's variables are located before the extending one's, which allows to safely down-cast to any non-abstract ancestor.
For example:

```nx
abstract struct Point
  decl length() : Float64
end

struct Point1 extends Point
  let x : Float64
  impl length() -> this.x
end

# The order between `y` and `z` is undefined. However,
# they both are guaranteed to be placed after `x`.
struct Point3 extends Point1
  let y, z : Float64

  # The implementation is inherited from `Point1`,
  # therefore it needs to be reimplemented.
  reimpl length() -> (this.x ^ 2 + this.y ^ 2).sqrt()
end

let p3 = Point3(1, 2, 3)
let p1 = p3 as Point1 # This is safe
```

A struct may contain a class field.

### Classes

A `class` instance is stored in dynamic memory, passed by reference and ARC'ed.

A `class` member and constructor is `private` by default.

A `class` type itself is implicitly `mut`, and it can not be changed during declaration.
However, its instance methods are still implicitly `const` by default.
A class type reference may be marked `const` (or `mut`) explicitly, e.g. `final list = const List([1, 2])`; this would disallow calling `mut` methods and modify the class'es fields.

```nx
final list = List<Int32>()
list.push(1) # Ok
```

```nx
final list = const List<Int32>()
# list.push(1) # Panic! Can not call a `mut` method on a `const` instance
```

It is impossible to directly cast a `const` class to a `mut` class.
However, one can always find a way to clone a `const Klass` into a `mut Klass` variable.
For example:

```nx
class List<T>
  const def clone() : self # `mut self` is implied
    final new = self.new(this.capacity)
    this.each([new](e) -> fragile! new.push(e))
    return new
  end
end
```

A `class` may extend another `class` or `struct`.
Extending a struct injects its fields into the class'es.
A `class` marked `abstract` can not be neither initialized nor safely cast to, but can contain unimplemented `decl`s.

A `class` contains RTTI, which allows to have, for example, an array of a class descendants without type erasure.
However, this disables the ability to statically cast a class descendant to its ancestor.
A class instance can be still dinamically cast:

```nx
class Animal;

class Dog {
  public static def new -> return self()
}

final dog = Dog.new
# final animal = dog as Animal # Panic! Static casting is not allowed
final animal = @cast<Animal>(dog) # Ok, RTTI in action!
dog = @cast<Dog>(animal) # Ok, but may throw if this is not actually a dog
```

A class header always contains a thread-safe mutex.
A fragile class method call or accessing a class field, from within a `threadsafe` context, is implicitly wrapped into the mutex.

```nx
final list = List.new([1, 2, 3])

threadsafe! do
  list.push(4) # Would implicitly wrap the `fragile def push` call into the mutex
  @assert(list.size == 4) # Would also wrap the field access
end
```

The mutex can be directly locked using the mutex lock intrinsics:

```nx
# From now on, the `list` instance is locked.
@mutexlock(list)

fragile! do
  # Calls the fragile method without implcitly wrapping it
  # into mutex, because the context is `fragile`.
  list.push(42)

  @assert(list.size == 4) # Ditto
end

# Note: A mutex is implicitly unlocked
# upon the containing scope termination.
# @mutexunlock(list)
```

#### `String`

A `String` maintains a mutable dynamically-allocated UTF-8 encoded null-terminated multi-byte string buffer (NTMBS).

A string literal in Onyx resolves to a `String`, i.e. a dynamically allocated class instance.

```nx
final str = "Hello, "
str += "🌎!" # Would re-allocate the underlying buffer

@assert(str.bytesize == 12)  # Including the null byte
@assert(str.length == 9)     # In Unicode chars, excluding the null byte

unsafe! $puts(str.pointer : $`const char*`) # => Hello, 🌎! (null-terminated)
```

A C string literal is a pointer to an immutable NTMBS.

```nx
final str = $L"Hello, 🌎!" : $`const wchar_t*`
unsafe! $fputws(str, $stdout)
```

#### `Box`

#### `Variant`

#### `Promise`

#### `Lambda`

### Enums

A `enum` is a collection of named values similar to those in C.

A `enum` type may have `static` variables and functions, and `instance` functions; all `public` by default.
Therefore, a `enum` type may implement an `interface`.

A enum value may be shortcut with a symbol (e.g. `:bar` instead of `Foo::Bar`) if unambigous.

```nx
enum Foo
  val Bar
  val Baz

  def to_s()
    switch (this)
    when :bar then "Bar"
    when :baz then "Baz"
    end
  end
end

final foo = Foo::Bar
@assert(foo.to_s() == "Bar")
```

### Units

A `unit` type is also a unit type instance, therefore referenced simply as *unit*.

A `unit` doesn't occupy any memory (but it would take space in a `Variant`).

A `unit` may have `static` members, `public` `static` by default.
However, these may be accessed both as instance (`.member`) and static (`::member`) members.

A `unit` constructor returns itself, i.e. the type.

A `unit` may implement one or more `interface`s.

```nx
unit Singleton
  let counter = 0
end

@assert(Singleton == Singleton())
Singleton.counter += 1
@assert(Singleton::counter == 1)
```

### Generic types

An `interface`, `struct`, `class` or `unit` type may be declared generic.

## Syntax

### Variables

A non-final variable may be declared using the `let` keyword.
In that case, it's value may be directly updated.

A final variable may be declared using the `final` keyword.
A final variable value shall not be directly updated.
However, it doesn't imply `const` of the underlying value.
For example, a final struct field can still be updated, because structs are `mut` by default.
It's the struct itself which can not be rewritten.

```nx
let point = Point(1, 2)
point.x = 3 # Ok
point = Point(4, 5) # Ok
```

```nx
final point = Point(1, 2)
point.x = 3 # Ok, the struct is mutable
# point = Point(4, 5) # Panic! Can not overwrite a final variable value
```

```nx
final point = const Point(1, 2)
# point.x = 3 # Panic! Can not mutate a constant struct
# point = Point(4, 5) # Panic! Can not overwrite a final variable value
```

### Functions

A function may be `decl`ared, `impl`emented or `def`ined, whereas the latter implies both `decl` and `impl`.
That said, it'd be a panic to attempt to implement an undeclared function, or to implement an already implemented function specialization.
However, it's legal to re-declare the same prototype multiple times.

An already implemented function may be re-implemented (or *overriden*) using the `reimpl` keyword.

An overload counts as a distinct implementation.
This also applies to generic specializations.

```nx
decl sum(a, b : T) forall T

# Both implementations are specialization-specific overloads.
impl sum(a, b : Int) { return a.add(b); }
impl sum(a, b : CString) { return a.append(b); }
```

A function argument is implicitly `final` by default.
An argument can be marked `let` explicitly, which would allow to change it within the function.
A `let`-`final` difference doesn't count as an overload.

```nx
def foo(bar)
  # bar = 42 # Panic! Can not overwrite a `final` variable
end

reimpl foo(let bar)
  bar = 43 # Ok
end
```

Note that different argument type mutability doesn't count as an overload either.

Functions may be declared within other functions and lambdas.
However, they won't support closures.

Any function or lambda can be called using the `@async` intrinsic.

```nx
def calc() -> return 42
final promise : Promise<Int> = @async(calc()) # Doesn't block
final result = @await(promise) # Blocks, may throw
final result = promise.resolve() # Blocks, may throw
@await(Promise.all([promise])) : Promise<Int>
@await(Promise.any([promise])) : Promise<Int>

# TODO: May work with generics, so that `List<Promise<T>>.all() : List<T>`.
# @await([promise].all()) # Wow.
```

### Lambdas

A lambda is an executable blocks of code.
It may have explicitly closured variables from the outer scope.
It shall also have its arguments size known in advance.

A lambda is a class instance, thus ARC'ed.

```nx
final c = 3

final sum = [c](a, b : Int32) -> a + b + c

# # A (slightly) longer version:
# final sum = [final c = c](a : Int32, b : Int32) : Int32 -> { return a + b + c; }

@assert(sum(1, 2) == 6)

c = 4
@assert(sum.call(1, 2) == 6) # Note how a copy of `c` is closured
```

A lambda literal creates a lambda instance with fragile context by default, unless created with `@parallel`: in that case, it'd contain a threadsafe context.

```nx
fragile! do
  final l1 = (() -> {
    # Here, the context is fragile.
  })

  final l2 = @parallel(() -> do
    # Here, the context is threadsafe.
  end)
end
```

A forward-type-restricted lambda literal works as expected, providing the desired safety within the lambda body.
A lambda function argument has the minimum of the declaration's safety by default.

```nx
# The *lambda* argument lambda safety is implicitly `fragile+`,
# inheriting from the function declaration's.
#
# A longer variant would be:
#
# ```
# fragile def foo(lambda : <fragile+ [](a : Int32, b : Int32) -> Int32>)
# ```
#
fragile def foo(lambda : <(a, b : Int32) ->>) {
  # A `fragile+` lambda can be safely called
  # from within a `fragile` context.
  return lambda(1, 2)
}
```

### Branches

#### `if`

`if` is similar to such in C, but with flexible branch body syntax.
The condition expression doesn't require parentheses neither.
Note that `then` is acceptable anywhere, but is required only for inline single-expression branch bodies.

```nx
if cond?() then a() else { b(); }
if (cond?()) { a(); } else b()

if cond?()
  a()
else {
  b()
}

if (cond?()) {
  a()
} else
  b()
end
```

A compiler narrows down a value type if possible.
This is applicable to any other branching (i.e. `switch`).

```nx
final foo = (rand?() ? 42 : "bar") : Int | CString

if foo.is_a?(Int)
  foo : Int # Ok, narrowed
else
  foo : CString # Ditto
end
```

An `if` statement's latest evaluated expression is deemed to be the statement's return value.
A `convey` instruction may be used to convey a value from a branch early.

```nx
final result : Bool = if rand?() then true else convey false
```

Labels may be used to determine which statement to `convey` the value from.

```nx
# Would never equal to `2`.
final result = if~%ifa rand?()
  if rand?()
    convey~%ifa 1
  else
    convey 2
  end

  convey 3
else
  convey 4
end
```

### `switch`

A `switch` is exhaustive and may only be used on numeric-like `enum`, `Variant`, `Bool`, `Int`, `Pointer` and `Char` values.

Note that two identical `CString`s are not guaranteed to point to the same address, thus switching on a `CString` instance it is not allowed; it shall be cast to a `Pointer` beforeahead.

Similar to `if`, a `switch` statement narrows down the switched value type.

```nx
final foo = (rand?() ? 42 : "bar") : Int | CString

switch (foo)
when Int then foo : Int
when CString
  foo : CString
end
```

```nx
enum Foo
  val Bar
  val Baz
  val Qux
end

final foo : Foo = rand?() ? :bar : :baz

switch (foo)
when :bar { @assert(foo.is?(:bar)); }
when :baz then @assert(foo.is?(:baz))
else
  puts("Something else")
end
```

Similar to `if`, a `switch` statement latest evaluated expression is deemed to be the statement's return value.

### Loops

#### `while`

A `while` loop executes its body while the condition evaluates to `true`.

The `break` instruction may be used to break from a loop.
An optional argument may be passed to a `break` instruction, which would be deemed the returned value of the loop.
Note that `convey` instruction doesn't break the loop.

```nx
final result : Bool = while (true)
  if rand?()
    break true
  end
end
```

The `continue` instruction skips the currect evaluation of the body of the loop and jumps to the condition evaluation once again.

Labels may be used to control which outside loop to `break` or `continue` from.

```nx
final result = while~%loopa true
  while true
    if rand?()
      break~%loopa 42 # Would break from the loop labeled `%loopa`
    else
      break # Would break from the down-second loop
    end
  end
end
```

## C Interop

Onyx allows easy C interoperation using the `extern` keyword.
A single C expression or an entier block of C code shall follow the keyword.
The C code would be parsed to find declarations, so that those declarations would be accessible from within the containing file.

```nx
extern #include "stdio.h"
final message = "Hello, world!\0"
unsafe! $puts(&message)
```

```sh
$ nx main.nx -I/usr/include -L/usr/lib -lc
```

A static `nx` binary would not need to link `libc` and `libm`, as their code is already included into the binary.
For example, an `nx` compiler would need to use `puts` to output progress.
That said, definitions and includes (e.g. `extern #include "stdio.h"`) from the standard C library within Onyx files would not require neither `-lc` nor `-lm`.
The `nx` binary would also have C standard library lookup logic implemented internally so that, for example on Linux, `-I/usr/include` and `-L/usr/lib` flags may be omitted.

Considering the above paragraph, the example above could be simply run with:

```sh
$ nx main.nx
```

Native C declarations (i.e. those not requiring any `include`s) are always accessible.
For example, `$int`.

Any other C declaration requires the file to include the header explicitly.
That means, for example, that you have to `extern #include "stdio.h"` in **every** Onyx source file calling `$puts`.

Note that the example above doesn't require to include the whole header file.
Instead, an explicit function declaration may be used:

```nx
extern void puts(char*);
final message = "Hello, world!\0"
unsafe! $puts(&message)
```

### C interop roadmap

*Inline* in current contexts means "inside an Onyx file".
*External* means reading from an external C source file.

* [ ] Inline C function declarations;
* [ ] Inline variable declarations;
* [ ] Inline C preprocessor directives:
  * [ ] `#define`, `#ifdef`, `#endif`;
* [ ] Inline blocks of C code;
  * [ ] C-style comments within those blocks;
* [ ] Parsing external C includes without function definitions;
* [ ] Inline and external C function definitions, compiled by the Onyx compiler.

## Development

Never use C++ `class` declarations.
Just don't.
