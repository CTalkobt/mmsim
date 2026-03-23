#pragma once

#include "iassembler.h"

class KickAssemblerBackend : public IAssembler {
public:
    const char* name() const override { return "KickAssembler"; }
    bool isaSupported(const std::string& isa) const override;

    AssemblerResult assemble(const std::string& sourcePath, const std::string& outputPath) override;
};
