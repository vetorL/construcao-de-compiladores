"""Regression checks with explicit token, lexeme and physical-location expectations."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
LEXER = Path(os.environ.get("COBOL_LEXER", ROOT / "cobol-lexer"))


def scan_bytes(source):
    with tempfile.NamedTemporaryFile(suffix=".cob") as file:
        file.write(source)
        file.flush()
        return subprocess.run([str(LEXER), file.name], capture_output=True, text=True)


def scan(source):
    return scan_bytes(source.encode())


def tokens(result):
    parsed = []
    for line in result.stdout.splitlines():
        location, category, lexeme = line.split(maxsplit=2)
        row, column = map(int, location.split(":"))
        parsed.append((row, column, category, json.loads(lexeme)))
    return parsed


def pairs(result):
    return [(category, lexeme) for _, _, category, lexeme in tokens(result)]


class ExtendedTests(unittest.TestCase):
    def clean(self, source):
        result = scan(source)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        return result

    def test_keyword_boundaries_and_case(self):
        result = self.clean("       program-id PIC IS 9(02).\n"
                            "       ADD IFOO MOVE-ACCOUNT TO PROG006_FILES.\n")
        self.assertEqual(pairs(result), [
            ("PROGRAM-ID", "program-id"), ("PIC", "PIC"), ("IS", "IS"),
            ("PICTURE_STRING", "9(02)"), ("PERIOD", "."),
            ("ADD", "ADD"), ("IDENTIFIER", "IFOO"),
            ("IDENTIFIER", "MOVE-ACCOUNT"), ("TO", "TO"),
            ("IDENTIFIER", "PROG006_FILES"), ("PERIOD", ".")])

    def test_operators_and_signed_numbers(self):
        result = self.clean("       = > < >= <= <> + - * ** /\n"
                            "       1+2 3-4 5*-6 (-7) -8 +9 -1.25\n")
        expected = [("EQ", "="), ("GT", ">"), ("LT", "<"), ("GE", ">="),
                    ("LE", "<="), ("NE", "<>"), ("PLUS", "+"),
                    ("MINUS", "-"), ("STAR", "*"), ("POWER", "**"),
                    ("SLASH", "/"), ("NUMBER", "1"), ("PLUS", "+"),
                    ("NUMBER", "2"), ("IDENTIFIER", "3-4"),
                    ("NUMBER", "5"), ("STAR", "*"),
                    ("MINUS", "-"), ("NUMBER", "6"), ("LPAREN", "("),
                    ("NUMBER", "-7"), ("RPAREN", ")"), ("NUMBER", "-8"),
                    ("NUMBER", "+9"), ("NUMBER", "-1.25")]
        self.assertEqual(pairs(result), expected)
        self.assertEqual(pairs(self.clean("       A+B (A)-B 3 - 4\n")), [
            ("IDENTIFIER", "A"), ("PLUS", "+"), ("IDENTIFIER", "B"),
            ("LPAREN", "("), ("IDENTIFIER", "A"), ("RPAREN", ")"),
            ("MINUS", "-"), ("IDENTIFIER", "B"),
            ("NUMBER", "3"), ("MINUS", "-"), ("NUMBER", "4")])

    def test_picture_masks_and_sentence_period(self):
        masks = ["9(10)", "S9(05)V99", "S9(05)V9(02)", "-ZZZZZZZZZ9.99",
                 "X(20)", "9(5)V99", "A(01)", "ZZ9.99", "9(02)"]
        for mask in masks:
            with self.subTest(mask=mask):
                result = self.clean(f"       PICTURE IS {mask}.\n")
                self.assertEqual(tokens(result)[2], (1, 19, "PICTURE_STRING", mask))
                self.assertEqual(pairs(result)[-1], ("PERIOD", "."))
        self.assertEqual(pairs(self.clean("       PIC 9.99. 123.45.\n")), [
            ("PIC", "PIC"), ("PICTURE_STRING", "9.99"), ("PERIOD", "."),
            ("NUMBER", "123.45"), ("PERIOD", ".")])

    def test_picture_state_is_local(self):
        result = self.clean("       PIC X. MOVE X TO Y.\n")
        self.assertIn(("IDENTIFIER", "X"), pairs(result))
        self.assertIn(("MOVE", "MOVE"), pairs(result))

    def test_continued_identifier_locations(self):
        result = self.clean('       MOVE "OK" TO T\n      -AB-TRABALHADOR.\n'
                            '       DISPLAY "NEXT".\n')
        self.assertEqual(tokens(result), [
            (1, 8, "MOVE", "MOVE"), (1, 13, "STRING", '"OK"'),
            (1, 18, "TO", "TO"), (1, 21, "IDENTIFIER", "TAB-TRABALHADOR"),
            (2, 22, "PERIOD", "."), (3, 8, "DISPLAY", "DISPLAY"),
            (3, 16, "STRING", '"NEXT"'), (3, 22, "PERIOD", ".")])

    def test_continuation_after_closed_string(self):
        result = self.clean('       DISPLAY "TOTAL"\n      -    TOTAL.\n')
        self.assertEqual(tokens(result)[2:], [
            (2, 12, "IDENTIFIER", "TOTAL"), (2, 17, "PERIOD", ".")])

    def test_string_continuations_both_quotes(self):
        for quote in ['"', "'"]:
            result = self.clean(f"       DISPLAY {quote}HELLO\n"
                                f"      -{quote} WORLD\n      -{quote}!{quote}.\n")
            self.assertEqual(pairs(result)[1], ("STRING", quote + "HELLO WORLD!" + quote))
            self.assertEqual(tokens(result)[-1], (3, 11, "PERIOD", "."))

    def test_comments_between_continued_lines(self):
        result = self.clean('       DISPLAY "A\n      * ignore\n      -"B".\n')
        self.assertEqual(pairs(result)[1], ("STRING", '"AB"'))
        self.assertEqual(tokens(result)[-1], (3, 11, "PERIOD", "."))

    def test_boundary_columns_and_line_endings(self):
        source = "123456 " + " " * 64 + "." + "@IGNORED\r\n"
        result = scan_bytes(source.encode())
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(tokens(result), [(1, 72, "PERIOD", ".")])
        self.assertEqual(pairs(self.clean("       MOVE X."))[-1], ("PERIOD", "."))
        self.assertEqual(tokens(self.clean("")), [])

    def test_recovery_and_invalid_extensions(self):
        for source, message in [
            ("       PIC QQQ.\n       DISPLAY 'OK'.\n", "malformed picture"),
            ("       PIC X(0).\n       DISPLAY 'OK'.\n", "malformed picture"),
            ("       PIC .\n       DISPLAY 'OK'.\n", "missing picture"),
            ("      -ORPHAN\n       DISPLAY 'OK'.\n", "continuation indicator"),
            ("       -ABC @ ABC-\n       DISPLAY 'OK'.\n", "invalid character"),
            ("       1.2ABC\n       DISPLAY 'OK'.\n", "malformed numeric"),
            ("       " + "A" * 31 + "\n       DISPLAY 'OK'.\n", "30-character"),
            ("       'UNCLOSED\n       DISPLAY 'OK'.\n", "unterminated"),
        ]:
            with self.subTest(source=source):
                result = scan(source)
                self.assertEqual(result.returncode, 1)
                self.assertIn(message, result.stderr)
                self.assertIn(("DISPLAY", "DISPLAY"), pairs(result))
        result = scan("       PIC")
        self.assertEqual(result.returncode, 1)
        self.assertIn("missing picture", result.stderr)
        self.assertNotIn("[0:0]", result.stderr)
        self.clean("       " + "A" * 30 + "\n")

    def test_examples(self):
        examples = sorted((ROOT / "tests/examples").glob("*.cob"))
        self.assertEqual(len(examples), 7)
        outputs = {}
        for file in examples:
            with self.subTest(file=file.name):
                result = subprocess.run([str(LEXER), str(file)], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stderr, "")
                outputs[file.name] = tokens(result)
        self.assertIn((28, 7 + 15, "PERIOD", "."),
                      outputs["prog004_folha_pgto.cob"])
        all_pairs = [(kind, value) for output in outputs.values()
                     for _, _, kind, value in output]
        for word in ["PROGRAM-ID", "PIC", "VALUE", "VALUES", "ADD", "SUBTRACT",
                     "UNTIL", "END-IF", "READ", "WRITE", "INSPECT", "EVALUATE",
                     "TO", "KW_ERROR", "I-O"]:
            self.assertTrue(any(kind == word for kind, _ in all_pairs), word)
        self.assertIn(("PICTURE_STRING", "-ZZZZZZZZZ9.99"), all_pairs)
        self.assertIn(("IDENTIFIER", "TAB-TRABALHADOR"), all_pairs)


if __name__ == "__main__":
    unittest.main()
