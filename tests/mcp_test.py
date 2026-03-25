import subprocess
import json
import sys

def mcp_request(proc, req):
    req_str = json.dumps(req)
    proc.stdin.write(req_str + "\n")
    proc.stdin.flush()
    line = proc.stdout.readline()
    if not line:
        return None
    return json.loads(line)

def run_test():
    proc = subprocess.Popen(["./bin/mmemu-mcp"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)

    # 1. Initialize
    req = {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "initialize",
        "params": {
            "protocolVersion": "2024-11-05",
            "capabilities": {},
            "clientInfo": {"name": "test", "version": "1.0"}
        }
    }
    res = mcp_request(proc, req)
    if not res or "result" not in res:
        print("Initialization failed")
        sys.exit(1)
    print("Initialize success")

    # 2. write_memory at 0x0000 with A9 42 (LDA #$42)
    req = {
        "jsonrpc": "2.0",
        "id": 2,
        "method": "tools/call",
        "params": {
            "name": "write_memory",
            "arguments": {
                "machine_id": "raw6502",
                "addr": 0x0000,
                "bytes": [0xA9, 0x42]
            }
        }
    }
    res = mcp_request(proc, req)
    print("Write memory:", res)

    # 3. step_cpu
    req = {
        "jsonrpc": "2.0",
        "id": 3,
        "method": "tools/call",
        "params": {
            "name": "step_cpu",
            "arguments": {
                "machine_id": "raw6502",
                "count": 1
            }
        }
    }
    res = mcp_request(proc, req)
    print("Step CPU:", res)

    # 4. read_registers
    req = {
        "jsonrpc": "2.0",
        "id": 4,
        "method": "tools/call",
        "params": {
            "name": "read_registers",
            "arguments": {
                "machine_id": "raw6502"
            }
        }
    }
    res = mcp_request(proc, req)
    print("Read Registers:", res)

    content = res["result"]["content"][0]["text"]
    if "A: $42" in content:
        print("PASS: Register A is $42")
    else:
        print("FAIL: Register A is not $42")
        sys.exit(1)

    # 5. resources/read
    req = {
        "jsonrpc": "2.0",
        "id": 5,
        "method": "resources/read",
        "params": {
            "uri": "machine_state"
        }
    }
    res = mcp_request(proc, req)
    print("Read Resource:", res)
    if "raw6502" in res["result"]["contents"][0]["text"]:
        print("PASS: Resource read successful")
    else:
        print("FAIL: Resource read missing raw6502")
        sys.exit(1)

    proc.terminate()
    print("All tests passed.")

if __name__ == "__main__":
    run_test()
