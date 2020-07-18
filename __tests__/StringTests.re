open Jest;
open Expect;
open String;

module ValidationTests = {
  module UserName: StringType =
    Make(
      (
        Validation.Compose(
          (
            Validation.MinLength({
              let n = 10;
            })
          ),
          (
            Validation.MaxLength({
              let n = 20;
            })
          ),
        )
      ),
      {},
    );

  let emailRegex = [%re {|/^\w+@\w+\.\w+$/|}];
  module Email =
    Make(
      (
        Validation.MatchRegex({
          let regex = emailRegex;
        })
      ),
      {},
    );

  describe("validation tests", () => {
    test("regex validation", () => {
      expect("foo@bar.com"->Email.fromString->Email.toString)
      ->toEqual("foo@bar.com");
      expect(() =>
        "foobarcom"->Email.fromString->Email.toString
      )
      ->toThrowSomething;
      expect("foobarcom"->Email.resultFromString)
      ->toEqual(Error(Validation.RegexMatchError("foobarcom", emailRegex)));
    });
    test("length validation", () => {
      let (short, long, ok) = (
        "tooshort",
        "toolooooooooooooooooooooooooooooong",
        "I am just fine",
      );
      expect(ok->UserName.fromString->UserName.toString)->toEqual(ok);

      expect(() =>
        short->UserName.fromString
      )->toThrowSomething;
      expect(short->UserName.resultFromString)
      ->toEqual(Error(Validation.TooShort(short, 10)));
      expect(() =>
        long->UserName.fromString
      )->toThrowSomething;
      expect(long->UserName.resultFromString)
      ->toEqual(Error(Validation.TooLong(long, 20)));
    });
  });
};
