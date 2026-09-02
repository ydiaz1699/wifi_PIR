#!/usr/bin/env python3
"""Command-line entry point for the wifi_PIR virtual laboratory."""

from __future__ import annotations

import argparse
import json
import sys

from .lab import SCENARIOS, run_scenarios


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Deterministic wifi_PIR functional simulator (no ESP hardware required)"
    )
    parser.add_argument("--scenario", default="all", help="scenario name or all")
    parser.add_argument("--list", action="store_true", help="list scenarios")
    parser.add_argument("--json", action="store_true", help="emit machine-readable results")
    parser.add_argument("--seed", type=int, default=7, help="deterministic lab seed")
    args = parser.parse_args(argv)

    if args.list:
        for name in SCENARIOS:
            print(name)
        return 0

    try:
        results = run_scenarios(args.scenario, seed=args.seed)
    except ValueError as error:
        parser.error(str(error))

    if args.json:
        print(json.dumps(results, indent=2, sort_keys=True))
    else:
        print("wifi_PIR virtual lab (deterministic Python model)")
        print("No ESP8266 firmware or electrical simulation is executed.")
        for result in results:
            status = "PASS" if result["passed"] else "FAIL"
            print("[{}] {:28s} {}".format(status, result["name"], result["detail"]))
        passed = sum(1 for result in results if result["passed"])
        print("Summary: {}/{} scenarios passed".format(passed, len(results)))

    return 0 if all(result["passed"] for result in results) else 1


if __name__ == "__main__":
    sys.exit(main())
