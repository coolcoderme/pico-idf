#!/usr/bin/env python3
"""pico-idf MCP server (stdio JSON-RPC).

This is the vibe-coding surface: Cursor and other MCP hosts call tools
to read the feature matrix, scaffold the next ESP-IDF-shaped component,
set the Pico W / Pico 2 W target, and build firmware.

No third-party MCP SDK — stdio is newline-delimited JSON-RPC 2.0.
Logs go to stderr only; stdout is protocol messages.
"""

from __future__ import annotations

import json
import sys
import traceback
from pathlib import Path
from typing import Any, Callable

TOOLS_DIR = Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

import pidf  # noqa: E402

SERVER_NAME = "pico-idf"
SERVER_VERSION = pidf.VERSION
PROTOCOL_VERSIONS = (
    "2024-11-05",
    "2025-03-26",
    "2025-06-18",
    "2026-06-18",
    "2026-07-28",
)


def _text(text: str, is_error: bool = False) -> dict:
    return {"content": [{"type": "text", "text": text}], "isError": is_error}


def _json(payload: Any) -> dict:
    return _text(json.dumps(payload, indent=2))


def tool_list_features(status: str | None = None, group: str | None = None) -> dict:
    data = pidf.load_features()
    rows = data["features"]
    if status:
        rows = [r for r in rows if r["status"] == status]
    if group:
        rows = [r for r in rows if r["group"] == group]
    counts: dict[str, int] = {}
    for r in data["features"]:
        counts[r["status"]] = counts.get(r["status"], 0) + 1
    return _json({"counts": counts, "features": rows})


def tool_get_feature(id: str) -> dict:
    try:
        row = pidf.feature_by_id(id)
    except KeyError:
        return _text(f"unknown feature id {id!r}", is_error=True)
    return _json(row)


def tool_vibe_next(id: str | None = None) -> dict:
    try:
        feature = pidf.next_planned_feature(id)
    except KeyError as exc:
        return _text(str(exc), is_error=True)
    return _json(
        {
            "feature": feature,
            "prompt": pidf.feature_implement_prompt(feature),
            "rules": [
                "Do not copy ESP-IDF sources.",
                "Keep void app_main(void) on FreeRTOS.",
                "Never mark an impossible row done.",
                "Cross-compile for pico_w and pico2_w.",
            ],
        }
    )


def tool_set_feature_status(id: str, status: str) -> dict:
    try:
        row = pidf.update_feature_status(id, status)
    except (KeyError, ValueError) as exc:
        return _text(str(exc), is_error=True)
    return _json({"updated": row})


def tool_get_target() -> dict:
    proj = pidf.project_dir()
    target = pidf.current_target(proj)
    spec = pidf.TARGETS.get(target, {})
    return _json(
        {
            "project": str(proj),
            "target": target,
            "board": spec.get("board"),
            "wireless": spec.get("wireless"),
            "sdkconfig": pidf.read_sdkconfig(proj),
        }
    )


def tool_set_target(target: str) -> dict:
    try:
        path = pidf.write_target(pidf.project_dir(), target)
    except ValueError as exc:
        return _text(str(exc), is_error=True)
    return _json({"target": target, "sdkconfig": str(path), "spec": pidf.TARGETS[target]})


def tool_set_config(key: str, value: str) -> dict:
    path = pidf.set_sdkconfig_value(pidf.project_dir(), key, value)
    return _json({"sdkconfig": str(path), "values": pidf.read_sdkconfig(pidf.project_dir())})


def tool_build(fresh: bool = False, jobs: int | None = None) -> dict:
    rc, log = pidf.run_build(pidf.project_dir(), fresh=fresh, jobs=jobs)
    uf2 = pidf.find_uf2(pidf.project_dir())
    return _json(
        {
            "returncode": rc,
            "uf2": str(uf2) if uf2 else None,
            "log_tail": log[-8000:],
        }
    )


def tool_create_project(path: str) -> dict:
    class Args:
        pass

    args = Args()
    args.path = path
    rc = pidf.cmd_create_project(args)
    return _json({"returncode": rc, "path": str(Path(path).resolve())})


