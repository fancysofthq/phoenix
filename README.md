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

TODO: A major language version is targeted at mainstream machines.
For example, when the QPU (Quantum Compute Unit) becomes ubiquitous, the language shall include primitives to operate with Quantum Bits.
Shifting to a new computing capatibilities requires bump of the language major version.

A new major language version implies the need to rewrite old code to use new features.
For example, if there was no built-in threading routines in Onyx 1.0, existing code would rely on some third-party thread management.
Once threading is built into language, there is no need to maintain those third-party libraries for the new language version anymore.
If it was a minor release instead, a threading library would only support, say, Onyx till version 1.5 (where the feature is added), but not higher, which would be confusing.
Instead, the library continues support for 1.* language versions, but drops support for 2.*.

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

## Type system

`:` is the type operator, which ensures a type both in runtime and during compilation.
For example, `let x : Int32 = (42 + 1) : Int32`.
`:?` evaluates to `true` or `false` based on whether the restriction is satisfied; it's considered a runtime expression.

Builtin types:

* `Int*` -- a signed binary integer with variable bitsize;
* `UInt*` -- an unsigned binary integer with variable bitsize;
* `Float*` -- an IEEE 754 binary floating point number with variable bitsize;
* `Char` -- a 4-byte Unicode character;
* `Byte` -- alias to `UInt8`;
* `Bool` -- either `true` or `false`;
* `Void` -- equivalent to `void` and `$void`;
* `Pointer<T> : T*` -- a typed pointer with unsafe access;
* `Variant<*T>` -- contains one or more types;
* `Function<safety (A) -> R>` -- a function type of zero size;
* `Ratio<T>` -- an integer ratio, e.g. `1/2 : Ratio<Int32>`;
* `Range<T>` -- a numeric range, e.g. `1..10 : Range<Int32>`;

* `Imaginary<T>` -- a single imaginary number, e.g. `1j`;
* `Hypercomplex<T, D>` -- a hypercomplex number;
* `Complex<T> : Hypercomplex<T, 1>` -- a complex number, e.g. `2 + 1j`;
* `Quaternion<T> : Hypercomplex<T, 4>` -- a quaternion;

* `Array<T, Z>` -- an on-stack container of fixed length, e.g. `[1, 2] : Array<Int32, 2> : Int32[2]`;
* `Tuple<*T>` -- an anonymous struct with indexed and named elements, e.g. `("foo", bar: 42) : Tuple<[0]: String, bar: Int32>`;
<!-- * `Map<K, V, Z>` -- an on-stack associative array of fixed size, e.g. `["foo" => 42] : Map<CString, Int32, 1>`; -->

* `Vector<T, Z>` -- an on-stack numeric vector of fixed size, e.g. `|1, 2, 3, 4| == %|1 2 3 4| : Vector<Int32, 4> : Tensor<Int32, 4>`;
* `Matrix<T, Z>` -- an on-stack square matrix of fixed size, e.g. `|[1, 2], [3, 4]| == %|[1 2][3 4]| : Matrix<Int32, 2> : Tensor<Int32, 2, 2>`
* `Tensor<T, *D>` -- an on-stack arbitrary-dimensioned tensor, e.g. `%|[[1 2][3 4]][[5 6][7 8]]| : Tensor<Int32, 2, 2, 2>`;

Std classes:

* `String` -- a mutable UTF-8 encoded string;
* `List<T>` -- a dynamic array, e.g. `List.new([1, 2]) : List<Int32>`;
* `LinkedList<T>`
* `DoublyLinkedList<T>`
* `Stack<T>` -- a LIFO queue;
* `Queue<T>` -- a FIFO queue;
* `Deque<T>` -- a double-ended queue;
* `Box<T>` -- wraps a value in a class instance;
* `Promise<T>` -- resolves at some later point of execution;
* `Lambda<safety [C](*A) ~> R>` -- a lambda expression;

Note that set, map and tree data types are not in the standard library, as they imply a wide list of implementations and require some hashing mechanism, which is hard to pick a standard one from.

Custom `trait`, `struct`, `class`, `enum`, `unit` and `annotation` types may be defined.

### Scalars

`Int*`, `UInt*`, `Float*`, `Char`, `Bool`, `Void`, `Pointer<T>` and `CString` types are scalar: they are stored on the stack and are passed by value.

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

#### `String`

A `String` maintains a mutable dynamically-allocated UTF-8 encoded null-terminated multi-byte string (NTMBS) buffer.

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

## Development

Never use C++ `class` declarations.
Just don't.

## Usage

`--nostd` option disables classes, threading, IO, memory modules, leaving only those features not requiring libc.

## Implementation

### ARC

Classes in Fancy Onyx compiler enjoy ARC with syncronous mark-and-sweep alhorithm (see [Wikipedia article](https://en.wikipedia.org/wiki/Reference_counting#Dealing_with_reference_cycles) to avoid dangling cyclic references.

```nx
decl class Post

class User
  final posts : List<Post>
end

class Post
  let author : User
end

# user.src += 1 == 1
# user.posts.wrc += 1 == 1
final user = User.new()

# post.src += 1 == 1
final post = Post.new()

# user.wrc += 1 == 1
post.author = user

# post.wrc += 1 == 1
user.posts.add(post)

# post.src -= 1 == 0 -> check attrs
#   author.src > 0 -> stop
@finalize(post)

# user.src -= 1 == 0 -> check attrs
#   user.posts.src == 0 -> check elements (need to declare a way to do so)
#     user.posts[0].src == 0 -> check attrs
#     user.posts[0].author === user -> mark(user.posts[0].author)
#     user.posts[0] has all attrs marked -> mark(user.posts[0])
#   user.posts has all elements marked -> mark(user.posts)
# user has all attrs and self marked -> finalize(user) (unmarks user in the beginning)
#   user.posts.wrc -= 1 == 0 -> finalize(user.posts)
#     user.posts[0].wrc -= 1 == 0 -> finalize(user.posts[0])
#       user.posts[0].author.wrc -= 1 == 0, but user is not marked -> skip user
#     free(user.posts[0])
#   free(user.posts)
# free(user)
@finalize(user)
```
