#include "include/mmemu_plugin_api.h"
#include "rom_discovery.h"
#include "rom_importer.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif

static const SimPluginHostAPI* g_host = nullptr;

// ---------------------------------------------------------------------------
// CLI command: importroms
// ---------------------------------------------------------------------------

static int cmdImportRoms(int argc, const char* const* argv, void* ctx) {
    (void)ctx;
    if (argc < 2) {
        std::cout << "Usage: importroms <machineId> [--list] [--source <n>] [--dest <path>] [--overwrite]\n";
        return 1;
    }

    std::string machineId = argv[1];
    bool listOnly = false;
    int sourceIdx = -1;
    std::string destDir = "roms/" + machineId;
    bool overwrite = false;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--list") listOnly = true;
        else if (arg == "--source" && i + 1 < argc) sourceIdx = std::stoi(argv[++i]);
        else if (arg == "--dest" && i + 1 < argc) destDir = argv[++i];
        else if (arg == "--overwrite") overwrite = true;
    }

    auto sources = discoverSources(machineId);
    if (sources.empty()) {
        std::cout << "No VICE installations found for machine: " << machineId << "\n";
        return 1;
    }

    if (listOnly) {
        std::cout << "Found " << sources.size() << " VICE installation(s):\n";
        for (size_t i = 0; i < sources.size(); ++i) {
            std::cout << "  [" << i << "] " << sources[i].label << " (" << sources[i].basePath << ")\n";
        }
        return 0;
    }

    if (sourceIdx < 0) {
        if (sources.size() == 1) {
            sourceIdx = 0;
        } else {
            std::cout << "Multiple sources found. Please specify one with --source <n>:\n";
            for (size_t i = 0; i < sources.size(); ++i) {
                std::cout << "  [" << i << "] " << sources[i].label << "\n";
            }
            return 1;
        }
    }

    if (sourceIdx >= (int)sources.size()) {
        std::cout << "Invalid source index: " << sourceIdx << "\n";
        return 1;
    }

    std::cout << "Importing ROMs from: " << sources[sourceIdx].label << " to " << destDir << "...\n";
    auto result = importRoms(sources[sourceIdx], machineId, destDir, overwrite);

    if (result.success) {
        std::cout << "Successfully imported " << result.copiedFiles.size() << " file(s):\n";
        for (const auto& f : result.copiedFiles) std::cout << "  - " << f << "\n";
        std::cout << "Please reset the machine to load the new ROMs.\n";
        return 0;
    } else {
        std::cerr << "Import failed: " << result.errorMessage << "\n";
        return 1;
    }
}

// ---------------------------------------------------------------------------
// MCP tool: import_roms
// ---------------------------------------------------------------------------

static void mcpImportRoms(const char* paramsJson, char** resultJson, void* ctx) {
    (void)ctx;
    std::string params = paramsJson ? paramsJson : "{}";

    // Extract string field from minimal JSON
    auto extractStr = [&](const std::string& key) -> std::string {
        std::string token = "\"" + key + "\"";
        size_t pos = params.find(token);
        if (pos == std::string::npos) return "";
        pos = params.find(':', pos + token.size());
        if (pos == std::string::npos) return "";
        pos = params.find('"', pos);
        if (pos == std::string::npos) return "";
        size_t end = params.find('"', pos + 1);
        if (end == std::string::npos) return "";
        return params.substr(pos + 1, end - pos - 1);
    };

    std::string machineId = extractStr("machineId");
    if (machineId.empty()) machineId = "vic20";

    bool overwrite = false;
    size_t owPos = params.find("\"overwrite\"");
    if (owPos != std::string::npos) {
        size_t valPos = params.find_first_not_of(" \t\r\n:", owPos + 11);
        if (valPos != std::string::npos && params.compare(valPos, 4, "true") == 0)
            overwrite = true;
    }

    int sourceIndex = 0;
    size_t siPos = params.find("\"sourceIndex\"");
    if (siPos != std::string::npos) {
        size_t colonPos = params.find(':', siPos);
        if (colonPos != std::string::npos) {
            size_t numPos = params.find_first_of("0123456789", colonPos);
            if (numPos != std::string::npos)
                sourceIndex = std::stoi(params.substr(numPos));
        }
    }

    auto sources = discoverSources(machineId);
    if (sources.empty()) {
        *resultJson = strdup("{\"success\":false,\"error\":\"No VICE installations found\"}");
        return;
    }
    if (sourceIndex < 0 || sourceIndex >= (int)sources.size()) {
        *resultJson = strdup("{\"success\":false,\"error\":\"Invalid sourceIndex\"}");
        return;
    }

    auto res = importRoms(sources[sourceIndex], machineId, "roms/" + machineId, overwrite);

    if (res.success) {
        std::string json = "{\"success\":true,\"files\":[";
        for (size_t i = 0; i < res.copiedFiles.size(); ++i) {
            if (i > 0) json += ",";
            json += "\"" + res.copiedFiles[i] + "\"";
        }
        json += "]}";
        *resultJson = strdup(json.c_str());
    } else {
        std::string escaped = res.errorMessage;
        for (size_t i = 0; i < escaped.size(); ++i) {
            if (escaped[i] == '"') { escaped.insert(i, "\\"); ++i; }
        }
        *resultJson = strdup(("{\"success\":false,\"error\":\"" + escaped + "\"}").c_str());
    }
}