def tool_create_component(name: str) -> dict:
    class Args:
        pass

    args = Args()
    args.name = name
    rc = pidf.cmd_create_component(args)
    dest = pidf.pidf_path() / "components" / name
    return _json({"returncode": rc, "path": str(dest)})


def tool_scaffold_feature(id: str) -> dict:
    try:
        feature = pidf.feature_by_id(id)
    except KeyError:
        return _text(f"unknown feature id {id!r}", is_error=True)
    if feature["status"] == "impossible":
        return _text(
            f"{id} is impossible on Pico hardware. {feature.get('notes') or ''}",
            is_error=True,
        )
    name = id.replace("-", "_")
    dest = pidf.pidf_path() / "components" / name
    dest.mkdir(parents=True, exist_ok=True)
    (dest / "include").mkdir(exist_ok=True)
    (dest / "CMakeLists.txt").write_text(
        f'idf_component_register(SRCS "{name}.c" INCLUDE_DIRS "include" REQUIRES esp_common)\n',
        encoding="utf-8",
    )
    (dest / f"{name}.c").write_text(
        f'#include "{name}.h"\n\n/* TODO: Pico backend for {feature["esp_idf"]} */\n',
        encoding="utf-8",
    )
    (dest / "include" / f"{name}.h").write_text(
        "#pragma once\n#include \"esp_err.h\"\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n"
        f"/* ESP-IDF-shaped API for {feature['title']} */\n\n"
        "#ifdef __cplusplus\n}\n#endif\n",
        encoding="utf-8",
    )
    example = pidf.pidf_path() / "examples" / feature["group"] / name
    example.mkdir(parents=True, exist_ok=True)
    (example / "main").mkdir(exist_ok=True)
    (example / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.13)\n"
        "include($ENV{PIDF_PATH}/tools/cmake/project.cmake)\n"
        f"project({name})\n",
        encoding="utf-8",
    )
    (example / "main" / "CMakeLists.txt").write_text(
        'idf_component_register(SRCS "main.c" INCLUDE_DIRS ".")\n',
        encoding="utf-8",
    )
    (example / "main" / "main.c").write_text(
        '#include "esp_log.h"\n\n'
        f'static const char *TAG = "{name}";\n\n'
        "void app_main(void)\n{\n"
        f'    ESP_LOGI(TAG, "TODO: {feature["title"]}");\n'
        "}\n",
        encoding="utf-8",
    )
    (example / "sdkconfig.defaults").write_text(
        'CONFIG_PIDF_TARGET="pico_w"\nCONFIG_FREERTOS_SMP=y\n',
        encoding="utf-8",
    )
    return _json(
        {
            "feature": feature,
            "component": str(dest),
            "example": str(example),
            "prompt": pidf.feature_implement_prompt(feature),
        }
    )


def tool_get_rules() -> dict:
    root = pidf.pidf_path()
    chunks = []
    for rel in ("AGENTS.md", "docs/COMPATIBILITY.md", "docs/VIBECODING.md"):
        path = root / rel
        if path.exists():
            chunks.append(f"# {rel}\n\n{path.read_text(encoding='utf-8')}")
    return _text("\n\n".join(chunks))


def tool_list_examples() -> dict:
    root = pidf.pidf_path() / "examples"
    found = []
    if root.exists():
        for cmake in sorted(root.rglob("CMakeLists.txt")):
            if cmake.parent.name == "main":
                continue
            found.append(str(cmake.parent.relative_to(pidf.pidf_path())))
    return _json({"examples": found})


