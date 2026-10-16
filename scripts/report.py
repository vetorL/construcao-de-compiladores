"""Generate the measured corpus results and complete token listings."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent.parent


def main():
    output = ROOT / "relatorio/saidas"
    output.mkdir(parents=True, exist_ok=True)
    lines = [
        "# Resultados dos exemplos", "",
        "| Programa | Tokens | Erros | Saída |",
        "| --- | ---: | ---: | --- |",
    ]
    failed = False
    for source in sorted((ROOT / "tests/examples").glob("*.cob")):
        run = subprocess.run([str(ROOT / "cobol-lexer"), str(source)],
                             capture_output=True, text=True, check=False)
        stem = source.stem
        (output / f"{stem}.tokens.txt").write_text(run.stdout, encoding="utf-8")
        (output / f"{stem}.errors.txt").write_text(run.stderr, encoding="utf-8")
        count = len(run.stdout.splitlines())
        errors = sum(line.startswith("Lexical error") for line in run.stderr.splitlines())
        lines.append(
            f"| {source.name} | {count} | {errors} | "
            f"[Tokens](saidas/{stem}.tokens.txt) |")
        failed |= run.returncode != 0 or bool(run.stderr)
    (ROOT / "relatorio/RESULTADOS.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("Relatório atualizado em relatorio/RESULTADOS.md")
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
