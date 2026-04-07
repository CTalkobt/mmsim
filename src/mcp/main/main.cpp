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
#include "libtoolchain/main/toolchain_registry.h"
#include "libtoolchain/main/idisasm.h"
#include "libdebug/main/debug_context.h"
#include "libdebug/main/breakpoint_list.h"
#include "libdebug/main/stack_trace.h"
#include "plugin_tool_registry.h"
#include "include/util/logging.h"

static std::string toHex(uint32_t v, int width = 4) {
    std::ostringstream ss;
    ss << std::uppercase << std::hex << std::setfill('0') << std::setw(width) << v;
    return ss.str();
}

struct MachineState {
    MachineDescriptor* machine = nullptr;
    ICore*        cpu   = nullptr;
    IBus*         bus   = nullptr;
    DebugContext* dbg   = nullptr;
    IDisassembler* disasm = nullptr;
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
        state.dbg = new DebugContext(state.cpu, state.bus);
        state.cpu->setObserver(state.dbg);
        state.bus->setObserver(state.dbg);
        state.disasm = ToolchainRegistry::instance().createDisassembler(state.cpu->isaName());
        if (md->onReset) md->onReset(*md);
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

    Json rstSchema(Json::OBJ);
    rstSchema.oVal["type"] = Json("object");
    Json rstProps(Json::OBJ);
    rstProps.oVal["machine_id"] = midProp;
    rstSchema.oVal["properties"] = rstProps;
    Json rstReq(Json::ARR); rstReq.push_back(Json("machine_id"));
    rstSchema.oVal["required"] = rstReq;

    addTool("reset_machine", "Reset a machine to its power-on state", rstSchema);

    Json emptySchema(Json::OBJ);
    emptySchema.oVal["type"] = Json("object");
    addTool("list_loggers", "List all registered loggers and their levels", emptySchema);

    Json sllSchema(Json::OBJ);
    sllSchema.oVal["type"] = Json("object");
    Json sllProps(Json::OBJ);
    Json sllTarget(Json::OBJ); sllTarget.oVal["type"] = Json("string");
    Json sllLevel(Json::OBJ); sllLevel.oVal["type"] = Json("string");
    sllProps.oVal["target"] = sllTarget;
    sllProps.oVal["level"] = sllLevel;
    sllSchema.oVal["properties"] = sllProps;
    Json sllReq(Json::ARR); sllReq.push_back(Json("target")); sllReq.push_back(Json("level"));
    sllSchema.oVal["required"] = sllReq;
    addTool("set_log_level", "Set log level for a logger or 'all'", sllSchema);

    // list_machines
    addTool("list_machines", "List all available machine types", emptySchema);

    // create_machine
    Json cmSchema(Json::OBJ); cmSchema.oVal["type"] = Json("object");
    Json cmProps(Json::OBJ); cmProps.oVal["machine_id"] = midProp;
    cmSchema.oVal["properties"] = cmProps;
    Json cmReq(Json::ARR); cmReq.push_back(Json("machine_id"));
    cmSchema.oVal["required"] = cmReq;
    addTool("create_machine", "Create (or re-create) a machine by ID", cmSchema);

    // run_cpu
    Json runSchema(Json::OBJ); runSchema.oVal["type"] = Json("object");
    Json runProps(Json::OBJ); runProps.oVal["machine_id"] = midProp;
    Json maxStepsProp(Json::OBJ); maxStepsProp.oVal["type"] = Json("integer");
    runProps.oVal["max_steps"] = maxStepsProp;
    runSchema.oVal["properties"] = runProps;
    Json runReq(Json::ARR); runReq.push_back(Json("machine_id"));
    runSchema.oVal["required"] = runReq;
    addTool("run_cpu", "Run until breakpoint, program end, or max_steps (default 10000000)", runSchema);

    // disassemble
    Json daSchema(Json::OBJ); daSchema.oVal["type"] = Json("object");
    Json daProps(Json::OBJ); daProps.oVal["machine_id"] = midProp; daProps.oVal["addr"] = addrProp; daProps.oVal["count"] = cntProp;
    daSchema.oVal["properties"] = daProps;
    Json daReq(Json::ARR); daReq.push_back(Json("machine_id"));
    daSchema.oVal["required"] = daReq;
    addTool("disassemble", "Disassemble instructions (addr defaults to PC, count defaults to 10)", daSchema);