TOOLS: dict[str, tuple[Callable[..., dict], dict]] = {
    "list_features": (
        tool_list_features,
        {
            "description": (
                "List pico-idf ESP-IDF feature rows and their status "
                "(done, partial, planned, impossible)."
            ),
            "inputSchema": {
                "type": "object",
                "properties": {
                    "status": {
                        "type": "string",
                        "enum": ["done", "partial", "planned", "impossible"],
                    },
                    "group": {"type": "string"},
                },
            },
        },
    ),
    "get_feature": (
        tool_get_feature,
        {
            "description": "Get one feature-matrix row by id (for example nvs, wifi, gpio).",
            "inputSchema": {
                "type": "object",
                "properties": {"id": {"type": "string"}},
                "required": ["id"],
            },
        },
    ),
    "vibe_next": (
        tool_vibe_next,
        {
            "description": (
                "Return the next planned ESP-IDF-shaped feature and a ready-to-run "
                "implementation prompt. Optional id selects a specific row."
            ),
            "inputSchema": {
                "type": "object",
                "properties": {"id": {"type": "string"}},
            },
        },
    ),
    "set_feature_status": (
        tool_set_feature_status,
        {
            "description": (
                "Update a feature row after implementing it. Refuses to mark "
                "impossible hardware as done."
            ),
            "inputSchema": {
                "type": "object",
                "properties": {
                    "id": {"type": "string"},
                    "status": {
                        "type": "string",
                        "enum": ["done", "partial", "planned", "impossible"],
                    },
                },
                "required": ["id", "status"],
            },
        },
    ),
    "get_target": (
        tool_get_target,
        {
            "description": "Show the current Pico target (pico_w, pico2_w, …) and sdkconfig.",
            "inputSchema": {"type": "object", "properties": {}},
        },
    ),
    "set_target": (
        tool_set_target,
        {
            "description": "Select pico_w, pico2_w, pico2_w_riscv, pico, or pico2.",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "target": {"type": "string", "enum": sorted(pidf.TARGETS)},
                },
                "required": ["target"],
            },
        },
    ),
    "set_config": (
        tool_set_config,
        {
            "description": "Set a sdkconfig CONFIG_* value (Wi-Fi SSID, blink GPIO, …).",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "key": {"type": "string"},
                    "value": {"type": "string"},
                },
                "required": ["key", "value"],
            },
        },
    ),
    "build": (
        tool_build,
        {
            "description": "Configure and cross-compile the current pico-idf project.",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "fresh": {"type": "boolean"},
                    "jobs": {"type": "integer"},
                },
            },
        },
    ),
    "create_project": (
        tool_create_project,
        {
            "description": "Create an app_main() pico-idf project directory.",
            "inputSchema": {
                "type": "object",
                "properties": {"path": {"type": "string"}},
                "required": ["path"],
            },
        },
    ),
    "create_component": (
        tool_create_component,
        {
            "description": "Create a components/<name> skeleton with idf_component_register.",
            "inputSchema": {
                "type": "object",
                "properties": {"name": {"type": "string"}},
                "required": ["name"],
            },
        },
    ),
    "scaffold_feature": (
        tool_scaffold_feature,
        {
            "description": (
                "Create the component + example skeleton for a planned feature, "
                "then return the implementation prompt."
            ),
            "inputSchema": {
                "type": "object",
                "properties": {"id": {"type": "string"}},
                "required": ["id"],
            },
        },
    ),
    "get_rules": (
        tool_get_rules,
        {
            "description": "Return AGENTS.md, compatibility, and vibe-coding rules.",
            "inputSchema": {"type": "object", "properties": {}},
        },
    ),
    "list_examples": (
        tool_list_examples,
        {
            "description": "List example projects under examples/.",
            "inputSchema": {"type": "object", "properties": {}},
        },
    ),
}


def _resource_list() -> list[dict]:
    return [
        {
            "uri": "pidf://features",
            "name": "Feature matrix",
            "mimeType": "application/json",
            "description": "All ESP-IDF-shaped features and their Pico status",
        },
        {
            "uri": "pidf://target",
            "name": "Current target",
            "mimeType": "application/json",
            "description": "Active Pico board and sdkconfig",
        },
        {
            "uri": "pidf://docs/architecture",
            "name": "Architecture",
            "mimeType": "text/markdown",
        },
        {
            "uri": "pidf://docs/agents",
            "name": "Agent rules",
            "mimeType": "text/markdown",
        },
        {
            "uri": "pidf://docs/compatibility",
            "name": "Compatibility",
            "mimeType": "text/markdown",
        },
        {
            "uri": "pidf://docs/vibecoding",
            "name": "MCP vibe-coding guide",
            "mimeType": "text/markdown",
        },
    ]


