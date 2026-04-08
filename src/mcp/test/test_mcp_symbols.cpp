#include "test_harness.h"
#include "mcp/main/minijson.h"
#include "libcore/main/machines/machine_registry.h"
#include "plugin_loader/main/plugin_loader.h"
#include <string>
#include <algorithm>

// Forward decl of the handlers we want to test
Json handleToolsCall(const Json& params);

TEST_CASE(mcp_symbol_tools) {
    // 1. Load plugins and create machine
    PluginLoader::instance().loadFromDir("./lib");
    
    std::string mid = "raw6502";

    Json createReq = Json::parse(R"({"name":"create_machine","arguments":{"machine_id":")" + mid + R"("}})");
    Json createRes = handleToolsCall(createReq);
    
    if (createRes["content"].aVal[0].contains("isError")) {
        std::cerr << "MCP CREATE ERROR: " << createRes["content"].aVal[0]["text"].sVal << std::endl;
    }
    ASSERT(!createRes["content"].aVal[0].contains("isError"));
    
    // 2. Add a symbol via MCP
    Json addReq = Json::parse(R"({"name":"add_symbol","arguments":{"machine_id":")" + mid + R"(","label":"mcp_test","addr":"$1234"}})");
    Json addRes = handleToolsCall(addReq);
    ASSERT(!addRes["content"].aVal[0].contains("isError"));
    
    // 3. List symbols and check if it's there
    Json listReq = Json::parse(R"({"name":"list_symbols","arguments":{"machine_id":")" + mid + R"("}})");
    Json listRes = handleToolsCall(listReq);
    std::string text = listRes["content"].aVal[0]["text"].sVal;
    EXPECT_TRUE(text.find("mcp_test") != std::string::npos);
    EXPECT_TRUE(text.find("1234") != std::string::npos);

    // 4. Test disassemble using that symbol
    Json disReq = Json::parse(R"({"name":"disassemble","arguments":{"machine_id":")" + mid + R"(","addr":"mcp_test","count":1}})");
    Json disRes = handleToolsCall(disReq);
    ASSERT(!disRes["content"].aVal[0].contains("isError"));

    // Cleanup
}
