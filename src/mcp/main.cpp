#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>

#include "minijson.h"
#include "libcore/machine_desc.h"
#include "libcore/machines/machine_registry.h"
#include "plugin_loader/plugin_loader.h"
#include "libmem/memory_bus.h"
#include "libcore/icore.h"

struct MachineState {
    MachineDescriptor* machine = nullptr;
    ICore* cpu = nullptr;
    IBus* bus = nullptr;
};

std::map<std::string, MachineState> g_machines;

MachineState* getMachine(const std::string& id) {
    if (g_machines.find(id) == g_machines.end()) {
        MachineDescriptor* md = MachineRegistry::instance().createMachine(id);
        if (!md) return nullptr;
        MachineState state;
        state.machine = md;
        state.cpu = md->cpus[0].cpu;
        state.bus = md->buses[0].bus;
        g_machines[id] = state;
    }
    return &g_machines[id];
}

void sendError(const Json& id, int code, const std::string& message) {
    Json res(Json::OBJ);
    res.oVal["jsonrpc"] = Json("2.0");
    res.oVal["id"] = id;
    Json err(Json::OBJ);
    err.oVal["code"] = Json(code);
    err.oVal["message"] = Json(message);
    res.oVal["error"] = err;
    std::cout << res.stringify() << "\n";
    std::cout.flush();
}

void sendResult(const Json& id, const Json& result) {
    Json res(Json::OBJ);
    res.oVal["jsonrpc"] = Json("2.0");
    res.oVal["id"] = id;
    res.oVal["result"] = result;
    std::cout << res.stringify() << "\n";
    std::cout.flush();
}

Json handleInitialize(const Json& params) {
    (void)params;
    Json res(Json::OBJ);
    res.oVal["protocolVersion"] = Json("2024-11-05");
    Json capabilities(Json::OBJ);
    capabilities.oVal["tools"] = Json(Json::OBJ);
    capabilities.oVal["resources"] = Json(Json::OBJ);
    res.oVal["capabilities"] = capabilities;
    Json serverInfo(Json::OBJ);
    serverInfo.oVal["name"] = Json("mmemu-mcp");
    serverInfo.oVal["version"] = Json("0.1.0");
    res.oVal["serverInfo"] = serverInfo;
    return res;
}

Json handleToolsList() {
    Json res(Json::OBJ);
    Json tools(Json::ARR);

    auto addTool = [&](const std::string& name, const std::string& desc, const Json& schema) {
        Json t(Json::OBJ);
        t.oVal["name"] = Json(name);
        t.oVal["description"] = Json(desc);
        t.oVal["inputSchema"] = schema;
        tools.push_back(t);
    };

    Json stepSchema(Json::OBJ);
    stepSchema.oVal["type"] = Json("object");
    Json stepProps(Json::OBJ);
    Json midProp(Json::OBJ); midProp.oVal["type"] = Json("string");
    Json cntProp(Json::OBJ); cntProp.oVal["type"] = Json("integer");
    stepProps.oVal["machine_id"] = midProp;
    stepProps.oVal["count"] = cntProp;
    stepSchema.oVal["properties"] = stepProps;
    Json stepReq(Json::ARR); stepReq.push_back(Json("machine_id")); stepReq.push_back(Json("count"));
    stepSchema.oVal["required"] = stepReq;

    addTool("step_cpu", "Execute N instructions", stepSchema);

    Json rmSchema(Json::OBJ);
    rmSchema.oVal["type"] = Json("object");
    Json rmProps(Json::OBJ);
    rmProps.oVal["machine_id"] = midProp;
    Json addrProp(Json::OBJ); addrProp.oVal["type"] = Json("integer");
    Json sizeProp(Json::OBJ); sizeProp.oVal["type"] = Json("integer");
    rmProps.oVal["addr"] = addrProp;
    rmProps.oVal["size"] = sizeProp;
    rmSchema.oVal["properties"] = rmProps;
    Json rmReq(Json::ARR); rmReq.push_back(Json("machine_id")); rmReq.push_back(Json("addr")); rmReq.push_back(Json("size"));
    rmSchema.oVal["required"] = rmReq;

    addTool("read_memory", "Read memory as hex dump", rmSchema);

    Json wmSchema(Json::OBJ);
    wmSchema.oVal["type"] = Json("object");
    Json wmProps(Json::OBJ);
    wmProps.oVal["machine_id"] = midProp;
    wmProps.oVal["addr"] = addrProp;
    Json bytesProp(Json::OBJ); bytesProp.oVal["type"] = Json("array");
    Json itemsProp(Json::OBJ); itemsProp.oVal["type"] = Json("integer");
    bytesProp.oVal["items"] = itemsProp;
    wmProps.oVal["bytes"] = bytesProp;
    wmSchema.oVal["properties"] = wmProps;
    Json wmReq(Json::ARR); wmReq.push_back(Json("machine_id")); wmReq.push_back(Json("addr")); wmReq.push_back(Json("bytes"));
    wmSchema.oVal["required"] = wmReq;

    addTool("write_memory", "Inject bytes into memory", wmSchema);

    Json rrSchema(Json::OBJ);
    rrSchema.oVal["type"] = Json("object");
    Json rrProps(Json::OBJ);
    rrProps.oVal["machine_id"] = midProp;
    rrSchema.oVal["properties"] = rrProps;
    Json rrReq(Json::ARR); rrReq.push_back(Json("machine_id"));
    rrSchema.oVal["required"] = rrReq;

    addTool("read_registers", "Get full CPU state", rrSchema);

    res.oVal["tools"] = tools;
    return res;
}

