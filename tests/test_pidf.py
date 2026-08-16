#!/usr/bin/env python3
"""Host tests for pidf.py and the feature matrix."""

from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PIDF = ROOT / "tools" / "pidf.py"
FEATURES = ROOT / "tools" / "features.json"


def run_pidf(*args: str, cwd: Path | None = None) -> subprocess.CompletedProcess:
    env = os.environ.copy()
    env["PIDF_PATH"] = str(ROOT)
    return subprocess.run(
        [sys.executable, str(PIDF), *args],
        cwd=cwd or ROOT,
        env=env,
        text=True,
        capture_output=True,
        check=False,
    )


class FeaturesJsonTests(unittest.TestCase):
    def setUp(self) -> None:
        self.data = json.loads(FEATURES.read_text(encoding="utf-8"))

    def test_schema(self) -> None:
        self.assertEqual(self.data["version"], 1)
        self.assertIn("pico_w", self.data["targets"])
        self.assertIn("pico2_w", self.data["targets"])
        self.assertIn("pico2_w_riscv", self.data["targets"])

    def test_unique_ids_and_status(self) -> None:
        allowed = {"done", "partial", "planned", "impossible"}
        ids = [row["id"] for row in self.data["features"]]
        self.assertEqual(len(ids), len(set(ids)))
        for row in self.data["features"]:
            self.assertIn(row["status"], allowed, row["id"])
            self.assertTrue(row["title"])
            self.assertTrue(row["esp_idf"])
            self.assertTrue(row["backend"])

    def test_foundation_is_done(self) -> None:
        by_id = {row["id"]: row for row in self.data["features"]}
        for key in ("cli", "esp_err", "esp_log", "freertos-smp", "gpio"):
            self.assertEqual(by_id[key]["status"], "done", key)

    def test_impossible_rows_stay_impossible(self) -> None:
        by_id = {row["id"]: row for row in self.data["features"]}
        for key in ("esp-now", "esp-mesh", "smartconfig", "thread-zigbee", "touch"):
            self.assertEqual(by_id[key]["status"], "impossible", key)


class PidfCliTests(unittest.TestCase):
    def test_version(self) -> None:
        proc = run_pidf("version")
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn("pidf.py", proc.stdout)

    def test_vibe_status(self) -> None:
        proc = run_pidf("vibe", "status")
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn("freertos-smp", proc.stdout)
        self.assertIn("done=", proc.stdout)

    def test_vibe_next_default_is_planned(self) -> None:
        proc = run_pidf("vibe", "next")
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn("Implement pico-idf feature", proc.stdout)
        self.assertIn("do not copy esp-idf", proc.stdout.lower())

    def test_vibe_next_impossible_id(self) -> None:
        proc = run_pidf("vibe", "next", "esp-now")
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn("esp-now", proc.stdout)
        self.assertIn("impossible", proc.stdout)

    def test_set_target_and_menuconfig(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            proj = Path(tmp)
            (proj / "CMakeLists.txt").write_text(
                "include($ENV{PIDF_PATH}/tools/cmake/project.cmake)\nproject(t)\n",
                encoding="utf-8",
            )
            env_proj = os.environ.copy()
            env_proj["PIDF_PATH"] = str(ROOT)
            env_proj["PIDF_PROJECT"] = str(proj)
            proc = subprocess.run(
                [sys.executable, str(PIDF), "set-target", "pico2_w"],
                env=env_proj,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(proc.returncode, 0, proc.stderr)
            text = (proj / "sdkconfig").read_text(encoding="utf-8")
            self.assertIn('CONFIG_PIDF_TARGET="pico2_w"', text)
            proc = subprocess.run(
                [sys.executable, str(PIDF), "menuconfig", "--set", "BLINK_GPIO=7"],
                env=env_proj,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(proc.returncode, 0, proc.stderr)
            text = (proj / "sdkconfig").read_text(encoding="utf-8")
            self.assertIn("CONFIG_BLINK_GPIO=7", text)

    def test_create_project(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dest = Path(tmp) / "demo"
            proc = run_pidf("create-project", str(dest))
            self.assertEqual(proc.returncode, 0, proc.stderr)
            self.assertTrue((dest / "main" / "main.c").exists())
            src = (dest / "main" / "main.c").read_text(encoding="utf-8")
            self.assertIn("app_main", src)


class HostCTests(unittest.TestCase):
    def test_esp_err_and_event(self) -> None:
        cc = "gcc"
        src_err = ROOT / "components" / "esp_common" / "esp_err.c"
        src_event = ROOT / "components" / "esp_event" / "esp_event.c"
        harness = ROOT / "tests" / "host" / "test_core.c"
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "test_core"
            cmd = [
                cc,
                "-std=c11",
                "-I",
                str(ROOT / "components" / "esp_common" / "include"),
                "-I",
                str(ROOT / "components" / "esp_event" / "include"),
                str(src_err),
                str(src_event),
                str(harness),
                "-o",
                str(out),
            ]
            proc = subprocess.run(cmd, text=True, capture_output=True, check=False)
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            proc = subprocess.run([str(out)], text=True, capture_output=True, check=False)
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertIn("ok", proc.stdout)


if __name__ == "__main__":
    unittest.main()
