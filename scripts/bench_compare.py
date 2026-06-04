#!/usr/bin/env python3
"""Compare two nanobench CSV exports (master vs PR) and gate on regressions.

Part of the container/algorithm CI gate (Phase 11, R50). The bench binary emits a
nanobench CSV when WCB_OUT is set (see bench/bench_main.cpp); this script diffs the
PR's CSV against master's and fails if any benchmark regressed beyond the tolerance
or went missing.

Keying: results are matched on the **(title, name)** pair, not name alone -- a few row
names repeat across suites (e.g. "Array<int> push x4096 (reserve)" appears under both
the contiguous and the worse-vs-std titles), so name alone is ambiguous.

Metric: nanobench's CSV "elapsed" column is the median elapsed seconds per op (lower is
faster). We compare PR/master and flag PR > master * (1 + tol).

Exit status:
  0  -> no regressions (or master has no comparable baseline yet)
  1  -> at least one benchmark regressed or went missing
  2  -> usage / parse error

Usage:
  bench_compare.py <master.csv> <pr.csv> [--tol 0.15] [--md REPORT.md]
"""

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path

# Result key = (suite title, bench name); value = median seconds/op.
Key = tuple[str, str]
Table = dict[Key, float]


def parse_csv(path: Path) -> Table:
    """Parse a nanobench CSV (';'-separated, quoted fields) into {(title, name): median_s}.

    Tolerates a missing/empty file (returns {}). Locates columns by header name so column
    order is not assumed. A repeated header line (the bench emits one per suite, de-duped at
    write time, but be defensive) and blank lines are skipped.
    """
    if not path.exists():
        return {}

    table: Table = {}
    with path.open(newline="") as f:
        reader = csv.reader(f, delimiter=";")
        rows = [r for r in reader if r]
    if not rows:
        return {}

    header = rows[0]
    try:
        i_title = header.index("title")
        i_name = header.index("name")
        i_elapsed = header.index("elapsed")
    except ValueError as exc:
        raise SystemExit(f"bench_compare: {path}: unexpected CSV header {header!r}: {exc}")

    for row in rows[1:]:
        if not row or row[0] == "title":  # skip any repeated header
            continue
        if len(row) <= max(i_title, i_name, i_elapsed):
            continue
        try:
            elapsed = float(row[i_elapsed])
        except ValueError:
            continue
        table[(row[i_title], row[i_name])] = elapsed

    return table


def ns(seconds: float) -> str:
    """Format seconds/op as ns/op for human-readable reports."""
    return f"{seconds * 1e9:,.1f}"


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Gate PR bench CSV against master bench CSV.")
    ap.add_argument("master", type=Path, help="master baseline CSV (may be empty/absent)")
    ap.add_argument("pr", type=Path, help="PR CSV to check")
    ap.add_argument("--tol", type=float, default=0.15,
                    help="regression tolerance as a fraction (default 0.15 = 15%%)")
    ap.add_argument("--md", type=Path, default=None,
                    help="also write the Markdown report to this path")
    args = ap.parse_args(argv)

    master = parse_csv(args.master)
    pr = parse_csv(args.pr)

    lines: list[str] = []

    def emit(s: str = "") -> None:
        lines.append(s)

    # No baseline yet (e.g. master predates the bench): can't compare -> pass.
    if not master:
        emit("## Benchmark gate: no baseline")
        emit()
        emit(f"`master` has no comparable benchmarks (parsed 0 rows from `{args.master}`).")
        emit("Nothing to compare against — **gate passes**. It becomes enforcing once master "
             "carries the bench baseline.")
        report = "\n".join(lines) + "\n"
        sys.stdout.write(report)
        if args.md:
            args.md.write_text(report)
        return 0

    tol = args.tol
    # rows: (delta_pct, verdict, fail, title, name, m_val, p_val)
    rows = []
    for key, m_val in master.items():
        title, name = key
        if key not in pr:
            rows.append((float("inf"), "MISSING", True, title, name, m_val, None))
            continue
        p_val = pr[key]
        if m_val <= 0:
            rows.append((0.0, "n/a", False, title, name, m_val, p_val))
            continue
        ratio = p_val / m_val
        delta = (ratio - 1.0) * 100.0
        if ratio > 1.0 + tol:
            rows.append((delta, "REGRESS", True, title, name, m_val, p_val))
        elif ratio < 1.0 - tol:
            rows.append((delta, "improved", False, title, name, m_val, p_val))
        else:
            rows.append((delta, "ok", False, title, name, m_val, p_val))

    new_keys = sorted(k for k in pr if k not in master)

    # Worst first so regressions surface at the top of the report.
    rows.sort(key=lambda r: r[0], reverse=True)

    failures = [r for r in rows if r[2]]
    regress = [r for r in rows if r[1] == "REGRESS"]
    missing = [r for r in rows if r[1] == "MISSING"]

    status = "FAIL" if failures else "PASS"
    icon = "❌" if failures else "✅"
    emit(f"## Benchmark gate: {icon} {status}")
    emit()
    emit(f"Tolerance: PR median > master median × {1 + tol:.2f} fails. "
         f"Compared **{len(rows)}** benchmarks "
         f"({len(regress)} regressed, {len(missing)} missing, {len(new_keys)} new).")
    emit()
    emit("| Δ% | verdict | benchmark | suite |")
    emit("|---:|:---|:---|:---|")
    for delta, verdict, _fail, title, name, m_val, p_val in rows:
        if verdict == "MISSING":
            d = "—"
            detail = f"master {ns(m_val)} → (absent)"
        else:
            d = f"{delta:+.1f}"
            detail = f"{ns(m_val)} → {ns(p_val)} ns/op"
        emit(f"| {d} | {verdict} | `{name}`<br/>{detail} | {title} |")
    for title, name in new_keys:
        emit(f"| — | new | `{name}`<br/>PR-only {ns(pr[(title, name)])} ns/op | {title} |")

    emit()
    if failures:
        if regress:
            emit(f"**{len(regress)} regression(s) over {tol * 100:.0f}% tolerance.**")
        if missing:
            emit(f"**{len(missing)} benchmark(s) present on master but missing from the PR.**")
    else:
        emit("No regressions over tolerance.")

    report = "\n".join(lines) + "\n"
    sys.stdout.write(report)
    if args.md:
        args.md.write_text(report)

    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
