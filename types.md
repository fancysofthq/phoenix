```nx
struct Point : Drawable, Entity; # This is how inheritance works
```

Can only use an implemented type in runtime.
For C interoperability, C structs are preferable, because they won't allow further expansion with succeeding impls.

```nx
decl struct Type<Type: T, Size: Z : \uint> {}

forall <T : Int, Z> impl Type<T, Z> {
  let foo : Int32 # OK
}

forall <T : Float, Z> impl Type<T, Z> {
  let foo : Float32 # OK
}

forall <T : Int32, Z> impl Type<T, Z> {
  # let foo : Int64 # Panic! Already declared with another type
  let bar : Int32   # OK
}
```

```nx
forall <TokenT: T : Token> def class Parser<LexerT: L : Lexer<T>> {
  final lexer = L()                  # These are not implemented yet
  let latest_token : T?              #
  final children = Std::List<self>() # Ditto for `self`
}

# Foralls are not used as template args
# upon reference (no need to pass `TokenT`).
class MyParser : Parser<MyLexer> {
  # TokenT             # `TokenT` is undeclared type here
  MyLexer::TokenT # Can do this
  MyToken         # Or this
  self::TokenT    # Or this
}
```

```nx
# NOTE: Returned type must be concrete.
decl to<Target: T : Number>() : T

impl to() : Float32 {
  # Target # `Target` is set to `Float32`, but cannot be used here
  that::Target : Float32
  that : Function<to>
  that() # Equivalent of writing `to()`
}

forall <T : Float> impl to<T>() : T {
  assert(\{{ nx["T"] < nx["Float"] }})
  T # Can use `T` directly
}
```

`that` refers to the function superdeclaration and has `Function<Function>` type.

A `Function` instance is similar to `Void`: it can't be assigned, but can be passed as an argument.
Its template argument is the function superdeclaration.
`that : that::Function : that::Function::Function  : Function<that>`.

Note to a compiler implementor:

We know all the sites where a `Function` instance is called, that means that we can search for implementations for all possible arg type combination options.

```
decl foo<T>(arg : T)

foo(42)    # Would specialize the `foo<Int32>` implementation
foo("bar") # Would specialize the `foo<String>` implementation
```

Template argument overloading is prohibited.

```nx
decl struct Foo<T>;
# decl struct Foo;            # Panic! Already declared with template args `<T>`
# decl struct Foo<T : Int>;   # Panic! Ditto
forall <T : Int> impl Foo<T>; # Can do this instead
```

Akin to functions, would require implementation for the narrowest matching (e.g. `Int` for `Int32`) declaration.

When implementing a narrower overload, variables are stacked upon the "higher type" implementations'.
In this example, `Number` specialization variables would be layed out first, followed by either `Int`'s or `Float`'s.

Implementing with exactly the same arguments (or the same type when zero arity) is legal, but it would require the `reimpl` keyword.
(TODO: Provide a way to erase previous `impl` functions and/or variables?)
In that case, older fields and functions are preserved.
The fields are stacked in the order of declaration.
Can `reimpl` or `unimpl` functions within a `reimpl`.

Redefining a field is illegal, because the memory layout matters.

```nx
decl struct Foo<T>;

impl Foo<T : Number> {
  let x : Int32 = 42
}

impl Foo<T : Int> {
  # let x : Int32 = 42 # Panic! Already defined
  let y : Int32 = 42
}

# Would be equivalent to:
#

def `Foo<T : Number>` {
  let x : Int32 = 69
}

def `Foo<T : Int>` {
  let x : Int32 = 42
  let y : Int32 = 69
}
```

Using `forall <T : U> impl Foo<T>;` would leave `Foo<U>` unimplemented,
because the implementation is meant for all `T`s matching `U`, i.e. it doesn't implement anything on its own.
It would be prepended to every matching implementation.
`Foo<T : U>` could still be implemented separetely, though (with `impl Foo<U>;`).
In that case, the `forall` contents would be prepended, because `T : U` is a particular case matching `forall <T : U>`.

Conflicting functions would require `unimpl` to solve the conflict, actual for traits.

`Void` panics if ever accessed any property.
It's the absolute void, it can not be interacted with.
It can not be assigned, but it can be passed as an argument.
An unexisting type behaves similiarly, but `Void` is a known type.
`Void`, being a defined type, really acts as an unexisting one. Spooky!

A nested type doesn't know anything about the outer type template arguments.
However, declaring or implementing a type within a partial implementation is possible, akin to functions.

