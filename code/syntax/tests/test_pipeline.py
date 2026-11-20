from pathlib import Path
import subprocess
import tempfile
import unittest

SYNTAX = Path(__file__).resolve().parents[1]
LEXICAL = SYNTAX.parent / "lexical"


def find_program(build_dir, name):
    # Windows adds .exe, and Visual Studio puts the result in build/Debug or build/Release.
    names = [name, name + ".exe"]
    dirs = [build_dir, build_dir / "Debug", build_dir / "Release"]
    for directory in dirs:
        for candidate in names:
            path = directory / candidate
            if path.is_file():
                return path
    return None


LEXER = find_program(LEXICAL / "build", "mini_lexer")
PARSER = find_program(SYNTAX / "build", "mini_parser")


class PipelineTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if LEXER is None or PARSER is None:
            raise RuntimeError("Build mini_lexer and mini_parser before running the tests")

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.inputs = self.root / "inputs"
        self.lex_dir = self.root / "lexical"
        self.syntax_dir = self.root / "syntax"
        for directory in (self.inputs, self.lex_dir, self.syntax_dir):
            directory.mkdir()

    def run_program(self, program, source, cwd, status):
        result = subprocess.run([str(program), str(source)], cwd=cwd, capture_output=True, text=True, timeout=5)
        self.assertEqual(result.returncode, status, result.stderr)
        return result

    def lex(self, name, data, status=0):
        source = self.inputs / (name + ".mini")
        source.write_bytes(data if isinstance(data, bytes) else data.encode())
        self.run_program(LEXER, source, self.lex_dir, status)
        return self.lex_dir / "output" / (name + ".dyd")

    def parse(self, dyd, status=0):
        result = self.run_program(PARSER, dyd, self.syntax_dir, status)
        return result, self.syntax_dir / "output"

    def test_lexical_tokens_and_errors(self):
        source = (LEXICAL / "tests" / "lexical_cases.mini").read_bytes()
        dyd = self.lex("lexical_cases", source, 1)
        self.assertEqual(dyd.read_text(), """begin 1
end 2
integer 3
if 4
then 5
else 6
function 7
read 8
write 9
EOLN 24
begin1 10
Begin 10
F9 10
0 11
123 11
12 11
abc 10
EOLN 24
= 12
<> 13
<= 14
< 15
>= 16
> 17
- 18
* 19
:= 20
( 21
) 22
; 23
EOLN 24
x 10
2 11
EOLN 24
EOF 25
""")
        self.assertEqual(dyd.with_suffix(".err").read_text(), """***LINE:4 Illegal character: :
***LINE:4 Illegal character: @
***LINE:4 Illegal character: _
""")

    def test_sample1(self):
        source = (LEXICAL / "tests" / "sample1.mini").read_bytes()
        dyd = self.lex("sample1", source)
        self.assertEqual(dyd.with_suffix(".err").read_bytes(), b"")
        self.assertEqual(dyd.read_text().count("EOLN 24\n"), len(source.splitlines()))
        result, out = self.parse(dyd)
        self.assertIn("Syntax analysis succeeded.", result.stdout)
        self.assertEqual((out / "sample1.var").read_text(), "k <main> 0 integer 0 0\nn F 1 integer 1 1\n")
        self.assertEqual((out / "sample1.pro").read_text(), "<main> - 0 0 0\nF integer 1 1 1\n")
        self.assertEqual((out / "sample1.err").read_bytes(), b"")

    def test_sample2(self):
        source = (LEXICAL / "tests" / "sample2.mini").read_bytes()
        dyd = self.lex("sample2", source)
        self.assertEqual(dyd.with_suffix(".err").read_bytes(), b"")
        self.assertIn("edn 10\n", dyd.read_text())
        _, out = self.parse(dyd, 1)
        self.assertEqual((out / "sample2.var").read_text(), "x <main> 0 integer 0 0\ny <main> 0 integer 0 1\n")
        self.assertEqual((out / "sample2.pro").read_text(), "<main> - 0 0 1\n")
        self.assertEqual((out / "sample2.err").read_text(), """***LINE:4 Expected ;, got 'read'
***LINE:6 Expected else, got ';'
***LINE:8 Expected :=, got 'EOF'
***LINE:8 Expected end, got 'EOF'
""")

    def test_nested_scopes(self):
        dyd = self.lex("nested", (SYNTAX / "tests" / "nested.mini").read_bytes())
        _, out = self.parse(dyd)
        self.assertEqual((out / "nested.var").read_text(), """a <main> 0 integer 0 0
b <main> 0 integer 0 1
n F 1 integer 1 2
z F 0 integer 1 3
after F 0 integer 1 4
t G 1 integer 2 5
u G 0 integer 2 6
""")
        self.assertEqual((out / "nested.pro").read_text(), "<main> - 0 0 1\nF integer 1 2 4\nG integer 2 5 6\n")
        self.assertEqual((out / "nested.err").read_bytes(), b"")

    def test_multiple_errors(self):
        dyd = self.lex("errors", (SYNTAX / "tests" / "errors.mini").read_bytes())
        _, out = self.parse(dyd, 1)
        self.assertEqual((out / "errors.var").read_text(), "x <main> 0 integer 0 0\ny <main> 0 integer 0 1\n")
        self.assertEqual((out / "errors.err").read_text(), """***LINE:3 Expected ;, got 'integer'
***LINE:5 Expected ;, got 'write'
***LINE:6 Expected identifier or number, got ';'
***LINE:7 Expected identifier, got ')'
***LINE:8 Expected statement, got 'end'
""")

    def test_grammar_cases(self):
        cases = [
            ("begin integer x; if x=1 then if x>2 then x:=3 else x:=4 else read(x) end", 0, ""),
            ("begin integer x; x:=1-2*3 end", 0, ""),
            ("begin integer x; if x=1 x:=1 else x:=2 end", 1, "Expected then"),
            ("begin integer x; if x 1 then x:=1 else x:=2 end", 1, "Expected relational operator"),
            ("begin integer x; read(x end", 1, "Expected )"),
            ("begin integer x; x:=(1) end", 1, "Expected identifier or number"),
            ("begin integer x; read(x) end read(x)", 1, "Expected EOF"),
        ]
        for source, status, message in cases:
            with self.subTest(source=source):
                dyd = self.lex("grammar", source)
                result, out = self.parse(dyd, status)
                if status: self.assertIn(message, (out / "grammar.err").read_text())
                else: self.assertEqual((out / "grammar.err").read_bytes(), b"")
        for operator in ("<", "<=", ">", ">=", "=", "<>"):
            with self.subTest(operator=operator):
                dyd = self.lex("relation", f"begin integer x; if x{operator}1 then x:=1 else x:=2 end")
                self.parse(dyd)

    def test_empty_and_line_endings(self):
        dyd = self.lex("empty", b"")
        self.assertEqual(dyd.read_text(), "EOF 25\n")
        _, out = self.parse(dyd, 1)
        self.assertIn("***LINE:1 Expected begin", (out / "empty.err").read_text())
        dyd = self.lex("lines", b'x\r\n\r\ny')
        self.assertEqual(dyd.read_text(), "x 10\nEOLN 24\nEOLN 24\ny 10\nEOLN 24\nEOF 25\n")

    def test_name_length_and_empty_scope(self):
        dyd = self.lex("names", "begin integer abcdefghijklmnop; read(abcdefghijklmnop) end")
        _, out = self.parse(dyd)
        self.assertIn("abcdefghijklmnop <main> 0 integer 0 0\n", (out / "names.var").read_text())
        dyd = self.lex("names", "begin integer abcdefghijklmnopq; integer y; read(y) end")
        _, out = self.parse(dyd, 1)
        self.assertIn("Symbol table error", (out / "names.err").read_text())
        self.assertEqual((out / "names.var").read_text(), "y <main> 0 integer 0 0\n")
        dyd = self.lex("scope", "begin integer function F(n); begin integer n; F:=n end; read(x) end")
        _, out = self.parse(dyd)
        self.assertEqual((out / "scope.pro").read_text(), "<main> - 0 -1 -1\nF integer 1 0 0\n")
        for source in (
            "begin integer function abcdefghijklmnopq(n); begin integer n; F:=n end; integer y; read(y) end",
            "begin integer function F(abcdefghijklmnopq); begin integer n; F:=n end; integer y; read(y) end",
        ):
            with self.subTest(source=source):
                dyd = self.lex("longname", source)
                _, out = self.parse(dyd, 1)
                self.assertIn("Symbol table error", (out / "longname.err").read_text())
                self.assertEqual((out / "longname.var").read_text(), "y <main> 0 integer 0 0\n")
                self.assertEqual((out / "longname.pro").read_text(), "<main> - 0 0 0\n")

    def test_function_header_recovery(self):
        cases = [
            ("begin integer function F(n; begin integer n; F:=n end; integer x; read(x) end", "Expected )"),
            ("begin integer function F(n) begin integer n; F:=n end; integer x; read(x) end", "Expected ;"),
        ]
        for source, message in cases:
            with self.subTest(source=source):
                dyd = self.lex("header", source)
                _, out = self.parse(dyd, 1)
                self.assertIn(message, (out / "header.err").read_text())
                self.assertEqual((out / "header.var").read_text(), "x <main> 0 integer 0 0\nn F 1 integer 1 1\n")
                self.assertEqual((out / "header.pro").read_text(), "<main> - 0 0 0\nF integer 1 1 1\n")

    def test_marker_names_as_identifiers(self):
        dyd = self.lex("markers", "begin integer EOF; integer EOLN; EOF:=1; EOLN:=EOF end")
        self.assertIn("EOF 10\n", dyd.read_text())
        self.assertIn("EOLN 10\n", dyd.read_text())
        _, out = self.parse(dyd)
        self.assertEqual((out / "markers.var").read_text(), "EOF <main> 0 integer 0 0\nEOLN <main> 0 integer 0 1\n")

    def test_malformed_dyd_preserves_old_results(self):
        out = self.syntax_dir / "output"
        out.mkdir()
        for suffix in ("var", "pro", "err"): (out / ("bad." + suffix)).write_text("previous\n")
        dyd = self.inputs / "bad.dyd"
        invalid = ["", "begin 1\nEOLN 24\n", "EOF 25\nEOF 25\n", "x 99\nEOF 25\n",
                   "begin 2\nEOLN 24\nEOF 25\n", "begin 1\nEOF 25\n", "EOF 24\nEOF 25\n"]
        for data in invalid:
            with self.subTest(data=data):
                dyd.write_text(data)
                result = self.run_program(PARSER, dyd, self.syntax_dir, 1)
                self.assertIn("Input error:", result.stderr)
                for suffix in ("var", "pro", "err"): self.assertEqual((out / ("bad." + suffix)).read_text(), "previous\n")

    def test_output_protection_and_old_error(self):
        dyd = self.lex("case", "begin integer x; read() end")
        _, out = self.parse(dyd, 1)
        self.assertNotEqual((out / "case.err").read_bytes(), b"")
        dyd = self.lex("case", "begin integer x; read(x) end")
        self.parse(dyd)
        self.assertEqual((out / "case.err").read_bytes(), b"")
        lexical_error = dyd.with_suffix(".err")
        lexical_error.write_text("original\n")
        result = self.run_program(PARSER, dyd, self.lex_dir, 1)
        self.assertIn("overwrite", result.stderr)
        self.assertEqual(lexical_error.read_text(), "original\n")
        (out / "case.err").unlink()
        try:
            (out / "case.err").symlink_to(lexical_error)
        except (OSError, NotImplementedError):
            # Windows needs Developer Mode or administrator rights to create symbolic links.
            self.skipTest("symbolic links are not available on this system")
        result = self.run_program(PARSER, dyd, self.syntax_dir, 1)
        self.assertIn("overwrite", result.stderr)
        self.assertEqual(lexical_error.read_text(), "original\n")


if __name__ == "__main__":
    unittest.main(verbosity=2)