def _resource_read(uri: str) -> dict:
    root = pidf.pidf_path()
    mapping = {
        "pidf://features": (
            "application/json",
            json.dumps(pidf.load_features(), indent=2),
        ),
        "pidf://target": (
            "application/json",
            json.dumps(
                {
                    "project": str(pidf.project_dir()),
                    "target": pidf.current_target(pidf.project_dir()),
                    "sdkconfig": pidf.read_sdkconfig(pidf.project_dir()),
                },
                indent=2,
            ),
        ),
        "pidf://docs/architecture": (
            "text/markdown",
            (root / "docs" / "ARCHITECTURE.md").read_text(encoding="utf-8"),
        ),
        "pidf://docs/agents": (
            "text/markdown",
            (root / "AGENTS.md").read_text(encoding="utf-8"),
        ),
        "pidf://docs/compatibility": (
            "text/markdown",
            (root / "docs" / "COMPATIBILITY.md").read_text(encoding="utf-8"),
        ),
        "pidf://docs/vibecoding": (
            "text/markdown",
            (root / "docs" / "VIBECODING.md").read_text(encoding="utf-8"),
        ),
    }
    if uri.startswith("pidf://features/"):
        fid = uri.rsplit("/", 1)[-1]
        try:
            text = json.dumps(pidf.feature_by_id(fid), indent=2)
        except KeyError as exc:
            raise FileNotFoundError(uri) from exc
        return {
            "contents": [
                {"uri": uri, "mimeType": "application/json", "text": text}
            ]
        }
    if uri not in mapping:
        raise FileNotFoundError(uri)
    mime, text = mapping[uri]
    return {"contents": [{"uri": uri, "mimeType": mime, "text": text}]}


def _prompt_list() -> list[dict]:
    return [
        {
            "name": "implement_feature",
            "description": "Implement the next (or named) planned pico-idf feature",
            "arguments": [
                {
                    "name": "id",
                    "description": "Feature id (default: next planned)",
                    "required": False,
                }
            ],
        },
        {
            "name": "port_esp_idf_app",
            "description": "Port an ESP-IDF app_main sketch onto Pico W / Pico 2 W",
            "arguments": [
                {
                    "name": "goal",
                    "description": "What the firmware should do",
                    "required": True,
                }
            ],
        },
        {
            "name": "explain_impossible",
            "description": "Explain why an ESP-IDF feature cannot run on Pico and the substitute",
            "arguments": [
                {"name": "id", "description": "Feature id", "required": True}
            ],
        },
    ]


def _prompt_get(name: str, arguments: dict | None) -> dict:
    args = arguments or {}
    if name == "implement_feature":
        feature = pidf.next_planned_feature(args.get("id"))
        text = pidf.feature_implement_prompt(feature)
    elif name == "port_esp_idf_app":
        goal = args.get("goal", "blink an LED")
        text = (
            f"Port this ESP-IDF-style application to pico-idf on Pico W / Pico 2 W:\n\n"
            f"{goal}\n\n"
            "Use void app_main(void), FreeRTOS, ESP_LOGI, and existing pico-idf "
            "components. Check pidf://features before inventing APIs. If the user "
            "asks for ESP-NOW, ESP-MESH, SmartConfig, Thread, or touch, explain "
            "the substitute from the compatibility doc instead of faking it."
        )
    elif name == "explain_impossible":
        feature = pidf.feature_by_id(args["id"])
        text = (
            f"Feature `{feature['id']}` is {feature['status']}.\n"
            f"ESP-IDF API: {feature['esp_idf']}\n"
            f"Pico backend: {feature['backend']}\n"
            f"Notes: {feature.get('notes') or '(none)'}\n"
        )
    else:
        raise KeyError(name)
    return {
        "description": name,
        "messages": [
            {"role": "user", "content": {"type": "text", "text": text}}
        ],
    }