Json handleResourcesList() {
    Json res(Json::OBJ);
    Json resources(Json::ARR);

    Json r(Json::OBJ);
    r.oVal["uri"] = Json("machine_state");
    r.oVal["name"] = Json("Machine State Snapshot");
    r.oVal["description"] = Json("Snapshot of current session state");
    resources.push_back(r);

    res.oVal["resources"] = resources;
    return res;
}

Json handleResourcesRead(const Json& params) {
    std::string uri = params["uri"].sVal;
    Json res(Json::OBJ);
    Json contents(Json::ARR);
    Json item(Json::OBJ);
    item.oVal["uri"] = Json(uri);
    item.oVal["mimeType"] = Json("text/plain");

    std::stringstream ss;
    for (const auto& kv : g_machines) {
        ss << "Machine [" << kv.first << "] Cycles: " << kv.second.cpu->cycles() << "\n";
    }
    if (g_machines.empty()) ss << "No machines initialized.\n";

    item.oVal["text"] = Json(ss.str());
    contents.push_back(item);
    res.oVal["contents"] = contents;
    return res;
}

Json handleToolsCall(const Json& params) {
    std::string name = params["name"].sVal;
    Json args = params["arguments"];
    
    Json res(Json::OBJ);
    Json content(Json::ARR);
    Json textItem(Json::OBJ);
    textItem.oVal["type"] = Json("text");

    if (name == "step_cpu") {
        std::string mid = args["machine_id"].sVal;
        int count = (int)args["count"].nVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            for (int i = 0; i < count; ++i) ms->cpu->step();
            textItem.oVal["text"] = Json("Executed " + std::to_string(count) + " instructions.");
        }
    } else if (name == "read_memory") {
        std::string mid = args["machine_id"].sVal;
        uint32_t addr = (uint32_t)args["addr"].nVal;
        uint32_t size = (uint32_t)args["size"].nVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            std::stringstream ss;
            for (uint32_t i = 0; i < size; i += 16) {
                ss << std::hex << std::setw(4) << std::setfill('0') << (addr + i) << ": ";
                for (uint32_t j = 0; j < 16 && (i + j) < size; ++j) {
                    ss << std::setw(2) << (int)ms->bus->read8(addr + i + j) << " ";
                }
                ss << "\n";
            }
            textItem.oVal["text"] = Json(ss.str());
        }
    } else if (name == "write_memory") {
        std::string mid = args["machine_id"].sVal;
        uint32_t addr = (uint32_t)args["addr"].nVal;
        Json bytes = args["bytes"];
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else if (!bytes.is_array()) {
            textItem.oVal["text"] = Json("Error: bytes must be an array");
            textItem.oVal["isError"] = Json(true);
        } else {
            for (size_t i = 0; i < bytes.aVal.size(); ++i) {
                ms->bus->write8(addr + i, (uint8_t)bytes.aVal[i].nVal);
            }
            textItem.oVal["text"] = Json("Wrote " + std::to_string(bytes.aVal.size()) + " bytes.");
        }
    } else if (name == "read_registers") {
        std::string mid = args["machine_id"].sVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            std::stringstream ss;
            int count = ms->cpu->regCount();
            for (int i = 0; i < count; ++i) {
                const auto* desc = ms->cpu->regDescriptor(i);
                if (desc->flags & REGFLAG_INTERNAL) continue;
                uint32_t val = ms->cpu->regRead(i);
                ss << desc->name << ": $" << std::hex << std::setw(desc->width == RegWidth::R16 ? 4 : 2) << std::setfill('0') << val << "  ";
            }
            ss << std::dec << "\nCycles: " << ms->cpu->cycles();
            textItem.oVal["text"] = Json(ss.str());
        }
    } else {
        textItem.oVal["text"] = Json("Error: Unknown tool");
        textItem.oVal["isError"] = Json(true);
    }

    content.push_back(textItem);
    res.oVal["content"] = content;
    return res;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    PluginLoader::instance().loadFromDir("./lib");

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        Json req = Json::parse(line);
        if (!req.is_object() || !req.contains("jsonrpc") || !req.contains("method")) {
            sendError(Json(Json::NULL_VAL), -32600, "Invalid Request");
            continue;
        }

        Json id = req.contains("id") ? req["id"] : Json(Json::NULL_VAL);
        std::string method = req["method"].sVal;
        Json params = req.contains("params") ? req["params"] : Json(Json::OBJ);

        if (method == "initialize") {
            sendResult(id, handleInitialize(params));
        } else if (method == "tools/list") {
            sendResult(id, handleToolsList());
        } else if (method == "tools/call") {
            sendResult(id, handleToolsCall(params));
        } else if (method == "resources/list") {
            sendResult(id, handleResourcesList());
        } else if (method == "resources/read") {
            sendResult(id, handleResourcesRead(params));
        } else if (method == "notifications/initialized") {
            // Ignore
        } else if (method == "ping") {
            sendResult(id, Json(Json::OBJ));
        } else if (!id.is_null()) {
            sendError(id, -32601, "Method not found");
        }
    }

    PluginLoader::instance().unloadAll();
    return 0;
}