```nx
struct Foo<T> {
  struct Bar {
    # T # Panic!
  }

  struct Baz<T> {
    T # OK, access the `Baz`'s `T`, different from `Foo`'s
  }
}

impl Foo::Baz<Int32>;      # Declared for all `Foo`s, thus no need for template arg
Foo<Float32>::Baz<Int32>() # OK
Foo::Baz<Int32>()          # Still OK

impl Foo<Float64> {
  impl Baz<self::T>; # Impls `Foo<Float64>::Baz<Float64>`
}

forall <T : Float & ~Float64> impl Foo<T> {
  # If `T` is NOT in the current impl `forall` block, then it belongs to the outer scope.
  impl Baz<T>; # Impls `Foo<T>::Baz<T>` for all `T : Float`, but not `T : Float64`
}

# Foo::Baz<Float64>()        # Panic! Unimplemented struct
Foo<Float64>::Baz<Float64>() # OK
Foo<Float32>::Baz<Float32>() # OK
Foo::Baz<Float32>()          # OK, `Foo<T>` is inferred to be `Foo<Float32>`

forall <T : Float> impl Foo<T>
  struct Qux;  # Impls `Foo<T>::Qux` for all `T : Float`
}

forall <T> impl Foo<T> {
  struct Quux; # Impls `Qux` for all `Foo`
}

# Foo::Qux()        # Panic! Undeclared struct (can not infer `T` in `Foo<T>`)
# Foo<Int32>::Qux() # Panic! Undeclared struct (wrong type)
Foo<Float32>::Qux() # OK
Foo::Quux()         # OK, `T` is freetype
Foo<Int32>::Quux()  # OK
```

Note that a function or template argument type could belong to the outer scope, which would allow to infer the chain of template arguments by looking at the argument type reference recursively.

```nx
struct Foo<T> {
  def bar(arg : T);
}

# Bad design?
Foo().bar(42) # Specializes `Foo<Int32>` and `Foo<Int32>::bar(arg : Int32)`
```

A type may be used as a runtime variable restriction iff it has defined bitsize (i.e. implemented).

```nx
decl struct Strukt;

class Klass {
  # let strukt : Strukt        # Panic! `Struct` is not implemented
  let strukt_box : Box<Strukt> # OK, because `Box<T>` only field is a pointer to `T`
  let klass : Klass            # OK, because any class reference has the same bitsize
}
```

```nx
# A `Function<Function>` instance can not be assigned,
# thus it can't be a runtime varible.
#

def sum(a : Int32, b : Int32) -> { return a + b } : Function<sum>
# let func = sum # Panic! A `Function` instance can not be assigned

# It can be an argument, though. In that case, the callee is a generator.
# def each(block : Function<(V) -> void>) -> void {
def each(block : (V) ->) -> {
  let i = 0z;

  while (i < this.size()) do {
    block(this[i])
    i += 1
  }
}

[1, 2].map((e) -> { e * 0.5 }) : Array<Float64, 2>

# `Lambda` is a class.
let lambda : (a : Int32, b : Int32) ~> Int32 =
  ((a, b) ~> { a + b }) :
  Lambda<[](a : Int32, b : Int32) ~> Int32>

# `Lambda` can be passed as a `Function` instance.
let sum = 0
[1, 2].each([&sum](e) ~> { sum += e })
```

Casting a class instance to a struct instance implicitly is dangerous because the class may be referencing itself as an rvalue within its methods.

### Virtual types

A virtual type instance is considered incomplete, thus can not be assigned.
The "virtualization" is temporal.

```nx
impl <self & Drawable>.draw();
(x ~ Drawable).draw()

# Shortcuts:
impl &Drawable.draw();
x~Drawable.draw()
```

```nx
# def draw(a ~ Drawable); # Panic! Unexpected `~`, expected `:` # TODO?
def draw<D : Drawable>(a : D) # OK
```

### Arrows

* `=>` means "translates to in current context"; applicable to `alias`es, single expression function and generators;
* `->` is used for straightforward functions without closures;
* `~>` is used for lambdas similar to `->`, but with closures.
The `~` denotes "at some point later";

```nx
# `->` expects either a returned type or a block afterwards
#

def sum(a, b) -> T throws E { a + b; }
def sum(a, b) -> { a + b; }

let sum = (a : T, b : T) -> T throws E { a + b; } # OK, can assign a `Function`
let sum = (a : T, b : T) -> { a + b; }            # The return type is inferred

map((e) -> { e * 2; }) # No closures allowed
```

```nx
# `=>` expects an expression or block afterwards
#

def sum(a, b) => a + b
def sum(a, b) => { a + b; }

# let sum = (a : T, b : T) => a + b # Panic! Can not assign a generator

let sum = 0
each((e) => sum += e)     # OK, executed in this context
each((e) => { sum += e; }) # Ditto

alias T<*A> => U<*A>
```

```nx
let sum = 0
each([&sum](e) ~> { unsafe! *sum += e; })

let sum = 0
final lambda = [sum](e) ~> discard throws E { sum += e; }
lambda : Lambda<fragile [sum : T](e : T) ~> void>
each(lambda)
@assert(lambda.sum == 42)

# `[~]` means "any closure, but can't access it".
# Note that `[sum : T]` restriction allows other closured variables, but requires `sum`.
Lambda<(arg : T)> ≡ Lambda<fragile [~](arg : T) ~> void>
```