def _handle(message: dict) -> dict | None:
    method = message.get("method")
    msg_id = message.get("id")
    params = message.get("params") or {}

    if method is None and "id" in message:
        return None
    if method and method.startswith("notifications/"):
        return None
    if method == "initialized" or method == "notifications/initialized":
        return None

    def result(payload: Any) -> dict:
        return {"jsonrpc": "2.0", "id": msg_id, "result": payload}

    def error(code: int, text: str) -> dict:
        return {
            "jsonrpc": "2.0",
            "id": msg_id,
            "error": {"code": code, "message": text},
        }

    if method == "initialize":
        requested = params.get("protocolVersion") or PROTOCOL_VERSIONS[1]
        version = requested if requested in PROTOCOL_VERSIONS else PROTOCOL_VERSIONS[1]
        return result(
            {
                "protocolVersion": version,
                "capabilities": {
                    "tools": {"listChanged": False},
                    "resources": {"subscribe": False, "listChanged": False},
                    "prompts": {"listChanged": False},
                },
                "serverInfo": {"name": SERVER_NAME, "version": SERVER_VERSION},
                "instructions": (
                    "pico-idf MCP: ESP-IDF-shaped firmware for Pico W and Pico 2 W. "
                    "Call vibe_next, then implement the Pico backend, then build. "
                    "Do not copy ESP-IDF sources. Impossible rows stay impossible."
                ),
            }
        )

    if method == "ping":
        return result({})

    if method == "tools/list":
        tools = []
        for name, (_fn, schema) in TOOLS.items():
            tools.append(
                {
                    "name": name,
                    "description": schema["description"],
                    "inputSchema": schema["inputSchema"],
                }
            )
        return result({"tools": tools})

    if method == "tools/call":
        name = params.get("name")
        arguments = params.get("arguments") or {}
        if name not in TOOLS:
            return error(-32601, f"unknown tool {name!r}")
        fn, _schema = TOOLS[name]
        try:
            return result(fn(**arguments))
        except TypeError as exc:
            return result(_text(f"bad arguments: {exc}", is_error=True))
        except Exception as exc:  # noqa: BLE001
            traceback.print_exc(file=sys.stderr)
            return result(_text(f"{type(exc).__name__}: {exc}", is_error=True))

    if method == "resources/list":
        return result({"resources": _resource_list()})

    if method == "resources/templates/list":
        return result(
            {
                "resourceTemplates": [
                    {
                        "uriTemplate": "pidf://features/{id}",
                        "name": "Feature row",
                        "mimeType": "application/json",
                    }
                ]
            }
        )

    if method == "resources/read":
        uri = params.get("uri", "")
        try:
            return result(_resource_read(uri))
        except FileNotFoundError:
            return error(-32002, f"resource not found: {uri}")

    if method == "prompts/list":
        return result({"prompts": _prompt_list()})

    if method == "prompts/get":
        try:
            return result(_prompt_get(params.get("name"), params.get("arguments")))
        except KeyError:
            return error(-32601, f"unknown prompt {params.get('name')!r}")

    return error(-32601, f"unknown method {method!r}")


def serve() -> int:
    print("pico-idf MCP server on stdio", file=sys.stderr)
    for raw in sys.stdin:
        line = raw.strip()
        if not line:
            continue
        try:
            message = json.loads(line)
        except json.JSONDecodeError as exc:
            print(
                json.dumps(
                    {
                        "jsonrpc": "2.0",
                        "id": None,
                        "error": {"code": -32700, "message": f"parse error: {exc}"},
                    }
                ),
                flush=True,
            )
            continue
        batch = message if isinstance(message, list) else [message]
        replies = []
        for item in batch:
            reply = _handle(item)
            if reply is not None:
                replies.append(reply)
        if not replies:
            continue
        out = replies[0] if not isinstance(message, list) else replies
        print(json.dumps(out, separators=(",", ":")), flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(serve())
