"""Automated CLI regression tests for ccal.

Compile the command line calculator and execute representative inputs to
validate numeric output formatting and expression handling.
"""

import argparse
import contextlib
import io
import os
import subprocess
import sys
import unittest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def _build_cli_executable() -> str:
    """Compile the CLI calculator and return the executable path."""
    exe_name = "ccal_test.exe" if os.name == "nt" else "ccal_test"
    exe_path = os.path.join(REPO_ROOT, "tests", exe_name)
    compile_cmd = [
        "gcc",
        "ccal.c",
        "-o",
        exe_path,
    ]
    result = subprocess.run(
        compile_cmd,
        cwd=REPO_ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(
            "Failed to build CLI calculator:\n" + result.stderr.rstrip()
        )
    return exe_path


CLI_SUCCESS_CASES = [
    ("addition", ["1", "+", "1"], "2"),
    ("multiplication_token", ["3", "x", "4"], "12"),
    (
        "nested_non_quote",
        [
            "[",
            "[",
            "34",
            "x",
            "11",
            "]",
            "/",
            "[",
            "10",
            "-",
            "5",
            "]",
            "]",
            "-",
            "[",
            "4",
            "x",
            "53",
            "x",
            "[",
            "30",
            "/",
            "10",
            "]",
            "]",
        ],
        "-561.2",
    ),
    (
        "quoted_complex",
        [
            "--quote",
            "[[34x11]/[10-5]]-[4x53x[30/10]+[2x[40-20]]]/[8x6x[12/2]+4]",
        ],
        "72.48493150684931",
    ),
    (
        "manually_grouped",
        [
            "[",
            "[",
            "34",
            "x",
            "11",
            "]",
            "/",
            "[",
            "10",
            "-",
            "5",
            "]",
            "]",
            "-",
            "[",
            "[",
            "[",
            "4",
            "x",
            "53",
            "]",
            "x",
            "[",
            "30",
            "/",
            "10",
            "]",
            "+",
            "[",
            "2",
            "x",
            "[",
            "40",
            "-",
            "20",
            "]",
            "]",
            "]",
            "/",
            "[",
            "8",
            "x",
            "6",
            "x",
            "[",
            "12",
            "/",
            "2",
            "]",
            "+",
            "4",
            "]",
            "]",
        ],
        "72.48493150684931",
    ),
    ("power_operator", ["2", "p", "2"], "4"),
    ("power_quote_caret", ["--quote", "2^2"], "4"),
    ("quote_commas", ["--quote", "2,000-1,000"], "1000"),
    ("manual_commas", ["2,000", "-", "1,000"], "1000"),
    ("decimal_precision_three", ["--quote", "1+1.001"], "2.001"),
    (
        "decimal_precision_mixed",
        ["--quote", "100-1,000+2.0010+1"],
        "-896.999",
    ),
    ("decimal_precision_two", ["--quote", "100-1,000+2.00+1"], "-897"),
    ("decimal_none", ["--quote", "100-1,000+2+1"], "-897"),
    ("decimal_trailing_zeros", ["--quote", "100-1,000+2.0000+1"], "-897"),
    ("small_decimal_sum", ["--quote", "0.3330+0.0005"], "0.3335"),
    ("currency_input", ["--quote", "$1,234.5678+1"], "1235.5678"),
    ("asterisk_with_quote", ["--quote", "(2+3)*4"], "20"),
]


class CLITestExamples(unittest.TestCase):
    """Execute README CLI scenarios."""

    @classmethod
    def setUpClass(cls) -> None:
        try:
            cls.exe_path = _build_cli_executable()
        except RuntimeError as exc:
            raise unittest.SkipTest(str(exc)) from exc

    def _run_cli(self, args):
        return subprocess.run(
            [self.exe_path, *args],
            cwd=REPO_ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

    def test_success_cases(self):
        for name, args, expected in CLI_SUCCESS_CASES:
            with self.subTest(case=name):
                proc = self._run_cli(args)
                self.assertEqual(
                    proc.returncode,
                    0,
                    msg=f"stderr: {proc.stderr.strip()}"
                )
                self.assertEqual(proc.stdout.strip(), expected)
                self.assertEqual(proc.stderr.strip(), "")


if __name__ == "__main__":  # pragma: no cover
    parser = argparse.ArgumentParser(
        add_help=False,
        prog=os.path.basename(sys.argv[0]),
        description="CLI calculator regression tests",
    )
    parser.add_argument(
        "-a",
        "--all",
        action="store_true",
        help="run the complete CLI example suite",
    )
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and the unittest help, then exit",
    )

    opts, remaining = parser.parse_known_args(sys.argv[1:])

    if opts.help:
        parser.print_help()
        print()
        captured = io.StringIO()
        with contextlib.redirect_stdout(captured):
            with contextlib.suppress(SystemExit):
                unittest.main(argv=[sys.argv[0], "--help"], exit=False)
        help_text = captured.getvalue().splitlines()
        processed = []
        skip_examples = False
        prog_name = os.path.basename(sys.argv[0])
        for line in help_text:
            if line.startswith("Examples:"):
                processed.append("Examples:")
                processed.append(f"  {prog_name}                       - run default CLI regression tests")
                processed.append(f"  {prog_name} -a                    - run complete CLI example suite")
                processed.append(f"  {prog_name} --all -v              - run complete suite with verbose output")
                skip_examples = True
                continue
            if skip_examples:
                if line.startswith("  ") and line.strip():
                    continue
                else:
                    skip_examples = False
            processed.append(line)
        print("\n".join(processed))
        sys.exit(0)

    unittest_argv = [sys.argv[0]] + remaining
    if opts.all:
        unittest_argv = [sys.argv[0]]

    unittest.main(argv=unittest_argv)