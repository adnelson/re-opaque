# re-opaque

A ReasonML library for opaque data types with validation.

## Opaque strings

At runtime, they are just strings. But since each of them is a distinct module, they form distinct types; so you can never e.g. mix up an Email with a Name. In addition,

### Example

Below we create three string-like modules, `UserName`, `Email`, and `MessageText`. Each has its own validation logic (or lack thereof).

```reason
open Opaque.String;

module UserName: StringType = Make(
  Validation.Compose(
    Validation.MinLength({let n = 10;}),
    Validation.MaxLength({let n = 80;}),
  ), ()
);

module Email = Opaque.String.Make(Opaque.RegexValidation({
  let regex = [%re {|/magic email regex/|}];
}, ()));

// This has no validation (although in practice a max length might be good)
module MessageText = Opaque.String.Make(String.NoValidation);
```

We can't create a username that's less than 5 characters:

```reason
// raises `TooShort("bad", 10)`
let badUsername = "bad"->UserName.fromString;
```

Or an email that doesn't match our regex:

```reason
// raises `RegexMatchError("i am not an email", <some regex>)`
let badEmail = "i am not an email"->Email.fromString;
```

This guarantee means that you have total confidence that you won't be handling invalid data, and pushes error boundaries as early as possible.

### Error handling

The `fromString` function will raise an `exn` exceptions to on failure. Different validators raise different exceptions, so you can switch on them with `try` or `| exception _` to do error handling.

Alternatively you can use `resultFromString`, which avoids potential error cascades. It's up to you.

```reason
// Ok("probablyok"), which is typed as result(UserName.t, exn)
let goodUsername = "probablyok"->UserName.resultFromString;

// Error(TooShort("nope", 10))
let badUsername = "nope"->UserName.resultFromString;
```

### Extras

The module also generates `fromJson` and `toJson` functions, allowing easy conversion to/from JSON for the custom types.

There's also a `MakeStringSet` functor which creates a `Set` module for managing sets of custom types with `Belt.String`. There might be more on this later.


## Build

```
yarn re:build

# Alternatively, in watch mode
yarn re:watch
```

## Test

```
yarn test

# Alternatively, in watch mode
yarn test:watch
```
