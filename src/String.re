// Creates a module with an opaque string-like type in it. A validation
// module is a parameter letting the user guarantee that all strings of
// this type conform to some rules. For example:
//
// module Validation = {
//   exception TooLong;
//   let checkError = Some(s => Js.String.length(s) > 10 ? Some(TooLong) : None);
// }
//
// module ShortString = MakeString(Validation, ());
//
// In the above example, aside from escape hatches like Obj.magic, it is impossible
// to construct a `ShortString.t` which is longer than 10 characters.
//
// In addition, the functor will produce *distinct* types, so for example:
//
// module Name = MakeString(NoValidation, ());
// module Email = MakeString(EmailValidator, ());
//
// Now `Name.t` and `Email.t` are different types, even though both are
// just strings at runtime. This lets one leverage the type system to
// make sure different string-like values don't get confused.
//
//

module A = Belt.Array;
module O = Belt.Option;

module Validation = {
  module type Validator = {let checkError: option(string => option(exn));};

  module NoValidation: Validator = {
    let checkError = None;
  };

  // Join two validators together
  module Compose = (V1: Validator, V2: Validator) : Validator => {
    let checkError =
      switch (V1.checkError, V2.checkError) {
      | (None, None) => None
      | (Some(v), None) => Some(v)
      | (None, Some(v)) => Some(v)
      | (Some(v1), Some(v2)) =>
        Some(
          s =>
            switch (s->v1) {
            | Some(e) => Some(e)
            | None => s->v2
            },
        )
      };
  };

  exception RegexMatchError(string, Js.Re.t);

  module MatchRegex = (Config: {let regex: Js.Re.t;}) : Validator => {
    let checkError =
      Some(
        s =>
          switch (Config.regex->Js.Re.exec_(s)) {
          | Some(_) => None
          | None => Some(RegexMatchError(s, Config.regex))
          },
      );
  };

  exception TooShort(string, int);

  module MinLength = (N: {let n: int;}) : Validator => {
    let checkError =
      Some(s => s->Js.String.length < N.n ? Some(TooShort(s, N.n)) : None);
  };

  exception TooLong(string, int);

  module MaxLength = (N: {let n: int;}) : Validator => {
    let checkError =
      Some(s => s->Js.String.length > N.n ? Some(TooLong(s, N.n)) : None);
  };
};

module type StringType = {
  type t;
  let fromString: string => t;
  let resultFromString: string => result(t, exn);
  let unsafeFromStringNoValidation: string => t;
  let toString: t => string;
  let toJson: Json.Encode.encoder(t);
  let fromJson: Json.Decode.decoder(t);
  let eq: (t, t) => bool;
};

module Make = (Validator: Validation.Validator, ()) : StringType => {
  type t;

  // Use with caution: the string is used as-is without validation.
  external unsafeFromStringNoValidation: string => t = "%identity";

  let fromString =
    switch (Validator.checkError) {
    | None => unsafeFromStringNoValidation
    | Some(check) => (
        s =>
          switch (s->check) {
          | None => unsafeFromStringNoValidation(s)
          | Some(exn) => raise(exn)
          }
      )
    };
  let resultFromString = s =>
    try(Ok(s->fromString)) {
    | e => Error(e)
    };
  external toString: t => string = "%identity";
  let toJson = s => s |> toString |> Json.Encode.string;
  let fromJson = j => j |> Json.Decode.string |> fromString;
  let eq = (==);
};

module MakeCompare = (S: StringType) =>
  Belt.Id.MakeComparable({
    type nonrec t = S.t;
    let cmp = Pervasives.compare;
  });

module MakeSet = (Item: Belt.Id.Comparable) => {
  include Belt.Set;
  type set = t(Item.t, Item.identity);
  let fromArray: array(Item.t) => set =
    Belt.Set.fromArray(~id=(module Item));
  let fromList: list(Item.t) => set = l => l->Belt.List.toArray->fromArray;
  let empty: set = fromArray([||]);
  let singleton: Item.t => set = x => fromArray([|x|]);
};

module MakeStringSet = (S: StringType) => MakeSet((MakeCompare(S)));
