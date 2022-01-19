# The TODO list

## In the works

* [x] Aliases
* [ ] Single level lookups: `Foo::Bar` and `Foo.bar`
* [ ] Export/Import records.

  ```
  export default { foo: bar() } : Record
  import Record from "./path"
  import { foo } from "./path"
  ```

## Questionable

* [ ] Nested type declarations, namespaces
* [ ] Multi-level lookups (e.g. `Foo::Bar::Baz`)

## Later

* [ ] `:` method access
* [ ] Annotations
* [ ] Virtual types and extensions (`x~Drawable`, `x~(Indexable && Enumerable)`)