    // fill_memory
    Json fmSchema(Json::OBJ); fmSchema.oVal["type"] = Json("object");
    Json fmProps(Json::OBJ); fmProps.oVal["machine_id"] = midProp; fmProps.oVal["addr"] = addrProp;
    Json valProp(Json::OBJ); valProp.oVal["type"] = Json("integer");
    fmProps.oVal["value"] = valProp; fmProps.oVal["size"] = sizeProp;
    fmSchema.oVal["properties"] = fmProps;
    Json fmReq(Json::ARR); fmReq.push_back(Json("machine_id")); fmReq.push_back(Json("addr")); fmReq.push_back(Json("value")); fmReq.push_back(Json("size"));
    fmSchema.oVal["required"] = fmReq;
    addTool("fill_memory", "Fill a memory range with a byte value", fmSchema);

    // copy_memory
    Json cpSchema(Json::OBJ); cpSchema.oVal["type"] = Json("object");
    Json cpProps(Json::OBJ); cpProps.oVal["machine_id"] = midProp;
    Json srcProp(Json::OBJ); srcProp.oVal["type"] = Json("integer");
    Json dstProp(Json::OBJ); dstProp.oVal["type"] = Json("integer");
    cpProps.oVal["src_addr"] = srcProp; cpProps.oVal["dst_addr"] = dstProp; cpProps.oVal["size"] = sizeProp;
    cpSchema.oVal["properties"] = cpProps;
    Json cpReq(Json::ARR); cpReq.push_back(Json("machine_id")); cpReq.push_back(Json("src_addr")); cpReq.push_back(Json("dst_addr")); cpReq.push_back(Json("size"));
    cpSchema.oVal["required"] = cpReq;
    addTool("copy_memory", "Copy a memory range to another address", cpSchema);

    // swap_memory
    Json swmSchema(Json::OBJ); swmSchema.oVal["type"] = Json("object");
    Json swmProps(Json::OBJ); swmProps.oVal["machine_id"] = midProp;
    Json addr1Prop(Json::OBJ); addr1Prop.oVal["type"] = Json("integer");
    Json addr2Prop(Json::OBJ); addr2Prop.oVal["type"] = Json("integer");
    swmProps.oVal["addr1"] = addr1Prop; swmProps.oVal["addr2"] = addr2Prop; swmProps.oVal["size"] = sizeProp;
    swmSchema.oVal["properties"] = swmProps;
    Json swmReq(Json::ARR); swmReq.push_back(Json("machine_id")); swmReq.push_back(Json("addr1")); swmReq.push_back(Json("addr2")); swmReq.push_back(Json("size"));
    swmSchema.oVal["required"] = swmReq;
    addTool("swap_memory", "Swap two memory ranges of equal size", swmSchema);

    // set_breakpoint
    Json sbpSchema(Json::OBJ); sbpSchema.oVal["type"] = Json("object");
    Json sbpProps(Json::OBJ); sbpProps.oVal["machine_id"] = midProp; sbpProps.oVal["addr"] = addrProp;
    sbpSchema.oVal["properties"] = sbpProps;
    Json sbpReq(Json::ARR); sbpReq.push_back(Json("machine_id")); sbpReq.push_back(Json("addr"));
    sbpSchema.oVal["required"] = sbpReq;
    addTool("set_breakpoint", "Set an execution breakpoint at an address", sbpSchema);

    // set_watchpoint
    Json swpSchema(Json::OBJ); swpSchema.oVal["type"] = Json("object");
    Json swpProps(Json::OBJ); swpProps.oVal["machine_id"] = midProp; swpProps.oVal["addr"] = addrProp;
    Json wpTypeProp(Json::OBJ); wpTypeProp.oVal["type"] = Json("string");
    swpProps.oVal["type"] = wpTypeProp;
    swpSchema.oVal["properties"] = swpProps;
    Json swpReq(Json::ARR); swpReq.push_back(Json("machine_id")); swpReq.push_back(Json("addr")); swpReq.push_back(Json("type"));
    swpSchema.oVal["required"] = swpReq;
    addTool("set_watchpoint", "Set a read or write watchpoint (type: \"read\" or \"write\")", swpSchema);

