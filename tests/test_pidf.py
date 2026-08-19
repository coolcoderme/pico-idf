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
            self.assertTrue(row["esp_claw"])
            self.assertTrue(row["backend"])
            self.assertEqual(row["group"], "claw", row["id"])
            self.assertTrue(row["id"].startswith("claw-"), row["id"])

    def test_no_esp_idf_rows(self) -> None:
        ids = {row["id"] for row in self.data["features"]}
        for banned in (
            "nvs",
            "wifi",
            "esp_netif",
            "mqtt",
            "gpio",
            "esp_err",
            "esp-now",
            "freertos-smp",
            "cli",
            "mcp-server",
        ):
            self.assertNotIn(banned, ids)

    def test_foundation_is_done(self) -> None:
        by_id = {row["id"]: row for row in self.data["features"]}
        for key in ("claw-runtime", "claw-sched"):
            self.assertEqual(by_id[key]["status"], "done", key)

    def test_claw_rows_are_honest(self) -> None:
        by_id = {row["id"]: row for row in self.data["features"]}
        self.assertEqual(by_id["claw-mcp-device"]["status"], "partial")
        self.assertEqual(by_id["claw-lua"]["status"], "planned")
        self.assertEqual(by_id["claw-im"]["status"], "planned")
        self.assertEqual(by_id["claw-llm"]["status"], "planned")

    def test_impossible_rows_are_absent(self) -> None:
        by_id = {row["id"]: row for row in self.data["features"]}
        for key in ("esp-now", "esp-mesh", "smartconfig", "thread-zigbee", "touch"):
            self.assertNotIn(key, by_id)


class PidfCliTests(unittest.TestCase):
    def test_version(self) -> None:
        proc = run_pidf("version")
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn("pidf.py", proc.stdout)

    def test_vibe_status(self) -> None:
        proc = run_pidf("vibe", "status")
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn("claw-runtime", proc.stdout)
        self.assertIn("ESP-Claw features", proc.stdout)
        self.assertNotIn("freertos-smp", proc.stdout)
        self.assertNotIn("nvs", proc.stdout)
        self.assertIn("done=", proc.stdout)

    def test_vibe_next_default_is_planned(self) -> None:
        proc = run_pidf("vibe", "next")
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn("Implement pico-idf ESP-Claw feature", proc.stdout)
        self.assertIn("claw-lua", proc.stdout)
        self.assertIn("do not copy esp-claw", proc.stdout.lower())
        self.assertNotIn("nvs", proc.stdout)

    def test_vibe_next_unknown_esp_idf_id(self) -> None:
        proc = run_pidf("vibe", "next", "esp-now")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("esp-now", proc.stderr)

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
            self.assertIn("claw_runtime_init", src)
            cmake = (dest / "main" / "CMakeLists.txt").read_text(encoding="utf-8")
            self.assertIn("REQUIRES claw", cmake)


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

    def test_claw_runtime_host(self) -> None:
        cc = "gcc"
        claw = ROOT / "components" / "claw"
        srcs = [
            claw / "json_mini.c",
            claw / "lock.c",
            claw / "cap.c",
            claw / "event.c",
            claw / "memory.c",
            claw / "core.c",
            claw / "sched.c",
            claw / "stubs.c",
            claw / "mcp.c",
            claw / "builtins.c",
            claw / "runtime.c",
            claw / "repl.c",
            ROOT / "components" / "esp_common" / "esp_err.c",
            ROOT / "components" / "log" / "esp_log.c",
            ROOT / "tests" / "host" / "claw_host_stubs.c",
            ROOT / "tests" / "host" / "test_claw.c",
        ]
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "test_claw"
            cmd = [
                cc,
                "-std=c11",
                "-DPIDF_HOST_TEST=1",
                "-I",
                str(claw / "include"),
                "-I",
                str(claw),
                "-I",
                str(ROOT / "components" / "esp_common" / "include"),
                "-I",
                str(ROOT / "components" / "log" / "include"),
                "-I",
                str(ROOT / "components" / "driver" / "include"),
                *[str(p) for p in srcs],
                "-o",
                str(out),
            ]
            proc = subprocess.run(cmd, text=True, capture_output=True, check=False)
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            proc = subprocess.run([str(out)], text=True, capture_output=True, check=False)
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertIn("ok", proc.stdout)

    def test_claw_example_exists(self) -> None:
        example = ROOT / "examples" / "claw" / "edge_agent"
        self.assertTrue((example / "CMakeLists.txt").exists())
        self.assertTrue((example / "main" / "app_main.c").exists())
        cmake = (example / "main" / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("REQUIRES claw", cmake)


if __name__ == "__main__":
    unittest.main()
