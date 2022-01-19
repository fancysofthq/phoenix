`auto` type means inferred type.
`discard` means drop any value and return void.

```onyx
# Annotation application proxies its documentation to the applied entity.
# Unused (delimeted with a newline) annotation is a panic.
@[Annotation]
def each(block : (T) => discard, temp: (U) => auto) : void {
  [0..this.size].each(i => yield block(this[i]))
  let f = yield temp(val)

  # This is better:
  [0..this.size].each(i => block(this[i]))
  let f = temp(val)
}
```
