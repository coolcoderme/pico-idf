#!/usr/bin/env python3
"""JSON-RPC tests for the pico-idf MCP server."""

from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SERVER = ROOT / "tools" / "pidf_mcp.py"


class McpClient:
    def __init__(self, proc: subprocess.Popen) -> None:
        self.proc = proc
        self._n = 0

    def call(self, method: str, params: dict | None = None) -> dict:
        self._n += 1
        msg = {"jsonrpc": "2.0", "id": self._n, "method": method}
        if params is not None:
            msg["params"] = params
        assert self.proc.stdin is not None
        assert self.proc.stdout is not None
        self.proc.stdin.write(json.dumps(msg) + "\n")
        self.proc.stdin.flush()
        line = self.proc.stdout.readline()
        if not line:
            raise RuntimeError("MCP server closed stdout")
        return json.loads(line)

    def notify(self, method: str, params: dict | None = None) -> None:
        msg = {"jsonrpc": "2.0", "method": method}
        if params is not None:
            msg["params"] = params
        assert self.proc.stdin is not None
        self.proc.stdin.write(json.dumps(msg) + "\n")
        self.proc.stdin.flush()


class PidfMcpTests(unittest.TestCase):
    def setUp(self) -> None:
        env = os.environ.copy()
        env["PIDF_PATH"] = str(ROOT)
        self.proc = subprocess.Popen(
            [sys.executable, str(SERVER)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=env,
        )
        self.client = McpClient(self.proc)
        init = self.client.call(
            "initialize",
            {
                "protocolVersion": "2025-03-26",
                "capabilities": {},
                "clientInfo": {"name": "test", "version": "0"},
            },
        )
        self.assertIn("result", init, init)
        self.assertEqual(init["result"]["serverInfo"]["name"], "pico-idf")
        self.client.notify("notifications/initialized")

    def tearDown(self) -> None:
        if self.proc.stdin:
            self.proc.stdin.close()
        if self.proc.stdout:
            self.proc.stdout.close()
        if self.proc.stderr:
            self.proc.stderr.close()
        self.proc.kill()
        self.proc.wait(timeout=5)

    def _tool(self, name: str, **arguments) -> dict:
        reply = self.client.call("tools/call", {"name": name, "arguments": arguments})
        self.assertIn("result", reply, reply)
        body = reply["result"]
        text = body["content"][0]["text"]
        return {"isError": body.get("isError", False), "text": text, "raw": body}

    def test_tools_list_includes_vibe_surface(self) -> None:
        reply = self.client.call("tools/list")
        names = {t["name"] for t in reply["result"]["tools"]}
        for needed in (
            "list_features",
            "vibe_next",
            "scaffold_feature",
            "set_target",
            "build",
            "get_rules",
            "list_claw_caps",
        ):
            self.assertIn(needed, names)

    def test_vibe_next_default(self) -> None:
        out = self._tool("vibe_next")
        self.assertFalse(out["isError"], out["text"])
        payload = json.loads(out["text"])
        self.assertEqual(payload["feature"]["id"], "nvs")
        self.assertIn("do not copy ESP-IDF", payload["prompt"])

    def test_list_features_planned(self) -> None:
        out = self._tool("list_features", status="planned")
        payload = json.loads(out["text"])
        self.assertGreater(payload["counts"]["planned"], 0)
        self.assertTrue(all(r["status"] == "planned" for r in payload["features"]))

    def test_refuses_impossible_done(self) -> None:
        out = self._tool("set_feature_status", id="esp-now", status="done")
        self.assertTrue(out["isError"], out["text"])
        self.assertIn("impossible", out["text"].lower())

    def test_resources_and_prompts(self) -> None:
        resources = self.client.call("resources/list")["result"]["resources"]
        uris = {r["uri"] for r in resources}
        self.assertIn("pidf://features", uris)
        self.assertIn("pidf://docs/claw", uris)
        self.assertIn("pidf://claw/caps", uris)
        read = self.client.call("resources/read", {"uri": "pidf://features"})
        data = json.loads(read["result"]["contents"][0]["text"])
        self.assertIn("features", data)
        prompts = self.client.call("prompts/list")["result"]["prompts"]
        names = {p["name"] for p in prompts}
        self.assertIn("implement_feature", names)
        self.assertIn("explain_claw", names)
        got = self.client.call(
            "prompts/get",
            {"name": "implement_feature", "arguments": {"id": "wifi"}},
        )
        text = got["result"]["messages"][0]["content"]["text"]
        self.assertIn("wifi", text)

    def test_claw_surface(self) -> None:
        out = self._tool("list_features", group="claw")
        payload = json.loads(out["text"])
        ids = {r["id"] for r in payload["features"]}
        self.assertIn("claw-runtime", ids)
        self.assertTrue(all(r["group"] == "claw" for r in payload["features"]))
        caps = json.loads(self._tool("list_claw_caps")["text"])
        cap_ids = {c["id"] for c in caps["caps"]}
        self.assertIn("gpio", cap_ids)
        self.assertIn("lua", cap_ids)
        claw_doc = self.client.call("resources/read", {"uri": "pidf://docs/claw"})
        self.assertIn("ESP-Claw", claw_doc["result"]["contents"][0]["text"])
        prompt = self.client.call(
            "prompts/get",
            {"name": "explain_claw", "arguments": {"id": "claw-lua"}},
        )
        text = prompt["result"]["messages"][0]["content"]["text"]
        self.assertIn("do not copy", text.lower())
        self.assertIn("claw-lua", text)

    def test_set_target_on_temp_project(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            proj = Path(tmp)
            (proj / "CMakeLists.txt").write_text(
                "include($ENV{PIDF_PATH}/tools/cmake/project.cmake)\nproject(t)\n",
                encoding="utf-8",
            )
            env = os.environ.copy()
            env["PIDF_PATH"] = str(ROOT)
            env["PIDF_PROJECT"] = str(proj)
            proc = subprocess.Popen(
                [sys.executable, str(SERVER)],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                env=env,
            )
            client = McpClient(proc)
            try:
                client.call(
                    "initialize",
                    {
                        "protocolVersion": "2024-11-05",
                        "capabilities": {},
                        "clientInfo": {"name": "test", "version": "0"},
                    },
                )
                reply = client.call(
                    "tools/call",
                    {"name": "set_target", "arguments": {"target": "pico2_w"}},
                )
                payload = json.loads(reply["result"]["content"][0]["text"])
                self.assertEqual(payload["target"], "pico2_w")
                self.assertIn('CONFIG_PIDF_TARGET="pico2_w"', (proj / "sdkconfig").read_text())
            finally:
                if proc.stdin:
                    proc.stdin.close()
                if proc.stdout:
                    proc.stdout.close()
                if proc.stderr:
                    proc.stderr.close()
                proc.kill()
                proc.wait(timeout=5)


if __name__ == "__main__":
    unittest.main()
