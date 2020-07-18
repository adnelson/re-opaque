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

module type StringType = {
  type t;
  let fromString: string => t;
  let toString: t => string;
  let toJson: Json.Encode.encoder(t);
  let fromJson: Json.Decode.decoder(t);
  let eq: (t, t) => bool;
};

module Make =
       (Validation: {let checkError: option(string => option(exn));}, ())
       : StringType => {
  type t;

  let fromString =
    switch (Validation.checkError) {
    | None => Obj.magic
    | Some(check) => (
        s =>
          switch (s->check) {
          | None => Obj.magic(s)
          | Some(exn) => raise(exn)
          }
      )
    };
  external toString: t => string = "%identity";
  let toJson = s => s |> toString |> Json.Encode.string;
  let fromJson = j => j |> Json.Decode.string |> fromString;
  let eq = (==);
};

module NoValidation = {
  let checkError = None;
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