    // delete_breakpoint / enable_breakpoint / disable_breakpoint share the same schema
    Json bpIdSchema(Json::OBJ); bpIdSchema.oVal["type"] = Json("object");
    Json bpIdProps(Json::OBJ); bpIdProps.oVal["machine_id"] = midProp;
    Json idProp(Json::OBJ); idProp.oVal["type"] = Json("integer");
    bpIdProps.oVal["id"] = idProp;
    bpIdSchema.oVal["properties"] = bpIdProps;
    Json bpIdReq(Json::ARR); bpIdReq.push_back(Json("machine_id")); bpIdReq.push_back(Json("id"));
    bpIdSchema.oVal["required"] = bpIdReq;
    addTool("delete_breakpoint",  "Delete a breakpoint or watchpoint by id", bpIdSchema);
    addTool("enable_breakpoint",  "Enable a breakpoint or watchpoint by id", bpIdSchema);
    addTool("disable_breakpoint", "Disable a breakpoint or watchpoint by id", bpIdSchema);

    // list_breakpoints
    addTool("list_breakpoints", "List all breakpoints and watchpoints", cmSchema);  // reuse machine_id-only schema

    // get_stack
    Json gsSchema(Json::OBJ); gsSchema.oVal["type"] = Json("object");
    Json gsProps(Json::OBJ); gsProps.oVal["machine_id"] = midProp; gsProps.oVal["count"] = cntProp;
    gsSchema.oVal["properties"] = gsProps;
    Json gsReq(Json::ARR); gsReq.push_back(Json("machine_id"));
    gsSchema.oVal["required"] = gsReq;
    addTool("get_stack", "Show stack trace (count defaults to 8, 0 = all)", gsSchema);

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
    } else if (name == "reset_machine") {
        std::string mid = args["machine_id"].sVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            if (ms->machine->onReset) ms->machine->onReset(*ms->machine);
            textItem.oVal["text"] = Json("Machine " + mid + " reset.");
        }
    } else if (name == "list_loggers") {
        auto names = LogRegistry::instance().getLoggerNames();
        std::stringstream ss;
        ss << "Registered loggers:\n";
        for (const auto& n : names) {
            auto l = LogRegistry::instance().getLogger(n);
            std::string lvl = spdlog::level::to_string_view(l->level()).data();
            ss << "  " << n << " [" << lvl << "]\n";
        }
        textItem.oVal["text"] = Json(ss.str());
    } else if (name == "set_log_level") {
        std::string target = args["target"].sVal;
        std::string levelStr = args["level"].sVal;
        spdlog::level::level_enum lvl = spdlog::level::from_str(levelStr);
        if (target == "all") {
            LogRegistry::instance().setGlobalLevel(lvl);
            textItem.oVal["text"] = Json("Set all loggers to " + levelStr);
        } else {
            auto l = LogRegistry::instance().getLogger(target);
            l->set_level(lvl);
            textItem.oVal["text"] = Json("Set logger '" + target + "' to " + levelStr);
        }
    } else if (name == "list_machines") {
        std::vector<std::string> ids;
        MachineRegistry::instance().enumerate(ids);
        std::stringstream ss;
        for (const auto& id : ids) ss << id << "\n";
        textItem.oVal["text"] = Json(ss.str().empty() ? "(none)\n" : ss.str());
    } else if (name == "create_machine") {
        std::string mid = args["machine_id"].sVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Unknown machine ID: " + mid);
            textItem.oVal["isError"] = Json(true);
        } else {
            textItem.oVal["text"] = Json("Created machine: " + std::string(ms->machine->displayName));
        }
    } else if (name == "run_cpu") {
        std::string mid = args["machine_id"].sVal;
        int maxSteps = args.contains("max_steps") ? (int)args["max_steps"].nVal : 10000000;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            ms->dbg->resume();
            int steps = 0;
            while (!ms->dbg->isPaused() && steps < maxSteps) {
                if (ms->machine && ms->machine->schedulerStep) {
                    ms->machine->schedulerStep(*ms->machine);
                } else {
                    ms->cpu->step();
                }
                if (ms->cpu->isProgramEnd(ms->bus)) break;
                ++steps;
            }
            std::stringstream ss;
            if (ms->dbg->isPaused())   ss << "Breakpoint hit. ";
            else if (steps >= maxSteps) ss << "Stopped after " << maxSteps << " steps. ";
            else                        ss << "Program ended. ";
            int regCount = ms->cpu->regCount();
            for (int i = 0; i < regCount; ++i) {
                const auto* desc = ms->cpu->regDescriptor(i);
                if (desc->flags & REGFLAG_INTERNAL) continue;
                uint32_t val = ms->cpu->regRead(i);
                ss << desc->name << ": $"
                   << toHex(val, desc->width == RegWidth::R16 ? 4 : 2) << "  ";
            }
            textItem.oVal["text"] = Json(ss.str());
        }
    } else if (name == "disassemble") {
        std::string mid = args["machine_id"].sVal;
        MachineState* ms = getMachine(mid);
        if (!ms || !ms->disasm) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID or no disassembler");
            textItem.oVal["isError"] = Json(true);
        } else {
            uint32_t addr = args.contains("addr") ? (uint32_t)args["addr"].nVal : ms->cpu->pc();
            int count = args.contains("count") ? (int)args["count"].nVal : 10;
            std::stringstream ss;
            for (int i = 0; i < count; ++i) {
                char buf[64];
                ss << toHex(addr) << ": ";
                int bytes = ms->disasm->disasmOne(ms->bus, addr, buf, sizeof(buf));
                ss << buf << "\n";
                addr += bytes;
            }
            textItem.oVal["text"] = Json(ss.str());
        }
    } else if (name == "fill_memory") {
        std::string mid = args["machine_id"].sVal;
        uint32_t addr  = (uint32_t)args["addr"].nVal;
        uint8_t  value = (uint8_t)args["value"].nVal;
        uint32_t size  = (uint32_t)args["size"].nVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            for (uint32_t i = 0; i < size; ++i) ms->bus->write8(addr + i, value);
            textItem.oVal["text"] = Json("Filled " + std::to_string(size) + " bytes at $" + toHex(addr));
        }
    } else if (name == "copy_memory") {
        std::string mid = args["machine_id"].sVal;
        uint32_t src  = (uint32_t)args["src_addr"].nVal;
        uint32_t dst  = (uint32_t)args["dst_addr"].nVal;
        uint32_t size = (uint32_t)args["size"].nVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            std::vector<uint8_t> buf(size);
            for (uint32_t i = 0; i < size; ++i) buf[i] = ms->bus->peek8(src + i);
            for (uint32_t i = 0; i < size; ++i) ms->bus->write8(dst + i, buf[i]);
            textItem.oVal["text"] = Json("Copied " + std::to_string(size) + " bytes from $" + toHex(src) + " to $" + toHex(dst));
        }
    } else if (name == "swap_memory") {
        std::string mid = args["machine_id"].sVal;
        uint32_t addr1 = (uint32_t)args["addr1"].nVal;
        uint32_t addr2 = (uint32_t)args["addr2"].nVal;
        uint32_t size  = (uint32_t)args["size"].nVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            std::vector<uint8_t> tmp(size);
            for (uint32_t i = 0; i < size; ++i) {
                uint8_t v1 = ms->bus->read8(addr1 + i);
                uint8_t v2 = ms->bus->read8(addr2 + i);
                tmp[i] = v1;
                ms->bus->write8(addr1 + i, v2);
            }
            for (uint32_t i = 0; i < size; ++i) ms->bus->write8(addr2 + i, tmp[i]);
            textItem.oVal["text"] = Json("Swapped " + std::to_string(size) + " bytes between $" + toHex(addr1) + " and $" + toHex(addr2));
        }
    } else if (name == "set_breakpoint") {
        std::string mid = args["machine_id"].sVal;
        uint32_t addr = (uint32_t)args["addr"].nVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            int id = ms->dbg->breakpoints().add(addr, BreakpointType::EXEC);
            textItem.oVal["text"] = Json("Breakpoint " + std::to_string(id) + " at $" + toHex(addr));
        }
    } else if (name == "set_watchpoint") {
        std::string mid = args["machine_id"].sVal;
        uint32_t addr = (uint32_t)args["addr"].nVal;
        std::string type = args["type"].sVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else if (type != "read" && type != "write") {
            textItem.oVal["text"] = Json("Error: type must be \"read\" or \"write\"");
            textItem.oVal["isError"] = Json(true);
        } else {
            BreakpointType btype = (type == "read") ? BreakpointType::READ_WATCH : BreakpointType::WRITE_WATCH;
            int id = ms->dbg->breakpoints().add(addr, btype);
            textItem.oVal["text"] = Json("Watchpoint " + std::to_string(id) + " (" + type + ") at $" + toHex(addr));
        }
    } else if (name == "delete_breakpoint") {
        std::string mid = args["machine_id"].sVal;
        int id = (int)args["id"].nVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            ms->dbg->breakpoints().remove(id);
            textItem.oVal["text"] = Json("Deleted breakpoint " + std::to_string(id));
        }
    } else if (name == "enable_breakpoint") {
        std::string mid = args["machine_id"].sVal;
        int id = (int)args["id"].nVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            ms->dbg->breakpoints().setEnabled(id, true);
            textItem.oVal["text"] = Json("Enabled breakpoint " + std::to_string(id));
        }
    } else if (name == "disable_breakpoint") {
        std::string mid = args["machine_id"].sVal;
        int id = (int)args["id"].nVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            ms->dbg->breakpoints().setEnabled(id, false);
            textItem.oVal["text"] = Json("Disabled breakpoint " + std::to_string(id));
        }
    } else if (name == "list_breakpoints") {
        std::string mid = args["machine_id"].sVal;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            const auto& bps = ms->dbg->breakpoints().breakpoints();
            if (bps.empty()) {
                textItem.oVal["text"] = Json("No breakpoints set.");
            } else {
                std::stringstream ss;
                for (const auto& bp : bps) {
                    std::string type;
                    switch (bp.type) {
                        case BreakpointType::EXEC:        type = "exec";  break;
                        case BreakpointType::READ_WATCH:  type = "read";  break;
                        case BreakpointType::WRITE_WATCH: type = "write"; break;
                    }
                    ss << bp.id << "  " << std::left << std::setw(6) << type
                       << (bp.enabled ? "y" : "n") << "  $" << toHex(bp.addr)
                       << "  hits=" << bp.hitCount << "\n";
                }
                textItem.oVal["text"] = Json(ss.str());
            }
        }
    } else if (name == "get_stack") {
        std::string mid = args["machine_id"].sVal;
        int count = args.contains("count") ? (int)args["count"].nVal : 8;
        MachineState* ms = getMachine(mid);
        if (!ms) {
            textItem.oVal["text"] = Json("Error: Invalid machine ID");
            textItem.oVal["isError"] = Json(true);
        } else {
            auto& st = ms->dbg->stackTrace();
            auto entries = st.recent(count);
            std::stringstream ss;
            ss << "Stack (depth " << st.depth() << ", showing " << entries.size() << "):\n";
            for (int i = 0; i < (int)entries.size(); ++i) {
                const auto& e = entries[i];
                ss << "  " << std::setw(3) << i << "  "
                   << std::left << std::setw(5) << stackPushTypeName(e.type) << "  ";
                if (e.type == StackPushType::CALL || e.type == StackPushType::BRK)
                    ss << "$" << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << e.value;
                else
                    ss << "$" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << e.value;
                ss << "  pushed by $" << std::setw(4) << e.pushedByPc << "\n";
            }
            textItem.oVal["text"] = Json(ss.str());
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

    LogRegistry::instance().init();

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
