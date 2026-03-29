#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <fstream>

#include "minijson.h"
#include "libcore/main/machine_desc.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/image_loader.h"
#include "plugin_loader/main/plugin_loader.h"
#include "libmem/main/memory_bus.h"
#include "libcore/main/icore.h"
#include "plugin_tool_registry.h"

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

    // Common property schemas
    Json midProp(Json::OBJ); midProp.oVal["type"] = Json("string");
    Json addrProp(Json::OBJ); addrProp.oVal["type"] = Json("integer");
    Json cntProp(Json::OBJ); cntProp.oVal["type"] = Json("integer");
    Json sizeProp(Json::OBJ); sizeProp.oVal["type"] = Json("integer");
    Json pattProp(Json::OBJ); pattProp.oVal["type"] = Json("string");
    Json ishProp(Json::OBJ); ishProp.oVal["type"] = Json("boolean");

    Json stepSchema(Json::OBJ);
    stepSchema.oVal["type"] = Json("object");
    Json stepProps(Json::OBJ);
    stepProps.oVal["machine_id"] = midProp;
    stepProps.oVal["count"] = cntProp;
    stepSchema.oVal["properties"] = stepProps;
    Json stepReq(Json::ARR); stepReq.push_back(Json("machine_id")); stepReq.push_back(Json("count"));
    stepSchema.oVal["required"] = stepReq;

    addTool("step_cpu", "Execute N instructions", stepSchema);

    Json spcSchema(Json::OBJ);
    spcSchema.oVal["type"] = Json("object");
    Json spcProps(Json::OBJ);
    spcProps.oVal["machine_id"] = midProp;
    spcProps.oVal["addr"] = addrProp;
    spcSchema.oVal["properties"] = spcProps;
    Json spcReq(Json::ARR); spcReq.push_back(Json("machine_id")); spcReq.push_back(Json("addr"));
    spcSchema.oVal["required"] = spcReq;

    addTool("set_pc", "Set CPU program counter", spcSchema);

    Json rmSchema(Json::OBJ);
    rmSchema.oVal["type"] = Json("object");
    Json rmProps(Json::OBJ);
    rmProps.oVal["machine_id"] = midProp;
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

    Json smSchema(Json::OBJ);
    smSchema.oVal["type"] = Json("object");
    Json smProps(Json::OBJ);
    smProps.oVal["machine_id"] = midProp;
    smProps.oVal["pattern"] = pattProp;
    smProps.oVal["is_hex"] = ishProp;
    smProps.oVal["start_addr"] = addrProp;
    smSchema.oVal["properties"] = smProps;
    Json smReq(Json::ARR); smReq.push_back(Json("machine_id")); smReq.push_back(Json("pattern"));
    smSchema.oVal["required"] = smReq;

    addTool("search_memory", "Search for pattern in memory", smSchema);

    Json kSchema(Json::OBJ);
    kSchema.oVal["type"] = Json("object");
    Json kProps(Json::OBJ);
    kProps.oVal["machine_id"] = midProp;
    Json knProp(Json::OBJ); knProp.oVal["type"] = Json("string");
    Json ksProp(Json::OBJ); ksProp.oVal["type"] = Json("boolean");
    kProps.oVal["key"] = knProp;
    kProps.oVal["down"] = ksProp;
    kSchema.oVal["properties"] = kProps;
    Json kReq(Json::ARR); kReq.push_back(Json("machine_id")); kReq.push_back(Json("key")); kReq.push_back(Json("down"));
    kSchema.oVal["required"] = kReq;

    addTool("press_key", "Inject a keystroke", kSchema);

    Json liSchema(Json::OBJ);
    liSchema.oVal["type"] = Json("object");
    Json liProps(Json::OBJ);
    liProps.oVal["machine_id"] = midProp;
    Json pathProp(Json::OBJ); pathProp.oVal["type"] = Json("string");
    liProps.oVal["path"] = pathProp;
    liProps.oVal["addr"] = addrProp;
    Json autoProp(Json::OBJ); autoProp.oVal["type"] = Json("boolean");
    liProps.oVal["auto_start"] = autoProp;
    liSchema.oVal["properties"] = liProps;
    Json liReq(Json::ARR); liReq.push_back(Json("machine_id")); liReq.push_back(Json("path"));
    liSchema.oVal["required"] = liReq;

    addTool("load_image", "Load program/binary into memory", liSchema);

    Json acSchema(Json::OBJ);
    acSchema.oVal["type"] = Json("object");
    Json acProps(Json::OBJ);
    acProps.oVal["machine_id"] = midProp;
    acProps.oVal["path"] = pathProp;
    acProps.oVal["reset"] = ishProp;
    acSchema.oVal["properties"] = acProps;
    Json acReq(Json::ARR); acReq.push_back(Json("machine_id")); acReq.push_back(Json("path"));
    acSchema.oVal["required"] = acReq;

    addTool("attach_cartridge", "Attach a cartridge image", acSchema);

    Json ecSchema(Json::OBJ);
    ecSchema.oVal["type"] = Json("object");
    Json ecProps(Json::OBJ);
    ecProps.oVal["machine_id"] = midProp;
    ecSchema.oVal["properties"] = ecProps;
    Json ecReq(Json::ARR); ecReq.push_back(Json("machine_id"));
    ecSchema.oVal["required"] = ecReq;

    addTool("eject_cartridge", "Eject currently attached cartridge", ecSchema);

    std::vector<std::string> pluginTools;
    PluginToolRegistry::instance().listTools(pluginTools);
    for (const auto& name : pluginTools) {
        std::string schema = PluginToolRegistry::instance().getSchema(name);
        Json s = Json::parse(schema);
        addTool(name, "Plugin-provided tool", s);
    }

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
    } else if (name == "set_pc") {
        std::string mid = args["machine_id"].sVal;
        uint32_t addr = (uint32_t)args["addr"].nVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            ms->cpu->setPc(addr);
            textItem.oVal["text"] = Json("Set PC to " + std::to_string(addr));
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
                    ss << std::setw(2) << (int)ms->bus->peek8(addr + i + j) << " ";
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
    } else if (name == "search_memory") {
        std::string mid = args["machine_id"].sVal;
        std::string pattern = args["pattern"].sVal;
        bool isHex = args.contains("is_hex") ? args["is_hex"].bVal : true;
        uint32_t startAddr = args.contains("start_addr") ? (uint32_t)args["start_addr"].nVal : 0;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            std::vector<uint8_t> bytes;
            if (isHex) {
                std::stringstream pss(pattern);
                std::string byteStr;
                while (pss >> byteStr) {
                    try { bytes.push_back((uint8_t)std::stoul(byteStr, nullptr, 16)); } catch (...) {}
                }
            } else {
                for (char c : pattern) bytes.push_back((uint8_t)c);
            }
            
            if (bytes.empty()) {
                textItem.oVal["text"] = Json("Error: Empty or invalid pattern");
                textItem.oVal["isError"] = Json(true);
            } else {
                uint32_t found = 0xFFFFFFFF;
                for (uint32_t i = startAddr; i < 0x10000 - bytes.size(); ++i) {
                    bool match = true;
                    for (size_t j = 0; j < bytes.size(); ++j) {
                        if (ms->bus->peek8(i + j) != bytes[j]) {
                            match = false; break;
                        }
                    }
                    if (match) { found = i; break; }
                }
                if (found != 0xFFFFFFFF) {
                    std::stringstream ss;
                    ss << "Found at $" << std::hex << std::setw(4) << std::setfill('0') << found;
                    textItem.oVal["text"] = Json(ss.str());
                } else {
                    textItem.oVal["text"] = Json("Pattern not found");
                }
            }
        }
    } else if (name == "press_key") {
        std::string mid = args["machine_id"].sVal;
        std::string key = args["key"].sVal;
        bool down = args["down"].bVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else if (!ms->machine->onKey) {
            textItem.oVal["text"] = Json("Error: Machine has no keyboard");
            textItem.oVal["isError"] = Json(true);
        } else {
            if (ms->machine->onKey(key, down)) {
                textItem.oVal["text"] = Json("Key " + key + (down ? " pressed" : " released"));
            } else {
                textItem.oVal["text"] = Json("Error: Unknown key " + key);
                textItem.oVal["isError"] = Json(true);
            }
        }
    } else if (name == "load_image") {
        std::string mid = args["machine_id"].sVal;
        std::string path = args["path"].sVal;
        uint32_t addr = args.contains("addr") ? (uint32_t)args["addr"].nVal : 0;
        bool autoStart = args.contains("auto_start") ? args["auto_start"].bVal : false;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            auto* loader = ImageLoaderRegistry::instance().findLoader(path);
            if (loader) {
                if (loader->load(path, ms->bus, ms->machine, addr)) {
                    uint32_t startAddr = addr;
                    if (startAddr == 0 && std::string(loader->name()).find("PRG") != std::string::npos) {
                        std::ifstream f(path, std::ios::binary);
                        uint8_t h[2];
                        f.read((char*)h, 2);
                        startAddr = h[0] | (h[1] << 8);
                    }
                    if (autoStart) {
                        ms->cpu->setPc(startAddr);
                    }
                    std::stringstream ss;
                    ss << "Loaded '" << path << "' at $" << std::hex << startAddr;
                    textItem.oVal["text"] = Json(ss.str());
                } else {
                    textItem.oVal["text"] = Json("Error: Failed to load image");
                    textItem.oVal["isError"] = Json(true);
                }
            } else {
                textItem.oVal["text"] = Json("Error: No loader found for file type");
                textItem.oVal["isError"] = Json(true);
            }
        }
    } else if (name == "attach_cartridge") {
        std::string mid = args["machine_id"].sVal;
        std::string path = args["path"].sVal;
        bool doReset = args.contains("reset") ? args["reset"].bVal : true;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            auto handler = ImageLoaderRegistry::instance().createCartridgeHandler(path);
            if (handler) {
                if (handler->attach(ms->bus, ms->machine)) {
                    auto md = handler->metadata();
                    ImageLoaderRegistry::instance().setActiveCartridge(ms->bus, std::move(handler));
                    if (doReset && ms->machine->onReset) ms->machine->onReset(*ms->machine);
                    textItem.oVal["text"] = Json("Attached cartridge: " + md.displayName + " (" + md.type + ")");
                } else {
                    textItem.oVal["text"] = Json("Error: Failed to attach cartridge");
                    textItem.oVal["isError"] = Json(true);
                }
            } else {
                textItem.oVal["text"] = Json("Error: Unsupported cartridge format");
                textItem.oVal["isError"] = Json(true);
            }
        }
    } else if (name == "eject_cartridge") {
        std::string mid = args["machine_id"].sVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            auto* cart = ImageLoaderRegistry::instance().getActiveCartridge(ms->bus);
            if (cart) {
                cart->eject(ms->bus);
                ImageLoaderRegistry::instance().setActiveCartridge(ms->bus, nullptr);
                if (ms->machine->onReset) ms->machine->onReset(*ms->machine);
                textItem.oVal["text"] = Json("Cartridge ejected.");
            } else {
                textItem.oVal["text"] = Json("No cartridge attached.");
            }
        }
    } else {
        std::string resultJson;
        if (PluginToolRegistry::instance().dispatch(name, args.stringify(), resultJson)) {
            content.aVal[0].oVal["text"] = Json(resultJson);
        } else {
            textItem.oVal["text"] = Json("Error: Unknown tool");
            textItem.oVal["isError"] = Json(true);
        }
    }

    if (content.aVal[0].oVal["text"].is_null()) {
        content.push_back(textItem);
    }
    res.oVal["content"] = content;
    return res;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    PluginLoader::instance().setMcpToolRegisterFn([](const PluginMcpToolInfo* info) {
        PluginToolRegistry::instance().registerTool(info);
    });

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