static void mcpFreeString(char* s) {
    free(s);
}

// ---------------------------------------------------------------------------
// GUI pane factory (implemented in rom_import_pane.cpp)
// ---------------------------------------------------------------------------

void* createRomImportPane(void* parent, void* ctx);

// ---------------------------------------------------------------------------
// Plugin entry point
// ---------------------------------------------------------------------------

SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    g_host = host;

    // CLI command
    static struct PluginCommandInfo cmdInfo;
    cmdInfo.name    = "importroms";
    cmdInfo.usage   = "<machineId> [--list] [--source <n>] [--dest <path>] [--overwrite]";
    cmdInfo.execute = cmdImportRoms;
    cmdInfo.ctx     = nullptr;
    host->registerCommand(&cmdInfo);

    // GUI pane
    static const char* paneMatchIds[] = { "vic20", nullptr };
    static struct PluginPaneInfo paneInfo;
    paneInfo.paneId      = "vice-importer.main";
    paneInfo.displayName = "VICE ROM Importer";
    paneInfo.menuSection = "Tools";
    paneInfo.machineIds  = paneMatchIds;
    paneInfo.createPane  = createRomImportPane;
    paneInfo.destroyPane = nullptr;  // wxWidgets manages lifetime
    paneInfo.refreshPane = nullptr;
    paneInfo.ctx         = nullptr;
    host->registerPane(&paneInfo);

    // MCP tool
    static struct PluginMcpToolInfo mcpInfo;
    mcpInfo.toolName   = "import_roms";
    mcpInfo.schemaJson =
        "{"
        "\"type\":\"object\","
        "\"properties\":{"
        "\"machineId\":{\"type\":\"string\",\"description\":\"Target machine ID (e.g. vic20)\"},"
        "\"sourceIndex\":{\"type\":\"integer\",\"description\":\"Index into discovered VICE installations\"},"
        "\"overwrite\":{\"type\":\"boolean\",\"description\":\"Overwrite existing ROM files\"}"
        "},"
        "\"required\":[\"machineId\"]"
        "}";
    mcpInfo.handle     = mcpImportRoms;
    mcpInfo.freeString = mcpFreeString;
    mcpInfo.ctx        = nullptr;
    host->registerMcpTool(&mcpInfo);

    // Manifest
    static const char* deps[]             = { "vic20", nullptr };
    static const char* supportedMachines[] = { "vic20", nullptr };

    static SimPluginManifest manifest;
    std::memset(&manifest, 0, sizeof(manifest));
    manifest.apiVersion          = MMEMU_PLUGIN_API_VERSION;
    manifest.pluginId            = "vice-importer";
    manifest.displayName         = "VICE ROM Importer";
    manifest.version             = "0.1.0";
    manifest.deps                = deps;
    manifest.supportedMachineIds = supportedMachines;

    return &manifest;
}

#ifdef __cplusplus
}
#endif
