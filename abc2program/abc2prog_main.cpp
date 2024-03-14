/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "abc2program_compiler.h"
#include "program_dump.h"
#include "abc2program_options.h"

void Abc2Prog(const std::string &input_file_path, const std::string &output_file_path)
{
    panda::abc2program::Abc2ProgramCompiler abc2prog_compiler;
    if (!abc2prog_compiler.OpenAbcFile(input_file_path)) {
        return;
    }
    panda::pandasm::Program prog;
    abc2prog_compiler.FillUpProgramData(prog);
    std::ofstream ofs;
    ofs.open(output_file_path, std::ios::trunc | std::ios::out);
    panda::abc2program::PandasmProgramDumper dumper(abc2prog_compiler.GetAbcFile(),
                                                    abc2prog_compiler.GetAbcStringTable());
    dumper.Dump(ofs, prog);
    ofs.close();
}

int main(int argc, const char **argv)
{
    panda::abc2program::Abc2ProgramOptions options;
    if (!options.Parse(argc, argv)) {
        return 1;
    }
    Abc2Prog(options.GetInputFilePath(), options.GetOutputFilePath());
    return 0;
}